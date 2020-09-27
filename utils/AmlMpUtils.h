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

#ifndef _AML_MP_UTILS_H_
#define _AML_MP_UTILS_H_

#include <utils/Log.h>
#include <chrono>
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <Aml_MP/Common.h>
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
            ALOGE("%s:%d return %s <- %s", __FUNCTION__, __LINE__, #error, #cond);   \
            return error;                                                            \
        }                                                                            \
    } while (0)

#define RETURN_VOID_IF(cond)                                                         \
    do {                                                                             \
        if (cond) {                                                                  \
            ALOGE("%s:%d return <- %s", __FUNCTION__, __LINE__, #cond);              \
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
            ALOG(LOG_DEBUG, m_tag, "%s() >> begin", m_func);
        }
	}

	~FunctionLogger()
	{
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - mBeginTime).count();

        if (mVerbose) {
            ALOG(LOG_DEBUG, m_tag, "%s() << end %lld ms", m_func, duration);
        }

		if(duration >= mThreshold)
			ALOG(LOG_WARN, m_tag, "%s() takes %lld ms, Seems slow, check!", m_func, duration);
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

am_tsplayer_audio_out_mode convertToTsPlayerAudioOutMode(Aml_MP_AudioOutputMode audioOutputMode);
void convertToMpVideoInfo(Aml_MP_VideoInfo* mpVideoInfo, am_tsplayer_video_info* tsVideoInfo);

DVB_DemuxSource_t convertToDVBDemuxSource(Aml_MP_DemuxSource source);
Aml_MP_DemuxSource convertToMpDemuxSource(DVB_DemuxSource_t source);


}
#endif
