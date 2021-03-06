/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlPlayerBase"
#include "utils/AmlMpLog.h"
#include <utils/AmlMpUtils.h>
#include "AmlPlayerBase.h"
#include "AmlTsPlayer.h"
#ifdef HAVE_CTC
#include "AmlCTCPlayer.h"
#endif

#ifdef ANDROID
#include <system/window.h>
#endif

#ifdef HAVE_SUBTITLE
#include <SubtitleNativeAPI.h>
#endif
#include "Aml_MP_PlayerImpl.h"

namespace aml_mp {

#ifdef HAVE_SUBTITLE
wptr<AmlPlayerBase> AmlPlayerBase::sSubtitleCbHandle;
#endif

sptr<AmlPlayerBase> AmlPlayerBase::create(Aml_MP_PlayerCreateParams* createParams, int instanceId)
{
    sptr<AmlPlayerBase> player;

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

AmlPlayerBase::AmlPlayerBase(Aml_MP_PlayerCreateParams* createParams, int instanceId)
:mInstanceId(instanceId)
, mEventCb(nullptr)
, mUserData(nullptr)
,mCreateParams(createParams)
#ifdef HAVE_SUBTITLE
, mSubtitleParams{}
#endif
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

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
#ifdef HAVE_SUBTITLE
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

int AmlPlayerBase::flush() {
#ifdef HAVE_SUBTITLE
#ifndef __ANDROID_VNDK__
    AmlSubtitleStatus ret = amlsub_Reset(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_UiShow failed! %d", ret);
        return -1;
    }
#endif
#endif
    return 0;
}

int AmlPlayerBase::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    MLOGI("setSubtitleParams");
    mSubtitleParams = *params;

    return 0;
}

int AmlPlayerBase::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
#ifdef HAVE_SUBTITLE
    stopSubtitleDecoding();
    setSubtitleParams(params);
    startSubtitleDecoding();
#endif
    return 0;
}

int AmlPlayerBase::showSubtitle()
{
    MLOGI("showSubtitle");

#ifdef HAVE_SUBTITLE
    RETURN_IF(0, mSubtitleHandle == nullptr);

    AmlSubtitleStatus ret = amlsub_UiShow(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_UiShow failed! %d", ret);
        return -1;
    }
#endif
    return 0;
}

int AmlPlayerBase::hideSubtitle()
{
    MLOGI("hideSubtitle");

#ifdef HAVE_SUBTITLE
    RETURN_IF(0, mSubtitleHandle == nullptr);

    AmlSubtitleStatus ret = amlsub_UiHide(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_UiHide failed!");
        return -1;
    }
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
    MLOGI("startSubtitleDecoding");

#ifdef HAVE_SUBTITLE
    if (mSubtitleParams.pid == AML_MP_INVALID_PID) {
        MLOGI("No subtitle info, not start subtitle");
        return 0;
    }
    AmlSubtitleParam subParam{};
    subParam.ioSource = AmlSubtitleIOType::E_SUBTITLE_DEMUX;
    if (!constructAmlSubtitleParam(&subParam, &mSubtitleParams)) {
        return 0;
    }

    if (mSubtitleHandle == nullptr) {
        mSubtitleHandle = amlsub_Create();
    }

    if (mSubtitleHandle == nullptr) {
        MLOGE("mSubtitleHandle is NULL");
        return -1;
    }


    sSubtitleCbHandle = this;
    amlsub_RegistOnDataCB(mSubtitleHandle, AmlMPSubtitleDataCb);
    amlsub_RegistOnSubtitleAvailCb(mSubtitleHandle, AmlMPSubtitleAvailCb);
    amlsub_RegistGetDimesionCb(mSubtitleHandle, AmlMPSubtitleDimensionCb);
    amlsub_RegistAfdEventCB(mSubtitleHandle, AmlMPSubtitleAfdEventCb);
    amlsub_RegistOnChannelUpdateCb(mSubtitleHandle, AmlMPSubtitleChannelUpdateCb);
    amlsub_RegistOnSubtitleLanguageCb(mSubtitleHandle, AmlMPSubtitleLanguageCb);
    amlsub_RegistOnSubtitleInfoCB(mSubtitleHandle, AmlMPSubtitleInfoCb);

    AmlSubtitleStatus ret = amlsub_Open(mSubtitleHandle, &subParam);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_Open failed!");
    }

    showSubtitle();
    MLOGI("Subtitle size is x:%d, y: %d, width: %d, height: %d", mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);
    ret = amlsub_UiSetSurfaceViewRect(mSubtitleHandle, mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_UiSetSurfaceViewRect failed!");
    }

