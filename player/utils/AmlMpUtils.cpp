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

#define LOG_TAG "AmlMpUtils"
#include <utils/Log.h>
#include "AmlMpUtils.h"

namespace aml_mp {

vformat_t convertToVFormat(Aml_MP_VideoCodec videoCodec)
{
    vformat_t vFormat;

    switch (videoCodec) {
    case AML_MP_VIDEO_CODEC_MPEG12:
        vFormat = VFORMAT_MPEG12;
        break;

    case AML_MP_VIDEO_CODEC_MPEG4:
        vFormat = VFORMAT_MPEG4;
        break;

    case AML_MP_VIDEO_CODEC_H264:
        vFormat = VFORMAT_H264;
        break;

    case AML_MP_VIDEO_CODEC_VC1:
        vFormat = VFORMAT_VC1;
        break;

    case AML_MP_VIDEO_CODEC_AVS:
        vFormat = VFORMAT_AVS;
        break;

    case AML_MP_VIDEO_CODEC_HEVC:
        vFormat = VFORMAT_HEVC;
        break;

    case AML_MP_VIDEO_CODEC_VP9:
        vFormat = VFORMAT_VP9;
        break;

    case AML_MP_VIDEO_CODEC_AVS2:
        vFormat = VFORMAT_AVS2;
        break;

    default:
        ALOGE("unknown videoCodec:%d", videoCodec);
        break;
    }

    return vFormat;

}

aformat_t convertToAForamt(Aml_MP_AudioCodec audioCodec)
{
    aformat_t aFormat;

    switch (audioCodec) {
    case AML_MP_AUDIO_CODEC_MP2:
        aFormat = AFORMAT_MPEG2;
        break;

    case AML_MP_AUDIO_CODEC_MP3:
        aFormat = AFORMAT_MPEG;
        break;

    case AML_MP_AUDIO_CODEC_AC3:
        aFormat = AFORMAT_AC3;
        break;

    case AML_MP_AUDIO_CODEC_EAC3:
        aFormat = AFORMAT_EAC3;
        break;

    case AML_MP_AUDIO_CODEC_DTS:
        aFormat = AFORMAT_DTS;
        break;

    case AML_MP_AUDIO_CODEC_AAC:
        aFormat = AFORMAT_AAC;
        break;

    case AML_MP_AUDIO_CODEC_LATM:
        aFormat = AFORMAT_AAC_LATM;
        break;

    case AML_MP_AUDIO_CODEC_PCM:
        aFormat = AFORMAT_PCM_S16LE;
        break;

    default:
        ALOGE("unknown audioCodec:%d", audioCodec);
        break;
    }

    return aFormat;
}

}
