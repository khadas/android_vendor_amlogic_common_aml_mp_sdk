/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_COMMON_H_
#define _AML_MP_COMMON_H_

#include <stdlib.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
//typedef void* AML_MP_HANDLE;
#define AML_MP_INVALID_HANDLE   (0)
#define AML_MP_INVALID_PID      (0x1FFF)
#define AML_MP_MAX_PATH_SIZE    512


typedef void* AML_MP_PLAYER;
typedef void* AML_MP_DVRRECORDER;
typedef void* AML_MP_DVRPLAYER;
typedef void* AML_MP_CASSESSION;
typedef void* AML_MP_SECMEM;

///////////////////////////////////////////////////////////////////////////////
typedef enum {
    AML_MP_CHANNEL_ID_AUTO             = 0,
    AML_MP_CHANNEL_ID_MAIN,
    AML_MP_CHANNEL_ID_PIP,
} Aml_MP_ChannelId;

typedef enum {
    AML_MP_DEMUX_ID_DEFAULT         = -1,
    AML_MP_HW_DEMUX_ID_0            = 0,
    AML_MP_HW_DEMUX_ID_1,
    AML_MP_HW_DEMUX_ID_2,
    AML_MP_HW_DEMUX_ID_3,
    AML_MP_HW_DEMUX_ID_4,
    AML_MP_HW_DEMUX_ID_5,
    AML_MP_HW_DEMUX_ID_6,
    AML_MP_HW_DEMUX_ID_7,
    AML_MP_HW_DEMUX_ID_MAX,
    AML_MP_HW_DEMUX_NB              = (AML_MP_HW_DEMUX_ID_MAX - AML_MP_HW_DEMUX_ID_0),
} Aml_MP_DemuxId;

typedef enum {
    AML_MP_DEMUX_SOURCE_TS0,  /**< Hardware TS input port 0.*/
    AML_MP_DEMUX_SOURCE_TS1,  /**< Hardware TS input port 1.*/
    AML_MP_DEMUX_SOURCE_TS2,  /**< Hardware TS input port 2.*/
    AML_MP_DEMUX_SOURCE_TS3,  /**< Hardware TS input port 3.*/
    AML_MP_DEMUX_SOURCE_TS4,  /**< Hardware TS input port 4.*/
    AML_MP_DEMUX_SOURCE_TS5,  /**< Hardware TS input port 5.*/
    AML_MP_DEMUX_SOURCE_TS6,  /**< Hardware TS input port 6.*/
    AML_MP_DEMUX_SOURCE_TS7,  /**< Hardware TS input port 7.*/
    AML_MP_DEMUX_SOURCE_DMA0,  /**< DMA input port 0.*/
    AML_MP_DEMUX_SOURCE_DMA1,  /**< DMA input port 1.*/
    AML_MP_DEMUX_SOURCE_DMA2,  /**< DMA input port 2.*/
    AML_MP_DEMUX_SOURCE_DMA3,  /**< DMA input port 3.*/
    AML_MP_DEMUX_SOURCE_DMA4,  /**< DMA input port 4.*/
    AML_MP_DEMUX_SOURCE_DMA5,  /**< DMA input port 5.*/
    AML_MP_DEMUX_SOURCE_DMA6,  /**< DMA input port 6.*/
    AML_MP_DEMUX_SOURCE_DMA7  /**< DMA input port 7.*/
} Aml_MP_DemuxSource;

///////////////////////////////////////////////////////////////////////////////
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

typedef struct {
    Aml_MP_InputBufferType type;
    uint8_t* address;
    size_t size;
} Aml_MP_Buffer;

