/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_Playback"
#include <utils/AmlMpLog.h>
#include "Playback.h"
#include <Aml_MP/Aml_MP.h>
#include <cutils/properties.h>
#include <vector>
#include <string>

static const char* mName = LOG_TAG;

namespace aml_mp {

Playback::Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sptr<ProgramInfo>& programInfo)
: mProgramInfo(programInfo)
, mDemuxId(demuxId)
{
    mIsDVBSource = sourceType == AML_MP_INPUT_SOURCE_TS_DEMOD;

    Aml_MP_PlayerCreateParams createParams;
    memset(&createParams, 0, sizeof(createParams));
    createParams.channelId = AML_MP_CHANNEL_ID_AUTO;
    createParams.demuxId = demuxId;
    createParams.sourceType = sourceType;
    createParams.drmMode = programInfo->scrambled ? AML_MP_INPUT_STREAM_ENCRYPTED : AML_MP_INPUT_STREAM_NORMAL;
    int ret = Aml_MP_Player_Create(&createParams, &mPlayer);
    printStreamsInfo();
    if (ret < 0) {
        MLOGE("create player failed!");
        return;
    }
}

Playback::~Playback()
{
    stop();

    if (mPlayer != AML_MP_INVALID_HANDLE) {
        Aml_MP_Player_Destroy(mPlayer);
        mPlayer = AML_MP_INVALID_HANDLE;
    }

    MLOGI("dtor playback end!");
}

int Playback::setSubtitleDisplayWindow(int x, int y, int width, int height) {
    MLOGI("setSubtitleDisplayWindow, x:%d, y: %d, width: %d, height: %d", x, y, width, height);
    int ret = Aml_MP_Player_SetSubtitleWindow(mPlayer, x, y,width, height);
    return ret;
}

int Playback::setVideoWindow(int x, int y, int width, int height)
{
    MLOGI("setVideoWindow, x:%d, y: %d, width: %d, height: %d", x, y, width, height);
    int ret = Aml_MP_Player_SetVideoWindow(mPlayer, x, y,width, height);
    return ret;
}

int Playback::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    int ret = Aml_MP_Player_SetParameter(mPlayer, key, parameter);
    return ret;
}

void Playback::setANativeWindow(const android::sp<ANativeWindow>& window)
{
    MLOGI("setANativeWindow");
    Aml_MP_Player_SetANativeWindow(mPlayer, window.get());
}

void Playback::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    mEventCallback = cb;
    mUserData = userData;
}

int Playback::start(PlayMode playMode)
{
    mPlayMode = playMode;

    int ret = 0;

    if (mPlayer == AML_MP_INVALID_HANDLE) {
        return -1;
    }

    if (mProgramInfo->scrambled) {
        if (mIsDVBSource) {
            startDVBDescrambling();
        } else {
            startIPTVDescrambling();
        }
    }

    Aml_MP_Player_RegisterEventCallBack(mPlayer,mEventCallback, mUserData);

    if (mPlayMode == PlayMode::START_AUDIO_START_VIDEO) {
        if (setAudioParams()) {
            ret |= Aml_MP_Player_StartAudioDecoding(mPlayer);
        }

        if (setVideoParams()) {
            ret |= Aml_MP_Player_StartVideoDecoding(mPlayer);
        }

        if (setSubtitleParams()) {
            ret |= Aml_MP_Player_StartSubtitleDecoding(mPlayer);
        }
    } else if (mPlayMode == PlayMode::START_VIDEO_START_AUDIO) {
        if (setVideoParams()) {
            ret |= Aml_MP_Player_StartVideoDecoding(mPlayer);
        }

        if (setAudioParams()) {
            ret |= Aml_MP_Player_StartAudioDecoding(mPlayer);
        }

        if (setSubtitleParams()) {
            ret |= Aml_MP_Player_StartSubtitleDecoding(mPlayer);
        }
    } else if (mPlayMode == PlayMode::START_ALL_STOP_ALL || mPlayMode == PlayMode::START_ALL_STOP_SEPARATELY) {
        setAudioParams();
        setVideoParams();
        setSubtitleParams();

        ret = Aml_MP_Player_Start(mPlayer);
    } else if (mPlayMode == PlayMode::START_SEPARATELY_STOP_ALL ||
            mPlayMode == PlayMode::START_SEPARATELY_STOP_SEPARATELY ||
            playMode == PlayMode::START_SEPARATELY_STOP_SEPARATELY_V2) {
        setAudioParams();
        setVideoParams();
        setSubtitleParams();

        if (playMode == PlayMode::START_SEPARATELY_STOP_SEPARATELY_V2) {
            ret |= Aml_MP_Player_StartVideoDecoding(mPlayer);
            ret |= Aml_MP_Player_StartAudioDecoding(mPlayer);
        } else {
            ret |= Aml_MP_Player_StartAudioDecoding(mPlayer);
            ret |= Aml_MP_Player_StartVideoDecoding(mPlayer);
        }
        ret |= Aml_MP_Player_StartSubtitleDecoding(mPlayer);
    } else {
        MLOGE("unknown playmode:%d", mPlayMode);
    }

    if (ret != 0) {
        MLOGE("player start failed!");
    }

    return ret;
}

