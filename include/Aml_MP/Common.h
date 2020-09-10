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

#ifndef _AML_MP_COMMON_H_
#define _AML_MP_COMMON_H_

#include <stdlib.h>
#include <stdint.h>

////////////////////////////////////////
typedef void* AML_MP_HANDLE;
#define AML_MP_INVALID_HANDLE   (0)
#define AML_MP_INVALID_PID      (0x1FFF)

////////////////////////////////////////
typedef enum {
    AML_MP_DEMUX_ID_DEFAULT         = -1,
    AML_MP_HW_DEMUX_ID_0            = 0,
    AML_MP_HW_DEMUX_ID_1,
    AML_MP_HW_DEMUX_ID_2,
    AML_MP_HW_DEMUX_ID_MAX,
    AML_MP_HW_DEMUX_NB              = (AML_MP_HW_DEMUX_ID_MAX - AML_MP_HW_DEMUX_ID_0),
} Aml_MP_DemuxId;

typedef enum {
    AML_MP_INPUT_SOURCE_TS_DEMOD,
    AML_MP_INPUT_SOURCE_TS_MEMORY,
} Aml_MP_InputSourceType;

typedef enum {
    AML_MP_INPUT_STREAM_NORMAL,
    AML_MP_INPUT_STREAM_ENCRYPTED,
} Aml_MP_InputStreamType;

typedef enum {
	AML_MP_INPUT_BUFFER_TYPE_NORMAL,
    AML_MP_INPUT_BUFFER_TYPE_SECURE,
    AML_MP_INPUT_BUFFER_TYPE_TVP,
} Aml_MP_InputBufferType;

////////////////////////////////////////
typedef enum {
    AML_MP_VIDEO_CODEC_UNKNOWN = -1,
    AML_MP_VIDEO_CODEC_MPEG12 = 0,
    AML_MP_VIDEO_CODEC_MPEG4,
    AML_MP_VIDEO_CODEC_H264,
    AML_MP_VIDEO_CODEC_VC1,
    AML_MP_VIDEO_CODEC_AVS,
    AML_MP_VIDEO_CODEC_HEVC,
    AML_MP_VIDEO_CODEC_VP9,
    AML_MP_VIDEO_CODEC_AVS2,
    AML_MP_VIDEO_CODEC_MAX,
    AML_MP_VIDEO_CODEC_UNSUPPORT = AML_MP_VIDEO_CODEC_MAX
} Aml_MP_VideoCodec;

typedef enum {
    AML_MP_AUDIO_CODEC_UNKNOWN = -1,
    AML_MP_AUDIO_CODEC_MP2 = 1,                // MPEG audio
    AML_MP_AUDIO_CODEC_MP3 = 2,                // MP3
    AML_MP_AUDIO_CODEC_AC3 = 3,                // AC3
    AML_MP_AUDIO_CODEC_EAC3 = 4,               // DD PLUS
    AML_MP_AUDIO_CODEC_DTS = 5,                // DD PLUS
    AML_MP_AUDIO_CODEC_AAC = 6,                // AAC
    AML_MP_AUDIO_CODEC_LATM = 7,               // AAC LATM
    AML_MP_AUDIO_CODEC_PCM = 8,                // PCM
} Aml_MP_AudioCodec;

typedef struct {
    uint16_t        		pid;
    Aml_MP_VideoCodec       videoCodec;
    uint32_t        		width;
    uint32_t        		height;
    uint32_t        		frameRate;
    uint32_t        		extraDataSize;
    uint8_t         		*extraData;
} Aml_MP_VideoParams;

typedef struct {
    uint16_t                pid;
    Aml_MP_AudioCodec       audioCodec;
    uint32_t                nChannels;
    uint32_t                nSampleRate;
    uint32_t                nExtraSize;
    uint8_t         		*extraData;
} Aml_MP_AudioParams;

////////////////////////////////////////
typedef enum {
    AML_MP_SUBTITLE_CODEC_UNKNOWN    = 0,
    AML_MP_SUBTITLE_CODEC_CC         = 2,
    AML_MP_SUBTITLE_CODEC_SCTE27     = 3,
    AML_MP_SUBTITLE_CODEC_DVB        = 4,
    AML_MP_SUBTITLE_CODEC_TELETEXT   = 5,
} Aml_MP_SubtitleCodec;

typedef struct {
    int ChannelID;
    Aml_MP_VideoCodec videoCodec;
} Aml_MP_CCParam;

typedef struct {
    int pid;
} Aml_MP_SCTE27Param;

typedef struct  {
    int pid;
    int CompositionPage;
    int AncillaryPage;
}Aml_MP_DvbSubtitleParam;

