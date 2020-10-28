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

namespace aml_mp {
class Playback : public ISourceReceiver
{
public:
    explicit Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sp<ProgramInfo>& programInfo);
    ~Playback();
    void setANativeWindow(const sp<ANativeWindow>& window);
    int start();
    int stop();
    void signalQuit();
    virtual int writeData(const uint8_t* buffer, size_t size) override;
    void processCommand(const std::vector<std::string>& args);
    int setSubtitleDisplayWindow(int x, int y, int width, int height);

private:
    void eventCallback(Aml_MP_PlayerEventType eventType, int64_t param);

    const sp<ProgramInfo> mProgramInfo;
    const Aml_MP_DemuxId mDemuxId;
    AML_MP_PLAYER mPlayer = AML_MP_INVALID_HANDLE;

    Playback(const Playback&) = delete;
    Playback& operator= (const Playback&) = delete;
};

}





#endif
