/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_CAS_BASE_H_
#define _AML_MP_CAS_BASE_H_

#include <Aml_MP/Common.h>
#include <Aml_MP/Cas.h>
#include <utils/RefBase.h>

namespace aml_mp {
using android::RefBase;
using android::sp;

class AmlCasBase : public RefBase
{
public:
    static sp<AmlCasBase> create(Aml_MP_InputSourceType inputType, const Aml_MP_CASParams* params);
    virtual ~AmlCasBase();

    virtual int openSession() = 0;
    virtual int closeSession() = 0;
    virtual int setPrivateData(const uint8_t* data, size_t size) = 0;
    virtual int processEcm(const uint8_t* data, size_t size) = 0;
    virtual int processEmm(const uint8_t* data, size_t size) = 0;
    int registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData);

protected:
    AmlCasBase();

private:
    AmlCasBase(const AmlCasBase&) = delete;
    AmlCasBase& operator= (const AmlCasBase&) = delete;
};


}


#endif
