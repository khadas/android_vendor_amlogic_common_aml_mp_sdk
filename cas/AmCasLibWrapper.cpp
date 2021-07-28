/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmCasLibWrapper"
#include "cas/AmCasLibWrapper.h"
#include <dlfcn.h>
#include <utils/AmlMpUtils.h>
#include <utils/Log.h>

namespace aml_mp
{

template <Aml_MP_CASServiceType ServiceType>
typename AmCasLibWrapper<ServiceType>::CasSymbols AmCasLibWrapper<ServiceType>::sCasSymbols;

template <Aml_MP_CASServiceType ServiceType>
void* AmCasLibWrapper<ServiceType>::sCasHandle;

template <Aml_MP_CASServiceType ServiceType>
std::once_flag AmCasLibWrapper<ServiceType>::sLoadCasLibFlag;

template <Aml_MP_CASServiceType ServiceType>
AmCasLibWrapper<ServiceType>::AmCasLibWrapper(const char *libName)
{
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    std::call_once(sLoadCasLibFlag, [libName] {
        loadLib(libName);
    });
    if (!sCasSymbols.createAmCas) {
        return;
    }
    AmCasStatus_t ret = sCasSymbols.createAmCas(&mCasObj);
    if (ret != CAS_STATUS_OK) {
        MLOGE("createAmCas failed");
    }
}

template <Aml_MP_CASServiceType ServiceType>
AmCasLibWrapper<ServiceType>::~AmCasLibWrapper()
{
    MLOG();
    if (mCasObj) {
        delete mCasObj;
        MLOGI("delete mCasObj");
    }
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::setCasInstanceId(int casInstanceId)
{
    if (!sCasSymbols.setCasInstanceId) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.setCasInstanceId(mCasObj, casInstanceId);
    if (ret != CAS_STATUS_OK) {
        MLOGE("setCasInstanceId failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
int AmCasLibWrapper<ServiceType>::getCasInstanceId()
{
    if (!sCasSymbols.getCasInstanceId) {
        return -1;
    }
    return sCasSymbols.getCasInstanceId(mCasObj);
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::setPrivateData(void *iDate, int iSize)
{
    if (!sCasSymbols.setPrivateData) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.setPrivateData(mCasObj, iDate, iSize);
    if (ret != CAS_STATUS_OK) {
        MLOGE("setPrivateData failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::provision()
{
    if (!sCasSymbols.provision) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.provision(mCasObj);
    if (ret != CAS_STATUS_OK) {
        MLOGE("provision failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::openSession(uint8_t *sessionId)
{
    if (!sCasSymbols.openSession) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.openSession(mCasObj, sessionId);
    if (ret != CAS_STATUS_OK) {
        MLOGE("openSession failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::closeSession(uint8_t *sessionId)
{
    if (!sCasSymbols.closeSession) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.closeSession(mCasObj, sessionId);
    if (ret != CAS_STATUS_OK) {
        MLOGE("closeSession failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::setPids(int vPid, int aPid)
{
    if (!sCasSymbols.setPids) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.setPids(mCasObj, vPid, aPid);
    if (ret != CAS_STATUS_OK) {
        MLOGE("setPids failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::processEcm(int isSection, int isVideoEcm, int vEcmPid, int aEcmPid, uint8_t *pBuffer, int iBufferLength)
{
    if (!sCasSymbols.processEcm) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.processEcm(mCasObj, isSection, isVideoEcm, vEcmPid, aEcmPid, pBuffer, iBufferLength);
    if (ret != CAS_STATUS_OK) {
        MLOGE("processEcm failed");
        return AM_CAS_ERROR;
    }

    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::processEmm(int isSection, int iPid, uint8_t *pBuffer, int iBufferLength)
{
    if (!sCasSymbols.processEmm) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.processEmm(mCasObj, isSection, iPid, pBuffer, iBufferLength);
    if (ret != CAS_STATUS_OK) {
        MLOGE("processEmm failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::decrypt(uint8_t *in, uint8_t *out, int size, void *ext_data)
{
    if (!sCasSymbols.decrypt) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.decrypt(mCasObj, in, out, size, ext_data);
    if (ret != CAS_STATUS_OK) {
        MLOGE("decrypt failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
uint8_t* AmCasLibWrapper<ServiceType>::getOutbuffer()
{
    if (!sCasSymbols.getOutbuffer) {
        return nullptr;
    }
    return sCasSymbols.getOutbuffer(mCasObj);
}

template <Aml_MP_CASServiceType ServiceType>
void AmCasLibWrapper<ServiceType>::loadLib(const char *libName)
{
    sCasHandle = dlopen(libName, RTLD_NOW);
    if (!sCasHandle) {
        ALOGE("%s dlopen failed because of: %s", libName, dlerror());
    } else {
        ALOGI("%s dlopen success, sCasHandle:%p", libName, sCasHandle);
    }
    sCasSymbols.createAmCas = lookupSymbol<createAmCasFunc>("createAmCas");
    sCasSymbols.setCasInstanceId = lookupSymbol<setCasInstanceIdFunc>("setCasInstanceId");
    sCasSymbols.getCasInstanceId = lookupSymbol<getCasInstanceIdFunc>("getCasInstanceId");
    sCasSymbols.setPrivateData = lookupSymbol<setPrivateDataFunc>("setPrivateData");
    sCasSymbols.provision = lookupSymbol<provisionFunc>("provision");
    sCasSymbols.setPids = lookupSymbol<setPidsFunc>("setPids");
    sCasSymbols.processEcm = lookupSymbol<processEcmFunc>("processEcm");
    sCasSymbols.processEmm = lookupSymbol<processEmmFunc>("processEmm");
    sCasSymbols.decrypt = lookupSymbol<decryptFunc>("decrypt");
    sCasSymbols.getOutbuffer = lookupSymbol<getOutbufferFunc>("getOutbuffer");
    sCasSymbols.openSession = lookupSymbol<openSessionFunc>("openSession");
    sCasSymbols.closeSession = lookupSymbol<closeSessionFunc>("closeSession");
    sCasSymbols.releaseAll = lookupSymbol<releaseAllFunc>("releaseAll");
}

template <Aml_MP_CASServiceType ServiceType>
AmCasCode_t AmCasLibWrapper<ServiceType>::releaseAll()
{
    if (!sCasSymbols.releaseAll) {
        return AM_CAS_ERROR;
    }
    AmCasStatus_t ret = sCasSymbols.releaseAll(mCasObj);
    if (ret != CAS_STATUS_OK) {
        ALOGE("releaseAll failed");
        return AM_CAS_ERROR;
    }
    return AM_CAS_SUCCESS;
}

template <Aml_MP_CASServiceType ServiceType>
void AmCasLibWrapper<ServiceType>::releaseLib()
{
    dlclose(sCasHandle);
    sCasHandle = nullptr;
}

template <Aml_MP_CASServiceType ServiceType>
template <typename F>
F AmCasLibWrapper<ServiceType>::lookupSymbol(const char *symbolName)
{
    F func = (F)dlsym(sCasHandle, symbolName);
    if (func == nullptr) {
        ALOGW("dlsym for %s failed, %s", symbolName, dlerror());
    }

    return func;
}

template class AmCasLibWrapper<AML_MP_CAS_SERVICE_VERIMATRIX_IPTV>;
template class AmCasLibWrapper<AML_MP_CAS_SERVICE_VERIMATRIX_WEB>;
template class AmCasLibWrapper<AML_MP_CAS_SERVICE_WIDEVINE>;

}