bool Playback::setAudioParams()
{
    Aml_MP_AudioParams audioParams;
    memset(&audioParams, 0, sizeof(audioParams));
    audioParams.audioCodec = mProgramInfo->audioCodec;
    audioParams.pid = mProgramInfo->audioPid;
    Aml_MP_Player_SetAudioParams(mPlayer, &audioParams);

    return audioParams.pid != AML_MP_INVALID_PID;
}

bool Playback::setVideoParams()
{
    Aml_MP_VideoParams videoParams;
    memset(&videoParams, 0, sizeof(videoParams));
    videoParams.videoCodec = mProgramInfo->videoCodec;
    videoParams.pid = mProgramInfo->videoPid;
    Aml_MP_Player_SetVideoParams(mPlayer, &videoParams);

    return videoParams.pid != AML_MP_INVALID_PID;
}

bool Playback::setSubtitleParams()
{
    Aml_MP_SubtitleParams subtitleParams{};
    subtitleParams.subtitleCodec = mProgramInfo->subtitleCodec;
    switch (subtitleParams.subtitleCodec) {
    case AML_MP_SUBTITLE_CODEC_DVB:
        subtitleParams.pid = mProgramInfo->subtitlePid;
        subtitleParams.compositionPageId = mProgramInfo->compositionPageId;
        subtitleParams.ancillaryPageId = mProgramInfo->ancillaryPageId;
        break;

    case AML_MP_SUBTITLE_CODEC_TELETEXT:
        subtitleParams.pid = mProgramInfo->subtitlePid;
        break;

    case AML_MP_SUBTITLE_CODEC_SCTE27:
        subtitleParams.pid = mProgramInfo->subtitlePid;
        break;

    default:
        break;
    }

    if (subtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        Aml_MP_Player_SetSubtitleParams(mPlayer, &subtitleParams);
        return true;
    }

    return false;
}

int Playback::stop()
{
    int ret = 0;

    if (mPlayMode == PlayMode::START_ALL_STOP_ALL || mPlayMode == PlayMode::START_SEPARATELY_STOP_ALL) {
        ret = Aml_MP_Player_Stop(mPlayer);
        if (ret < 0) {
            MLOGE("player stop failed!");
        }
    } else {
        if (mPlayMode == PlayMode::START_VIDEO_START_AUDIO) {
            ret |= Aml_MP_Player_StopVideoDecoding(mPlayer);
            ret |= Aml_MP_Player_StopAudioDecoding(mPlayer);
        } else {
            ret |= Aml_MP_Player_StopAudioDecoding(mPlayer);
            ret |= Aml_MP_Player_StopVideoDecoding(mPlayer);
        }
        ret |= Aml_MP_Player_StopSubtitleDecoding(mPlayer);

        if (ret != 0) {
            MLOGE("player stop separately failed!");
        }
    }

    if (mProgramInfo->scrambled) {
        if (mIsDVBSource) {
            stopDVBDescrambling();
        } else {
            stopIPTVDescrambling();
        }
    }

    return ret;
}

void Playback::signalQuit()
{
    MLOGI("signalQuit!");
}

int Playback::writeData(const uint8_t* buffer, size_t size)
{
    int wlen = Aml_MP_Player_WriteData(mPlayer, buffer, size);

    return wlen;
}