typedef struct {
    int pid;
    int MagzineNo;
    int PageNo;
    int SubPageNo;
} Aml_MP_TeletextParam;

typedef struct {
    Aml_MP_SubtitleCodec subtitleCodec;
    int pid;
    Aml_MP_VideoCodec videoFormat;     //cc
    int channelId;                     //cc
    int ancillaryPageId;               //dvb
    int compositionPageId;             //dvb
} Aml_MP_SubtitleParams;

////////////////////////////////////////
typedef enum {
    AML_MP_CAS_UNKNOWN,
    AML_MP_CAS_VERIMATRIX_DVB,
    AML_MP_CAS_VERIMATRIX_IPTV,
    AML_MP_CAS_VERIMATRIX_WEB,
    AML_MP_CAS_IRDETO,
} Aml_MP_CASType;

typedef struct {
    Aml_MP_VideoCodec videoCodec;
    Aml_MP_AudioCodec audioCodec;
    int videoPid;
    int audioPid;
    int ecmPid;
    Aml_MP_DemuxId demuxId;

    char serverAddress[100];
    int serverPort;
    char keyPath[100];
} Aml_MP_IptvCasParam;

typedef enum {
    AML_MP_CAS_LIVE_PLAY,
    AML_MP_CAS_PVR_RECORDING,
    AML_MP_CAS_PVR_PLAY,
    AML_MP_CAS_INVALID_SERVICE,
} Aml_MP_CAsServiceType;

typedef struct {
    int serviceId;
    Aml_MP_DemuxId demuxId;
    int descrambleDeviceId;
    int ecmPid;
    int emmPid;
    Aml_MP_CAsServiceType serviceType;
    int streamPids[10];
    int streamNum;
    uint8_t privateData[100];
    int privateDataSize;
} Aml_MP_DvbCasParam;

typedef struct {
    Aml_MP_CASType type;
    union {
        Aml_MP_IptvCasParam iptvCasParam;
        Aml_MP_DvbCasParam dvbCasParam;
        struct {
            uint8_t data[1024];
            size_t size;
        } casParam;
    };
} Aml_MP_CASParams;

////////////////////////////////////////
typedef enum {
    AML_MP_STREAM_TYPE_UNKNOWN,
    AML_MP_STREAM_TYPE_VIDEO,
    AML_MP_STREAM_TYPE_AUDIO,
    AML_MP_STREAM_TYPE_AD,
    AML_MP_STREAM_TYPE_SUBTITLE,
} Aml_MP_StreamType;

////////////////////////////////////////
typedef struct {
    int size;
    int dataLen;
    int bufferedMs;
} Aml_MP_BufferItem;

typedef struct {
    Aml_MP_BufferItem audioBuffer;
    Aml_MP_BufferItem videoBuffer;
    Aml_MP_BufferItem subtitleBuffer;
} Aml_MP_BufferStat;

