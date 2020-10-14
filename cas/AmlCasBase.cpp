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

#define LOG_TAG "AmlCasBase"
#include <utils/Log.h>
#include "AmlCasBase.h"
#include "AmlIptvCas.h"
#include "AmlDvbCasHal.h"
#include <utils/AmlMpUtils.h>

namespace aml_mp {
sp<AmlCasBase> AmlCasBase::create(Aml_MP_InputSourceType inputType, const Aml_MP_CASParams* casParams)
{
    sp<AmlCasBase> cas;

    switch (inputType) {
    case AML_MP_INPUT_SOURCE_TS_MEMORY:
    {
        const Aml_MP_IptvCasParam *iptvCasParam = &casParams->u.iptvCasParam;
        cas = new AmlIptvCas(casParams->type, iptvCasParam);
    }
    break;

    default:
        ALOGI("unknown input source type!");
        break;
    }

    return cas;
}

AmlCasBase::AmlCasBase()
{

}

AmlCasBase::~AmlCasBase()
{

}

int AmlCasBase::registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData)
{
    AML_MP_UNUSED(cb);
    AML_MP_UNUSED(userData);

    return 0;
}

}
