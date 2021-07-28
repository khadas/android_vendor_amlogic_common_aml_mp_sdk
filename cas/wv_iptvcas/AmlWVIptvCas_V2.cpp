/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlWVIptvCas_V2"
#include <utils/Log.h>
#include <utils/AmlMpUtils.h>
#include "AmlWVIptvCas_V2.h"
#include <dlfcn.h>
#include <cutils/properties.h>
#include "utils/Amlsysfsutils.h"
#include <unistd.h>
#include "cas/AmCasLibWrapper.h"
#include <utils/AmlMpLog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

namespace aml_mp {

const int convertToAmlMPErrorCode_V2(AmCasCode_t casResult) {
    switch (casResult) {
        case AM_CAS_SUCCESS:
            return AML_MP_OK;
        case AM_CAS_ERROR:
            return AML_MP_ERROR;
        case AM_CAS_ERR_SYS:
            return AML_MP_ERROR_PERMISSION_DENIED;
        default:
            return AML_MP_ERROR;
    }
}

AmlWVIptvCas_V2::AmlWVIptvCas_V2(Aml_MP_CASServiceType serviceType)
:AmlCasBase(serviceType)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

    MLOGI("ctor AmlWVIptvCas_V2");

}

AmlWVIptvCas_V2::~AmlWVIptvCas_V2()
{
    MLOGI("dtor AmlWVIptvCas_V2");
    int ret = 0;

    if (mDscFd) {
        ret = close(mDscFd);
        if (ret)
            MLOGE("~AmlWVIptvCas_V2 fd= %d error=%d \n", mDscFd, errno);
    }
    pIptvCas.clear();
}

