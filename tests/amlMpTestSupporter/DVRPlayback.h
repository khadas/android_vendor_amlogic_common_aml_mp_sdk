/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "Parser.h"
#include "TestModule.h"
#include <system/window.h>
#include <Aml_MP/Aml_MP.h>

namespace aml_mp {

// for dvr (encrypt) playback
class DVRPlayback : public TestModule, public ISourceReceiver
{
public:
    DVRPlayback(Aml_MP_DemuxId demuxId, const sptr<ProgramInfo>& programInfo);
    ~DVRPlayback();
    void setANativeWindow(const sptr<ANativeWindow>& window);
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int start();
    int stop();
    void signalQuit();
    virtual int writeData(const uint8_t* buffer, size_t size) override;

protected:
    virtual const Command* getCommandTable() const override;
    virtual void* getCommandHandle() const override;

private:
    DVRPlayback(const DVRPlayback&) = delete;
    DVRPlayback& operator=(const DVRPlayback&) = delete;
};
}
