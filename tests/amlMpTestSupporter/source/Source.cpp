/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_Source"
#include <utils/AmlMpLog.h>
#include "Source.h"
#include "UdpSource.h"
#include "DvbSource.h"
#include "FileSource.h"

static const char* mName = LOG_TAG;

namespace aml_mp {

sptr<Source> Source::create(const char* url)
{
    char proto[10]{};
    char address[100]{};
    Aml_MP_DemuxId demuxId = AML_MP_HW_DEMUX_ID_0;
    int programNumber = -1;
    Aml_MP_DemuxSource sourceId = AML_MP_DEMUX_SOURCE_TS0;

    const char* p = nullptr;
    if ((p = strstr(url, "://")) == nullptr) {
        strcpy(proto, "file");
        p = url;
    } else {
        strncpy(proto, url, p - url);
        p += 3;
    }

    const char* p2 = strrchr(p, '?');
    if (p2 != nullptr) {
        strncpy(address, p, p2 - p);

        std::string arguments(p2+1);
        std::string::size_type pos = 0, prev = 0, equal = 0;
        std::string key, value;
        while (pos != std::string::npos) {
            pos = arguments.find_first_of('&', prev);
            if (pos != std::string::npos) {
                equal = arguments.find_first_of('=', prev);
                //MLOGI("pos:%d, equal:%d prev:%d", pos, equal, prev);
                key = arguments.substr(prev, equal-prev);
                value = arguments.substr(equal+1, pos-equal-1);

                prev = pos+1;
            } else {
                equal = arguments.find_first_of('=', prev);
                key = arguments.substr(prev, equal-prev);
                value = arguments.substr(equal+1);
            }

            MLOGI("key[%s], value[%s]", key.c_str(), value.c_str());
            if (key == "demuxid") {
                demuxId = (Aml_MP_DemuxId)std::stoi(value);
            } else if (key == "program") {
                programNumber = std::stoi(value);
            } else if (key == "sourceid") {
                sourceId = (Aml_MP_DemuxSource)std::stoi(value);
            }
        }
    } else {
        strncpy(address, p, sizeof(address)-1);
    }

    MLOGV("proto:%s, address:%s, programNumber:%d, demuxId:%d, sourceid:%d", proto, address, programNumber, demuxId, sourceId);

    bool isUdpSource = false;
    bool isDvbSource = false;
    bool isFileSource = false;
    bool isDVRSource = false;
    if (!strcmp(proto, "udp") || !strcmp(proto, "igmp")) {
        isUdpSource = true;
    } else if (!strncmp(proto, "dvb", 3)) {
        isDvbSource = true;
    } else if (!strncmp(proto, "file", 4)) {
        isFileSource = true;
    } else if (!strncmp(proto, "dvr", 3)) {
        isDVRSource = true;
    } else {
        MLOGE("unsupported proto:%s\n", proto);
        return nullptr;
    }

    uint32_t flags = 0;
    sptr<Source> source = nullptr;

    InputParameter inputParameter;
    inputParameter.demuxId = demuxId;
    inputParameter.programNumber = programNumber;
    inputParameter.sourceId = sourceId;

    if (isUdpSource) {
        flags |= Source::kIsMemorySource;
        source = new UdpSource(address, inputParameter, flags);
    } else if (isDvbSource) {
        flags |= Source::kIsHardwareSource;
        source = new DvbSource(proto, address, inputParameter, flags);
    } else if (isFileSource) {
        flags |= Source::kIsMemorySource;
        source = new FileSource(address, inputParameter, flags);
    } else if (isDVRSource) {
        flags |= Source::kIsDVRSource;
        source = new DVRSource(inputParameter, flags);
    }

    return  source;
}

Source::Source(const InputParameter& inputParameter, uint32_t flags)
: mInputParameter(inputParameter)
, mFlags(flags)
{

}

Source::~Source()
{

}

}
