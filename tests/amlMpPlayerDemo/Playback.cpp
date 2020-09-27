#define LOG_TAG "AmlMpPlayerDemo_Playback"
#include <utils/Log.h>
#include "Playback.h"
#include <Aml_MP/Aml_MP.h>
#include <cutils/properties.h>
#include <vector>
#include <string>

namespace aml_mp {

Playback::Playback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType, const sp<ProgramInfo>& programInfo)
: mProgramInfo(programInfo)
, mDemuxId(demuxId)
{
    Aml_MP_PlayerCreateParams createParams;
    memset(&createParams, 0, sizeof(createParams));
    createParams.userId = 0;
    createParams.demuxId = demuxId;
    createParams.sourceType = sourceType;
    createParams.drmMode = programInfo->scrambled ? AML_MP_INPUT_STREAM_ENCRYPTED : AML_MP_INPUT_STREAM_NORMAL;
    int ret = Aml_MP_Player_Create(&createParams, &mPlayer);
    if (ret < 0) {
        ALOGE("craete player failed!");
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

    ALOGI("dtor playback end!");
}

int Playback::setSubtitleDisplayWindow(int x, int y, int width, int height) {
    ALOGI("setSubtitleDisplayWindow, x:%d, y: %d, width: %d, height: %d", x, y, width, height);
    int ret = Aml_MP_Player_SetSubtitleWindow(mPlayer, x, y,width, height);
    return ret;
}

void Playback::setANativeWindow(const sp<ANativeWindow>& window)
{
    Aml_MP_Player_SetANativeWindow(mPlayer, window.get());
}

int Playback::start()
{
    int ret = 0;

    if (mPlayer == AML_MP_INVALID_HANDLE) {
        return -1;
    }

    if (mProgramInfo->scrambled) {
        Aml_MP_CASParams casParams;

        switch (mProgramInfo->caSystemId) {
        case 0x5601:
        {
            ALOGI("verimatrix iptv!");
            casParams.type = AML_MP_CAS_VERIMATRIX_IPTV;
            casParams.iptvCasParam.videoCodec = mProgramInfo->videoCodec;
            casParams.iptvCasParam.audioCodec = mProgramInfo->audioCodec;
            casParams.iptvCasParam.videoPid = mProgramInfo->videoPid;
            casParams.iptvCasParam.audioPid = mProgramInfo->audioPid;
            casParams.iptvCasParam.ecmPid = mProgramInfo->ecmPid[0];
            casParams.iptvCasParam.demuxId = mDemuxId;

            char value[PROPERTY_VALUE_MAX];
            property_get("config.media.vmx.dvb.key_file", value, "/data/mediadrm");
            strncpy(casParams.iptvCasParam.keyPath, value, sizeof(casParams.iptvCasParam.keyPath)-1);

            property_get("config.media.vmx.dvb.server_ip", value, "client-test-3.verimatrix.com");
            strncpy(casParams.iptvCasParam.serverAddress, value, sizeof(casParams.iptvCasParam.serverAddress)-1);

            casParams.iptvCasParam.serverPort = property_get_int32("config.media.vmx.dvb.server_port", 12686);
        }
        break;

        case 0x1724:
        {
            ALOGI("VMX DVB");
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

        default:
            ALOGI("unknown caSystemId:%#x", mProgramInfo->caSystemId);
            break;
        }

        if (casParams.type != AML_MP_CAS_UNKNOWN) {
            Aml_MP_Player_SetCASParams(mPlayer, &casParams);
        }
    }

    Aml_MP_AudioParams audioParams;
    memset(&audioParams, 0, sizeof(audioParams));
    audioParams.audioCodec = mProgramInfo->audioCodec;
    audioParams.pid = mProgramInfo->audioPid;
    Aml_MP_Player_SetAudioParams(mPlayer, &audioParams);

    Aml_MP_VideoParams videoParams;
    memset(&videoParams, 0, sizeof(videoParams));
    videoParams.videoCodec = mProgramInfo->videoCodec;
    videoParams.pid = mProgramInfo->videoPid;
    Aml_MP_Player_SetVideoParams(mPlayer, &videoParams);

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
    }

    Aml_MP_Player_RegisterEventCallBack(mPlayer, [](void* userData, Aml_MP_PlayerEvent* event) {
        static_cast<Playback*>(userData)->eventCallback(event);
    }, this);

    ret = Aml_MP_Player_Start(mPlayer);
    if (ret < 0) {
        ALOGE("player start failed!");
    }

    return ret;
}

int Playback::stop()
{
    int ret = 0;

    ret = Aml_MP_Player_Stop(mPlayer);
    if (ret < 0) {
        ALOGE("player stop failed!");
    }

    return ret;
}

void Playback::signalQuit()
{
    ALOGI("signalQuit!");
}

int Playback::writeData(const uint8_t* buffer, size_t size)
{
    int wlen = Aml_MP_Player_WriteData(mPlayer, buffer, size);

    return wlen;
}

void Playback::eventCallback(Aml_MP_PlayerEvent* event __unused)
{
}

///////////////////////////////////////////////////////////////////////////////
struct Command {
    const char* name;
    size_t argCount;
    const char* help;
    int (*fn)(AML_MP_HANDLE player, const std::vector<std::string>& args);
};

static void printCommands(const Command* pCommands, bool printHeader);

static struct Command g_commandTable[] = {
    {
        "help", 0, "help",
        [](AML_MP_HANDLE player __unused, const std::vector<std::string>& args __unused) -> int {
            printCommands(g_commandTable, true);
            return 0;
        }
    },

    {
        "pause", 0, "pause player",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_Pause(player);
        }
    },

    {
        "resume", 0, "resume player",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_Resume(player);
        }
    },

    {
        "hide", 0, "hide video",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_HideVideo(player);
        }
    },

    {
        "show", 0, "show video",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            return Aml_MP_Player_ShowVideo(player);
        }
    },

    {
        "gPts", 0, "get Pts",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
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
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            float volume;
            int ret = Aml_MP_Player_GetVolume(player, &volume);
            printf("Current volume: %f, ret: %d\n", volume, ret);
            return ret;
        }
    },
