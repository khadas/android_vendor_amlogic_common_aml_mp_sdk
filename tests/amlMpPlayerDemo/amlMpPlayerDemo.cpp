/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo"
#include <utils/Log.h>
#include <Aml_MP/Aml_MP.h>
#include <libgen.h>
#include <getopt.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <string>
#include "Source.h"
#include "Parser.h"
#include "Playback.h"
#include <signal.h>
#include <thread>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>

#include <media/stagefright/foundation/ADebug.h>

using namespace android;
using namespace aml_mp;

///////////////////////////////////////////////////////////////////////////////
static void usage();
struct NativeUI;
static std::vector<std::string> split(const std::string& str);
static bool processCommand(const std::vector<std::string>& args, NativeUI* ui);

///////////////////////////////////////////////////////////////////////////////
struct NativeUI {
    NativeUI();
    ~NativeUI();
    sp<ANativeWindow> getNativeWindow() const;
    void controlSurface(int zorder);
    void controlSurface(int left, int top, int right, int bottom);
    int getSurfaceWidth();
    int getSurfaceHeight();

private:
    sp<android::SurfaceComposerClient> mComposerClient;
    sp<android::SurfaceControl> mSurfaceControl;
    sp<android::Surface> mSurface;

    sp<android::SurfaceControl> mSurfaceControlUi;
    sp<android::Surface> mSurfaceUi;

    int mDisplayWidth = 1920;
    int mDisplayHeight = 1080;

    int mSurfaceWidth = 1920;
    int mSurfaceHeight = 1080;

    static const int UI_LAYER = 1;

    NativeUI(const NativeUI&) = delete;
    NativeUI& operator=(const NativeUI&);
};

///////////////////////////////////////////////////////////////////////////////
static bool g_quit = false;