#endif
    return 0;
}

int AmlPlayerBase::stopSubtitleDecoding()
{
    MLOGI("stopSubtitleDecoding");

#ifdef HAVE_SUBTITLE
    RETURN_IF(-1, mSubtitleHandle == nullptr);

    if (sSubtitleCbHandle == this) {
        sSubtitleCbHandle = nullptr;
    }

    hideSubtitle();

    AmlSubtitleStatus ret = amlsub_Close(mSubtitleHandle);
    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_Close failed!");
        return -1;
    }
#endif

    return 0;
}

int AmlPlayerBase::setSubtitleWindow(int x, int y, int width, int height)
{
    MLOGI("setSubtitleWindow");
    MLOGI("param x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    mSubWindowX = x;
    mSubWindowY = y;
    mSubWindowWidth = width;
    mSubWindowHeight = height;

#ifdef HAVE_SUBTITLE
    RETURN_IF(0, mSubtitleHandle == nullptr);

    AmlSubtitleStatus ret = amlsub_UiSetSurfaceViewRect(mSubtitleHandle, mSubWindowX, mSubWindowY, mSubWindowWidth, mSubWindowHeight);

    if (ret != SUB_STAT_OK) {
        MLOGE("amlsub_UiSetSurfaceViewRect failed!");
        return -1;
    }
#endif

    return 0;
}

int AmlPlayerBase::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
#ifdef HAVE_SUBTITLE
    AmlSubtitleStatus ret = SUB_STAT_INV;
    AmlTeletextCtrlParam amlTeletextCtrlParam;

    switch (key) {
        case AML_MP_PLAYER_PARAMETER_TELETEXT_CONTROL:
            amlTeletextCtrlParam = convertToTeletextCtrlParam((AML_MP_TeletextCtrlParam*)parameter);
            ret = amlsub_TeletextControl(mSubtitleHandle, &amlTeletextCtrlParam);
            break;
        default:
            ret = SUB_STAT_INV;
    }

    if (ret != SUB_STAT_OK) {
        return -1;
    }
#endif
    return 0;
}

int AmlPlayerBase::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    //MLOGI("TODO!!! %s, type:%d, buffer:%p, size:%d, pts:%lld", __FUNCTION__, type, buffer, size, pts);

    return size;
}

#ifdef HAVE_SUBTITLE
void AmlPlayerBase::AmlMPSubtitleDataCb(const char * data, int size, AmlSubDataType type,
                                            int x, int y, int width, int height, int videoWidth,
                                            int videoHeight, int showing) {
    ALOG(LOG_INFO, nullptr, "Call AmlMPSubtitleDataCb");
    if (sSubtitleCbHandle == nullptr) {
        return;
    }

    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }

    cbHandle->mSubtitleData.data = data;
    cbHandle->mSubtitleData.size = size;

    cbHandle->mSubtitleData.type = convertToMpSubtitleDataType(type);
    cbHandle->mSubtitleData.x = x;
    cbHandle->mSubtitleData.y = y;
    cbHandle->mSubtitleData.width = width;
    cbHandle->mSubtitleData.height = height;
    cbHandle->mSubtitleData.videoWidth = videoWidth;
    cbHandle->mSubtitleData.videoHeight = videoHeight;
    cbHandle->mSubtitleData.showing = showing;
    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_DATA, (int64_t)(&(cbHandle->mSubtitleData)));
}

#define ERROR_DECODER_NORMAL 1
#define ERROR_DECODER_TIMEOUT 2
#define ERROR_DECODER_LOSEDATA 3
#define ERROR_DECODER_INVALIDDATA 4
#define ERROR_DECODER_TIMEERROR 5

