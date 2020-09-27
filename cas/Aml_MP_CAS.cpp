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

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpCAS"
#include <Aml_MP/Cas.h>
#include "AmlDvbCasHal.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>

using namespace aml_mp;
using namespace android;

int Aml_MP_CAS_OpenSession(AML_MP_HANDLE* casSession)
{
    sp<AmlDvbCasHal> dvbCasHal = new AmlDvbCasHal();
    dvbCasHal->incStrong(dvbCasHal.get());

    *casSession = aml_handle_cast(dvbCasHal);

    return 0;
}

int Aml_MP_CAS_CloseSession(AML_MP_HANDLE casSession)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);
    dvbCasHal->decStrong(casSession);

    return 0;
}

int Aml_MP_CAS_RegisterEventCallback(AML_MP_HANDLE casSession, Aml_MP_CAS_EventCallback cb, void* userData)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    int ret = 0;

    if (dvbCasHal) {
        ret = dvbCasHal->registerEventCallback(cb, userData);
    } else {
        CAS_EventFunction_t eventFn = reinterpret_cast<CAS_EventFunction_t>(cb);
#if !defined (__ANDROID_VNDK__)
        ret = AM_CA_RegisterEventCallback((CasSession)nullptr, eventFn);
#else
        AML_MP_UNUSED(eventFn);
#endif
    }

    return ret;
}

int Aml_MP_CAS_StartDescrambling(AML_MP_HANDLE casSession, Aml_MP_CASServiceInfo* serviceInfo)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDescrambling(serviceInfo);
}

int Aml_MP_CAS_StopDescrambling(AML_MP_HANDLE casSession)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDescrambling();
}

int Aml_MP_CAS_UpdateDescramblingPid(AML_MP_HANDLE casSession, int oldStreamPid, int newStreamPid)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->updateDescramblingPid(oldStreamPid, newStreamPid);
}

int Aml_MP_CAS_StartDVRRecord(AML_MP_HANDLE casSession, Aml_MP_CASServiceInfo *serviceInfo)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDVRRecord(serviceInfo);
}

int Aml_MP_CAS_StopDVRRecord(AML_MP_HANDLE casSession)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDVRRecord();
}

int Aml_MP_CAS_SetDVRReplayPreParam(AML_MP_HANDLE casSession, struct Aml_MP_CASPreParam *params)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->setDVRReplayPreParam(params);
}

int Aml_MP_CAS_StartDVRReplay(AML_MP_HANDLE casSession, Aml_MP_CASCryptoParams *cryptoParams)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->startDVRReplay(cryptoParams);
}

int Aml_MP_CAS_StopDVRReplay(AML_MP_HANDLE casSession)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->stopDVRReplay();
}

int Aml_MP_CAS_DVREncrypt(AML_MP_HANDLE casSession, Aml_MP_CASCryptoParams *cryptoParams)
{
    android::sp<aml_mp::AmlDvbCasHal> dvbCasHal = aml_mp::aml_handle_cast<aml_mp::AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->DVREncrypt(cryptoParams);
}

int Aml_MP_CAS_DVRDecrypt(AML_MP_HANDLE casSession, Aml_MP_CASCryptoParams *cryptoParams)
{
    android::sp<aml_mp::AmlDvbCasHal> dvbCasHal = aml_mp::aml_handle_cast<aml_mp::AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->DVRDecrypt(cryptoParams);
}

AML_MP_HANDLE Aml_MP_CAS_CreateSecmem(AML_MP_HANDLE casSession, Aml_MP_CASServiceType type, void **pSecbuf, uint32_t *size)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(AML_MP_INVALID_HANDLE, dvbCasHal == nullptr);

    return dvbCasHal->createSecmem(type, pSecbuf, size);
}

int Aml_MP_CAS_DestroySecmem(AML_MP_HANDLE casSession, AML_MP_HANDLE secMem)
{
    sp<AmlDvbCasHal> dvbCasHal = aml_handle_cast<AmlDvbCasHal>(casSession);
    RETURN_IF(-1, dvbCasHal == nullptr);

    return dvbCasHal->destroySecmem(secMem);
}












