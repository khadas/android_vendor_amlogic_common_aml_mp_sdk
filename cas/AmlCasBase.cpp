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
#include "AmlDvbCasHal.h"
#include "wv_iptvcas/AmlWVIptvCas.h"
#include "wv_iptvcas/AmlWVIptvCas_V2.h"
#include "vmx_iptvcas/AmlVMXIptvCas.h"
#include "vmx_iptvcas/AmlVMXIptvCas_V2.h"
#include "vmx_webcas/AmlVMXWebCas.h"
#include <utils/AmlMpUtils.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

sptr<AmlCasBase> AmlCasBase::create(Aml_MP_CASServiceType serviceType)
{
    sptr<AmlCasBase> cas = nullptr;

    MLOG("cas serviceType: %s", mpCASServiceType2Str(serviceType));
    switch (serviceType) {
    case AML_MP_CAS_SERVICE_LIVE_PLAY:
    case AML_MP_CAS_SERVICE_PVR_PLAY:
    case AML_MP_CAS_SERVICE_PVR_RECORDING:
    {
        MLOGI("dvb cas_hal support!");
        cas = new AmlDvbCasHal(serviceType);
    }
    break;

    case AML_MP_CAS_SERVICE_VERIMATRIX_IPTV:
    {
#ifdef HAVE_VMXIPTV_CAS_V2
        MLOGI("%s, iptv vmxcas v2 support", __func__);
        cas = new AmlVMXIptvCas_V2(serviceType);
#elif HAVE_VMXIPTV_CAS
        MLOGI("%s, iptv vmxcas support", __func__);
        cas = new AmlVMXIptvCas(serviceType);
#endif
    }
    break;

    case AML_MP_CAS_SERVICE_WIDEVINE:
    {
#ifdef HAVE_WVIPTV_CAS_V2
        MLOGI("%s, iptv wvcas v2 support", __func__);
        cas = new AmlWVIptvCas_V2(serviceType);
#elif HAVE_WVIPTV_CAS
        MLOGI("%s, iptv wvcas support", __func__);
        cas = new AmlWVIptvCas(serviceType);
#endif
    }
    break;

    case AML_MP_CAS_SERVICE_VERIMATRIX_WEB:
    {
#ifdef HAVE_VMXWEB_CAS
        MLOGI("%s, vmx_web support", __func__);
        cas = new AmlVMXWebCas(serviceType);
#endif
    }
    break;

    default:
        MLOGE("unsupported ca type!");
        break;
    }

    return cas;
}

AmlCasBase::AmlCasBase(Aml_MP_CASServiceType serviceType)
:mServiceType(serviceType)
{
    memset(&mIptvCasParam, 0, sizeof(mIptvCasParam));
}

AmlCasBase::~AmlCasBase()
{
    MLOG();
}

int AmlCasBase::registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData)
{
    AML_MP_UNUSED(cb);
    AML_MP_UNUSED(userData);

    return 0;
}

int AmlCasBase::startDescrambling(Aml_MP_CASServiceInfo* params)
{
    AML_MP_UNUSED(params);

    return 0;
}

int AmlCasBase::startDescrambling(const Aml_MP_IptvCASParams* params)
{
    MLOG();
    mIptvCasParam = *params;

    return 0;
}

int AmlCasBase::setPrivateData(const uint8_t* data, size_t size)
{
    AML_MP_UNUSED(data);
    AML_MP_UNUSED(size);

    return 0;
}

int AmlCasBase::processEcm(bool isSection, int ecmPid, const uint8_t* data, size_t size)
{
    AML_MP_UNUSED(isSection);
    AML_MP_UNUSED(ecmPid);
    AML_MP_UNUSED(data);
    AML_MP_UNUSED(size);

    return 0;
}

int AmlCasBase::processEmm(const uint8_t* data, size_t size)
{
    AML_MP_UNUSED(data);
    AML_MP_UNUSED(size);

    return 0;
}

int AmlCasBase::decrypt(uint8_t *in, int size, void *ext_data, Aml_MP_Buffer* outbuffer)
{
    AML_MP_UNUSED(in);
    AML_MP_UNUSED(size);
    AML_MP_UNUSED(ext_data);
    outbuffer->address = in;
    outbuffer->size = size;
    return 0;
}

int AmlCasBase::updateDescramblingPid(int oldStreamPid, int newStreamPid)
{
    AML_MP_UNUSED(oldStreamPid);
    AML_MP_UNUSED(newStreamPid);

    return 0;
}

int AmlCasBase::startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo)
{
    AML_MP_UNUSED(serviceInfo);

    return 0;
}

int AmlCasBase::stopDVRRecord()
{
    return 0;
}

int AmlCasBase::startDVRReplay(Aml_MP_CASDVRReplayParams* dvrReplayParams)
{
    AML_MP_UNUSED(dvrReplayParams);

    return 0;
}

int AmlCasBase::stopDVRReplay()
{
    return 0;
}

int AmlCasBase::DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams)
{
    AML_MP_UNUSED(cryptoParams);

    return 0;
}

int AmlCasBase::DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams)
{
    AML_MP_UNUSED(cryptoParams);

    return 0;
}

AML_MP_SECMEM AmlCasBase::createSecmem(Aml_MP_CASServiceType type, void** pSecBuf, uint32_t* size)
{
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(pSecBuf);
    AML_MP_UNUSED(size);

    return nullptr;
}

int AmlCasBase::destroySecmem(AML_MP_SECMEM secMem)
{
    AML_MP_UNUSED(secMem);

    return 0;
}

int AmlCasBase::ioctl(const char* inJson, char* outJson, uint32_t outLen)
{
    AML_MP_UNUSED(inJson);
    AML_MP_UNUSED(outJson);
    AML_MP_UNUSED(outLen);

    return 0;
}

int AmlCasBase::getStoreRegion(Aml_MP_CASStoreRegion* region, uint8_t* regionCount)
{
    AML_MP_UNUSED(region);
    AML_MP_UNUSED(regionCount);

    return 0;
}

int AmlCasBase::getEcmPids(std::vector<int>& ecmPids)
{
    ecmPids.clear();

    int pid;
    for (size_t i = 0; i < MAX_ECM_PIDS_NUM; ++i) {
        pid = mIptvCasParam.ecmPid[i];
        if (pid <= 0 || pid >= AML_MP_INVALID_PID) {
            continue;
        }

        ecmPids.push_back(pid);
    }

    return 0;
}


}
