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

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayer"
#include <Aml_MP/Aml_MP.h>
#include "Aml_MP_PlayerImpl.h"
#include "utils/AmlMpUtils.h"
#include "utils/AmlMpHandle.h"

using namespace aml_mp;
using namespace android;

int Aml_MP_Player_Create(Aml_MP_PlayerCreateParams* createParams, AML_MP_HANDLE* handle)
{
    sp<AmlMpPlayerImpl> player = new AmlMpPlayerImpl(createParams);
    player->incStrong(player.get());

    *handle = aml_handle_cast(player);

    return 0;
}

int Aml_MP_Player_Destroy(AML_MP_HANDLE handle)
{
    sp<AmlMpHandle> amlMpHandle = aml_handle_cast<AmlMpHandle>(handle);
    RETURN_IF(-1, amlMpHandle == nullptr);
    amlMpHandle->decStrong(handle);

    return 0;
}

int Aml_MP_Player_RegisterEventCallBack(AML_MP_HANDLE handle, Aml_MP_PlayerEventCallback cb, void* userData)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->registerEventCallback(cb, userData);
}

int Aml_MP_Player_SetVideoParams(AML_MP_HANDLE handle, Aml_MP_VideoParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVideoParams(params);
}

int Aml_MP_Player_SetAudioParams(AML_MP_HANDLE handle, Aml_MP_AudioParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAudioParams(params);
}

int Aml_MP_Player_SetSubtitleParams(AML_MP_HANDLE handle, Aml_MP_SubtitleParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setSubtitleParams(params);
}

int Aml_MP_Player_SetCASParams(AML_MP_HANDLE handle, Aml_MP_CASParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setCASParams(params);
}

int Aml_MP_Player_Start(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->start();
}

int Aml_MP_Player_Stop(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stop();
}

int Aml_MP_Player_Pause(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->pause();
}

int Aml_MP_Player_Resume(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->resume();
}

int Aml_MP_Player_Flush(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->flush();
}

int Aml_MP_Player_SetPlaybackRate(AML_MP_HANDLE handle, float rate)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setPlaybackRate(rate);
}

int Aml_MP_Player_SwitchAudioTrack(AML_MP_HANDLE handle, Aml_MP_AudioParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->switchAudioTrack(params);
}

int Aml_MP_Player_SwitchSubtitleTrack(AML_MP_HANDLE handle, Aml_MP_SubtitleParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->switchSubtitleTrack(params);
}

int Aml_MP_Player_WriteData(AML_MP_HANDLE handle, const uint8_t* buffer, size_t size)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->writeData(buffer, size);
}

int Aml_MP_Player_WriteEsData(AML_MP_HANDLE handle, Aml_MP_StreamType streamType, const uint8_t* buffer, size_t size, int64_t pts)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->writeEsData(streamType, buffer, size, pts);
}

int Aml_MP_Player_GetCurrentPts(AML_MP_HANDLE handle, Aml_MP_StreamType streamType, int64_t* pts)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getCurrentPts(streamType, pts);
}

int Aml_MP_Player_GetBufferStat(AML_MP_HANDLE handle, Aml_MP_BufferStat* bufferStat)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getBufferStat(bufferStat);
}

int Aml_MP_Player_SetANativeWindow(AML_MP_HANDLE handle, void* nativeWindow)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAnativeWindow(nativeWindow);
}

int Aml_MP_Player_SetVideoWindow(AML_MP_HANDLE handle, int32_t x, int32_t y, int32_t width, int32_t height)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVideoWindow(x, y, width, height);
}

int Aml_MP_Player_SetVolume(AML_MP_HANDLE handle, float volume)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVolume(volume);
}

int Aml_MP_Player_GetVolume(AML_MP_HANDLE handle, float* volume)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getVolume(volume);
}


int Aml_MP_Player_ShowVideo(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showVideo();
}

int Aml_MP_Player_HideVideo(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideVideo();
}

//int Aml_MP_Player_MuteAudio(AML_MP_HANDLE handle)
//{
    //return 0;
//}

//int Aml_MP_Player_UnmuteAudio(AML_MP_HANDLE handle)
//{
    //return 0;
//}

int Aml_MP_Player_ShowSubtitle(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showSubtitle();
}

int Aml_MP_Player_HideSubtitle(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideSubtitle();
}

int Aml_MP_Player_SetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setParameter(key, parameter);
}

int Aml_MP_Player_GetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getParameter(key, parameter);
}

int Aml_MP_Player_SetSubtitleWindow(AML_MP_HANDLE handle, int x, int y, int width, int height)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setSubtitleWindow(x, y, width, height);
}

//********************* BASIC INTERFACES END **************************/
int Aml_MP_Player_SetAVSyncSource(AML_MP_HANDLE handle, Aml_MP_AVSyncSource syncSource)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAVSyncSource(syncSource);
}

int Aml_MP_Player_SetPcrPid(AML_MP_HANDLE handle, int pid)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setPcrPid(pid);
}

int Aml_MP_Player_StartVideoDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startVideoDecoding();
}

int Aml_MP_Player_StopVideoDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopVideoDecoding();
}

int Aml_MP_Player_StartAudioDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startAudioDecoding();
}

int Aml_MP_Player_StopAudioDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopAudioDecoding();
}

int Aml_MP_Player_StartSubtitleDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->startSubtitleDecoding();
}

int Aml_MP_Player_StopSubtitleDecoding(AML_MP_HANDLE handle)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stopSubtitleDecoding();
}

//int Aml_MP_Player_StartDescrambling(AML_MP_HANDLE handle)
//{
    //sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    //RETURN_IF(-1, player == nullptr);

    //return player->startDescrambling();
//}

//int Aml_MP_Player_StopDescrambling(AML_MP_HANDLE handle)
//{
    //sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    //RETURN_IF(-1, player == nullptr);

    //return player->stopDescrambling();
//}

int Aml_MP_Player_SetADParams(AML_MP_HANDLE handle, Aml_MP_AudioParams* params)
{
    sp<AmlMpPlayerImpl> player = aml_handle_cast<AmlMpPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setADParams(params);
}


