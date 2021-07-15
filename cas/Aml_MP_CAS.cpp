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

static const char* mName = LOG_TAG;

using namespace aml_mp;
//using namespace android;

///////////////////////////////////////////////////////////////////////////////
//global CAS functions
pthread_once_t g_dvbCasInitFlag = PTHREAD_ONCE_INIT;
#ifdef HAVE_CAS_HAL
CasHandle g_casHandle = 0;

static AM_CA_SECTION convertToCASection(Aml_MP_CASSectionType casSection)
{
    switch (casSection) {
    case AML_MP_CAS_SECTION_PMT:
        return AM_CA_SECTION_PMT;

    case AML_MP_CAS_SECTION_CAT:
        return AM_CA_SECTION_CAT;

    case AML_MP_CAS_SECTION_NIT:
        return AM_CA_SECTION_NIT;
    }
}

static int convertToAmlMPErrorCode(AM_RESULT CasResult) {
    switch (CasResult) {
        case AM_ERROR_SUCCESS:
            return AML_MP_OK;
        case AM_ERROR_NOT_LOAD:
            return AML_MP_BAD_INDEX;
        case AM_ERROR_NOT_SUPPORTED:
            return AML_MP_BAD_TYPE;
        case AM_ERROR_OVERFLOW:
            return AML_MP_NO_MEMORY;
        default:
            return AML_MP_ERROR;
    }
}
#endif

int Aml_MP_CAS_Initialize()
{
    MLOG();

#ifdef HAVE_CAS_HAL
    pthread_once(&g_dvbCasInitFlag, [] {
        AM_CA_Init(&g_casHandle);
    });
#endif

    return 0;
}

int Aml_MP_CAS_Terminate()
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_Term(g_casHandle));

    g_casHandle = 0;
#endif

    return ret;
}

int Aml_MP_CAS_IsNeedWholeSection()
{
    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    ret = AM_CA_IsNeedWholeSection();
#endif

    MLOG("isNeedWholeSection = %d", ret);
    return ret;
}

int Aml_MP_CAS_IsSystemIdSupported(int caSystemId)
{
    bool ret = false;

#ifdef HAVE_CAS_HAL
    ret = AM_CA_IsSystemIdSupported(caSystemId);
#else
    AML_MP_UNUSED(caSystemId);
#endif

    return ret;
}

int Aml_MP_CAS_ReportSection(Aml_MP_CASSectionReportAttr* pAttr, uint8_t* data, size_t len)
{
    MLOG("dmx_dev:%d, service_id:%d", pAttr->dmxDev, pAttr->serviceId);

    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    AM_CA_SecAttr_t attr;

    attr.dmx_dev = pAttr->dmxDev;
    attr.service_id = pAttr->serviceId;
    attr.section_type = convertToCASection(pAttr->sectionType);

    ret = convertToAmlMPErrorCode(AM_CA_ReportSection(&attr, data, len));
#else
    AML_MP_UNUSED(data);
    AML_MP_UNUSED(len);
#endif

    return ret;
}

int Aml_MP_CAS_SetEmmPid(int dmxDev, uint16_t emmPid)
{
    MLOG("dmxDev:%d, emmPid:%d", dmxDev, emmPid);

    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    RETURN_IF(-1, g_casHandle == 0);

    ret = convertToAmlMPErrorCode(AM_CA_SetEmmPid(g_casHandle, dmxDev, emmPid));
#endif

    return ret;
}

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
#ifdef HAVE_CAS_HAL
        CAS_EventFunction_t eventFn = reinterpret_cast<CAS_EventFunction_t>(cb);
        ret = AM_CA_RegisterEventCallback((CasSession)nullptr, eventFn);
#endif
    }

    return ret;
}

int Aml_MP_CAS_Ioctl(AML_MP_CASSESSION casSession, const char* inJson, char* outJson, uint32_t outLen)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->ioctl(inJson, outJson, outLen);
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

int Aml_MP_CAS_GetStoreRegion(AML_MP_CASSESSION casSession, Aml_MP_CASStoreRegion* region, uint8_t* regionCount)
{
    sptr<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->getStoreRegion(region, regionCount);
}

