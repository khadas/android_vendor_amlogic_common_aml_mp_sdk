/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_DVB_CAS_HAL_H_
#define _AML_MP_DVB_CAS_HAL_H_

#include <Aml_MP/Cas.h>
#include <utils/AmlMpHandle.h>
#ifdef HAVE_CAS_HAL
#include "am_cas.h"
#endif
#include "AmlCasBase.h"
#include <map>
#include <mutex>

namespace aml_mp {

class AmlDvbCasHal : public AmlCasBase
{
public:
    AmlDvbCasHal(Aml_MP_CASServiceType serviceType);
    ~AmlDvbCasHal();

    virtual int registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData) override;

    virtual int startDescrambling(Aml_MP_CASServiceInfo* serviceInfo) override;
    virtual int stopDescrambling() override;


    virtual int updateDescramblingPid(int oldStreamPid, int newStreamPid) override;

    virtual int startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo) override;
    virtual int stopDVRRecord() override;

    virtual int startDVRReplay(Aml_MP_CASDVRReplayParams* dvrReplayParams) override;
    virtual int stopDVRReplay() override;

    virtual int DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams) override;
    int DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams) override;

    virtual AML_MP_SECMEM createSecmem(Aml_MP_CASServiceType, void** pSecBuf, uint32_t* size) override;
    virtual int destroySecmem(AML_MP_SECMEM secMem) override;

    virtual int ioctl(const char* inJson, char* outJson, uint32_t outLen) override;
    virtual int getStoreRegion(Aml_MP_CASStoreRegion* region, uint8_t* regionCount) override;

#ifdef HAVE_CAS_HAL
    static AM_RESULT sCasHalCb(CasSession session, char* json);
#endif

private:
    void notifyListener(char* json);
#ifdef HAVE_CAS_HAL
    CasSession mCasSession = 0;
    static std::mutex sCasHalSessionLock;
    static std::map<CasSession, wptr<AmlDvbCasHal>> sCasHalSessionMap;

#endif
    Aml_MP_CAS_EventCallback mCb = nullptr;
    void* mUserData = nullptr;
    bool mDvrReplayInited = false;

private:
    AmlDvbCasHal(const AmlDvbCasHal&) = delete;
    AmlDvbCasHal& operator= (const AmlDvbCasHal&) = delete;
};

}

#endif
