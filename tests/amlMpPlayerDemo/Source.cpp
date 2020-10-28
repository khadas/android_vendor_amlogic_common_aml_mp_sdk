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
#include <utils/Log.h>
#include "Source.h"
#include "UdpSource.h"
#include "DvbSource.h"
#include "FileSource.h"

namespace aml_mp {
using namespace android;

sp<Source> Source::create(const char* url)
{
    char proto[10]{};
    char address[100]{};
    int programNumber = -1;

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
        programNumber = strtol(p2+1, nullptr, 0);
        strncpy(address, p, p2 - p);
    } else {
        strncpy(address, p, sizeof(address)-1);
    }

    ALOGV("proto:%s, address:%s, programNumber:%d", proto, address, programNumber);

    bool isUdpSource = false;
    bool isDvbSource = false;
    bool isFileSource = false;
    if (!strcmp(proto, "udp") || !strcmp(proto, "igmp")) {
        isUdpSource = true;
    } else if (!strncmp(proto, "dvb", 3)) {
        isDvbSource = true;
    } else if (!strncmp(proto, "file", 4)) {
        isFileSource = true;
    } else {
        ALOGE("unsupported proto:%s\n", proto);
        return nullptr;
    }

    uint32_t flags = 0;
    sp<Source> source = nullptr;
    if (isUdpSource) {
        flags |= Source::kIsMemorySource;
        source = new UdpSource(address, programNumber, flags);
    } else if (isDvbSource) {
        flags |= Source::kIsHardwareSource;
        source = new DvbSource(proto, address, programNumber, flags);
    } else if (isFileSource) {
        flags |= Source::kIsMemorySource;
        source = new FileSource(address, programNumber, flags);
    }

    return  source;
}

Source::Source(int programNumber, uint32_t flags)
: mProgramNumber(programNumber)
, mFlags(flags)
{

}

Source::~Source()
{

}

}
