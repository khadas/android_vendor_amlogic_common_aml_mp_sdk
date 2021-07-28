/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _AML_VMX_IPTV_CAS_V2_H_
#define _AML_VMX_IPTV_CAS_V2_H_

#include <cas/AmlCasBase.h>
#include <Aml_MP/Common.h>
#include "cas/AmCasLibWrapper.h"

#include <mutex>

namespace aml_mp {

class AmlVMXIptvCas_V2 : public AmlCasBase
{
public:
    AmlVMXIptvCas_V2(Aml_MP_CASServiceType serviceType);
    ~AmlVMXIptvCas_V2();
    virtual int startDescrambling(const Aml_MP_IptvCASParams* params) override;
    virtual int stopDescrambling() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(bool isSection, int ecmPid, const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;

private:
    char mServerPort[10];
    sptr<AmCasLibWrapper<AML_MP_CAS_SERVICE_VERIMATRIX_IPTV>> pIptvCas;
    uint8_t sessionId[8]{};
    int mInstanceId;

    int mDscFd = -1;
    int mFirstEcm = 0;
    uint8_t mEcmTsPacket[188];

    int setDscSource(const char* source);
    int dscDevOpen(const char *port_addr, int flags);
    int checkEcmProcess(uint8_t* pBuffer, uint32_t vEcmPid, uint32_t aEcmPid, size_t * nSize);


    AmlVMXIptvCas_V2(const AmlVMXIptvCas_V2&) = delete;
    AmlVMXIptvCas_V2& operator= (const AmlVMXIptvCas_V2&) = delete;
};
}

#endif