////////////////////////////////////////
typedef enum {
    AML_MP_CODEC_UNKNOWN = -1,
    AML_MP_VIDEO_CODEC_BASE = 0,
    AML_MP_VIDEO_CODEC_MPEG12,
    AML_MP_VIDEO_CODEC_MPEG4,
    AML_MP_VIDEO_CODEC_H264,
    AML_MP_VIDEO_CODEC_VC1,
    AML_MP_VIDEO_CODEC_AVS,
    AML_MP_VIDEO_CODEC_HEVC,
    AML_MP_VIDEO_CODEC_VP9,
    AML_MP_VIDEO_CODEC_AVS2,

    AML_MP_AUDIO_CODEC_BASE = 1000,
    AML_MP_AUDIO_CODEC_MP2,                // MPEG audio
    AML_MP_AUDIO_CODEC_MP3,                // MP3
    AML_MP_AUDIO_CODEC_AC3,                // AC3
    AML_MP_AUDIO_CODEC_EAC3,               // DD PLUS
    AML_MP_AUDIO_CODEC_DTS,                // DD PLUS
    AML_MP_AUDIO_CODEC_AAC,                // AAC
    AML_MP_AUDIO_CODEC_LATM,               // AAC LATM
    AML_MP_AUDIO_CODEC_PCM,                // PCM

    AML_MP_SUBTITLE_CODEC_BASE = 2000,
    AML_MP_SUBTITLE_CODEC_CC,
    AML_MP_SUBTITLE_CODEC_SCTE27,
    AML_MP_SUBTITLE_CODEC_DVB,
    AML_MP_SUBTITLE_CODEC_TELETEXT,
} Aml_MP_CodecID;

typedef struct {
    uint16_t                pid;
    Aml_MP_CodecID          videoCodec;
    uint32_t                width;
    uint32_t                height;
    uint32_t                frameRate;
    uint8_t                 extraData[512];
    uint32_t                extraDataSize;
} Aml_MP_VideoParams;

typedef struct {
    uint16_t                pid;
    Aml_MP_CodecID          audioCodec;
    uint32_t                nChannels;
    uint32_t                nSampleRate;
    uint8_t                 extraData[512];
    uint32_t                extraDataSize;
} Aml_MP_AudioParams;

////////////////////////////////////////
typedef struct {
    int pid;
    Aml_MP_CodecID subtitleCodec;
    Aml_MP_CodecID videoFormat;        //cc
    int channelId;                     //cc
    int ancillaryPageId;               //dvb
    int compositionPageId;             //dvb
} Aml_MP_SubtitleParams;

////////////////////////////////////////
typedef enum {
    AML_MP_CAS_UNKNOWN,
    AML_MP_CAS_VERIMATRIX_IPTV,
    AML_MP_CAS_WIDEVINE,
} Aml_MP_CASType;

//add for get url info from setdatasource,such as wv
typedef struct {
    char* license_url;
    char* provision_url;
    char* request_header;
    char* request_body;
    char* content_type;
}Aml_MP_IptvCasHeaders;


typedef struct {
    Aml_MP_CASType type;
    Aml_MP_CodecID videoCodec;
    Aml_MP_CodecID audioCodec;
    unsigned int  caSystemId;
    int videoPid;
    int audioPid;
    int ecmPid[3];
    Aml_MP_DemuxId demuxId;

    //add for vmx get url info
    char serverAddress[AML_MP_MAX_PATH_SIZE];
    int serverPort;
    char keyPath[AML_MP_MAX_PATH_SIZE];

    //add for private data to cas
    //should care audio and video not same
    uint8_t private_data[AML_MP_MAX_PATH_SIZE];
    size_t private_size;

    Aml_MP_IptvCasHeaders *headers;
} Aml_MP_IptvCasParams;

