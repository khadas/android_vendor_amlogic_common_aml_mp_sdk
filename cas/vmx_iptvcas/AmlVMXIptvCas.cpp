/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlVMXIptvCas"
#include <utils/Log.h>
#include <utils/AmlMpUtils.h>
#include "AmlVMXIptvCas.h"
#include <dlfcn.h>
#include <cutils/properties.h>
#include <utils/Amlsysfsutils.h>
#include <unistd.h>
#include <AM_MPP/AmlDVB.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

struct CasLibWrapper {
    CasLibWrapper();
    ~CasLibWrapper();

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
CasLibWrapper* AmlVMXIptvCas::sCasLibWrapper = nullptr;
std::once_flag AmlVMXIptvCas::sLoadCasLibFlag;

AmlVMXIptvCas::AmlVMXIptvCas(const Aml_MP_IptvCASParams* param, int instanceId)
: mIptvCasParam(*param)
{
    MLOGI("ctor AmlVMXIptvCas, instanceId:%d", instanceId);
    AML_MP_UNUSED(mIptvCasParam);

    std::call_once(sLoadCasLibFlag, [] {
        sCasLibWrapper = new CasLibWrapper();
    });

    aml_dvb_init_para_t initPara{.type = CA_TYPE_UNKNOWN};

    initPara.type = CA_TYPE_VMX;
    initPara.vmx_para.vpid = param->videoPid;
    initPara.vmx_para.apid = param->audioPid;
    initPara.vmx_para.vfmt = convertToVFormat(param->videoCodec);
    initPara.vmx_para.afmt = convertToAForamt(param->audioCodec);
    initPara.vmx_para.ecmpid = param->ecmPid[0];
    strncpy(initPara.vmx_para.key_file_path, param->keyPath, sizeof(initPara.vmx_para.key_file_path)-1);
    strncpy(initPara.vmx_para.server_ip, param->serverAddress, sizeof(initPara.vmx_para.server_ip)-1);
    initPara.vmx_para.server_port = param->serverPort;

    int err;
    mCasHandle = sCasLibWrapper->create(&initPara, &err);
}

AmlVMXIptvCas::~AmlVMXIptvCas()
{
    MLOGI("dtor AmlVMXIptvCas");

    if (mCasHandle != nullptr) {
        sCasLibWrapper->destroy(mCasHandle);
        mCasHandle = nullptr;
    }
}

int AmlVMXIptvCas::openSession()
{
    MLOGI("openSession");

    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->start(mCasHandle);
}

int AmlVMXIptvCas::closeSession()
{
#define DMX_RESET_PATH    "/sys/class/stb/demux_reset"
    int ret = 0;
    MLOGI("closeSession");

    RETURN_IF(-1, mCasHandle == nullptr);

    ret = amsysfs_set_sysfs_str(DMX_RESET_PATH, "1");
    if (ret)
        MLOGI("Error ret 0x%x\n", ret);

    return sCasLibWrapper->stop(mCasHandle);
}

int AmlVMXIptvCas::setPrivateData(const uint8_t* data, size_t size)
{
    RETURN_IF(-1, mCasHandle == nullptr);

    AML_MP_UNUSED(data);
    AML_MP_UNUSED(size);

    return 0;
}

int AmlVMXIptvCas::processEcm(const uint8_t* data, size_t size)
{
    RETURN_IF(-1, mCasHandle == nullptr);

    return sCasLibWrapper->writeData(mCasHandle, data, size);
}

int AmlVMXIptvCas::processEmm(const uint8_t* data, size_t size)
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
        MLOGE("dlopen %s failed! %s", libPath, dlerror());
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

CasLibWrapper::~CasLibWrapper() {
    if (mCasLibHandle != nullptr) {
        dlclose(mCasLibHandle);
    }
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
