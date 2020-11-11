/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpConfig"
#include <utils/Log.h>
#include "AmlMpConfig.h"
#include <cutils/properties.h>
#include <string>

namespace aml_mp {
template <typename T>
void AmlMpConfig::initProperty(const char* propertyName, T& value)
{
    char bufs[PROP_VALUE_MAX];

    if (property_get(propertyName, bufs, 0) > 0) {
        value = strtol(bufs, nullptr, 0);
    }
}

template<>
void AmlMpConfig::initProperty(const char* propertyName, std::string& value)
{
    char bufs[PROP_VALUE_MAX];

    if (property_get(propertyName, bufs, 0) > 0) {
        value = bufs;
    }
}

void AmlMpConfig::reset()
{
    mLogDebug = 0;
    mCtcDebug = 0;
}

void AmlMpConfig::init()
{
    initProperty("vendor.amlmp.log-debug", mLogDebug);
    initProperty("vendor.amlmp.use-ctc", mCtcDebug);
}

AmlMpConfig::AmlMpConfig()
{
    reset();
    init();
}


}
