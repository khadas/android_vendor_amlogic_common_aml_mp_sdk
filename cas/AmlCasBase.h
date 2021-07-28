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
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#include <vector>

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


class AmlCasBase : public AmlMpHandle
{
public:
    static sptr<AmlCasBase> create(Aml_MP_CASServiceType serviceType);
    virtual ~AmlCasBase();

    virtual int registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData);
    virtual int startDescrambling(Aml_MP_CASServiceInfo* params);
    virtual int startDescrambling(const Aml_MP_IptvCASParams* params);
    virtual int stopDescrambling() = 0;
    virtual int setPrivateData(const uint8_t* data, size_t size);
    virtual int processEcm(bool isSection, int ecmPid, const uint8_t* data, size_t size);
    virtual int processEmm(const uint8_t* data, size_t size);
    virtual int decrypt(uint8_t *in, int size, void *ext_data, Aml_MP_Buffer* outbuffer);

    virtual int updateDescramblingPid(int oldStreamPid, int newStreamPid);
    virtual int startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo);
    virtual int stopDVRRecord();

    virtual int startDVRReplay(Aml_MP_CASDVRReplayParams* dvrReplayParams);
    virtual int stopDVRReplay();

    virtual int DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams);
    virtual int DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams);

    virtual AML_MP_SECMEM createSecmem(Aml_MP_CASServiceType type, void** pSecBuf, uint32_t* size);
    virtual int destroySecmem(AML_MP_SECMEM secMem);

    virtual int ioctl(const char* inJson, char* outJson, uint32_t outLen);
    virtual int getStoreRegion(Aml_MP_CASStoreRegion* region, uint8_t* regionCount);

    Aml_MP_CASServiceType serviceType() const {
        return mServiceType;
    }

    int getEcmPids(std::vector<int>& ecmPids);

protected:
    AmlCasBase(Aml_MP_CASServiceType serviceType);

    Aml_MP_CASServiceType mServiceType;
    Aml_MP_IptvCASParams mIptvCasParam;

private:
    AmlCasBase(const AmlCasBase&) = delete;
    AmlCasBase& operator= (const AmlCasBase&) = delete;
};


}


#endif
