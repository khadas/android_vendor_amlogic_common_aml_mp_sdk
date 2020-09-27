/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#define LOG_TAG "AmlIptvCas"
#include <utils/Log.h>
#include <utils/AmlMpUtils.h>
#include "AmlIptvCas.h"
#include <dlfcn.h>
#include <AM_MPP/AmlDVB.h>

namespace aml_mp {

struct CasLibWrapper {
    CasLibWrapper();

    void init();
    dvb_ca_t* create(aml_dvb_init_para_t* init_para, int* err);
    int start(dvb_ca_t* ca);
    int writeData(dvb_ca_t* ca, const uint8_t* buffer, size_t len, void* userdata = nullptr);
    int setCallback(dvb_ca_t* ca, dvb_ca_callback cb);
    int stop(dvb_ca_t* ca);
    int destroy(dvb_ca_t* ca);

private:
    void loadCasLib();

    void* mCasLibHandle = nullptr;
    void (*ca_lib_init)() = nullptr;
    dvb_ca_t* (*ca_create)(aml_dvb_init_para_t* init_para, int* err) = nullptr;
    int (*ca_start)(dvb_ca_t* ca) = nullptr;
    int (*ca_writedata)(dvb_ca_t* ca, uint8_t* buf, size_t len, void* userdata) = nullptr;
    int (*ca_callback)(dvb_ca_t* ca, dvb_ca_callback cb) = nullptr;
    int (*ca_stop)(dvb_ca_t* ca) = nullptr;
    int (*ca_destory)(dvb_ca_t* ca) = nullptr;

    CasLibWrapper(const CasLibWrapper&) = delete;
    CasLibWrapper& operator= (const CasLibWrapper&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
CasLibWrapper* AmlIptvCas::sCasLibWrapper = nullptr;
std::once_flag AmlIptvCas::sLoadCasLibFlag;

AmlIptvCas::AmlIptvCas(Aml_MP_CASType casType, const Aml_MP_IptvCasParam* param)
: mCasType(casType)
, mIptvCasParam(*param)
{
    ALOGI("ctor AmlIptvCas, casType:%d", casType);
    AML_MP_UNUSED(mCasType);
    AML_MP_UNUSED(mIptvCasParam);

    std::call_once(sLoadCasLibFlag, [] {
        sCasLibWrapper = new CasLibWrapper();
    });

    aml_dvb_init_para_t initPara{.type = CA_TYPE_UNKNOWN};

    switch (casType) {
    case AML_MP_CAS_VERIMATRIX_IPTV:
    {
        initPara.type = CA_TYPE_VMX;
        initPara.vmx_para.vpid = param->videoPid;
        initPara.vmx_para.apid = param->audioPid;
        initPara.vmx_para.vfmt = convertToVFormat(param->videoCodec);
        initPara.vmx_para.afmt = convertToAForamt(param->audioCodec);
        initPara.vmx_para.ecmpid = param->ecmPid;
        strncpy(initPara.vmx_para.key_file_path, param->keyPath, sizeof(initPara.vmx_para.key_file_path)-1);
        strncpy(initPara.vmx_para.server_ip, param->serverAddress, sizeof(initPara.vmx_para.server_ip)-1);
        initPara.vmx_para.server_port = param->serverPort;
    }
    break;

    default:
        break;
    }

    if (initPara.type == CA_TYPE_UNKNOWN) {
        ALOGE("unsupported ca type!");
        return;
    }

    int err;
    mCasHandle = sCasLibWrapper->create(&initPara, &err);
}

AmlIptvCas::~AmlIptvCas()
{
    ALOGI("dtor AmlIptvCas");

    if (mCasHandle != nullptr) {
        sCasLibWrapper->destroy(mCasHandle);
        mCasHandle = nullptr;
    }
}

int AmlIptvCas::openSession()
{
    ALOGI("openSession");

    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->start(mCasHandle);
}

int AmlIptvCas::closeSession()
{
    ALOGI("closeSession");

    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->stop(mCasHandle);
}

int AmlIptvCas::setPrivateData(const uint8_t* data, size_t size)
{
    RETURN_IF(-1, mCasHandle == nullptr);

    AML_MP_UNUSED(data);
    AML_MP_UNUSED(size);

    return 0;
}

int AmlIptvCas::processEcm(const uint8_t* data, size_t size)
{
    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->writeData(mCasHandle, data, size);
}

int AmlIptvCas::processEmm(const uint8_t* data, size_t size)
{
    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->writeData(mCasHandle, data, size);
}

///////////////////////////////////////////////////////////////////////////////
CasLibWrapper::CasLibWrapper()
{
    const char* libPath = "libamlCtcCas.so";

    void* casHandle = dlopen(libPath, RTLD_NOW);
    if (casHandle == nullptr) {
        ALOGE("dlopen %s failed! %s", libPath, dlerror());
        return;
    }

    mCasLibHandle = casHandle;

    ca_lib_init = (void (*)()) dlsym(casHandle, "AM_MP_DVB_lib_init");
    ca_create = (dvb_ca_t *(*)(aml_dvb_init_para_t *, int *))dlsym(casHandle, "AM_MP_DVB_create");
    ca_start       = (int (*)(dvb_ca_t *)) dlsym(casHandle, "AM_MP_DVB_start");
    ca_writedata   = (int (*)(dvb_ca_t *, uint8_t *, size_t, void *)) dlsym(casHandle, "AM_MP_DVB_write");
    ca_callback    = (int (*)(dvb_ca_t *, dvb_ca_callback)) dlsym(casHandle, "AM_MP_DVB_set_callback");
    ca_stop        = (int (*)(dvb_ca_t *))dlsym(casHandle, "AM_MP_DVB_stop");
    ca_destory     = (int (*)(dvb_ca_t *))dlsym(casHandle, "AM_MP_DVB_destory");

    init();
}

void CasLibWrapper::init()
{
    RETURN_VOID_IF(ca_lib_init == nullptr);

    ca_lib_init();
}

dvb_ca_t* CasLibWrapper::create(aml_dvb_init_para_t* init_para, int* err)
{
    RETURN_IF(nullptr, ca_create == nullptr);

    return ca_create(init_para, err);
}

int CasLibWrapper::start(dvb_ca_t* ca)
{
    RETURN_IF(-1, ca_start == nullptr);
    AML_MP_UNUSED(ca);

    return ca_start(ca);
}

int CasLibWrapper::writeData(dvb_ca_t* ca, const uint8_t* buffer, size_t len, void* userdata)
{
    RETURN_IF(-1, ca_writedata == nullptr);

    return ca_writedata(ca, const_cast<uint8_t*>(buffer), len, userdata);
}

int CasLibWrapper::setCallback(dvb_ca_t* ca, dvb_ca_callback cb)
{
    RETURN_IF(-1, ca_callback == nullptr);

    return ca_callback(ca, cb);
}

int CasLibWrapper::stop(dvb_ca_t* ca)
{
    RETURN_IF(-1, ca_stop == nullptr);

    return ca_stop(ca);
}

int CasLibWrapper::destroy(dvb_ca_t* ca)
{
    RETURN_IF(-1, ca_destory == nullptr);

    return ca_destory(ca);
}


}
