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
#include "wv_iptvcas/AmlWVIptvCas.h"
#include "vmx_iptvcas/AmlVMXIptvCas.h"
#include "vmx_iptvcas/AmlVMXIptvCas_V2.h"
#include <utils/AmlMpUtils.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

sptr<AmlCasBase> AmlCasBase::create(const Aml_MP_IptvCASParams* casParams, int instanceId)
{
    sptr<AmlCasBase> cas = nullptr;

    MLOGI("%s, casParams->type= %s", __func__, mpCASType2Str(casParams->type));
    switch (casParams->type) {
    case AML_MP_CAS_VERIMATRIX_IPTV:
    {
#ifdef HAVE_VMXIPTV_CAS_V2
        MLOGI("%s, iptv vmxcas v2 support", __func__);
        cas = new AmlVMXIptvCas_V2(casParams, instanceId);
#elif HAVE_VMXIPTV_CAS
        MLOGI("%s, iptv vmxcas support", __func__);
        cas = new AmlVMXIptvCas(casParams, instanceId);
#endif
    }
    break;

    case AML_MP_CAS_WIDEVINE:
    {
#ifdef HAVE_WVIPTV_CAS
        MLOGI("%s, iptv wvcas support", __func__);
        cas = new AmlWVIptvCas(casParams, instanceId);
#endif
    }
    break;

    default:
        break;
    }

    if (casParams->type == AML_MP_CAS_UNKNOWN) {
        MLOGE("unsupported ca type!");
        return nullptr;
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
