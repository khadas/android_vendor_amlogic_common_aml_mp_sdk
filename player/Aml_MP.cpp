/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMp"
#define mName LOG_TAG
#include <utils/AmlMpLog.h>
#include <Aml_MP/Aml_MP.h>
#include <utils/AmlMpUtils.h>

///////////////////////////////////////////////////////////////////////////////
#define AML_MP_VERSION_MAJOR   1
#define AML_MP_VERSION_MINOR   0

#define AML_MP_VERSION_INT     AML_MP_VERSION_MAJOR << 16 | AML_MP_VERSION_MINOR
#define AML_MP_VERSION         "AML_MP v" AML_MP_STRINGIFY(AML_MP_VERSION_GLUE(AML_MP_VERSION_MAJOR, AML_MP_VERSION_MINOR))

#define AML_MP_VERSION_GLUE(a, b)   AML_MP_VERSION_DOT(a, b)
#define AML_MP_VERSION_DOT(a, b)   a ## . ## b
#define AML_MP_STRINGIFY(x)    AML_MP_TOSTRING(x)
#define AML_MP_TOSTRING(x)     #x

///////////////////////////////////////////////////////////////////////////////

int Aml_MP_Initialize()
{
    MLOG();

    return 0;
}

int Aml_MP_GetVersion(const char** versionString)
{
    if (versionString) {
        *versionString = AML_MP_VERSION;
    }

    return AML_MP_VERSION_INT;
}

int Aml_MP_SetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource source)
{
    MLOG("demuxId:%d, source:%d", demuxId, source);

    return dvb_set_demux_source(demuxId, aml_mp::convertToDVBDemuxSource(source));
}

int Aml_MP_GetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource* source)
{
    MLOG("demuxId:%d", demuxId);

    DVB_DemuxSource_t demuxSource;
    int ret = dvb_get_demux_source(demuxId, &demuxSource);
    *source = aml_mp::convertToMpDemuxSource(demuxSource);
    return ret;
}





