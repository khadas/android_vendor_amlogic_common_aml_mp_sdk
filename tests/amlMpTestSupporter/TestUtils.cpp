/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_TestUtils"
#include <utils/Log.h>
#include "TestUtils.h"
#include <signal.h>
#include <thread>
#include <media/stagefright/foundation/ADebug.h>
#include <poll.h>
#include <unistd.h>

namespace aml_mp {
using namespace android;

NativeUI::NativeUI()
{
#ifndef __ANDROID_VNDK__
    mComposerClient = new android::SurfaceComposerClient;
    CHECK_EQ(mComposerClient->initCheck(), OK);

    sp<android::IBinder> displayToken  = nullptr;
    displayToken = mComposerClient->getInternalDisplayToken();

    android::DisplayInfo displayInfo;
    CHECK_EQ(OK, mComposerClient->getDisplayInfo(displayToken, &displayInfo));

    //mDisplayWidth = displayInfo.w;
    //mDisplayHeight = displayInfo.h;

    mSurfaceWidth = mDisplayWidth >> 1;
    mSurfaceHeight = mDisplayHeight >> 1;
    ALOGI("mSurfaceWidth: %d, mSurfaceHeight: %d", mSurfaceWidth, mSurfaceHeight);

    mSurfaceControl = mComposerClient->createSurface(android::String8("AmlMpPlayer"), mSurfaceWidth, mSurfaceHeight, android::PIXEL_FORMAT_RGB_565, 0);
    CHECK(mSurfaceControl->isValid());
    mSurface = mSurfaceControl->getSurface();

    android::SurfaceComposerClient::Transaction()
        .setFlags(mSurfaceControl, android::layer_state_t::eLayerOpaque, android::layer_state_t::eLayerOpaque)
        .setCrop_legacy(mSurfaceControl, android::Rect(mSurfaceWidth, mSurfaceHeight))
        .show(mSurfaceControl)
        .apply();

    mSurfaceControlUi = mComposerClient->createSurface(android::String8("AmlMpPlayer-ui"), mDisplayWidth, mDisplayHeight, android::PIXEL_FORMAT_RGBA_8888, 0);
    CHECK(mSurfaceControlUi->isValid());
    mSurfaceUi = mSurfaceControlUi->getSurface();

    int ret = native_window_api_connect(mSurfaceUi.get(), NATIVE_WINDOW_API_CPU);
    if (ret < 0) {
        ALOGE("mSurfaceUi connect failed with %d!", ret);
        return;
    }

    mSurfaceUi->allocateBuffers();

    android::SurfaceComposerClient::Transaction()
        .setCrop_legacy(mSurfaceControlUi, android::Rect(mDisplayWidth, mDisplayHeight))
        .setLayer(mSurfaceControlUi, UI_LAYER)
        .show(mSurfaceControlUi)
        .apply();

    ANativeWindow* nativeWindow = static_cast<ANativeWindow*>(mSurfaceUi.get());

    int err = 0;
    ANativeWindowBuffer* buf;
    err = nativeWindow->dequeueBuffer_DEPRECATED(nativeWindow, &buf);
    if (err != 0) {
        ALOGE("dequeueBuffer failed:%d\n", err);
        return;
    }

    nativeWindow->lockBuffer_DEPRECATED(nativeWindow, buf);
    sp<android::GraphicBuffer> graphicBuffer = android::GraphicBuffer::from(buf);

    char* vaddr;
    graphicBuffer->lock(1, (void **)&vaddr);
    if (vaddr != nullptr) {
        memset(vaddr, 0x0, graphicBuffer->getWidth() * graphicBuffer->getHeight() * 4); /*to show video in osd hole...*/
    }
    graphicBuffer->unlock();
    graphicBuffer.clear();
    nativeWindow->queueBuffer_DEPRECATED(nativeWindow, buf);
#endif
}

NativeUI::~NativeUI()
{

}

sp<ANativeWindow> NativeUI::getNativeWindow() const
{
#ifndef __ANDROID_VNDK__
    return mSurface;
#else
    return nullptr;
#endif
}

void NativeUI::controlSurface(int zorder)
{
#ifndef __ANDROID_VNDK__
    auto transcation = android::SurfaceComposerClient::Transaction();

    transcation.setLayer(mSurfaceControl, zorder);
    transcation.setLayer(mSurfaceControlUi, zorder);

    transcation.apply();
#endif
}

void NativeUI::controlSurface(int left, int top, int right, int bottom)
{
#ifndef __ANDROID_VNDK__
    auto transcation = android::SurfaceComposerClient::Transaction();

    if (left >= 0 && top >= 0) {
        transcation.setPosition(mSurfaceControl, left, top);
    }

    if (right > left && bottom > top) {
        int width = right - left;
        int height = bottom - top;
        transcation.setSize(mSurfaceControl, width, height);
        transcation.setCrop_legacy(mSurfaceControl, android::Rect(width, height));
    }

    transcation.apply();
#endif
}

int NativeUI::getDefaultSurfaceWidth() const
{
    return mSurfaceWidth;
}

int NativeUI::getDefaultSurfaceHeight() const
{
    return mSurfaceHeight;
}


CommandProcessor::CommandProcessor(const std::string& prompt)
: mPrompt(prompt)
{

}

CommandProcessor::~CommandProcessor()
{

}

int CommandProcessor::setCommandVisitor(const std::function<Visitor>& visitor)
{
    mCommandVisitor = visitor;

    return 0;
}

int CommandProcessor::setInterrupter(const std::function<Interrupter>& interrupter)
{
    mInterrupter = interrupter;

    return 0;
}

int CommandProcessor::fetchAndProcessCommands()
{
    bool prompt = false;
    std::string lastCommand;

    for (;;) {
        if (mInterrupter()) {
            break;
        }

        if (tcgetpgrp(0) != getpid()) {
            usleep(10 * 1000);
            continue;
        }

        if (prompt) {
            prompt = false;
            fprintf(stderr, "%s", mPrompt.c_str());
        }

        struct pollfd fds;
        fds.fd = STDIN_FILENO;
        fds.events = POLL_IN;
        fds.revents = 0;
        int ret = ::poll(&fds, 1, 1000);
        if (ret < 0) {
            //printf("poll STDIN_FILENO failed! %d\n", -errno);
        } else if (ret > 0) {
            if (fds.revents & POLL_ERR) {
                ALOGE("poll error!");
                continue;
            } else if (!(fds.revents & POLL_IN)) {
                continue;
            }

            prompt = true;

            char buffer[100]{0};
            int len = ::read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len <= 0) {
                ALOGE("read failed! %d", -errno);
                continue;
            }
            buffer[len-1] = '\0';
            //printf("read buf:%s, size:%d\n", buffer, len);

            std::string buf(buffer);
            size_t b = 0;
            size_t e = buf.size();
            while (b < e) {
                if (isspace(buf[b])) ++b;
                else if (isspace(buf[e])) --e;
                else break;
            }

            if (b < e) {
                buf = buf.substr(b, e - b);
                lastCommand = buf;
            } else if (b == e && !lastCommand.empty()) {
                buf = lastCommand;
            } else {
                continue;
            }

            std::vector<std::string> args = split(buf);
            mCommandVisitor(args);
        }
    }