void AmlPlayerBase::AmlMPSubtitleAvailCb(int avail) {
    ALOG(LOG_INFO, nullptr, "Call AmlMPSubtitleAvailCb: %d", avail);
    if (sSubtitleCbHandle == nullptr) {
        return;
    }

    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }

    switch (avail) {
    case ERROR_DECODER_NORMAL:
        cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_AVAIL, (int64_t)(&avail));
        break;
    case ERROR_DECODER_TIMEOUT:
        cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_TIMEOUT, (int64_t)(&avail));
        break;
    case ERROR_DECODER_LOSEDATA:
        cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_LOSEDATA, (int64_t)(&avail));
        break;
    case ERROR_DECODER_INVALIDDATA:
        cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_INVALID_DATA, (int64_t)(&avail));
        break;
    case ERROR_DECODER_TIMEERROR:
        cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_INVALID_TIMESTAMP, (int64_t)(&avail));
        break;
    default:
        ALOG(LOG_INFO, nullptr, "Unhandled avail callback param: %d", avail);
        break;
    }
}

void AmlPlayerBase::AmlMPSubtitleDimensionCb(int width, int height) {
    if (sSubtitleCbHandle == nullptr) {
        return;
    }
    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }
    cbHandle->mSubtitleDimension.width = width;
    cbHandle->mSubtitleDimension.height = height;
    ALOG(LOG_INFO, nullptr, "[AmlMPSubtitleDimensionCb] Get call back info width: %d, height: %d\n", cbHandle->mSubtitleDimension.width, cbHandle->mSubtitleDimension.height);
    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_DIMENSION, (int64_t)(&(cbHandle->mSubtitleDimension)));
}

void AmlPlayerBase::AmlMPSubtitleAfdEventCb(int event) {
    ALOG(LOG_INFO, nullptr, "AmlMPSubtitleAfdEventCb: %d", event);
    if (sSubtitleCbHandle == nullptr) {
        return;
    }
    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }

    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_AFD_EVENT, (int64_t)(&event));
}

void AmlPlayerBase::AmlMPSubtitleChannelUpdateCb(int event, int id) {
    if (sSubtitleCbHandle == nullptr) {
        return;
    }
    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }
    ALOG(LOG_INFO, nullptr, "AmlMPSubtitleChannelUpdateCb [event: %d, id: %d]", event, id);

    Aml_MP_SubtitleChannelUpdate subtitleChannelUpdate;
    subtitleChannelUpdate.event = event;
    subtitleChannelUpdate.id = id;
    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_CHANNEL_UPDATE, (int64_t)(&subtitleChannelUpdate));
}

void AmlPlayerBase::AmlMPSubtitleLanguageCb(std::string lang) {
    if (sSubtitleCbHandle == nullptr) {
        return;
    }
    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }
    lang.copy(cbHandle->mSubtitleIso639Code, 3, 0);
    cbHandle->mSubtitleIso639Code[3] = '\0';
    ALOG(LOG_INFO, nullptr, "[AmlMPSubtitleLanguageCb] Get callback info iso639Code: %s\n", cbHandle->mSubtitleIso639Code);
    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_LANGUAGE, (int64_t)(&(cbHandle->mSubtitleIso639Code)));
}

void AmlPlayerBase::AmlMPSubtitleInfoCb(int what, int extra) {
    ALOG(LOG_INFO, nullptr, "AmlMPSubtitleInfoCb [what: %d, extra: %d]", what, extra);
    if (sSubtitleCbHandle == nullptr) {
        return;
    }
    sptr<AmlPlayerBase> cbHandle = sSubtitleCbHandle.promote();
    if (cbHandle == nullptr) {
        return;
    }

    Aml_MP_SubtitleInfo subtitleInfo;
    subtitleInfo.what = what;
    subtitleInfo.extra = extra;
    cbHandle->notifyListener(AML_MP_PLAYER_EVENT_SUBTITLE_INFO, (int64_t)(&subtitleInfo));
}

#endif

///////////////////////////////////////////////////////////////////////////////
void AmlPlayerBase::notifyListener(Aml_MP_PlayerEventType eventType, int64_t param)
{
    if (mEventCb) {
        mEventCb(mUserData, eventType, param);
    } else {
        MLOGE("mEventCb is NULL! %s", mpPlayerEventType2Str(eventType));
    }
}

///////////////////////////////////////////////////////////////////////////////


}

