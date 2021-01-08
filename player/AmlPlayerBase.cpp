/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlPlayerBase"
#define KEEP_ALOGX
#include "utils/AmlMpLog.h"
#include <utils/AmlMpUtils.h>
#include "AmlPlayerBase.h"
#include "AmlTsPlayer.h"
#ifdef HAVE_CTC
#include "AmlCTCPlayer.h"
#endif
#include <system/window.h>
#include <amlogic/am_gralloc_ext.h>
#ifdef HAVE_SUBTITLE
#include <SubtitleNativeAPI.h>
#endif
#include "Aml_MP_PlayerImpl.h"

namespace aml_mp {

sp<AmlPlayerBase> AmlPlayerBase::create(Aml_MP_PlayerCreateParams* createParams, int instanceId)
{
    sp<AmlPlayerBase> player;
    int mUseCtc = AmlMpConfig::instance().mCtcDebug;

    if (createParams->channelId == AML_MP_CHANNEL_ID_MAIN ||
        !AmlMpPlayerRoster::instance().isAmTsPlayerExist() ||
        isSupportMultiHwDemux() ||
        AmlMpConfig::instance().mTsPlayerNonTunnel) {
        player = new AmlTsPlayer(createParams, instanceId);
    } else {
#ifdef HAVE_CTC
        player = new AmlCTCPlayer(createParams, instanceId);
#endif
    }

    return player;
}

AmlPlayerBase::AmlPlayerBase(int instanceId)
:mInstanceId(instanceId)
, mEventCb(nullptr)
, mUserData(nullptr)
#ifdef HAVE_SUBTITLE
, mSubtitleParams{}
#endif
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);

#ifdef HAVE_SUBTITLE
    //Set a default size for subtitle window parament.
    mSubWindowX = 0;
    mSubWindowY = 0;
    mSubWindowWidth = 1920;
    mSubWindowHeight = 1080;
#endif
}

AmlPlayerBase::~AmlPlayerBase()
{
#ifndef __ANDROID_VNDK__
#ifdef HAVE_SUBTITLE
    if (mSubtitleHandle) {
        amlsub_Destroy(mSubtitleHandle);
        mSubtitleHandle = nullptr;
    }
#endif
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
    RETURN_IF(-1, nativeWindow == nullptr);
    int ret = 0;

    native_handle_t* sidebandHandle = am_gralloc_create_sideband_handle(AM_TV_SIDEBAND, AM_VIDEO_DEFAULT);
    mSidebandHandle = NativeHandle::create(sidebandHandle, true);

    ALOGI("setAnativeWindow:%p, sidebandHandle:%p", nativeWindow, sidebandHandle);

    ret = native_window_set_sideband_stream(nativeWindow, sidebandHandle);
    if (ret < 0) {
        ALOGE("set sideband stream failed!");
    }

    return ret;
}

int AmlPlayerBase::start()
{
#ifdef HAVE_SUBTITLE
    startSubtitleDecoding();
#endif

    return 0;
}

int AmlPlayerBase::stop()
{
#ifdef HAVE_SUBTITLE
    stopSubtitleDecoding();
#endif

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

#ifdef HAVE_SUBTITLE
    RETURN_IF(-1, mSubtitleHandle == nullptr);

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_UiShow(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiShow failed! %d", ret);
        return -1;
    }
#endif
#endif
    return 0;
}

int AmlPlayerBase::hideSubtitle()
{
    ALOGI("hideSubtitle");

#ifdef HAVE_SUBTITLE
    RETURN_IF(-1, mSubtitleHandle == nullptr);

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_UiHide(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiHide failed!");
        return -1;
    }
#endif
#endif

    return 0;
}

#ifdef HAVE_SUBTITLE
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
#endif

int AmlPlayerBase::startSubtitleDecoding()
{
    ALOGI("startSubtitleDecoding");

#ifdef HAVE_SUBTITLE
    AmlSubtitleParam subParam{};
    subParam.ioSource = AmlSubtitleIOType::E_SUBTITLE_DEMUX;
    if (!constructAmlSubtitleParam(&subParam, &mSubtitleParams)) {
        return 0;
    }

#ifndef __ANDROID_VNDK__
    if (mSubtitleHandle == nullptr) {
        mSubtitleHandle = amlsub_Create();
    }
#endif

    if (mSubtitleHandle == nullptr) {
        ALOGE("mSubtitleHandle is NULL");
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

    amlsub_RegistGetDimesionCb(mSubtitleHandle, [](int width, int height) {
        //printf("subtitle size:%d x %d\n", width, height);
    });

#endif

    showSubtitle();
#endif
    return 0;
}

int AmlPlayerBase::stopSubtitleDecoding()
{
    ALOGI("stopSubtitleDecoding");

#ifdef HAVE_SUBTITLE
    RETURN_IF(-1, mSubtitleHandle == nullptr);

    hideSubtitle();

#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_Close(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_Close failed!");
        return -1;
    }
#endif
#endif

    return 0;
}

int AmlPlayerBase::setSubtitleWindow(int x, int y, int width, int height)
{
    ALOGI("setSubtitleWindow");
    ALOGI("param x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    mSubWindowX = x;
    mSubWindowY = y;
    mSubWindowWidth = width;
    mSubWindowHeight = height;

#ifdef HAVE_SUBTITLE
    RETURN_IF(-1, mSubtitleHandle == nullptr);
#ifndef __ANDROID_VNDK__

    AmlSubtitleStatus ret = amlsub_UiSetSurfaceViewRect(mSubtitleHandle, mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);

    if (ret != SUB_STAT_OK) {
        ALOGE("amlsub_UiSetSurfaceViewRect failed!");
        return -1;
    }
#endif
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

