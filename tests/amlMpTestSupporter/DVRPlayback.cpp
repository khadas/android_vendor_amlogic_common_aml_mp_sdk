/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_DVRPlayback"
#include <utils/Log.h>
#include "DVRPlayback.h"

namespace aml_mp {
DVRPlayback::DVRPlayback(Aml_MP_DemuxId demuxId, const sp<ProgramInfo>& programInfo)
{

}

DVRPlayback::~DVRPlayback()
{

}

void DVRPlayback::setANativeWindow(const sp<ANativeWindow>& window)
{

}

void DVRPlayback::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{

}

int DVRPlayback::start()
{
    return 0;
}

int DVRPlayback::stop()
{
    return 0;
}

void DVRPlayback::signalQuit()
{

}

int DVRPlayback::writeData(const uint8_t* buffer, size_t size)
{
    return 0;
}

const TestModule::Command* DVRPlayback::getCommandTable() const
{
    return nullptr;
}

void* DVRPlayback::getCommandHandle() const
{
    return nullptr;
}



}
