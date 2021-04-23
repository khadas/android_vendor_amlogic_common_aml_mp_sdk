/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_VMX_IPTV_CAS_H_
#define _AML_VMX_IPTV_CAS_H_

#include <cas/AmlCasBase.h>
#include <Aml_MP/Common.h>
#include <mutex>

typedef struct _dvb_ca dvb_ca_t;

namespace aml_mp {
struct CasLibWrapper;

class AmlVMXIptvCas : public AmlCasBase
{
public:
    AmlVMXIptvCas(const Aml_MP_IptvCASParams* param, int instanceId);
    ~AmlVMXIptvCas();
    virtual int openSession() override;
    virtual int closeSession() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;

private:
    static CasLibWrapper* sCasLibWrapper;
    static std::once_flag sLoadCasLibFlag;

    Aml_MP_IptvCASParams mIptvCasParam;

    dvb_ca_t* mCasHandle = nullptr;

    AmlVMXIptvCas(const AmlVMXIptvCas&) = delete;
    AmlVMXIptvCas& operator= (const AmlVMXIptvCas&) = delete;
};

}

#endif
