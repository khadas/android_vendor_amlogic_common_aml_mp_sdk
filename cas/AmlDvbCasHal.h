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
#include "am_cas.h"

namespace aml_mp {

class AmlDvbCasHal : public AmlMpHandle
{
public:
    AmlDvbCasHal(Aml_MP_CASServiceType serviceType);
    ~AmlDvbCasHal();

    int registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData);

    int startDescrambling(Aml_MP_CASServiceInfo* serviceInfo);
    int stopDescrambling();
    int updateDescramblingPid(int oldStreamPid, int newStreamPid);

    int startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo);
    int stopDVRRecord();

    int startDVRReplay(Aml_MP_CASDVRReplayParams* dvrReplayParams);
    int stopDVRReplay();

    int DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams);
    int DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams);

    AML_MP_SECMEM createSecmem(Aml_MP_CASServiceType, void** pSecBuf, uint32_t* size);
    int destroySecmem(AML_MP_SECMEM secMem);

private:
    Aml_MP_CASServiceType mServiceType = AML_MP_CAS_SERVICE_TYPE_INVALID;
    CasSession mCasSession = 0;
    bool mDvrReplayInited = false;

private:
    AmlDvbCasHal(const AmlDvbCasHal&) = delete;
    AmlDvbCasHal& operator= (const AmlDvbCasHal&) = delete;
};

}

#endif