/*
 * TODO: set Volume not work. But can read value with get function
 */
    {
        "sVolume", 0, "set volume",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
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
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            float fastRate;
            if (args.size() != 2) {
                printf("Input example: sFast rate\n");
                return -1;
            }
            printf("String input: %s\n", args[1].data());
            fastRate = stof(args[1]);
            int ret = Aml_MP_Player_SetPlaybackRate(player, fastRate);
            printf("set rate: %f, ret: %d\n", fastRate, ret);
            return ret;
        }
    },

    {
        "gBuffer", 0, "get Buffer state",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            Aml_MP_BufferStat bufferStat;
            int ret = Aml_MP_Player_GetBufferStat(player, &bufferStat);
            printf("Audio buffer stat: size: %d, dataLen: %d, bufferedMs: %d\n", bufferStat.audioBuffer.size, bufferStat.audioBuffer.dataLen, bufferStat.audioBuffer.bufferedMs);
            printf("Video buffer stat: size: %d, dataLen: %d, bufferedMs: %d\n", bufferStat.videoBuffer.size, bufferStat.videoBuffer.dataLen, bufferStat.videoBuffer.bufferedMs);
            return ret;
        }
    },

    {
        "sWindow", 0, "set video window",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
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
            //ret += Aml_MP_Player_SetSubtitleWindow(player, x, y,width, height);
            printf("set x: %d, y: %d, width: %d, height: %d, ret: %d\n", x, y, width, height, ret);
            return ret;
        }
    },

    {
        "sParam", 0, "set param",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
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
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
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
            return ret;
        }
    },

    {
        "sSync", 0, "set sync mode",
        [](AML_MP_HANDLE player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_Player_SetAVSyncSource(player, AML_MP_AVSYNC_SOURCE_PCR);
            printf("set sync mode: %d, ret: %d\n", AML_MP_AVSYNC_SOURCE_PCR, ret);
            return ret;
        }
    },

    {nullptr, 0, nullptr, nullptr}
};

static const Command* findCommand(const std::string& cmdName, const Command* commandTable)
{
    if (commandTable == nullptr)
        return nullptr;

    const struct Command* command = commandTable;
    const struct Command* result  = nullptr;

    while (command && command->name) {
        if (command->name == cmdName) {
            result = command;
            break;
        }

        ++command;
    }

    return result;
}

static void printCommands(const Command* pCommands, bool printHeader)
{
    if (pCommands == nullptr) return;

    const struct Command* command = pCommands;

    if (printHeader && command && command->name) {
        printf("commands help:\n%20s | %s | %-s\n", "Command Name", "Args count", "Help");
    }

    while (command && command->name) {
        const char* help = command->help ? command->help : "";
        printf("%20s | %10u | %s\n", command->name, command->argCount, help);

        ++command;
    }
}

void Playback::processCommand(const std::vector<std::string>& args)
{
    std::string cmdName = *args.begin();
    const struct Command* command = nullptr;

    command = findCommand(cmdName, g_commandTable);
    if (command == nullptr) {
        printf("Unknown command: %s\n", cmdName.c_str());
        return;
    }

    if (args.size() <= command->argCount) {
        printf("missing arguments, expect %d args\n", command->argCount);
        return;
    }

    command->fn(mPlayer, args);
}



}
