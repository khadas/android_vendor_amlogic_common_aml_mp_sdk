/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#define LOG_TAG "AmlPlayerBase"
#include "utils/AmlMpLog.h"
#include <utils/AmlMpUtils.h>
#include "AmlPlayerBase.h"
#include "AmlTsPlayer.h"
#include "AmlCTCPlayer.h"
#include <system/window.h>
#include <amlogic/am_gralloc_ext.h>
#include <SubtitleNativeAPI.h>

namespace aml_mp {

sp<AmlPlayerBase> AmlPlayerBase::create(Aml_MP_PlayerCreateParams* createParams, int instanceId)
{
    sp<AmlPlayerBase> player;

    player = new AmlTsPlayer(createParams, instanceId);

    return player;
}

AmlPlayerBase::AmlPlayerBase(int instanceId)
:mEventCb(nullptr)
, mUserData(nullptr)
, mSubtitleParams{}
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);

    //Set a default size for subtitle window parament.
    mSubWindowX = 0;
    mSubWindowY = 0;
    mSubWindowWidth = 1920;
    mSubWindowHeight = 1080;
}

AmlPlayerBase::~AmlPlayerBase()
{
#ifndef __ANDROID_VNDK__
    if (mSubtitleHandle) {
        amlsub_Destroy(mSubtitleHandle);
        mSubtitleHandle = nullptr;
    }
#endif
}

int AmlPlayerBase::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlPlayerBase::setANativeWindow(ANativeWindow* nativeWindow)
{
    native_handle_t* sidebandHandle = am_gralloc_create_sideband_handle(AM_TV_SIDEBAND, AM_VIDEO_DEFAULT);
    mSidebandHandle = NativeHandle::create(sidebandHandle, true);

    ALOGI("setAnativeWindow:%p, sidebandHandle:%p", nativeWindow, sidebandHandle);

    int ret = native_window_set_sideband_stream(nativeWindow, sidebandHandle);
    if (ret < 0) {
        ALOGE("set sideband stream failed!");
    }

    return ret;
}

int AmlPlayerBase::start()
{
    startSubtitleDecoding();
    showSubtitle();

    return 0;
}

int AmlPlayerBase::stop()
{
    hideSubtitle();
    stopSubtitleDecoding();

    return 0;
}

int AmlPlayerBase::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    ALOGI("setSubtitleParams");
    mSubtitleParams = *params;

    return 0;
}

int AmlPlayerBase::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
    AML_MP_UNUSED(params);

    return 0;
}

int AmlPlayerBase::showSubtitle()
{
    ALOGI("showSubtitle");

    RETURN_IF(-1, mSubtitleHandle == nullptr);

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_UiShow(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiShow failed! %d", ret);
        return -1;
    }
#endif

    return 0;
}

int AmlPlayerBase::hideSubtitle()
{
    ALOGI("hideSubtitle");

    RETURN_IF(-1, mSubtitleHandle == nullptr);

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_UiHide(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiHide failed!");
        return -1;
    }
#endif

    return 0;
}

bool AmlPlayerBase::constructAmlSubtitleParam(AmlSubtitleParam* amlSubParam, Aml_MP_SubtitleParams* params)
{
    bool ret = true;

    switch (params->subtitleCodec) {
    case AML_MP_SUBTITLE_CODEC_CC:
        amlSubParam->subtitleType = AmlSubtitletype::TYPE_SUBTITLE_CLOSED_CATPTION;
        break;

    case AML_MP_SUBTITLE_CODEC_SCTE27:
        amlSubParam->subtitleType = AmlSubtitletype::TYPE_SUBTITLE_SCTE27;
        break;

    case AML_MP_SUBTITLE_CODEC_DVB:
        amlSubParam->subtitleType = AmlSubtitletype::TYPE_SUBTITLE_DVB;
        break;

    case AML_MP_SUBTITLE_CODEC_TELETEXT:
        amlSubParam->subtitleType = AmlSubtitletype::TYPE_SUBTITLE_DVB_TELETEXT;
        break;

    default:
        ret = false;
        break;
    }

    amlSubParam->pid = params->pid;
    amlSubParam->videoFormat = params->videoFormat;
    amlSubParam->channelId = params->channelId;
    amlSubParam->ancillaryPageId = params->ancillaryPageId;
    amlSubParam->compositionPageId = params->compositionPageId;
    return ret;
}

int AmlPlayerBase::startSubtitleDecoding()
{
    ALOGI("startSubtitleDecoding");

    AmlSubtitleParam subParam{};
    subParam.ioSource = AmlSubtitleIOType::E_SUBTITLE_DEMUX;
    if (!constructAmlSubtitleParam(&subParam, &mSubtitleParams)) {
        return -1;
    }

#ifndef __ANDROID_VNDK__
    if (mSubtitleHandle == nullptr) {
        mSubtitleHandle = amlsub_Create();
    }
#endif

    if (mSubtitleHandle == nullptr) {
        return -1;
    }


#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_Open(mSubtitleHandle, &subParam);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_Open failed!");
    }

    ALOGI("Subtitle size is x:%d, y: %d, width: %d, height: %d", mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);
    ret = amlsub_UiSetSurfaceViewRect(mSubtitleHandle, mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiSetSurfaceViewRect failed!");
    }

#endif

    return 0;
}

int AmlPlayerBase::stopSubtitleDecoding()
{
    ALOGI("stopSubtitleDecoding");

    RETURN_IF(-1, mSubtitleHandle == nullptr);

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_Close(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_Close failed!");
        return -1;
    }
#endif

    return 0;
}

int AmlPlayerBase::setSubtitleWindow(int x, int y, int width, int height) {
    ALOGI("setSubtitleWindow");
    ALOGI("param x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    mSubWindowX = x;
    mSubWindowY = y;
    mSubWindowWidth = width;
    mSubWindowHeight = height;

    RETURN_IF(-1, mSubtitleHandle == nullptr);
#ifndef __ANDROID_VNDK__

    AmlSubtitleStatus ret = amlsub_UiSetSurfaceViewRect(mSubtitleHandle, mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);

    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiSetSurfaceViewRect failed!");
        return -1;
    }
#endif

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void AmlPlayerBase::notifyListener(Aml_MP_PlayerEventType eventType, int64_t param)
{
    if (mEventCb) {
        mEventCb(mUserData, eventType, param);
    } else {
        ALOGE("mEventCb is NULL! %d", eventType);
    }
}

///////////////////////////////////////////////////////////////////////////////


}

