/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpCAS"
#include <Aml_MP/Cas.h>
#include "AmlDvbCasHal.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>

using namespace aml_mp;
//using namespace android;

int Aml_MP_CAS_OpenSession(AML_MP_CASSESSION* casSession, Aml_MP_CASServiceType serviceType)
{
    sptr<AmlDvbCasHal> dvbCasHal = new AmlDvbCasHal(serviceType);
    dvbCasHal->incStrong(dvbCasHal.get());

    *casSession = aml_handle_cast(dvbCasHal);

    return 0;
}

int Aml_MP_CAS_CloseSession(AML_MP_CASSESSION casSession)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);
    dvbCasHal->decStrong(casSession);

    return 0;
}

int Aml_MP_CAS_RegisterEventCallback(AML_MP_CASSESSION casSession, Aml_MP_CAS_EventCallback cb, void* userData)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    int ret = 0;

    if (dvbCasHal) {
        ret = dvbCasHal->registerEventCallback(cb, userData);
    } else {
        CAS_EventFunction_t eventFn = reinterpret_cast<CAS_EventFunction_t>(cb);
#if !defined (__ANDROID_VNDK__) || ANDROID_PLATFORM_SDK_VERSION >= 30
        ret = AM_CA_RegisterEventCallback((CasSession)nullptr, eventFn);
#else
        AML_MP_UNUSED(eventFn);
#endif
    }

    return ret;
}

int Aml_MP_CAS_StartDescrambling(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo* serviceInfo)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDescrambling(serviceInfo);
}

int Aml_MP_CAS_StopDescrambling(AML_MP_CASSESSION casSession)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDescrambling();
}

int Aml_MP_CAS_UpdateDescramblingPid(AML_MP_CASSESSION casSession, int oldStreamPid, int newStreamPid)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->updateDescramblingPid(oldStreamPid, newStreamPid);
}

int Aml_MP_CAS_StartDVRRecord(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo *serviceInfo)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDVRRecord(serviceInfo);
}

int Aml_MP_CAS_StopDVRRecord(AML_MP_CASSESSION casSession)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDVRRecord();
}

int Aml_MP_CAS_StartDVRReplay(AML_MP_CASSESSION casSession, Aml_MP_CASDVRReplayParams *dvrReplayParams)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDVRReplay(dvrReplayParams);
}

int Aml_MP_CAS_StopDVRReplay(AML_MP_CASSESSION casSession)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDVRReplay();
}

int Aml_MP_CAS_DVREncrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoParams)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->DVREncrypt(cryptoParams);
}

int Aml_MP_CAS_DVRDecrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoParams)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->DVRDecrypt(cryptoParams);
}

AML_MP_SECMEM Aml_MP_CAS_CreateSecmem(AML_MP_CASSESSION casSession, Aml_MP_CASServiceType type, void **pSecbuf, uint32_t *size)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(AML_MP_INVALID_HANDLE, dvbCasHal == nullptr);

    return dvbCasHal->createSecmem(type, pSecbuf, size);
}

int Aml_MP_CAS_DestroySecmem(AML_MP_CASSESSION casSession, AML_MP_SECMEM secMem)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->destroySecmem(secMem);
}