int Playback::startDVBDescrambling()
{
    MLOG();

    Aml_MP_CAS_Initialize();

    Aml_MP_CAS_SetEmmPid(mDemuxId, mProgramInfo->emmPid);

    if (!Aml_MP_CAS_IsSystemIdSupported(mProgramInfo->caSystemId)) {
        MLOGE("unsupported caSystemId:%#x", mProgramInfo->caSystemId);
        return -1;
    }

    int ret = Aml_MP_CAS_OpenSession(&mCasSession, AML_MP_CAS_SERVICE_LIVE_PLAY);
    if (ret < 0) {
        MLOGE("open session failed!");
        return -1;
    }


    Aml_MP_CAS_RegisterEventCallback(mCasSession, [](AML_MP_CASSESSION session, const char* json) {
        MLOGI("ca_cb:%s", json);
        return 0;
    }, this);

    Aml_MP_CASServiceInfo casServiceInfo;
    memset(&casServiceInfo, 0, sizeof(casServiceInfo));
    casServiceInfo.service_id = mProgramInfo->serviceNum;
    casServiceInfo.serviceType = AML_MP_CAS_SERVICE_LIVE_PLAY;
    casServiceInfo.ecm_pid = mProgramInfo->ecmPid[0];
    casServiceInfo.stream_pids[0] = mProgramInfo->audioPid;
    casServiceInfo.stream_pids[1] = mProgramInfo->videoPid;
    casServiceInfo.stream_num = 2;
    casServiceInfo.ca_private_data_len = 0;
    ret = Aml_MP_CAS_StartDescrambling(mCasSession, &casServiceInfo);
    if (ret < 0) {
        MLOGE("start descrambling failed with %d", ret);
        return ret;
    }

    mSecMem = Aml_MP_CAS_CreateSecmem(mCasSession, AML_MP_CAS_SERVICE_LIVE_PLAY, nullptr, nullptr);
    if (mSecMem == nullptr) {
        MLOGE("create secmem failed!");
    }

    return 0;
}

int Playback::stopDVBDescrambling()
{
    MLOG("mCasSession:%p", mCasSession);

    if (mCasSession == nullptr) {
        return 0;
    }

    Aml_MP_CAS_DestroySecmem(mCasSession, mSecMem);
    mSecMem = nullptr;

    Aml_MP_CAS_StopDescrambling(mCasSession);

    Aml_MP_CAS_CloseSession(mCasSession);
    mCasSession = nullptr;

    return 0;
}

int Playback::startIPTVDescrambling()
{
    MLOG();

    Aml_MP_IptvCasParams casParams;
    int ret = 0;

    casParams.type = AML_MP_CAS_UNKNOWN;
    switch (mProgramInfo->caSystemId) {
    case 0x5601:
    {
        MLOGI("verimatrix iptv!");
        casParams.type = AML_MP_CAS_VERIMATRIX_IPTV;
        casParams.videoCodec = mProgramInfo->videoCodec;
        casParams.audioCodec = mProgramInfo->audioCodec;
        casParams.videoPid = mProgramInfo->videoPid;
        casParams.audioPid = mProgramInfo->audioPid;
        casParams.ecmPid[0] = mProgramInfo->ecmPid[0];
        casParams.ecmPid[1] = mProgramInfo->ecmPid[1];
        casParams.demuxId = mDemuxId;

        char value[PROPERTY_VALUE_MAX];
        property_get("config.media.vmx.dvb.key_file", value, "/data/mediadrm");
        strncpy(casParams.keyPath, value, sizeof(casParams.keyPath)-1);

        property_get("config.media.vmx.dvb.server_ip", value, "client-test-3.verimatrix.com");
        strncpy(casParams.serverAddress, value, sizeof(casParams.serverAddress)-1);

        casParams.serverPort = property_get_int32("config.media.vmx.dvb.server_port", 12686);
    }
    break;

    case 0x1724:
    {
        MLOGI("VMX DVB");
#if 0
        casParams.type = AML_MP_CAS_VERIMATRIX_DVB;
        casParams.dvbCasParam.demuxId = mDemuxId;
        casParams.dvbCasParam.emmPid = mProgramInfo->emmPid;
        casParams.dvbCasParam.serviceId = mProgramInfo->serviceNum;
        casParams.dvbCasParam.ecmPid = mProgramInfo->ecmPid[0];
        casParams.dvbCasParam.streamPids[0] = mProgramInfo->videoPid;
        casParams.dvbCasParam.streamPids[1] = mProgramInfo->audioPid;
#endif
    }
    break;

    case 0x4AD4:
    case 0x4AD5:
    {
        MLOGI("wvcas iptv, systemid=0x%x!", mProgramInfo->caSystemId);
        casParams.type = AML_MP_CAS_WIDEVINE;
        casParams.caSystemId = mProgramInfo->caSystemId;
        casParams.videoCodec = mProgramInfo->videoCodec;
        casParams.audioCodec = mProgramInfo->audioCodec;
        casParams.videoPid = mProgramInfo->videoPid;
        casParams.audioPid = mProgramInfo->audioPid;
        casParams.ecmPid[0] = mProgramInfo->ecmPid[0];
        casParams.ecmPid[1] = mProgramInfo->ecmPid[1];
        casParams.demuxId = mDemuxId;

        casParams.private_size = 0;
        if (mProgramInfo->privateDataLength > 0) {
            memcpy(casParams.private_data, mProgramInfo->privateData, mProgramInfo->privateDataLength);
            casParams.private_size =  mProgramInfo->privateDataLength;
        }
        MLOGI("wvcas iptv, private_size=%d", casParams.private_size);
    }
    break;

    default:
        MLOGI("unknown caSystemId:%#x", mProgramInfo->caSystemId);
        break;
    }

    if (casParams.type != AML_MP_CAS_UNKNOWN) {
        ret = Aml_MP_Player_SetIptvCASParams(mPlayer, &casParams);
    }

    return ret;
}

