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
#include <utils/AmlMpLog.h>
#include "AmlMpTestSupporter.h"
#include "TestUtils.h"
#include <Aml_MP/Aml_MP.h>
#include "source/Source.h"
#include "demux/AmlTsParser.h"
#include "ParserReceiver.h"
#include "Playback.h"
#include "DVRRecord.h"
#include "DVRPlayback.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

static const char* mName = LOG_TAG;

namespace aml_mp {
using namespace android;

AmlMpTestSupporter::AmlMpTestSupporter()
{
    ALOGI("AmlMpTestSupporter structure\n");
}

AmlMpTestSupporter::~AmlMpTestSupporter()
{
    MLOGI("%s", __FUNCTION__);
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
        MLOGE("create source failed!");
        return -1;
    }

    ret = mSource->initCheck();
    if (ret < 0) {
        MLOGE("source initCheck failed!");
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
            MLOGW("change source id to %d", sourceId);
        }
        Aml_MP_SetDemuxSource(demuxId, sourceId);
    }

    if (mSource->getFlags()&Source::kIsDVRSource) {
        mIsDVRPlayback = true;
        MLOGI("dvr playback");
        return 0;
    }

    mParser = new Parser(demuxId, mSource->getFlags()&Source::kIsHardwareSource, mSource->getFlags()&Source::kIsHardwareSource);
    mParser->setProgram(programNumber);
    if (mParser == nullptr) {
        MLOGE("create parser failed!");
        return -1;
    }

    mParserReceiver = new ParserReceiver(mParser);

    mSource->addSourceReceiver(mParserReceiver);

    ret = mSource->start();
    if (ret < 0) {
        MLOGE("source start failed!");
        return -1;
    }

    ret = mParser->open();
    if (ret < 0) {
        MLOGE("parser open failed!");
        return -1;
    }

    MLOGI("parsing...");
    ret = mParser->wait();
    if (ret < 0) {
        MLOGE("parser wait failed!");
        return -1;
    }

    MLOGI("parsed done!");
    mSource->removeSourceReceiver(mParserReceiver);
    mSource->restart();
    mParser->close();

    mProgramInfo = mParser->getProgramInfo();
    if (mProgramInfo == nullptr) {
        MLOGE("get programInfo failed!");
        return -1;
    }

    return ret;
}

int AmlMpTestSupporter::startPlay(PlayMode playMode)
{
    int ret = 0;
    mPlayMode = playMode;
    ALOGI(">>>> AmlMpTestSupporter startPlay\n");
    if (mIsDVRPlayback) {
        return startDVRPlayback();
    }

    Aml_MP_DemuxId demuxId = mParser->getDemuxId();

    Aml_MP_InputSourceType sourceType = AML_MP_INPUT_SOURCE_TS_MEMORY;
    if (mSource->getFlags() & Source::kIsHardwareSource) {
        sourceType = AML_MP_INPUT_SOURCE_TS_DEMOD;
    }

    if (mPlayback == nullptr) {
        MLOGI("sourceType:%d, scrambled:%d\n", sourceType, mProgramInfo->scrambled);
        mTestModule = mPlayback = new Playback(demuxId, sourceType, mProgramInfo->scrambled ? AML_MP_INPUT_STREAM_ENCRYPTED : AML_MP_INPUT_STREAM_NORMAL);
    }

    if (mEventCallback != nullptr) {
        mPlayback->registerEventCallback(mEventCallback, mUserData);
    }
    #ifdef ANDROID
    if (mNativeUI == nullptr) {
        mNativeUI = new NativeUI();
    }

    if (mDisplayParam.width < 0) {
        mDisplayParam.width = mNativeUI->getDefaultSurfaceWidth();
    }

    if (mDisplayParam.height < 0) {
        mDisplayParam.height = mNativeUI->getDefaultSurfaceHeight();
    }

    ret = mPlayback->setSubtitleDisplayWindow(mDisplayParam.width, 0, mDisplayParam.width, mDisplayParam.height);
    #endif

    if (!mDisplayParam.videoMode) {
    #ifdef ANDROID
        if (mDisplayParam.aNativeWindow) {
            mPlayback->setANativeWindow(mDisplayParam.aNativeWindow);
        } else {
            mNativeUI->controlSurface(
                    mDisplayParam.x,
                    mDisplayParam.y,
                    mDisplayParam.x + mDisplayParam.width,
                    mDisplayParam.y + mDisplayParam.height);

            mNativeUI->controlSurface(mDisplayParam.zorder);
            sp<ANativeWindow> window = mNativeUI->getNativeWindow();
            if (window == nullptr) {
                MLOGE("create native window failed!");
                return -1;
            }

            mPlayback->setANativeWindow(window);
        }
    #endif
    } else {
        setOsdBlank(1);
        mPlayback->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER, &mDisplayParam.zorder);
        mPlayback->setVideoWindow(mDisplayParam.x, mDisplayParam.y, mDisplayParam.width, mDisplayParam.height);
        MLOGI("x:%d y:%d width:%d height:%d\n", mDisplayParam.x, mDisplayParam.y, mDisplayParam.width, mDisplayParam.height);
    }
    ret = mPlayback->start(mProgramInfo, mPlayMode);
    if (ret < 0) {
        MLOGE("playback start failed!");
        return -1;
    }

    mSource->addSourceReceiver(mPlayback);
    ALOGI("<<<< AmlMpTestSupporter startPlay\n");
    return 0;
}

