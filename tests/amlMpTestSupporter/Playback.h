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
#include <system/window.h>
#include <Aml_MP/Aml_MP.h>
#include "TestModule.h"
#include "AmlMpTestSupporter.h"
#include <utils/RefBase.h>

namespace aml_mp {

// for iptv (encrypt) playback, dvb (encrypt) playback
class Playback : public TestModule, public ISourceReceiver
{
public:
    using PlayMode = AmlMpTestSupporter::PlayMode;

    Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sptr<ProgramInfo>& programInfo);
    ~Playback();
    void setANativeWindow(const android::sp<ANativeWindow>& window);
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int start(PlayMode mode);
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

private:
    const sptr<ProgramInfo> mProgramInfo;
    const Aml_MP_DemuxId mDemuxId;
    AML_MP_PLAYER mPlayer = AML_MP_INVALID_HANDLE;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    bool mIsDVBSource = false;

    AML_MP_CASSESSION mCasSession = nullptr;
    AML_MP_SECMEM mSecMem = nullptr;

    PlayMode mPlayMode = PlayMode::START_ALL_STOP_ALL;

private:
    Playback(const Playback&) = delete;
    Playback& operator= (const Playback&) = delete;
};

}





#endif
