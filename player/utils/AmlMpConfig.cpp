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

AmlMpConfig::AmlMpConfig(int instanceId)
: mInstanceId(instanceId)
{
    reset();
    init();
}

AmlMpConfig::~AmlMpConfig()
{

}

void AmlMpConfig::reset()
{
    mLogDebug = 0;
}

void AmlMpConfig::init()
{
    initProperty("vendor.amlmp.log-debug", mLogDebug);
}


}
