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

#define LOG_TAG "AmlMpDVR"

#include <Aml_MP/Dvr.h>
#include "AmlDVRPlayer.h"
#include "AmlDVRRecorder.h"
#include "utils/AmlMpUtils.h"
#include "utils/AmlMpHandle.h"
#include <dvr_segment.h>

using namespace aml_mp;
using namespace android;

int Aml_MP_DVRRecorder_Create(Aml_MP_DVRRecorderCreateParams* createParams, AML_MP_HANDLE* handle)
{
    RETURN_IF(-1, createParams == nullptr);

    AmlDVRRecorder* recorder = new AmlDVRRecorder(&createParams->basicParams, &createParams->timeshiftParams, &createParams->encryptParams);
    if (recorder == nullptr) {
        ALOGE("new AmlDVRRecorder failed!");
        return -1;
    }

    recorder->incStrong(recorder);
    *handle = aml_handle_cast(recorder);

    return 0;
}

int Aml_MP_DVRRecorder_Destroy(AML_MP_HANDLE recorder)
{
    sp<AmlDVRRecorder> dvrRecorder = aml_handle_cast<AmlDVRRecorder>(recorder);
    RETURN_IF(-1, dvrRecorder == nullptr);

    dvrRecorder->decStrong(recorder);

    return 0;
}

int Aml_MP_DVRRecorder_RegisterEventCallback(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderEventCallback cb, void* userData) {
    sp<AmlDVRRecorder> amlMpHandle = aml_handle_cast<AmlDVRRecorder>(recorder);
    int ret;
    RETURN_IF(-1, amlMpHandle == nullptr);

    ret = amlMpHandle->registerEventCallback(cb, userData);

    return ret;
}

int Aml_MP_DVRRecorder_SetStreams(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderStreams* streams)
{
    sp<AmlDVRRecorder> amlMpHandle = aml_handle_cast<AmlDVRRecorder>(recorder);
    RETURN_IF(-1, amlMpHandle == nullptr);

    int ret = amlMpHandle->setStreams(streams);
    return ret;
}

int Aml_MP_DVRRecorder_Start(AML_MP_HANDLE recorder)
{
    sp<AmlDVRRecorder> amlMpHandle = aml_handle_cast<AmlDVRRecorder>(recorder);
    RETURN_IF(-1, amlMpHandle == nullptr);

    int ret = amlMpHandle->start();

    return ret;
}

int Aml_MP_DVRRecorder_Stop(AML_MP_HANDLE recorder)
{
    sp<AmlDVRRecorder> amlMpHandle = aml_handle_cast<AmlDVRRecorder>(recorder);
    RETURN_IF(-1, amlMpHandle == nullptr);

    int ret = amlMpHandle->stop();
    return ret;
}

int Aml_MP_DVRRecorder_GetStatus(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderStatus* status)
{
    sp<AmlDVRRecorder> amlMpHandle = aml_handle_cast<AmlDVRRecorder>(recorder);
    RETURN_IF(-1, amlMpHandle == nullptr);

    int ret = amlMpHandle->getStatus(status);

    return ret;
}

int Aml_MP_DVRRecorder_GetSegmentList(const char* location, uint32_t* segmentNums, uint64_t** segmentIds)
{
    return dvr_segment_get_list(location, segmentNums, segmentIds);
}

