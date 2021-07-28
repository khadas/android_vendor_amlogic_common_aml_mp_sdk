/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _PLAYBACK_H_
#define _PLAYBACK_H_

#include "demux/AmlTsParser.h"
#include "source/Source.h"
#ifdef ANDROID
#include <system/window.h>
#endif
#include <Aml_MP/Aml_MP.h>
#include "TestModule.h"
#include "AmlMpTestSupporter.h"
#include <utils/RefBase.h>

namespace aml_mp {
class CasPlugin : public AmlMpRefBase
{
public:
    CasPlugin(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sptr<ProgramInfo>& programInfo);
    ~CasPlugin();

    int start();
    int stop();

    AML_MP_CASSESSION casSession() const {
        return mCasSession;
    }

    Aml_MP_InputStreamType inputStreamType() const {
        return mInputStreamType;
    }

private:
    int startDVBDescrambling();
    int startIPTVDescrambling();
    int stopDVBDescrambling();
    int stopIPTVDescrambling();

    const Aml_MP_DemuxId mDemuxId{AML_MP_DEMUX_ID_DEFAULT};
    const Aml_MP_InputSourceType mSourceType;
    const sptr<ProgramInfo> mProgramInfo;

    Aml_MP_InputStreamType mInputStreamType{AML_MP_INPUT_STREAM_ENCRYPTED};

    AML_MP_CASSESSION mCasSession = nullptr;
    AML_MP_SECMEM mSecMem = nullptr;

    CasPlugin(const CasPlugin&) = delete;
    CasPlugin& operator=(const CasPlugin&) = delete;
};

// for iptv (encrypt) playback, dvb (encrypt) playback
class Playback : public TestModule, public ISourceReceiver
{
public:
    using PlayMode = AmlMpTestSupporter::PlayMode;

    Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, Aml_MP_InputStreamType streamType);
    ~Playback();
#ifdef ANDROID
    void setANativeWindow(const android::sp<ANativeWindow>& window);
#endif
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int start(const sptr<ProgramInfo>& programInfo, AML_MP_CASSESSION casSession, PlayMode mode);
    int stop();
    void signalQuit();
    virtual int writeData(const uint8_t* buffer, size_t size) override;
    int setSubtitleDisplayWindow(int x, int y, int width, int height);
    int setVideoWindow(int x, int y, int width, int height);
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter);

protected:
    const Command* getCommandTable() const override;
    void* getCommandHandle() const override;

private:
    int startDVBDescrambling();
    int stopDVBDescrambling();

    int startIPTVDescrambling();
    int stopIPTVDescrambling();

    bool setAudioParams();
    bool setVideoParams();
    bool setSubtitleParams();

    void printStreamsInfo();

private:
    void eventCallback(Aml_MP_PlayerEventType eventType, int64_t param);

    const Aml_MP_DemuxId mDemuxId;
    sptr<ProgramInfo> mProgramInfo;
    AML_MP_PLAYER mPlayer = AML_MP_INVALID_HANDLE;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    AML_MP_CASSESSION mCasSession = nullptr;

    PlayMode mPlayMode = PlayMode::START_ALL_STOP_ALL;

private:
    Playback(const Playback&) = delete;
    Playback& operator= (const Playback&) = delete;
};

}

#endif
