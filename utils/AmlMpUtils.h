/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_UTILS_H_
#define _AML_MP_UTILS_H_

#include "AmlMpLog.h"
#include <chrono>
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <Aml_MP/Common.h>
#ifdef HAVE_SUBTITLE
#include <string>
#include <SubtitleNativeAPI.h>
#endif
#include "AmlMpConfig.h"

#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H
struct list_head {
    struct list_head *next, *prev;
};
#endif
#include <dvr_wrapper.h>
#include <dvb_utils.h>

namespace aml_mp {

#define AML_MP_UNUSED(x) (void)(x)

///////////////////////////////////////////////////////////////////////////////
#define RETURN_IF(error, cond)                                                       \
    do {                                                                             \
        if (cond) {                                                                  \
            MLOGE("%s:%d return %s <- %s", __FUNCTION__, __LINE__, #error, #cond);   \
            return error;                                                            \
        }                                                                            \
    } while (0)

#define RETURN_VOID_IF(cond)                                                         \
    do {                                                                             \
        if (cond) {                                                                  \
            MLOGE("%s:%d return <- %s", __FUNCTION__, __LINE__, #cond);              \
            return;                                                                  \
        }                                                                            \
    } while (0)

///////////////////////////////////////////////////////////////////////////////
#define AML_MP_TRACE(thresholdMs) FunctionLogger __flogger##__LINE__(mName, __FILE__, __FUNCTION__, __LINE__, thresholdMs, AmlMpConfig::instance().mLogDebug)

struct FunctionLogger
{
public:
    FunctionLogger(const char * tag, const char * file, const char * func, const int line, int threshold, bool verbose)
    : m_tag(tag), m_file(file), m_func(func), m_line(line), mThreshold(threshold), mVerbose(verbose)
    {
        (void)m_file;
        (void)m_line;

        mBeginTime = std::chrono::steady_clock::now();
        if (mVerbose) {
            ALOG(LOG_DEBUG, "Aml_MP", "%s %s() >> begin", m_tag, m_func);
        }
    }

    ~FunctionLogger()
    {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - mBeginTime).count();

        if (mVerbose) {
            ALOG(LOG_DEBUG, "Aml_MP", "%s %s() << end %lld ms", m_tag, m_func, duration);
        }

        if(duration >= mThreshold)
            ALOG(LOG_WARN, "Aml_MP", "%s %s() takes %lld ms, Seems slow, check!", m_tag, m_func, duration);
    }

private:
    const char * m_tag;
    const char * m_file;
    const char * m_func;
    const int m_line;
    int mThreshold;
    bool mVerbose;
    std::chrono::steady_clock::time_point mBeginTime;
};

///////////////////////////////////////////////////////////////////////////////
const char* mpCodecId2Str(Aml_MP_CodecID codecId);
const char* mpStreamType2Str(Aml_MP_StreamType streamType);

vformat_t convertToVFormat(Aml_MP_CodecID videoCodec);
aformat_t convertToAForamt(Aml_MP_CodecID audioCodec);

Aml_MP_StreamType convertToMpStreamType(DVR_StreamType_t streamType);
DVR_StreamType_t convertToDVRStreamType(Aml_MP_StreamType streamType);

Aml_MP_CodecID convertToMpCodecId(DVR_VideoFormat_t fmt);
Aml_MP_CodecID convertToMpCodecId(DVR_AudioFormat_t fmt);
DVR_VideoFormat_t convertToDVRVideoFormat(Aml_MP_CodecID codecId);
DVR_AudioFormat_t convertToDVRAudioFormat(Aml_MP_CodecID codecId);

void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamPid_t* dvrStream);
void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamInfo_t* dvrStreamInfo);
void convertToMpDVRSourceInfo(Aml_MP_DVRSourceInfo* dest, DVR_WrapperInfo_t* source);

am_tsplayer_video_match_mode convertToTsPlayerVideoMatchMode(Aml_MP_VideoDisplayMode videoDisplayMode);
am_tsplayer_video_trick_mode convertToTsplayerVideoTrickMode(Aml_MP_VideoDecodeMode videoDecodeMode);
am_tsplayer_audio_out_mode convertToTsPlayerAudioOutMode(Aml_MP_AudioOutputMode audioOutputMode);
void convertToMpVideoInfo(Aml_MP_VideoInfo* mpVideoInfo, am_tsplayer_video_info* tsVideoInfo);
am_tsplayer_audio_stereo_mode convertToTsPlayerAudioStereoMode(Aml_MP_AudioBalance audioBalance);

DVB_DemuxSource_t convertToDVBDemuxSource(Aml_MP_DemuxSource source);
Aml_MP_DemuxSource convertToMpDemuxSource(DVB_DemuxSource_t source);
am_tsplayer_stream_type convertToTsplayerStreamType(Aml_MP_StreamType streamType);
Aml_MP_StreamType convertToAmlMPStreamType(am_tsplayer_stream_type streamType);

#ifdef HAVE_SUBTITLE
AmlTeletextCtrlParam convertToTeletextCtrlParam(AML_MP_TeletextCtrlParam* teletextCtrlParam);
AmlTeletextEvent convertToTeletextEvent(Aml_MP_TeletextEvent teletextEvent);
AML_MP_SubtitleDataType convertToMpSubtitleDataType(AmlSubDataType subDataType);
#endif

bool isSupportMultiHwDemux();

}
#endif