////////////////////////////////////////
typedef enum {
    AML_MP_STREAM_TYPE_UNKNOWN = 0,
    AML_MP_STREAM_TYPE_VIDEO,
    AML_MP_STREAM_TYPE_AUDIO,
    AML_MP_STREAM_TYPE_AD,
    AML_MP_STREAM_TYPE_SUBTITLE,
    AML_MP_STREAM_TYPE_TELETEXT,
    AML_MP_STREAM_TYPE_ECM,
    AML_MP_STREAM_TYPE_EMM,
    AML_MP_STREAM_TYPE_SECTION,
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
    AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE,             //setVideoDisplayMode(Aml_MP_VideoDisplayMode*)
    AML_MP_PLAYER_PARAMETER_BLACK_OUT,                      //setVideoBlackOut(bool*)
    AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE,              //setVideoDecodeMode(Aml_MP_VideoDecodeMode*)
    AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET,               //setVideoPtsOffset(int* ms)
    AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE,              //setAudioOutputMode(Aml_MP_AudioOutputMode*)
    AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE,            //setAudioOutputDevice(Aml_MP_AudioOutputDevice*)
    AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET,               //setAudioPtsOffset(int* ms)
    AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE,                  //setAudioBalance(Aml_MP_AudioBalance*)
    AML_MP_PLAYER_PARAMETER_AUDIO_MUTE,                     //setAudioMute(bool*)
    AML_MP_PLAYER_PARAMETER_CREATE_PARAMS,                  //setCreateParams(Aml_MP_PlayerCreateParams*)

    AML_MP_PLAYER_PARAMETER_NETWORK_JITTER,                 //setNetworkJitter(int* ms)

    AML_MP_PLAYER_PARAMETER_AD_STATE,                       //setADState(int*)
    AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL,                   //setADMixLevel(Aml_MP_ADVolume*)

    AML_MP_PLAYER_PARAMETER_WORK_MODE,                      //setWorkMode(Aml_MP_PlayerWorkMode*)
    AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER,            //setZorder(int*)
    AML_MP_PLAYER_PARAMETER_TELETEXT_CONTROL,               //amlsub_TeletextControl(AML_MP_TeletextCtrlParam*)

    AML_MP_PLAYER_PARAMETER_VENDOR_ID,                      //setVendorID(int*)
    AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID,                //setVideoTunnelID(int*)
    AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE,                 //setSurface(void*)

    //get only
    AML_MP_PLAYER_PARAMETER_GET_BASE        = 0x2000,
    AML_MP_PLAYER_PARAMETER_VIDEO_INFO,                     //getVideoInfo(Aml_MP_VideoInfo*)
    AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT,              //getVdecStat(Aml_MP_VdecStat*)
    AML_MP_PLAYER_PARAMETER_AUDIO_INFO,                     //getAudioInfo(Aml_MP_AudioInfo*)
    AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT,              //getAudioDecStat(Aml_MP_AdecStat*)
    AML_MP_PLAYER_PARAMETER_SUBTITLE_INFO,                  //getSubtitleInfo(Aml_MP_SubtitleInfo*)
    AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT,           //getSubtitleDecStat(Aml_MP_SubDecStat*)
    AML_MP_PLAYER_PARAMETER_AD_INFO,                        //getADInfo(Aml_MP_AudioInfo*)
    AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT,                 //getADDecodeStat(Aml_MP_AdecStat*)
    AML_MP_PLAYER_PARAMETER_INSTANCE_ID,                    //getInstanceId(uint32_t*)
    AML_MP_PLAYER_PARAMETER_SYNC_ID,                        //getSyncId(int32_t*)
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
//AML_MP_PLAYER_PARAMETER_PLAYER_WORK_MODE
typedef enum {
    AML_MP_PLAYER_MODE_NORMAL = 0,             // Normal mode
    AML_MP_PLAYER_MODE_CACHING_ONLY = 1,       // Only caching data, do not decode. Used in FCC
    AML_MP_PLAYER_MODE_DECODE_ONLY = 2         // Decode data but do not output
} Aml_MP_PlayerWorkMode;

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
//AML_MP_SUBTITLE_EVENT_DATA
typedef enum {
    AML_MP_SUB_DATA_TYPE_STRING = 0,
    AML_MP_SUB_DATA_TYPE_CC_JSON = 1,
    AML_MP_SUB_DATA_TYPE_BITMAP = 2,
    AML_MP_SUB_DATA_TYPE_POSITON_BITMAP = 4,
}AML_MP_SubtitleDataType;

typedef struct {
    const char *data;
    int size;
    AML_MP_SubtitleDataType type;
    int x;
    int y;
    int width;
    int height;
    int videoWidth;
    int videoHeight;
    int showing;
}Aml_MP_SubtitleData;

////////////////////////////////////////
//AML_MP_SUBTITLE_EVENT_DIMENSION
typedef struct {
    uint32_t width;
    uint32_t height;
} Aml_MP_SubtitleDimension;

////////////////////////////////////////
//AML_MP_SUBTITLE_EVENT_CHANNEL_UPDATE
typedef struct {
    int event;
    int id;
}Aml_MP_SubtitleChannelUpdate;

////////////////////////////////////////
//AML_MP_SUBTITLE_EVENT_SUBTITLE_INFO
typedef struct {
    int what;
    int extra;
}Aml_MP_SubtitleInfo;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT
typedef struct {
    uint32_t frameCount;
    uint32_t errorFrameCount;
    uint32_t dropFrameCount;
} Aml_MP_SubDecStat;

////////////////////////////////////////
//AML_MP_PLAYER_PARAMETER_TELETEXT_CONTROL
typedef enum {
    AML_MP_TT_EVENT_INVALID          = -1,

    // These are the four FastText shortcuts, usually represented by red, green,
    // yellow and blue keys on the handset.
    AML_MP_TT_EVENT_QUICK_NAVIGATE_RED = 0,
    AML_MP_TT_EVENT_QUICK_NAVIGATE_GREEN,
    AML_MP_TT_EVENT_QUICK_NAVIGATE_YELLOW,
    AML_MP_TT_EVENT_QUICK_NAVIGATE_BLUE,

    // The ten numeric keys used to input page indexes.
    AML_MP_TT_EVENT_0,
    AML_MP_TT_EVENT_1,
    AML_MP_TT_EVENT_2,
    AML_MP_TT_EVENT_3,
    AML_MP_TT_EVENT_4,
    AML_MP_TT_EVENT_5,
    AML_MP_TT_EVENT_6,
    AML_MP_TT_EVENT_7,
    AML_MP_TT_EVENT_8,
    AML_MP_TT_EVENT_9,

    // This is the home key, which returns to the nominated index page for this service.
    AML_MP_TT_EVENT_INDEXPAGE,

    // These are used to quickly increment/decrement the page index.
    AML_MP_TT_EVENT_NEXTPAGE,
    AML_MP_TT_EVENT_PREVIOUSPAGE,

    // These are used to navigate the sub-pages when in 'hold' mode.
    AML_MP_TT_EVENT_NEXTSUBPAGE,
    AML_MP_TT_EVENT_PREVIOUSSUBPAGE,

    // These are used to traverse the page history (if caching requested).
    AML_MP_TT_EVENT_BACKPAGE,
    AML_MP_TT_EVENT_FORWARDPAGE,

    // This is used to toggle hold on the current page.
    AML_MP_TT_EVENT_HOLD,
    // Reveal hidden page content (as defined in EBU specification)
    AML_MP_TT_EVENT_REVEAL,
    // This key toggles 'clear' mode (page hidden until updated)
    AML_MP_TT_EVENT_CLEAR,
    // This key toggles 'clock only' mode (page hidden until updated)
    AML_MP_TT_EVENT_CLOCK,
    // Used to toggle transparent background ('video mix' mode)
    AML_MP_TT_EVENT_MIX_VIDEO,
    // Used to toggle double height top / double-height bottom / normal height display.
    AML_MP_TT_EVENT_DOUBLE_HEIGHT,
    // Functional enhancement may offer finer scrolling of double-height display.
    AML_MP_TT_EVENT_DOUBLE_SCROLL_UP,
    AML_MP_TT_EVENT_DOUBLE_SCROLL_DOWN,
    // Used to initiate/cancel 'timer' mode (clear and re-display page at set time)
    AML_MP_TT_EVENT_TIMER,
    AML_MP_TT_EVENT_GO_TO_PAGE,
    AML_MP_TT_EVENT_GO_TO_SUBTITLE
} Aml_MP_TeletextEvent;

typedef struct {
    int magazine;
    int page;
    Aml_MP_TeletextEvent event;
} AML_MP_TeletextCtrlParam;

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

///////////////////////////////////////////////////////////////////////////////
#define AML_MP_DVR_STREAMS_COUNT 16

typedef struct {
    Aml_MP_StreamType   type;
    int                 pid;
    Aml_MP_CodecID      codecId;
} Aml_MP_DVRStream;

typedef struct {
    int nbStreams;
    Aml_MP_DVRStream streams[AML_MP_DVR_STREAMS_COUNT];
#define AML_MP_DVR_VIDEO_INDEX      0
#define AML_MP_DVR_AUDIO_INDEX      1
#define AML_MP_DVR_AD_INDEX         2
#define AML_MP_DVR_SUBTITLE_INDEX   3
#define AML_MP_DVR_PCR_INDEX        4
#define AML_MP_DVR_STREAM_NB        5
} Aml_MP_DVRStreamArray;

typedef struct {
  time_t              time;       /**< time duration, unit on ms*/
  loff_t              size;       /**< size*/
  uint32_t            pkts;       /**< number of ts packets*/
} Aml_MP_DVRSourceInfo;

///////////////////////////////////////////////////////////////////////////////
typedef enum {
    AML_MP_EVENT_UNKNOWN,
    AML_MP_PLAYER_EVENT_VIDEO_CHANGED,
    AML_MP_PLAYER_EVENT_AUDIO_CHANGED,
    AML_MP_PLAYER_EVENT_FIRST_FRAME,
    AML_MP_PLAYER_EVENT_AV_SYNC_DONE,

    AML_MP_PLAYER_EVENT_DATA_LOSS,
    AML_MP_PLAYER_EVENT_DATA_RESUME,
    AML_MP_PLAYER_EVENT_SCRAMBLING,

    AML_MP_PLAYER_EVENT_USERDATA_AFD,
    AML_MP_PLAYER_EVENT_USERDATA_CC,

    AML_MP_PLAYER_EVENT_PID_CHANGED,

    //DVR player
    AML_MP_DVRPLAYER_EVENT_ERROR              = 0x1000,   /**< Signal a critical playback error*/
    AML_MP_DVRPLAYER_EVENT_TRANSITION_OK    ,             /**< transition ok*/
    AML_MP_DVRPLAYER_EVENT_TRANSITION_FAILED,             /**< transition failed*/
    AML_MP_DVRPLAYER_EVENT_KEY_FAILURE,                   /**< key failure*/
    AML_MP_DVRPLAYER_EVENT_NO_KEY,                        /**< no key*/
    AML_MP_DVRPLAYER_EVENT_REACHED_BEGIN     ,            /**< reached begin*/
    AML_MP_DVRPLAYER_EVENT_REACHED_END,                    /**< reached end*/
    AML_MP_DVRPLAYER_EVENT_NOTIFY_PLAYTIME,               /**< notify play cur segmeng time ms*/

    //Subtitle event
    AML_MP_SUBTITLE_EVENT_BASE = 0x2000,
    AML_MP_SUBTITLE_EVENT_DATA,                         //param: Aml_MP_SubtitleData
    AML_MP_SUBTITLE_EVENT_SUBTITLE_AVAIL,               //param: int
    AML_MP_SUBTITLE_EVENT_DIMENSION,                    //param: Aml_MP_SubtitleDimension
    AML_MP_SUBTITLE_EVENT_AFD_EVENT,                    //param: int
    AML_MP_SUBTITLE_EVENT_CHANNEL_UPDATE,               //param: Aml_MP_SubtitleChannelUpdate
    AML_MP_SUBTITLE_EVENT_SUBTITLE_LANGUAGE,            //param: char[4]
    AML_MP_SUBTITLE_EVENT_SUBTITLE_INFO,                //param: Aml_MP_SubtitleInfo
} Aml_MP_PlayerEventType;


//AML_MP_PLAYER_EVENT_VIDEO_CHANGED,
typedef struct {
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_rate;
    uint32_t frame_aspectratio;
} Aml_MP_PlayerEventVideoFormat;

//AML_MP_PLAYER_EVENT_AUDIO_CHANGED,
typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
} Aml_MP_PlayerEventAudioFormat;

//AML_MP_PLAYER_EVENT_SCRAMBLING,
typedef struct {
    Aml_MP_StreamType type;
    char scramling;
} Aml_MP_PlayerEventScrambling;

//AML_MP_PLAYER_EVENT_PID_CHANGED
typedef struct {
    int programPid;
    int programNumber;
    int oldStreamPid;
    int newStreamPid;
} Aml_MP_PlayerEventPidChangeInfo;


//AML_MP_PLAYER_EVENT_USERDATA_AFD,
//AML_MP_PLAYER_EVENT_USERDATA_CC,
typedef struct {
    uint8_t  *data;
    size_t   len;
} Aml_MP_PlayerEventMpegUserData;

typedef void (*Aml_MP_PlayerEventCallback)(void* userData, Aml_MP_PlayerEventType event, int64_t param);

typedef struct ANativeWindow ANativeWindow;

#endif
