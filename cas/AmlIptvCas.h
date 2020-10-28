/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "AmlCasBase.h"
#include <Aml_MP/Common.h>
#include <mutex>

typedef struct _dvb_ca dvb_ca_t;

namespace aml_mp {
struct CasLibWrapper;

class AmlIptvCas : public AmlCasBase
{
public:
    AmlIptvCas(Aml_MP_CASType casType, const Aml_MP_IptvCasParam* param);
    ~AmlIptvCas();
    virtual int openSession() override;
    virtual int closeSession() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;

private:
    static CasLibWrapper* sCasLibWrapper;
    static std::once_flag sLoadCasLibFlag;

    Aml_MP_CASType mCasType;
    Aml_MP_IptvCasParam mIptvCasParam;

    dvb_ca_t* mCasHandle = nullptr;

    AmlIptvCas(const AmlIptvCas&) = delete;
    AmlIptvCas& operator= (const AmlIptvCas&) = delete;
};
}