///////////////////////////////////////////////////////////////////////////////
typedef enum {
    //set/get
    AML_MP_PLAYER_PARAMETER_SET_BASE        = 0x1000,
    AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE,             //setVideoDisplayMode(Aml_MP_VideoDisplayMode)
    AML_MP_PLAYER_PARAMETER_BLACK_OUT,                      //setVideoBlackOut(bool)
    AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE,              //setVideoDecodeMode(Aml_MP_VideoDecodeMode)
    AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET,               //setVideoPtsOffset(int ms)
    AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE,              //setAudioOutputMode(Aml_MP_AudioOutputMode)
    AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE,            //setAudioOutputDevice(Aml_MP_AudioOutputDevice)
    AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET,               //setAudioPtsOffset(int ms)
    AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE,                  //setAudioBalance(Aml_MP_AudioBalance)

    AML_MP_PLAYER_PARAMETER_NETWORK_JITTER,                 //setNetworkJitter(int ms)

    AML_MP_PLAYER_PARAMETER_AD_STATE,                       //setADState(int)
    AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL,                   //setADMixLevel(Aml_MP_ADVolume)


    //get only
    AML_MP_PLAYER_PARAMETER_GET_BASE        = 0x2000,
    AML_MP_PLAYER_PARAMETER_VIDEO_INFO,                     //getVideoInfo(Aml_MP_VideoInfo)
    AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT,              //getVdecStat(Aml_MP_VdecStat)
    AML_MP_PLAYER_PARAMETER_AUDIO_INFO,                     //getAudioInfo(Aml_MP_AudioInfo)
    AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT,              //getAudioDecStat(Aml_MP_AdecStat)
    AML_MP_PLAYER_PARAMETER_SUBTITLE_INFO,                  //getSubtitleInfo(Aml_MP_SubtitleInfo)
    AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT,           //getSubtitleDecStat(Aml_MP_SubDecStat)
    AML_MP_PLAYER_PARAMETER_AD_INFO,                        //getADInfo(Aml_MP_AudioInfo)
    AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT,                 //getADDecodeStat(Aml_MP_AdecStat)
} Aml_MP_PlayerParameterKey;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE
typedef enum {
    AML_MP_VIDEO_DISPLAY_MODE_NORMAL,
    AML_MP_VIDEO_DISPLAY_MODE_FULLSCREEN,
    AML_MP_VIDEO_DISPLAY_MODE_LETTER_BOX,
    AML_MP_VIDEO_DISPLAY_MODE_PAN_SCAN,
    AML_MP_VIDEO_DISPLAY_MODE_COMBINED,
    AML_MP_VIDEO_DISPLAY_MODE_WIDTHFULL,
    AML_MP_VIDEO_DISPLAY_MODE_HEIGHTFULL,
} Aml_MP_VideoDisplayMode;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE
typedef enum {
    AML_MP_VIDEO_DECODE_MODE_NONE,
    AML_MP_VIDEO_DECODE_MODE_IONLY,
} Aml_MP_VideoDecodeMode;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE
typedef enum {
    AML_MP_AUDIO_OUTPUT_PCM,
    AML_MP_AUDIO_OUTPUT_PASSTHROUGH,
    AML_MP_AUDIO_OUTPUT_AUTO,
} Aml_MP_AudioOutputMode;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE
typedef enum {
    AML_MP_AUDIO_OUTPUT_DEVICE_NORMAL,
    AML_MP_AUDIO_OUTPUT_DEVICE_BT,
} Aml_MP_AudioOutputDevice;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE
typedef enum {
    AML_MP_AUDIO_BALANCE_STEREO,
    AML_MP_AUDIO_BALANCE_LEFT,
    AML_MP_AUDIO_BALANCE_RIGHT,
    AML_MP_AUDIO_BALANCE_SWAP,
    AML_MP_AUDIO_BALANCE_LRMIX,
} Aml_MP_AudioBalance;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL
typedef struct {
    int masterVolume;
    int slaveVolume;
} Aml_MP_ADVolume;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_VIDEO_INFO
typedef struct {
    int width;
    int height;
    int frameRate;
    int bitrate;
    int ratio64;
} Aml_MP_VideoInfo;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT
typedef struct {
    uint32_t num;
    uint32_t type;
    uint32_t size;
    uint32_t pts;
    uint32_t max_qp;
    uint32_t avg_qp;
    uint32_t min_qp;
    uint32_t max_skip;
    uint32_t avg_skip;
    uint32_t min_skip;
    uint32_t max_mv;
    uint32_t min_mv;
    uint32_t avg_mv;
    uint32_t decode_buffer;
} Aml_MP_VideoQos;

typedef struct {
    Aml_MP_VideoQos qos;
    uint32_t  decode_time_cost;/*us*/
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_rate;
    uint32_t bit_depth_luma;//original bit_rate;
    uint32_t frame_dur;
    uint32_t bit_depth_chroma;//original frame_data;
    uint32_t error_count;
    uint32_t status;
    uint32_t frame_count;
    uint32_t error_frame_count;
    uint32_t drop_frame_count;
    uint64_t total_data;
    uint32_t double_write_mode;//original samp_cnt;
    uint32_t offset;
    uint32_t ratio_control;
    uint32_t vf_type;
    uint32_t signal_type;
    uint32_t pts;
    uint64_t pts_us64;
} Aml_MP_VdecStat;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AUDIO_INFO
typedef struct {
    uint32_t sample_rate;                  // Audio sample rate
    uint32_t channels;                     // Audio channels
    uint32_t channel_mask;                 // Audio channel mask
    uint32_t bitrate;                      // Audio bit rate
} Aml_MP_AudioInfo;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT
typedef struct {
    uint32_t frame_count;
    uint32_t error_frame_count;
    uint32_t drop_frame_count;
} Aml_MP_AdecStat;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_SUBTITLE_INFO
typedef struct {

} Aml_MP_SubtitleInfo;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT
typedef struct {

} Aml_MP_SubDecStat;


///////////////////////////////////////////////////////////////////////////////
typedef enum {
    AML_MP_AVSYNC_SOURCE_DEFAULT,
    AML_MP_AVSYNC_SOURCE_VIDEO,
    AML_MP_AVSYNC_SOURCE_AUDIO,
    AML_MP_AVSYNC_SOURCE_PCR,
} Aml_MP_AVSyncSource;

///////////////////////////////////////////////////////////////////////////////
enum {
	Aml_MP_ERROR_BASE = -2000,

};




#endif
