/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayer"
#include <Aml_MP/Aml_MP.h>
#include "Aml_MP_PlayerImpl.h"
#include "utils/AmlMpUtils.h"
#include "utils/AmlMpHandle.h"

static const char* mName = LOG_TAG;

using namespace aml_mp;
using namespace android;

int Aml_MP_Player_Create(Aml_MP_PlayerCreateParams* createParams, AML_MP_PLAYER* handle)
{
    sptr<AmlMpPlayerImpl> player = new AmlMpPlayerImpl(createParams);
    player->incStrong(player.get());

    *handle = aml_handle_cast(player);

    return 0;
}

int Aml_MP_Player_Destroy(AML_MP_PLAYER handle)
{
    sptr<AmlMpHandle> amlMpHandle = aml_handle_cast<AmlMpHandle>(handle);
    RETURN_IF(-1, amlMpHandle == nullptr);
    amlMpHandle->decStrong(handle);

    return 0;
}

int Aml_MP_Player_RegisterEventCallBack(AML_MP_PLAYER handle, Aml_MP_PlayerEventCallback cb, void* userData)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->registerEventCallback(cb, userData);
}

int Aml_MP_Player_SetVideoParams(AML_MP_PLAYER handle, Aml_MP_VideoParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVideoParams(params);
}

int Aml_MP_Player_SetAudioParams(AML_MP_PLAYER handle, Aml_MP_AudioParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAudioParams(params);
}

int Aml_MP_Player_SetSubtitleParams(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setSubtitleParams(params);
}

int Aml_MP_Player_SetCasSession(AML_MP_PLAYER handle, AML_MP_CASSESSION casSession)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setCasSession(casSession);
}

int Aml_MP_Player_SetCASParams(AML_MP_PLAYER handle, Aml_MP_CASParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    Aml_MP_CASServiceType serviceType{AML_MP_CAS_SERVICE_TYPE_INVALID};
    Aml_MP_IptvCASParams iptvCasParam{};

    if (params->type == AML_MP_CAS_VERIMATRIX_IPTV) {
        serviceType = AML_MP_CAS_SERVICE_VERIMATRIX_IPTV;
    } else if (params->type == AML_MP_CAS_WIDEVINE) {
        serviceType = AML_MP_CAS_SERVICE_WIDEVINE;
    } else {
        MLOGE("unsupported cas type! %s", mpCASType2Str(params->type));
        return -1;
    }

    iptvCasParam.videoCodec = params->u.iptvCasParam.videoCodec;
    iptvCasParam.audioCodec = params->u.iptvCasParam.audioCodec;
    iptvCasParam.videoPid = params->u.iptvCasParam.videoPid;
    iptvCasParam.audioPid = params->u.iptvCasParam.audioPid;
    iptvCasParam.ecmPid[0] = params->u.iptvCasParam.ecmPid;
    iptvCasParam.demuxId = params->u.iptvCasParam.demuxId;
    strncpy(iptvCasParam.serverAddress, params->u.iptvCasParam.serverAddress, sizeof(params->u.iptvCasParam.serverAddress));
    iptvCasParam.serverPort = params->u.iptvCasParam.serverPort;
    strncpy(iptvCasParam.keyPath, params->u.iptvCasParam.keyPath, sizeof(params->u.iptvCasParam.keyPath));

    return player->setIptvCASParams(serviceType, &iptvCasParam);
}

int Aml_MP_Player_Start(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->start();
}

int Aml_MP_Player_Stop(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stop();
}

int Aml_MP_Player_Pause(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->pause();
}

int Aml_MP_Player_Resume(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->resume();
}

int Aml_MP_Player_Flush(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->flush();
}

int Aml_MP_Player_SetPlaybackRate(AML_MP_PLAYER handle, float rate)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setPlaybackRate(rate);
}

int Aml_MP_Player_SwitchAudioTrack(AML_MP_PLAYER handle, Aml_MP_AudioParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->switchAudioTrack(params);
}

int Aml_MP_Player_SwitchSubtitleTrack(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->switchSubtitleTrack(params);
}

int Aml_MP_Player_WriteData(AML_MP_PLAYER handle, const uint8_t* buffer, size_t size)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->writeData(buffer, size);
}

int Aml_MP_Player_WriteEsData(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, const uint8_t* buffer, size_t size, int64_t pts)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->writeEsData(streamType, buffer, size, pts);
}

int Aml_MP_Player_GetCurrentPts(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, int64_t* pts)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getCurrentPts(streamType, pts);
}

int Aml_MP_Player_GetBufferStat(AML_MP_PLAYER handle, Aml_MP_BufferStat* bufferStat)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getBufferStat(bufferStat);
}

int Aml_MP_Player_SetANativeWindow(AML_MP_PLAYER handle, ANativeWindow* nativeWindow)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setANativeWindow(nativeWindow);
}

int Aml_MP_Player_SetVideoWindow(AML_MP_PLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVideoWindow(x, y, width, height);
}

int Aml_MP_Player_SetVolume(AML_MP_PLAYER handle, float volume)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVolume(volume);
}

int Aml_MP_Player_GetVolume(AML_MP_PLAYER handle, float* volume)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getVolume(volume);
}


int Aml_MP_Player_ShowVideo(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showVideo();
}

int Aml_MP_Player_HideVideo(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideVideo();
}

//int Aml_MP_Player_MuteAudio(AML_MP_PLAYER handle)
//{
    //return 0;
//}

//int Aml_MP_Player_UnmuteAudio(AML_MP_PLAYER handle)
//{
    //return 0;
//}

int Aml_MP_Player_ShowSubtitle(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showSubtitle();
}

int Aml_MP_Player_HideSubtitle(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideSubtitle();
}

int Aml_MP_Player_SetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setParameter(key, parameter);
}

int Aml_MP_Player_GetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getParameter(key, parameter);
}

int Aml_MP_Player_SetSubtitleWindow(AML_MP_PLAYER handle, int x, int y, int width, int height)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setSubtitleWindow(x, y, width, height);
}

//********************* BASIC INTERFACES END **************************/
int Aml_MP_Player_SetAVSyncSource(AML_MP_PLAYER handle, Aml_MP_AVSyncSource syncSource)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAVSyncSource(syncSource);
}

int Aml_MP_Player_SetPcrPid(AML_MP_PLAYER handle, int pid)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setPcrPid(pid);
}

int Aml_MP_Player_StartVideoDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startVideoDecoding();
}

int Aml_MP_Player_StopVideoDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopVideoDecoding();
}

int Aml_MP_Player_StartAudioDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startAudioDecoding();
}

int Aml_MP_Player_StopAudioDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopAudioDecoding();
}

int Aml_MP_Player_StartSubtitleDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startSubtitleDecoding();
}

int Aml_MP_Player_StopSubtitleDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopSubtitleDecoding();
}

int Aml_MP_Player_SetADParams(AML_MP_PLAYER handle, Aml_MP_AudioParams* params)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setADParams(params);
}

int Aml_MP_Player_StartADDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startADDecoding();
}

int Aml_MP_Player_StopADDecoding(AML_MP_PLAYER handle)
{
    sptr<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopADDecoding();
}