int Aml_MP_DVRRecorder_GetSegmentInfo(const char* location, uint64_t segmentId, Aml_MP_DVRSegmentInfo* segmentInfo)
{
    DVR_RecordSegmentInfo_t info;
    int ret = dvr_segment_get_info(location, segmentId, &info);

    memset(segmentInfo, 0, sizeof(*segmentInfo));
    segmentInfo->id = info.id;
    segmentInfo->streams.nbStreams = info.nb_pids;
    ALOGI("nb_pids:%d", info.nb_pids);
    for (size_t i = 0; i < info.nb_pids; ++i) {
        convertToMpDVRStream(&segmentInfo->streams.streams[i], &info.pids[i]);

        ALOGI("streamType:%d, pid:%d, codecId:%d(%s)", segmentInfo->streams.streams[i].type,
                segmentInfo->streams.streams[i].pid,
                segmentInfo->streams.streams[i].codecId,
                mpCodecId2Str(segmentInfo->streams.streams[i].codecId));
    }

    segmentInfo->duration = info.duration;
    segmentInfo->size = info.size;
    segmentInfo->nbPackets = info.nb_packets;

    return ret;
}

int Aml_MP_DVRRecorder_DeleteSegment(const char* location, uint64_t segmentId)
{
    return dvr_segment_delete(location, segmentId);
}

///////////////////////////////////////////////////////////////////////////////
int Aml_MP_DVRPlayer_Create(Aml_MP_DVRPlayerCreateParams* createParams, AML_MP_HANDLE* handle)
{
    RETURN_IF(-1, createParams == nullptr);

    AmlDVRPlayer* player = new AmlDVRPlayer(&createParams->basicParams, &createParams->decryptParams);
    if (player == nullptr) {
        ALOGE("new AmlDVRPlayer failed!");
        return -1;
    }

    player->incStrong(player);
    *handle = player;

    return 0;
}

int Aml_MP_DVRPlayer_Destroy(AML_MP_HANDLE player)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    dvrPlayer->decStrong(player);

    return 0;
}

int Aml_MP_DVRPlayer_RegisterEventCallback(AML_MP_HANDLE player, Aml_MP_DVRPlayerEventCallback cb, void* userData)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->registerEventCallback(cb, userData);

    return ret;
}

int Aml_MP_DVRPlayer_SetStreams(AML_MP_HANDLE player, Aml_MP_DVRStreamArray* streams)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->setStreams(streams);

    return ret;
}

int Aml_MP_DVRPlayer_Start(AML_MP_HANDLE player, bool initialPaused)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->start(initialPaused);

    return ret;
}

int Aml_MP_DVRPlayer_Stop(AML_MP_HANDLE player)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->stop();

    return ret;
}

int Aml_MP_DVRPlayer_Pause(AML_MP_HANDLE player)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->pause();

    return ret;
}

int Aml_MP_DVRPlayer_Resume(AML_MP_HANDLE player)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->resume();

    return ret;
}

int Aml_MP_DVRPlayer_Seek(AML_MP_HANDLE player, int timeOffset)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->seek(timeOffset);

    return ret;
}

int Aml_MP_DVRPlayer_SetPlaybackRate(AML_MP_HANDLE player, float rate)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->setPlaybackRate(rate);

    return ret;
}

int Aml_MP_DVRPlayer_GetStatus(AML_MP_HANDLE player, Aml_MP_DVRPlayerStatus* status)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(player);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->getStatus(status);

    return ret;
}

int Aml_MP_DVRPlayer_ShowVideo(AML_MP_HANDLE handle)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->showVideo();

    return ret;
}

int Aml_MP_DVRPlayer_HideVideo(AML_MP_HANDLE handle)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->hideVideo();

    return ret;
}

int Aml_MP_DVRPlayer_SetVolume(AML_MP_HANDLE handle, float volume)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->setVolume(volume);

    return ret;
}

int Aml_MP_DVRPlayer_GetVolume(AML_MP_HANDLE handle, float* volume)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->getVolume(volume);

    return ret;
}

int Aml_MP_DVRPlayer_SetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->setParameter(key, parameter);

    return ret;
}

int Aml_MP_DVRPlayer_GetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter)
{
    sp<AmlDVRPlayer> dvrPlayer = aml_handle_cast<AmlDVRPlayer>(handle);
    RETURN_IF(-1, dvrPlayer == nullptr);

    int ret = dvrPlayer->getParameter(key, parameter);

    return ret;
}


