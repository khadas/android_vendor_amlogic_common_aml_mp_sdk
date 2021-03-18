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
#include <utils/AmlMpRefBase.h>

namespace aml_mp {
//using android::RefBase;
//using android::sp;

#define TS_PACKET_SIZE 188
//add for x2
#define DMX0_SOURCE_PATH    "/sys/class/stb/demux0_source"
#define DMX_SRC             "hiu"
#define DSC0_SOURCE_PATH    "/sys/class/stb/dsc0_source"
#define DSC_SRC             "dmx0"
#define DSC_DEVICE          "/dev/dvb0.ca0"

//add for x4
#define TSN_PATH            "/sys/class/stb/tsn_source"
#define TSN_IPTV            "local"
#define TSN_DVB             "demod"


class AmlCasBase : public AmlMpRefBase
{
public:
    static sptr<AmlCasBase> create(const Aml_MP_IptvCasParams* params, int instanceId);
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
