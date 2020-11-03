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

#include "Parser.h"
#include <system/window.h>
#include <Aml_MP/Aml_MP.h>
#include "TestModule.h"
#include "AmlMpTestSupporter.h"

namespace aml_mp {

// for iptv (encrypt) playback, dvb (encrypt) playback
class Playback : public TestModule, public ISourceReceiver
{
public:
    using PlayMode = AmlMpTestSupporter::PlayMode;

    Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sp<ProgramInfo>& programInfo);
    ~Playback();
    void setANativeWindow(const sp<ANativeWindow>& window);
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int start(PlayMode mode);
    int stop();
    void signalQuit();
    virtual int writeData(const uint8_t* buffer, size_t size) override;
    int setSubtitleDisplayWindow(int x, int y, int width, int height);

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
    const sp<ProgramInfo> mProgramInfo;
    const Aml_MP_DemuxId mDemuxId;
    AML_MP_PLAYER mPlayer = AML_MP_INVALID_HANDLE;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    bool mIsDVBSource = false;

    AML_MP_CASSESSION mCasSession = nullptr;

    PlayMode mPlayMode = PlayMode::START_ALL_STOP_ALL;

private:
    Playback(const Playback&) = delete;
    Playback& operator= (const Playback&) = delete;
};

}





#endif