int main(int argc, char *argv[])
{
    int ret = 0;
    const char* url = nullptr;
    const char* optstring = "h";

    int opt = -1;
    while ((opt = getopt(argc, argv, optstring)) > 0) {
        switch (opt) {
        case 'h':
        case '?':
        default:
            usage();
            exit(0);
            break;
        }
    }

    if (optind < argc) {
        url = argv[argc-1];
        printf("url: %s\n", url);
    } else {
        usage();
        exit(0);
    }

    sp<Source> source;
    sp<Parser> parser;
    sp<Playback> playback;

    sigset_t blockSet, oldMask;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    ret = pthread_sigmask(SIG_SETMASK, &blockSet, &oldMask);
    if (ret != 0) {
        ALOGE("pthread_sigmask failed! %d", ret);
        return -1;
    }

    std::thread signalHandlerThread([blockSet, oldMask, &source, &parser, &playback] {
        int signo;
        int err;

        for (;;) {
            err = sigwait(&blockSet, &signo);
            if (err != 0) {
                ALOGE("sigwait error! %d", err);
                exit(0);
            }

            printf("%s\n", strsignal(signo));

            switch (signo) {
            case SIGINT:
            {
                g_quit = true;

                if (source) source->signalQuit();
                if (parser) parser->signalQuit();
                if (playback) playback->signalQuit();
            }
            break;

            default:
                break;
            }

            if (pthread_sigmask(SIG_SETMASK, &oldMask, nullptr) != 0) {
                ALOGE("restore sigmask failed!");
            }
        }
    });
    signalHandlerThread.detach();

    source = Source::create(url);
    if (source == nullptr) {
        ALOGE("create source failed!");
        return -1;
    }

    ret = source->initCheck();
    if (ret < 0) {
        ALOGE("source initCheck failed!");
        return -1;
    }

    int programNumber = source->getProgramNumber();
    parser = new Parser(programNumber, source->getFlags()&Source::kIsHardwareSource);
    if (parser == nullptr) {
        ALOGE("create parser failed!");
        return -1;
    }

    source->addSourceReceiver(parser);

    ret = source->start();
    if (ret < 0) {
        ALOGE("source start failed!");
        return -1;
    }

    ret = parser->open();
    if (ret < 0) {
        ALOGE("parser open failed!");
        return -1;
    }

    ret = parser->wait();
    if (ret < 0) {
        ALOGE("parser wait failed!");
        return -1;
    }

    sp<ProgramInfo> programInfo = parser->getProgramInfo();
    if (programInfo == nullptr) {
        ALOGE("get programInfo failed!");
        return -1;
    }

    source->removeSourceReceiver(parser);
    source->restart();

    Aml_MP_DemuxId demuxId = parser->getDemuxId();

    Aml_MP_InputSourceType sourceType = AML_MP_INPUT_SOURCE_TS_MEMORY;
    if (source->getFlags() & Source::kIsHardwareSource) {
        sourceType = AML_MP_INPUT_SOURCE_TS_DEMOD;
    }

    playback = new Playback(demuxId, sourceType, programInfo);

    NativeUI nativeUi;
    int width = nativeUi.getSurfaceWidth();
    int height = nativeUi.getSurfaceHeight();
    ret = playback->setSubtitleDisplayWindow(width, 0, width, height);
    sp<ANativeWindow> window = nativeUi.getNativeWindow();
    if (window == nullptr) {
        ALOGE("create native window failed!");
        return -1;
    }

    playback->setANativeWindow(window);
    ret = playback->start();
    if (ret < 0) {
        ALOGE("playback start failed!");
        return -1;
    }

    //parser->linkNextReceiver(playback);
    source->addSourceReceiver(playback);

    bool prompt = false;
    char promptBuf[50];
    snprintf(promptBuf, sizeof(promptBuf), "%s >", basename(argv[0]));

    std::string lastCommand;

    while (!g_quit) {
        if (tcgetpgrp(0) != getpid()) {
            usleep(10 * 1000);
            continue;
        }

        if (prompt) {
            prompt = false;
            fprintf(stderr, "%s", promptBuf);
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
            if (processCommand(args, &nativeUi)) {
                continue;
            }

            if (playback) {
                playback->processCommand(args);
            }
        }
    }

    if (source != nullptr) source->stop();
    if (parser != nullptr) parser->close();
    if (playback != nullptr) playback->stop();

    printf("%s exited!\n", basename(argv[0]));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void usage()
{
    printf("AmlMpPlayerDemo <url>\n"
            "url format:\n"
            "    udp://ip:port?<program number>\n"
            "    dvbc://demux id>?<program number>\n"
            "    dvbt://demux id>?<program number>\n"
            );
}

///////////////////////////////////////////////////////////////////////////////
static bool isDelimiter(const std::string& delimiters, char c)
{
    return delimiters.find(c) != std::string::npos;
}

static std::vector<std::string> split(const std::string& str)
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
                else                        break;
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

static bool processCommand(const std::vector<std::string>& args, NativeUI* ui)
{
    bool ret = true;
    std::string cmd = *args.begin();

    if (cmd == "osd") {
        ui->controlSurface(-2);
    } else if (cmd == "video") {
        ui->controlSurface(0);
    } else if (cmd == "zorder") {
        if (args.size() == 1) {
            int zorder = std::stoi(args[0]);
            ui->controlSurface(zorder);
        }
    } else if (cmd == "resize") {
        int x = -1;
        int y = -1;
        int width = -1;
        int height = -1;

        if (args.size() == 5) {
            x = std::stoi(args[1]);
            y = std::stoi(args[2]);
            width = std::stoi(args[3]);
            height = std::stoi(args[4]);
            ui->controlSurface(x, y, width, height);
        }
    } else {
        ret = false;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
NativeUI::NativeUI()
{
    mComposerClient = new android::SurfaceComposerClient;
    CHECK_EQ(mComposerClient->initCheck(), OK);

    sp<android::IBinder> displayToken  = nullptr;
    displayToken = mComposerClient->getInternalDisplayToken();

    android::DisplayInfo displayInfo;
    CHECK_EQ(OK, mComposerClient->getDisplayInfo(displayToken, &displayInfo));

    mDisplayWidth = displayInfo.w;
    mDisplayHeight = displayInfo.h;

    mSurfaceWidth = mDisplayWidth >> 1;
    mSurfaceHeight = mDisplayHeight >> 1;
    ALOGI("mSurfaceWidth: %d, mSurfaceHeight: %d", mSurfaceWidth, mSurfaceHeight);

    mSurfaceControl = mComposerClient->createSurface(android::String8("AmlMpPlayerDemo"), mSurfaceWidth, mSurfaceHeight, android::PIXEL_FORMAT_RGB_565, 0);
    CHECK(mSurfaceControl->isValid());
    mSurface = mSurfaceControl->getSurface();

    android::SurfaceComposerClient::Transaction()
        .setFlags(mSurfaceControl, android::layer_state_t::eLayerOpaque, android::layer_state_t::eLayerOpaque)
        .setCrop_legacy(mSurfaceControl, android::Rect(mSurfaceWidth, mSurfaceHeight))
        .show(mSurfaceControl)
        .apply();

    mSurfaceControlUi = mComposerClient->createSurface(android::String8("AmlMpPlayerDemo-ui"), mDisplayWidth, mDisplayHeight, android::PIXEL_FORMAT_RGBA_8888, 0);
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
}

NativeUI::~NativeUI()
{

}

sp<ANativeWindow> NativeUI::getNativeWindow() const
{
    return mSurface;
}

void NativeUI::controlSurface(int zorder)
{
    auto transcation = android::SurfaceComposerClient::Transaction();

    transcation.setLayer(mSurfaceControl, zorder);
    transcation.setLayer(mSurfaceControlUi, zorder);

    transcation.apply();

}

void NativeUI::controlSurface(int left, int top, int right, int bottom)
{
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
}

int NativeUI::getSurfaceWidth() {
    return mSurfaceWidth;
}

int NativeUI::getSurfaceHeight() {
    return mSurfaceHeight;
}
