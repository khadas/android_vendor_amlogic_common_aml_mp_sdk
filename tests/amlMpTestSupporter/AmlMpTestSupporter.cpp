/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpTestSupporter"
#include <utils/Log.h>
#include "AmlMpTestSupporter.h"
#include "TestUtils.h"
#include <Aml_MP/Aml_MP.h>
#include "source/Source.h"
#include "Parser.h"
#include "Playback.h"

namespace aml_mp {
using namespace android;

AmlMpTestSupporter::AmlMpTestSupporter()
{

}

AmlMpTestSupporter::~AmlMpTestSupporter()
{
    ALOGI("%s", __FUNCTION__);
}

void AmlMpTestSupporter::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    mEventCallback = cb;
    mUserData = userData;
}

int AmlMpTestSupporter::setDataSource(const std::string& url)
{
    mUrl = url;

    return 0;
}

int AmlMpTestSupporter::prepare()
{
    int ret = 0;

    mSource = Source::create(mUrl.c_str());
    if (mSource == nullptr) {
        ALOGE("create source failed!");
        return -1;
    }

    ret = mSource->initCheck();
    if (ret < 0) {
        ALOGE("source initCheck failed!");
        return -1;
    }

    int programNumber = mSource->getProgramNumber();
    mParser = new Parser(programNumber, mSource->getFlags()&Source::kIsHardwareSource);
    if (mParser == nullptr) {
        ALOGE("create parser failed!");
        return -1;
    }

    mSource->addSourceReceiver(mParser);

    ret = mSource->start();
    if (ret < 0) {
        ALOGE("source start failed!");
        return -1;
    }

    ret = mParser->open();
    if (ret < 0) {
        ALOGE("parser open failed!");
        return -1;
    }

    ret = mParser->wait();
    if (ret < 0) {
        ALOGE("parser wait failed!");
        return -1;
    }

    mProgramInfo = mParser->getProgramInfo();
    if (mProgramInfo == nullptr) {
        ALOGE("get programInfo failed!");
        return -1;
    }

    return ret;
}

int AmlMpTestSupporter::startPlay(PlayMode playMode)
{
    mPlayMode = playMode;

    mSource->removeSourceReceiver(mParser);
    mSource->restart();

    Aml_MP_DemuxId demuxId = mParser->getDemuxId();

    Aml_MP_InputSourceType sourceType = AML_MP_INPUT_SOURCE_TS_MEMORY;
    if (mSource->getFlags() & Source::kIsHardwareSource) {
        sourceType = AML_MP_INPUT_SOURCE_TS_DEMOD;
    }

    if (startPlayback(demuxId, sourceType) < 0) {
        return -1;
    }

    return 0;
}

int AmlMpTestSupporter::startPlayback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType)
{
    mTestModule = mPlayback = new Playback(demuxId, sourceType, mProgramInfo);

    if (mEventCallback != nullptr) {
        mPlayback->registerEventCallback(mEventCallback, mUserData);
    }

    mNativeUI = new NativeUI();
    int width = mNativeUI->getSurfaceWidth();
    int height = mNativeUI->getSurfaceHeight();
    int ret = mPlayback->setSubtitleDisplayWindow(width, 0, width, height);
    sp<ANativeWindow> window = mNativeUI->getNativeWindow();
    if (window == nullptr) {
        ALOGE("create native window failed!");
        return -1;
    }

    mPlayback->setANativeWindow(window);
    ret = mPlayback->start(mPlayMode);
    if (ret < 0) {
        ALOGE("playback start failed!");
        return -1;
    }

    //parser->linkNextReceiver(playback);
    mSource->addSourceReceiver(mPlayback);

    return 0;
}

int AmlMpTestSupporter::stop()
{
    if (mSource != nullptr) mSource->stop();
    if (mParser != nullptr) mParser->close();
    if (mPlayback != nullptr) mPlayback->stop();

    return 0;
}

bool AmlMpTestSupporter::hasVideo() const
{
    if (mProgramInfo) {
        return mProgramInfo->videoCodec != AML_MP_CODEC_UNKNOWN && mProgramInfo->videoPid != AML_MP_INVALID_PID;
    } else {
        return false;
    }
}

int AmlMpTestSupporter::installSignalHandler()
{
    int ret = 0;
    sigset_t blockSet, oldMask;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    ret = pthread_sigmask(SIG_SETMASK, &blockSet, &oldMask);
    if (ret != 0) {
        ALOGE("pthread_sigmask failed! %d", ret);
        return -1;
    }

    mSignalHandleThread = std::thread([blockSet, oldMask, this] {
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
                signalQuit();
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
    mSignalHandleThread.detach();

    return ret;
}

int AmlMpTestSupporter::fetchAndProcessCommands()
{
    char promptBuf[50];
    snprintf(promptBuf, sizeof(promptBuf), "AmlMpTestSupporter >");

    mCommandProcessor = new CommandProcessor(promptBuf);
    if (mCommandProcessor == nullptr) {
        return -1;
    }

    mCommandProcessor->setCommandVisitor(std::bind(&AmlMpTestSupporter::processCommand, this, std::placeholders::_1));
    mCommandProcessor->setInterrupter([this]() {return mQuitPending;});
    return mCommandProcessor->fetchAndProcessCommands();
}

bool AmlMpTestSupporter::processCommand(const std::vector<std::string>& args)
{
    bool ret = true;
    std::string cmd = *args.begin();

    if (cmd == "osd") {
        mNativeUI->controlSurface(-2);
    } else if (cmd == "video") {
        mNativeUI->controlSurface(0);
    } else if (cmd == "zorder") {
        if (args.size() == 2) {
            int zorder = std::stoi(args[1]);
            mNativeUI->controlSurface(zorder);
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
            mNativeUI->controlSurface(x, y, width, height);
        }
    } else {
        mTestModule->processCommand(args);
    }

    return ret;
}

void AmlMpTestSupporter::signalQuit()
{
    ALOGI("%s", __FUNCTION__);

    mQuitPending = true;

    if (mSource) mSource->signalQuit();
    if (mParser) mParser->signalQuit();
    if (mPlayback) mPlayback->signalQuit();
}

}
