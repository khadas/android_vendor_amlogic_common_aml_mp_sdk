/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
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
#ifdef HAVE_IPTV_CAS
        const Aml_MP_IptvCasParam *iptvCasParam = &casParams->u.iptvCasParam;
        cas = new AmlIptvCas(casParams->type, iptvCasParam);
#endif
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