int AmlWVIptvCas_V2::startDescrambling(const Aml_MP_IptvCASParams* param)
{
    MLOG();
    AmlCasBase::startDescrambling(param);

    int ret = 0;

    CasStreamInfo initPara;
    initPara.ca_system_id = param->caSystemId;
    initPara.video_pid = param->videoPid;
    initPara.audio_pid = param->audioPid;
    initPara.ecm_pid[0] = param->ecmPid[0];
    initPara.ecm_pid[1] = param->ecmPid[1];
    initPara.av_diff_ecm = false;
    if (initPara.ecm_pid[0] != 0x1FFF && initPara.ecm_pid[1] != 0x1FFF &&
        initPara.ecm_pid[0] != initPara.ecm_pid[1]) {
        initPara.av_diff_ecm = true;
    }

    MLOGI("%s,vpid=0x%x,apid=0x%x,ecmpid=0x%x,0x%x", __func__, initPara.video_pid, initPara.audio_pid,
        initPara.ecm_pid[0], initPara.ecm_pid[1]);

    bool useThirdPartyLicServer = false;
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("wvcas.proxy.vendor", value, "GOOGLE");
    if (!memcmp(value, "GOOGLE",6) && param->private_size == 0) {
        useThirdPartyLicServer = false;
        MLOGI("wvcas use google default license server.\n");
    } else {
        useThirdPartyLicServer = true;
        MLOGI("wvcas use third-party license server.\n");
    }

    if (useThirdPartyLicServer) {
        uint8_t *pdata = const_cast<uint8_t *>(param->private_data);
        initPara.private_data = pdata;  //get from pmt
        initPara.pri_data_len = param->private_size;
        MLOGI("wvcas use third private data: 0x%x,0x%x, len=%d",
            initPara.private_data[0],initPara.private_data[1], initPara.pri_data_len);
    } else {
        initPara.private_data = NULL;
        initPara.pri_data_len = 0;
    }

    initPara.headers = NULL;

    //initPara->headers = cas_stream_info->headers;


#ifndef __ANDROID_VNDK__
    pIptvCas = new AmCasLibWrapper<AML_MP_CAS_SERVICE_WIDEVINE>("libdec_ca_wvcas.system.so");
#else
    pIptvCas = new AmCasLibWrapper<AML_MP_CAS_SERVICE_WIDEVINE>("libdec_ca_wvcas.so");
#endif

    if (pIptvCas == nullptr) {
        MLOGE(" call AmCasIPTV() failed");
        return -1;
    }

    mDscFd = 0;
    mFirstEcm = 0;
    pIptvCas->setCasInstanceId(param->demuxId);

    ret = pIptvCas->setPrivateData((void *)&initPara, sizeof(CasStreamInfo));
    MLOGE(" after call AmCasIPTV() setPrivateData, ret=%d", ret);
    if (ret != 0) {
        MLOGI("setPrivateData failed, ret =%d", ret);
        return -1;
    }

    ret = pIptvCas->provision();
    MLOGE(" after call AmCasIPTV() provision, ret=%d", ret);
    if (ret != 0) {
        MLOGI("provision failed, ret =%d", ret);
        return -1;
    }

    if (pIptvCas) {
        setDscSource();
        MLOGI("%s, vpid=0x%x, apid=0x%x", __func__, mIptvCasParam.videoPid, mIptvCasParam.audioPid);
        pIptvCas->setPids(mIptvCasParam.videoPid, mIptvCasParam.audioPid);
        ret = pIptvCas->openSession(&sessionId[0]);
    }

    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

int AmlWVIptvCas_V2::stopDescrambling()
{
    MLOGI("closeSession");
    int ret = 0;

    if (pIptvCas) {
        ret = pIptvCas->closeSession(&sessionId[0]);
        pIptvCas.clear();
    }

    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

int AmlWVIptvCas_V2::setPrivateData(const uint8_t* data, size_t size)
{
    int ret = 0;

    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        ret = pIptvCas->setPrivateData((void*)pdata, size);
        if (ret != 0) {
            MLOGI("setPrivateData failed, ret =%d", ret);
            return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
        }
    }

    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

int AmlWVIptvCas_V2::checkEcmProcess(uint8_t* pBuffer, uint32_t vEcmPid, uint32_t aEcmPid,size_t * nSize)
{
  int ret = 0;
  int len = 0,pid = 0;
  unsigned int rem = *nSize;

  uint8_t * psync = pBuffer;
  uint8_t * current = NULL;

  while (rem >= TS_PACKET_SIZE)
  {
      if (*psync != 0x47)
      {
          ++psync;
          --rem;
          continue;
      }
      if ((*(psync) == 0x47) && ((rem == TS_PACKET_SIZE) || (*(psync+TS_PACKET_SIZE) == 0x47)))
      {
          current = psync;
          pid = (( current[1] << 8 | current[2]) & 0x1FFF);
          if (pid == vEcmPid || pid == aEcmPid)
          {
              if (memcmp(mEcmTsPacket + 4,psync + 4,TS_PACKET_SIZE- 4))
              {
                  memcpy(mEcmTsPacket, psync, TS_PACKET_SIZE);
                  std::string ecmDataStr;
                  char hex[3];
                  for (int i = 0; i < 64; i++) {
                      snprintf(hex, sizeof(hex), "%02X", mEcmTsPacket[i]);
                      ecmDataStr.append(hex);
                      ecmDataStr.append(" ");
                  }
                  MLOGI("checkEcmProcess, ecmDataStr.c_str()=%s", ecmDataStr.c_str());
                  if (pIptvCas)
                  {
                      if (pid == mIptvCasParam.ecmPid[1])
                         ret = pIptvCas->processEcm(0, 1, mIptvCasParam.ecmPid[1], mIptvCasParam.ecmPid[0], mEcmTsPacket, TS_PACKET_SIZE);
                      else
                         ret = pIptvCas->processEcm(0, 0, mIptvCasParam.ecmPid[1], mIptvCasParam.ecmPid[0], mEcmTsPacket, TS_PACKET_SIZE);

                  }
              }
              if (mFirstEcm != 1) {
                  MLOGI("first_SetECM find\n");
                  mFirstEcm = 1;
              }
          }
      }
      psync += TS_PACKET_SIZE;
      rem -= TS_PACKET_SIZE;
  }

  return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}


int AmlWVIptvCas_V2::processEcm(bool isSection, int ecmPid, const uint8_t* data, size_t size)
{
    int ret = 0;

    //==============FIXME=============
    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        MLOGV("%s, pid=0x%x, size=%zu", __func__, mIptvCasParam.videoPid, size);
        if (isSection) {
            MLOGI("%s, pid=0x%x, size=%zu", __func__, mIptvCasParam.videoPid, size);
            if (ecmPid == mIptvCasParam.ecmPid[1])
                ret = pIptvCas->processEcm(1, 1, mIptvCasParam.ecmPid[1], mIptvCasParam.ecmPid[0], pdata, size);
            else
                ret = pIptvCas->processEcm(1, 0, mIptvCasParam.ecmPid[1], mIptvCasParam.ecmPid[0], pdata, size);
        } else {
            checkEcmProcess(pdata, mIptvCasParam.ecmPid[0], mIptvCasParam.ecmPid[1], &size);
        }
    }

    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

int AmlWVIptvCas_V2::processEmm(const uint8_t* data, size_t size)
{
    int ret = 0;

    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        ret = pIptvCas->processEmm(0, mIptvCasParam.videoPid ,pdata, size);
    }

    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

int AmlWVIptvCas_V2::dscDevOpen(const char *port_addr, int flags)
{
    int r;
    int retry_open_times = 0;
retry_open:
    r = open(port_addr, flags);
    if (r < 0 /*&& r == EBUSY */)
    {
        retry_open_times++;
        usleep(10*1000);
        if (retry_open_times < 100)
        {
            goto retry_open;
        }
        MLOGI("retry_open [%s] failed,ret = %d error=%d used_times=%d*10(ms)\n", port_addr, r, errno, retry_open_times);
        return r;
    }
    if (retry_open_times > 0)
    {
        MLOGI("retry_open [%s] success\n", port_addr);
    }
    return (int)r;
}


int AmlWVIptvCas_V2::setDscSource()
{
    int ret = 0;
    bool use_hw_multi_demux = false;

    if (access("/sys/module/dvb_demux/", F_OK) == 0) {
        MLOGI("Work with Hw Multi Demux.");
        use_hw_multi_demux = true;
    } else {
        MLOGI("Work with Hw Demux.");
        use_hw_multi_demux = false;
    }
#if 1
    if (!use_hw_multi_demux) {
        ret = amsysfs_set_sysfs_str(DMX0_SOURCE_PATH, DMX_SRC);
        if (ret)
            MLOGI("Error ret 0x%x\n", ret);
        mDscFd = dscDevOpen(DSC_DEVICE, O_RDWR);
        MLOGI("%s, dsc_fd=%d\n", __func__, mDscFd);
        ret = amsysfs_set_sysfs_str(DSC0_SOURCE_PATH, DSC_SRC);
        if (ret)
            MLOGI("Error ret 0x%x\n", ret);
    }
    else
    {
       /*
        if (parm.source == TS_MEMORY)
            ret = stb_set_tsn_source(TSN_IPTV);
        else if (parm.source == TS_DEMOD)
            ret = stb_set_tsn_source(TSN_DVB);
        */
        //widevine cas plugin have do it
        /*ret = amsysfs_set_sysfs_str(TSN_PATH, TSN_IPTV);
        if (ret) {
            MLOGI("hw multi demux Error ret 0x%x\n", ret);
        }
        */
    }
#endif
    return convertToAmlMPErrorCode_V2((AmCasCode_t)ret);
}

}
