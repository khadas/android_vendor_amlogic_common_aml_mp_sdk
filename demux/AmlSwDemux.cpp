/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlSwDemux"
#include <utils/Log.h>
#include "AmlSwDemux.h"
#include <AM_MPP/AmlHalPlayer.h>
#include <utils/AmlMpUtils.h>

namespace aml_mp {

AmlSwDemux::AmlSwDemux()
{
}

AmlSwDemux::~AmlSwDemux()
{
    close();
}

int AmlSwDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId)
{
    if (isHardwareSource) {
        ALOGE("swdemux don't support hw source!!!");
        return -1;
    }

    mDemuxHandle = aml::AML_DMX_OpenDemux(demuxId);
    if (mDemuxHandle == nullptr) {
        ALOGE("open swdemux failed!");
        return -1;
    } else {
        ALOGE("mDemuxHandle:%p", mDemuxHandle);
    }

    return 0;
}

int AmlSwDemux::close()
{
    if (mDemuxHandle != AML_MP_INVALID_HANDLE) {
        aml::AML_DMX_CloseDemux(mDemuxHandle);
        mDemuxHandle = AML_MP_INVALID_HANDLE;
    }

    return 0;
}

int AmlSwDemux::start()
{
    int ret = aml::AML_DMX_StartDemux(mDemuxHandle);
    if (ret < 0) {
        ALOGE("start swdemux failed! %d", ret);
    }

    return ret;
}

int AmlSwDemux::stop()
{
    RETURN_IF(0, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_StopDemux(mDemuxHandle);
}

int AmlSwDemux::flush()
{
    RETURN_IF(0, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_FlushDemux(mDemuxHandle);
}

void* AmlSwDemux::createChannel(int pid)
{
    RETURN_IF(AML_INVALID_HANDLE, mDemuxHandle == AML_MP_INVALID_HANDLE);

    void* channel = aml::AML_DMX_CreateChannel(mDemuxHandle, pid);
    return channel;
}

int AmlSwDemux::destroyChannel(void* channel)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_DestroyChannel(mDemuxHandle, channel);
}

int AmlSwDemux::openChannel(void* channel)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_OpenChannel(mDemuxHandle, channel);
}

int AmlSwDemux::closeChannel(void* channel)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_CloseChannel(mDemuxHandle, channel);
}

void* AmlSwDemux::createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData)
{
    RETURN_IF(AML_INVALID_HANDLE, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_CreateFilter(mDemuxHandle, (aml::AML_SECTION_FILTER_CB)(cb), userData, mFilterId++);
}

int AmlSwDemux::destroyFilter(void* filter)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_DestroyFilter(mDemuxHandle, filter);
}

int AmlSwDemux::attachFilter(void* filter, void* channel)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_AttachFilter(mDemuxHandle, filter, channel);
}

int AmlSwDemux::detachFilter(void* filter, void* channel)
{
    RETURN_IF(-1, mDemuxHandle == AML_MP_INVALID_HANDLE);

    return aml::AML_DMX_DetachFilter(mDemuxHandle, filter, channel);
}

int AmlSwDemux::feedTs(const uint8_t* buffer, size_t size)
{
    if (mDemuxHandle == nullptr) {
        return -1;
    }

    return aml::AML_DMX_PushTsBuffer(nullptr, mDemuxHandle, buffer, size);
}



}
