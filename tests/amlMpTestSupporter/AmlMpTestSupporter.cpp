/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_TestSupporter"
#include <utils/Log.h>
#include "AmlMpTestSupporter.h"
#include "TestUtils.h"
#include <Aml_MP/Aml_MP.h>
#include "source/Source.h"
#include "Parser.h"
#include "Playback.h"
#include "DVRRecord.h"
#include "DVRPlayback.h"

#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

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

int AmlMpTestSupporter::prepare(bool cryptoMode)
{
    int ret = 0;

    mCryptoMode = cryptoMode;

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

    Aml_MP_DemuxId demuxId = mSource->getDemuxId();
    int programNumber = mSource->getProgramNumber();
    Aml_MP_DemuxSource sourceId = mSource->getSourceId();

    Aml_MP_Initialize();

    //set default demux source
    if (mSource->getFlags()&Source::kIsHardwareSource) {
        Aml_MP_SetDemuxSource(demuxId, sourceId);
    } else {
        if (sourceId < AML_MP_DEMUX_SOURCE_DMA0) {
            sourceId = Aml_MP_DemuxSource(sourceId + AML_MP_DEMUX_SOURCE_DMA0);
            ALOGW("change source id to %d", sourceId);
        }
        Aml_MP_SetDemuxSource(demuxId, sourceId);
    }

    if (mSource->getFlags()&Source::kIsDVRSource) {
        mIsDVRPlayback = true;
        ALOGI("dvr playback");
        return 0;
    }

    mParser = new Parser(demuxId, programNumber, mSource->getFlags()&Source::kIsHardwareSource);
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

    ALOGI("parsing...");
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
    int ret = 0;
    mPlayMode = playMode;

    if (mIsDVRPlayback) {
        return startDVRPlayback();
    }

    mSource->removeSourceReceiver(mParser);
    mSource->restart();

    Aml_MP_DemuxId demuxId = mParser->getDemuxId();

    Aml_MP_InputSourceType sourceType = AML_MP_INPUT_SOURCE_TS_MEMORY;
    if (mSource->getFlags() & Source::kIsHardwareSource) {
        sourceType = AML_MP_INPUT_SOURCE_TS_DEMOD;
    }

    mTestModule = mPlayback = new Playback(demuxId, sourceType, mProgramInfo);

    if (mEventCallback != nullptr) {
        mPlayback->registerEventCallback(mEventCallback, mUserData);
    }

    mNativeUI = new NativeUI();
    if (mDisplayParam.width < 0) {
        mDisplayParam.width = mNativeUI->getDefaultSurfaceWidth();
    }

    if (mDisplayParam.height < 0) {
        mDisplayParam.height = mNativeUI->getDefaultSurfaceHeight();
    }

    ret = mPlayback->setSubtitleDisplayWindow(mDisplayParam.width, 0, mDisplayParam.width, mDisplayParam.height);

    if (!mDisplayParam.videoMode) {
        mNativeUI->controlSurface(
                mDisplayParam.x,
                mDisplayParam.y,
                mDisplayParam.x + mDisplayParam.width,
                mDisplayParam.y + mDisplayParam.height);
        mNativeUI->controlSurface(mDisplayParam.zorder);
        sp<ANativeWindow> window = mNativeUI->getNativeWindow();
        if (window == nullptr) {
            ALOGE("create native window failed!");
            return -1;
        }

        mPlayback->setANativeWindow(window);
    } else {
        setOsdBlank(1);
        mPlayback->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER, &mDisplayParam.zorder);
        mPlayback->setVideoWindow(mDisplayParam.x, mDisplayParam.y, mDisplayParam.width, mDisplayParam.height);
    }

    ret = mPlayback->start(mPlayMode);
    if (ret < 0) {
        ALOGE("playback start failed!");
        return -1;
    }

    mSource->addSourceReceiver(mPlayback);
    return 0;
}

int AmlMpTestSupporter::startRecord()
{
    mSource->removeSourceReceiver(mParser);
    mSource->restart();

    Aml_MP_DemuxId demuxId = mParser->getDemuxId();
    mTestModule = mRecorder = new DVRRecord(mCryptoMode, demuxId, mProgramInfo);

    int ret = mRecorder->start();
    if (ret < 0) {
        ALOGE("start recorder failed!");
        return -1;
    }

    mSource->addSourceReceiver(mRecorder);
    return 0;
}

int AmlMpTestSupporter::startDVRPlayback()
{
    MLOG();
    int ret = 0;

    Aml_MP_DemuxId demuxId = mSource->getDemuxId();
    mTestModule = mDVRPlayback = new DVRPlayback(mUrl, mCryptoMode, demuxId);

    if (mEventCallback != nullptr) {
        mDVRPlayback->registerEventCallback(mEventCallback, mUserData);
    }

    mNativeUI = new NativeUI();
    if (mDisplayParam.width < 0) {
        mDisplayParam.width = mNativeUI->getDefaultSurfaceWidth();
    }

    if (mDisplayParam.height < 0) {
        mDisplayParam.height = mNativeUI->getDefaultSurfaceHeight();
    }

    mNativeUI->controlSurface(
            mDisplayParam.x,
            mDisplayParam.y,
            mDisplayParam.x + mDisplayParam.width,
            mDisplayParam.y + mDisplayParam.height);
    mNativeUI->controlSurface(mDisplayParam.zorder);
    sp<ANativeWindow> window = mNativeUI->getNativeWindow();
    if (window == nullptr) {
        ALOGE("create native window failed!");
        return -1;
    }

    mDVRPlayback->setANativeWindow(window);

    ret = mDVRPlayback->start();
    if (ret < 0) {
        ALOGE("DVR playback start failed!");
        return -1;
    }

    return 0;
}

int AmlMpTestSupporter::stop()
{
    ALOGI("stopping...");

    if (mSource != nullptr) mSource->stop();
    if (mParser != nullptr) mParser->close();
    if (mPlayback != nullptr) mPlayback->stop();
    if (mRecorder != nullptr) mRecorder->stop();
    if (mDVRPlayback != nullptr) mDVRPlayback->stop();
    if (mDisplayParam.videoMode) {
        setOsdBlank(0);
    }

    ALOGI("stop end!");
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
    ALOGI("received SIGINT, %s", __FUNCTION__);

    mQuitPending = true;

    if (mSource) mSource->signalQuit();
    if (mParser) mParser->signalQuit();
    if (mPlayback) mPlayback->signalQuit();
}

void AmlMpTestSupporter::setDisplayParam(const DisplayParam& param)
{
    mDisplayParam = param;
    ALOGI("x:%d, y:%d, width:%d, height:%d, zorder:%d, videoMode:%d",
            mDisplayParam.x, mDisplayParam.y, mDisplayParam.width, mDisplayParam.height, mDisplayParam.zorder, mDisplayParam.videoMode);
}

int AmlMpTestSupporter::setOsdBlank(int blank)
{
    auto writeNode = [] (const char *path, int value) -> int {
        int fd;
        char cmd[128] = {0};
        fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (fd >= 0)
        {
            sprintf(cmd,"%d",value);
            write (fd,cmd,strlen(cmd));
            close(fd);
            return 0;
        }
        return -1;
    };
    ALOGI("setOsdBlank: %d", blank);
    int ret = 0;
    #if ANDROID_PLATFORM_SDK_VERSION == 30
        ret += writeNode("/sys/kernel/debug/dri/0/vpu/blank", blank);
    #else
        ret += writeNode("/sys/class/graphics/fb0/osd_display_debug", blank);
        ret += writeNode("/sys/class/graphics/fb0/blank", blank);
    #endif
    return ret;
}

}