int Playback::stopIPTVDescrambling()
{
    MLOG();

    return 0;
}

void Playback::printStreamsInfo() {
    printf("\n");
    printf("Video Streams:\n");
    for (int i = 0; i < mProgramInfo->audioStreams.size(); i++) {
        printf("\tVideo[%d] pid: %d, codec: %d\n", i, mProgramInfo->videoStreams.at(i).pid, mProgramInfo->videoStreams.at(i).codecId);
    }
    printf("\n");

    printf("Audio streams:\n");
    for (int i = 0; i < mProgramInfo->audioStreams.size(); i++) {
        printf("\tAudio[%d] pid: %d, codec: %d\n", i, mProgramInfo->audioStreams.at(i).pid, mProgramInfo->audioStreams.at(i).codecId);
    }

    printf("Subtitle streams:\n");
    for (int i = 0; i < mProgramInfo->subtitleStreams.size(); i++) {
        printf("\tSubtitle[%d] pid: %d, codec: %d", i, mProgramInfo->subtitleStreams.at(i).pid, mProgramInfo->subtitleStreams.at(i).codecId);
        if (mProgramInfo->subtitleStreams.at(i).codecId == AML_MP_SUBTITLE_CODEC_DVB) {
            printf("[%d, %d]\n", mProgramInfo->subtitleStreams.at(i).compositionPageId, mProgramInfo->subtitleStreams.at(i).ancillaryPageId);
        } else {
            printf("\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
static struct TestModule::Command g_commandTable[] = {
    {
        "help", 0, "help",
        [](AML_MP_PLAYER player __unused, const std::vector<std::string>& args __unused) -> int {
            TestModule::printCommands(g_commandTable, true);
            return 0;
        }
    },

    {
        "pause", 0, "pause player",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_Pause(player);
        }
    },

    {
        "resume", 0, "resume player",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_Resume(player);
        }
    },

    {
        "hide", 0, "hide video",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_HideVideo(player);
        }
    },

    {
        "show", 0, "show video",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_ShowVideo(player);
        }
    },

    {
        "flush", 0, "call flush",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_Flush(player);
            printf("call flush ret: %d\n", ret);
            return ret;
        }
    },

    {
        "switchAudio", 0, "switch audio",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_AudioParams audioParams;
            int ret;
            int audioCodec, audioPid;
            if (args.size() != 3) {
                printf("Input example: switchAudio Pid codec_ID\n");
                return -1;
            }
            printf("String input: %s %s\n", args[1].data(), args[2].data());
            audioPid = stof(args[1]);
            audioCodec = stof(args[2]);
            memset(&audioParams, 0, sizeof(audioParams));
            audioParams.audioCodec = (Aml_MP_CodecID)audioCodec;
            audioParams.pid = audioPid;
            ret = Aml_MP_Player_SwitchAudioTrack(player, &audioParams);
            printf("Switch audio param[%d, %d], ret: %d\n", audioParams.pid, audioParams.audioCodec, ret);
            return ret;
        }
    },

    {
        "switchSubtitle", 0, "switch subtitle",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_SubtitleParams subtitleParams;
            int ret;
            int subtitleCodec, subtitlePid, compositionPageId, ancillaryPageId;
            if (args.size() != 5) {
                printf("Input example: switchSubtitle Pid codec_ID compositionPageId ancillaryPageId\n");
                return -1;
            }
            printf("String input: %s %s\n", args[1].data(), args[2].data());
            subtitlePid = stof(args[1]);
            subtitleCodec = stof(args[2]);
            compositionPageId = stof(args[3]);
            ancillaryPageId = stof(args[4]);
            memset(&subtitleParams, 0, sizeof(subtitleParams));
            subtitleParams.subtitleCodec = (Aml_MP_CodecID)subtitleCodec;
            subtitleParams.pid = subtitlePid;
            if (subtitleParams.subtitleCodec == AML_MP_SUBTITLE_CODEC_DVB) {
                subtitleParams.compositionPageId = compositionPageId;
                subtitleParams.ancillaryPageId = ancillaryPageId;
            }
            ret = Aml_MP_Player_SwitchSubtitleTrack(player, &subtitleParams);
            printf("Switch audio param[%d, %d], ret: %d\n", subtitleParams.pid, subtitleParams.subtitleCodec, ret);
            return ret;
        }
    },

    {
        "gPts", 0, "get Pts",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int64_t pts;
            int ret = Aml_MP_Player_GetCurrentPts(player, AML_MP_STREAM_TYPE_VIDEO, &pts);
            printf("Current video pts: 0x%llx, ret: %d\n", pts, ret);
            ret = Aml_MP_Player_GetCurrentPts(player, AML_MP_STREAM_TYPE_AUDIO, &pts);
            printf("Current audio pts: 0x%llx, ret: %d\n", pts, ret);
            return ret;
        }
    },

    {
        "gVolume", 0, "get volume",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            float volume;
            int ret = Aml_MP_Player_GetVolume(player, &volume);
            printf("Current volume: %f, ret: %d\n", volume, ret);
            return ret;
        }
    },

    {
        "sVolume", 0, "set volume",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            float volume, volume2;
            if (args.size() != 2) {
                printf("Input example: sVolume volume\n");
                return -1;
            }
            printf("String input: %s\n", args[1].data());
            volume = stof(args[1]);
            int ret = Aml_MP_Player_SetVolume(player,volume);
            ret = Aml_MP_Player_GetVolume(player, &volume2);
            printf("Get volume: %f, set volume: %f, ret: %d\n", volume2, volume, ret);
            return ret;
        }
    },

    {
        "sFast", 0, "set Fast rate",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            float fastRate;
            int ret;
            if (args.size() != 2) {
                printf("Input example: sFast rate\n");
                return -1;
            }
            printf("String input: %s\n", args[1].data());
            fastRate = stof(args[1]);
            if (fastRate < 2) {
                ret = Aml_MP_Player_SetPlaybackRate(player, fastRate);
            } else {
                Aml_MP_VideoDecodeMode trickMode = AML_MP_VIDEO_DECODE_MODE_IONLY;
                ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE, &trickMode);
            }
            printf("set rate: %f, ret: %d\n", fastRate, ret);
            return ret;
        }
    },

    {
        "gBuffer", 0, "get Buffer state",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_BufferStat bufferStat;
            int ret = Aml_MP_Player_GetBufferStat(player, &bufferStat);
            printf("Audio buffer stat: size: %d, dataLen: %d, bufferedMs: %d\n", bufferStat.audioBuffer.size, bufferStat.audioBuffer.dataLen, bufferStat.audioBuffer.bufferedMs);
            printf("Video buffer stat: size: %d, dataLen: %d, bufferedMs: %d\n", bufferStat.videoBuffer.size, bufferStat.videoBuffer.dataLen, bufferStat.videoBuffer.bufferedMs);
            return ret;
        }
    },

    {
        "sWindow", 0, "set video window",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int32_t x, y, width, height;
            if (args.size() != 5) {
                printf("Input example: sWindow x y width height\n");
                return -1;
            }
            printf("String input x: %s, y: %s, width: %s, height: %s\n", args[1].data(), args[2].data(), args[3].data(), args[4].data());
            x = stoi(args[1]);
            y = stoi(args[2]);
            width = stoi(args[3]);
            height = stoi(args[4]);
            int ret = Aml_MP_Player_SetVideoWindow(player, x, y,width, height);
            printf("set x: %d, y: %d, width: %d, height: %d, ret: %d\n", x, y, width, height, ret);
            return ret;
        }
    },

    {
        "sZorder", 0, "set zorder",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int32_t zorder;
            if (args.size() != 2) {
                printf("Input example: sZorder zorder\n");
                return -1;
            }
            printf("String input zorder: %s\n", args[1].data());
            zorder = stoi(args[1]);
            int ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER, &zorder);
            printf("set zorder: %d, ret: %d\n", zorder, ret);
            return ret;
        }
    },

    {
        "sParam", 0, "set param",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_VideoDisplayMode videoMode = AML_MP_VIDEO_DISPLAY_MODE_NORMAL;
            int ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, &videoMode);
            printf("AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE set mode: %d, ret: %d\n", videoMode, ret);
            bool blackOut = true;
            ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &blackOut);
            printf("AML_MP_PLAYER_PARAMETER_BLACK_OUT set mode: %d, ret: %d\n", blackOut, ret);
            Aml_MP_AudioOutputMode audioMode = AML_MP_AUDIO_OUTPUT_PCM;
            ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, &audioMode);
            printf("AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE set mode: %d, ret: %d\n", audioMode, ret);
            Aml_MP_ADVolume adVolume = {5, 5};
            ret = Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &adVolume);
            printf("AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL set mode: %d, %d, ret: %d\n", adVolume.masterVolume, adVolume.slaveVolume, ret);
            return ret;
        }
    },

    {
        "gParam", 0, "get param",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_VideoInfo videoInfo;
            int ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, &videoInfo);
            printf("AML_MP_PLAYER_PARAMETER_VIDEO_INFO, width: %d, height: %d, framerate: %d, bitrate: %d, ratio64: %d, ret: %d\n", videoInfo.width, videoInfo.height, videoInfo.frameRate, videoInfo.bitrate, videoInfo.ratio64, ret);
            Aml_MP_VdecStat vdecStat;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, &vdecStat);
            printf("AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, frame_width: %d, frame_height: %d, frame_rate: %d, ret: %d\n", vdecStat.frame_width, vdecStat.frame_height, vdecStat.frame_rate, ret);
            Aml_MP_AudioInfo audioInfo;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, &audioInfo);
            printf("AML_MP_PLAYER_PARAMETER_AUDIO_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d, ret: %d\n", audioInfo.sample_rate, audioInfo.channels, audioInfo.channel_mask, audioInfo.bitrate, ret);
            Aml_MP_AdecStat adecStat;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, &adecStat);
            printf("AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT get audio decode stat frame_count: %d, error_frame_count: %d, drop_frame_count: %d, ret: %d\n", adecStat.frame_count, adecStat.error_frame_count, adecStat.drop_frame_count, ret);
            Aml_MP_AudioInfo adInfo;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AD_INFO, &adInfo);
            printf("AML_MP_PLAYER_PARAMETER_AD_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d, ret: %d\n", adInfo.sample_rate, adInfo.channels, adInfo.channel_mask, adInfo.bitrate, ret);
            Aml_MP_AdecStat adStat;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT, &adStat);
            printf("AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT get audio decode stat frame_count: %d, error_frame_count: %d, drop_frame_count: %d, ret: %d\n", adStat.frame_count, adStat.error_frame_count, adStat.drop_frame_count, ret);
            uint32_t instanceId;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_INSTANCE_ID, &instanceId);
            printf("AML_MP_PLAYER_PARAMETER_INSTANCE_ID get instance id: %d\n", instanceId);
            int32_t syncId;
            ret = Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_SYNC_ID, &syncId);
            printf("AML_MP_PLAYER_PARAMETER_SYNC_ID get sync id: %d\n", syncId);
            return ret;
        }
    },

    {
        "sSync", 0, "set sync mode",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_SetAVSyncSource(player, AML_MP_AVSYNC_SOURCE_PCR);
            printf("set sync mode: %d, ret: %d\n", AML_MP_AVSYNC_SOURCE_PCR, ret);
            return ret;
        }
    },

    {
        "Start", 0, "call start",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_Start(player);
            printf("call start ret: %d\n", ret);
            return ret;
        }
    },

    {
        "Stop", 0, "call stop",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_Stop(player);
            printf("call stop ret: %d\n", ret);
            return ret;
        }
    },

    {
        "Destory", 0, "call destroy",
        [](AML_MP_PLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_Destroy(player);
            printf("call destroy ret: %d\n", ret);
            return ret;
        }
    },

    {nullptr, 0, nullptr, nullptr}
};

const TestModule::Command* Playback::getCommandTable() const
{
    return g_commandTable;
}

void* Playback::getCommandHandle() const
{
    return mPlayer;
}

}