    return 0;
}

std::vector<std::string> CommandProcessor::split(const std::string& str)
{
    std::vector<std::string> result;
    std::string::size_type pos, prev = 0;
    std::string s;
    std::string cmdDelimiter = " ";
    std::string argDelimiter = ", ";

    pos = str.find_first_of(cmdDelimiter);

    if (pos == std::string::npos) {
        result.emplace_back(std::move(str));
        return result;
    }

    result.emplace_back(str, 0, pos);
    prev = pos + 1;

    while ((pos = str.find_first_of(argDelimiter, prev)) != std::string::npos) {
        if (pos > prev) {
            size_t b = prev;
            size_t e = pos;
            while (b < e) {
                if (isspace(str[b]) || isDelimiter(argDelimiter, str[b]))        ++b;
                else if (isspace(str[e]) || isDelimiter(argDelimiter, str[e]))   --e;
                else break;
            }

            if (b <= e) {
                result.emplace_back(str, b, e - b + 1);
            }
        }

        prev = pos + 1;
    }

    while (prev < str.size()) {
        if (isspace(str[prev]) || isDelimiter(argDelimiter, str[prev]))
            ++prev;
        else
            break;
    }

    if (prev < str.size())
        result.emplace_back(str, prev, str.size() - prev);

    return result;
}


}
