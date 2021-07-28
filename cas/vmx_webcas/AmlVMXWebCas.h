/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _AML_VMX_WEB_CAS_V2_H_
#define _AML_VMX_WEB_CAS_V2_H_

#include <cas/AmlCasBase.h>
#include <Aml_MP/Common.h>
#include "cas/AmCasLibWrapper.h"

#include <mutex>

namespace aml_mp {

class AmlVMXWebCas : public AmlCasBase
{
public:
    AmlVMXWebCas(Aml_MP_CASServiceType serviceType);
    ~AmlVMXWebCas();
    virtual int startDescrambling(const Aml_MP_IptvCASParams* params) override;
    virtual int stopDescrambling() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(bool isSection, int ecmPid, const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;
    virtual int decrypt(uint8_t *in, int size, void *ext_data, Aml_MP_Buffer* outbuffer) override;


private:
    char mServerPort[10];
    sptr<AmCasLibWrapper<AML_MP_CAS_SERVICE_VERIMATRIX_WEB>> pIptvCas;
    uint8_t sessionId[8];
    int mInstanceId;

    int mDscFd;
    int mFirstEcm;
    uint8_t mEcmTsPacket[188];

    int setDscSource();
    int dscDevOpen(const char *port_addr, int flags);
    int checkEcmProcess(uint8_t* pBuffer, uint32_t vEcmPid, uint32_t aEcmPid, size_t * nSize);


    AmlVMXWebCas(const AmlVMXWebCas&) = delete;
    AmlVMXWebCas& operator= (const AmlVMXWebCas&) = delete;
};
}

#endif