int AmlMpTestSupporter::startRecord()
{
    ALOGI("enter startRecord\n");
    Aml_MP_DemuxId demuxId = mParser->getDemuxId();
    mTestModule = mRecorder = new DVRRecord(mCryptoMode, demuxId, mProgramInfo);
    ALOGI("before mRecorder start\n");
    int ret = mRecorder->start();
    if (ret < 0) {
        MLOGE("start recorder failed!");
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
    #ifdef ANDROID
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
        MLOGE("create native window failed!");
        return -1;
    }

    mDVRPlayback->setANativeWindow(window);
    #endif
    ret = mDVRPlayback->start();
    if (ret < 0) {
        MLOGE("DVR playback start failed!");
        return -1;
    }

    return 0;
}

int AmlMpTestSupporter::stop()
{
    MLOGI("stopping...");

    if (mSource != nullptr) {
        mSource->stop();
        mSource.clear();
    }

    if (mParser != nullptr) {
        mParser->close();
        mParser.clear();
    }

    if (mPlayback != nullptr) {
        mPlayback->stop();
    }

    if (mRecorder != nullptr) {
        mRecorder->stop();
    }

    if (mDVRPlayback != nullptr) {
        mDVRPlayback->stop();
    }

    if (mDisplayParam.videoMode) {
        setOsdBlank(0);
    }

    MLOGI("stop end!");
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
        MLOGE("pthread_sigmask failed! %d", ret);
        return -1;
    }

    mSignalHandleThread = std::thread([blockSet, oldMask, this] {
        int signo;
        int err;

        for (;;) {
            err = sigwait(&blockSet, &signo);
            if (err != 0) {
                MLOGE("sigwait error! %d", err);
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
                MLOGE("restore sigmask failed!");
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
    #ifdef ANDORID
        mNativeUI->controlSurface(-2);
    #endif
    } else if (cmd == "video") {
    #ifdef ANDORID
        mNativeUI->controlSurface(0);
    #endif
    } else if (cmd == "zorder") {
    #ifdef ANDORID
        if (args.size() == 2) {
            int zorder = std::stoi(args[1]);
            mNativeUI->controlSurface(zorder);
        }
    #endif
    } else if (cmd == "resize") {
        int x = -1;
        int y = -1;
        int width = -1;
        int height = -1;
    #ifdef ANDROID
        if (args.size() == 5) {
            x = std::stoi(args[1]);
            y = std::stoi(args[2]);
            width = std::stoi(args[3]);
            height = std::stoi(args[4]);
            mNativeUI->controlSurface(x, y, width, height);
        }
    #endif
    } else {
        mTestModule->processCommand(args);
    }

    return ret;
}

void AmlMpTestSupporter::signalQuit()
{
    MLOGI("received SIGINT, %s", __FUNCTION__);

    mQuitPending = true;

    if (mSource) mSource->signalQuit();
    if (mParser) mParser->signalQuit();
    if (mPlayback) mPlayback->signalQuit();
}

void AmlMpTestSupporter::setDisplayParam(const DisplayParam& param)
{
    mDisplayParam = param;
    MLOGI("x:%d, y:%d, width:%d, height:%d, zorder:%d, videoMode:%d\n",
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
    MLOGI("setOsdBlank: %d", blank);
    int ret = 0;
    #if ANDROID_PLATFORM_SDK_VERSION == 30
        ret += writeNode("/sys/kernel/debug/dri/0/vpu/blank", blank);
    #else
        ret += writeNode("/sys/class/graphics/fb0/osd_display_debug", blank);
        ret += writeNode("/sys/class/graphics/fb0/blank", blank);
    #endif
    return ret;
}

sptr<TestModule> AmlMpTestSupporter::getPlayback() const
{
    return mPlayback;
}

}
