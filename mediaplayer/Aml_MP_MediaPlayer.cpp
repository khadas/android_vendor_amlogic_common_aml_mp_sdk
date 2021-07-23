/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpMediaPlayer"
#include <Aml_MP/Aml_MediaPlayer.h>
#include "Aml_MP_MediaPlayerImpl.h"
#include "utils/AmlMpUtils.h"
#include "utils/AmlMpHandle.h"

static const char* mName = LOG_TAG;

using namespace aml_mp;
using namespace android;

int Aml_MP_MediaPlayer_Create(AML_MP_MEDIAPLAYER* handle)
{
    sptr<AmlMpMediaPlayerImpl> player = new AmlMpMediaPlayerImpl();
    player->incStrong(player.get());

    *handle = aml_handle_cast(player);

    return 0;
}

int Aml_MP_MediaPlayer_Destroy(AML_MP_MEDIAPLAYER handle)
{
    //release
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);
    player->reset();

    //destroy
    player->decStrong(handle);

    return 0;
}

int Aml_MP_MediaPlayer_SetANativeWindow(AML_MP_MEDIAPLAYER handle, ANativeWindow* nativeWindow)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setANativeWindow(nativeWindow);
}

int Aml_MP_MediaPlayer_SetVideoWindow(AML_MP_MEDIAPLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVideoWindow(x, y, width, height);
}

int Aml_MP_MediaPlayer_SetDataSource(AML_MP_MEDIAPLAYER handle, const char *uri)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setDataSource(uri);
}

int Aml_MP_MediaPlayer_Prepare(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->prepare();
}

int Aml_MP_MediaPlayer_PrepareAsync(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->prepareAsync();
}

int Aml_MP_MediaPlayer_Start(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->start();
}

int Aml_MP_MediaPlayer_SeekTo(AML_MP_MEDIAPLAYER handle, int msec)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->seekTo(msec);
}

int Aml_MP_MediaPlayer_Pause(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->pause();
}

int Aml_MP_MediaPlayer_Resume(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->resume();
}

int Aml_MP_MediaPlayer_Stop(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->stop();
}

int Aml_MP_MediaPlayer_Reset(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->reset();
}

int Aml_MP_MediaPlayer_GetCurrentPosition(AML_MP_MEDIAPLAYER handle, int* msec)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getCurrentPosition(msec);
}

int Aml_MP_MediaPlayer_GetDuration(AML_MP_MEDIAPLAYER handle, int* msec)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getDuration(msec);
}

int Aml_MP_MediaPlayer_SetVolume(AML_MP_MEDIAPLAYER handle, float volume)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setVolume(volume);
}

int Aml_MP_MediaPlayer_GetVolume(AML_MP_MEDIAPLAYER handle, float* volume)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getVolume(volume);
}

int Aml_MP_MediaPlayer_ShowVideo(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showVideo();
}

int Aml_MP_MediaPlayer_HideVideo(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideVideo();
}

int Aml_MP_MediaPlayer_ShowSubtitle(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->showSubtitle();
}

int Aml_MP_MediaPlayer_HideSubtitle(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->hideSubtitle();
}

int Aml_MP_MediaPlayer_SetSubtitleWindow(AML_MP_MEDIAPLAYER handle, int x, int y, int width, int height)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setSubtitleWindow(x, y, width, height);
}

int Aml_MP_MediaPlayer_Invoke(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->invoke(request, reply);
}

int Aml_MP_MediaPlayer_SetAVSyncSource(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerAVSyncSource syncSource)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setAVSyncSource(syncSource);
}

int Aml_MP_MediaPlayer_SetParameter(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setParameter(key, parameter);
}

int Aml_MP_MediaPlayer_GetParameter(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->getParameter(key, parameter);
}

bool Aml_MP_MediaPlayer_IsPlaying(AML_MP_MEDIAPLAYER handle)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->isPlaying();
}

int Aml_MP_MediaPlayer_RegisterEventCallBack(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerEventCallback cb, void* userData)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->registerEventCallBack(cb, userData);
}

int Aml_MP_MediaPlayer_SetPlaybackRate(AML_MP_MEDIAPLAYER handle, float rate)
{
    sptr<AmlMpMediaPlayerImpl> player = aml_handle_cast<AmlMpMediaPlayerImpl>(handle);
    RETURN_IF(-1, player == nullptr);

    return player->setPlaybackRate(rate);
}

