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
#include "Amlsysfsutils.h"
#include <unistd.h>
#include "amCasIPTV.h"


namespace aml_mp {

AmlVMXIptvCas::AmlVMXIptvCas(const Aml_MP_IptvCasParams* param, int instanceId)
:mIptvCasParam(*param)
,mInstanceId(instanceId)
{
    MLOGI("ctor AmlVMXIptvCas");
    //AML_MP_UNUSED(mIptvCasParam);
#if 0
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
    if (!memcmp(value, "GOOGLE",6)) {
        useThirdPartyLicServer = false;
        MLOGI("wvcas use google default license server.\n");
    } else {
        useThirdPartyLicServer = true;
        MLOGI("wvcas use third-party license server.\n");
    }

    if (useThirdPartyLicServer) {
        initPara.private_data = &test_private_data[0];  //get from pmt
        initPara.pri_data_len = sizeof(test_private_data);
    } else {
        initPara.private_data = NULL;
        initPara.pri_data_len = 0;
    }


    pIptvCas = new AmCasIPTV();
    if (pIptvCas == nullptr) {
        MLOGE(" call AmCasIPTV() failed");
        return;
    }

    mDscFd = 0;
    mFirstEcm = 0;

    int ret = 0;
    ret = pIptvCas->provision();
    MLOGE(" after call AmCasIPTV() provision, ret=%d", ret);
    if (ret != 0) {
        MLOGI("provision failed, ret =%d", ret);
        return;
    }

    ret = pIptvCas->setPrivateData((void *)&initPara, sizeof(CasStreamInfo));
    MLOGE(" after call AmCasIPTV() setPrivateData, ret=%d", ret);
    if (ret != 0) {
        MLOGI("setPrivateData failed, ret =%d", ret);
        return;
    }
#endif
}

AmlVMXIptvCas::~AmlVMXIptvCas()
{
    MLOGI("dtor AmlVMXIptvCas");
    int ret = 0;

    if (mDscFd) {
        ret = close(mDscFd);
        if (ret)
            MLOGE("~AmlVMXIptvCas fd= %d error=%d \n", mDscFd, errno);
    }


    if (pIptvCas) {
        delete pIptvCas;
        pIptvCas = nullptr;
    }

}

int AmlVMXIptvCas::openSession()
{
    MLOGI("openSession");
    int ret = 0;

    if (pIptvCas) {
        setDscSource();
        MLOGI("%s, vpid=0x%x, apid=0x%x", __func__, mIptvCasParam.videoPid, mIptvCasParam.audioPid);
        pIptvCas->setPids(mIptvCasParam.videoPid, mIptvCasParam.audioPid);
        ret = pIptvCas->openSession(&sessionId[0]);
    }

    return ret;
}

int AmlVMXIptvCas::closeSession()
{
    MLOGI("closeSession");
    int ret = 0;

    if (pIptvCas) {
        ret = pIptvCas->closeSession(&sessionId[0]);
    }

    return ret;
}

int AmlVMXIptvCas::setPrivateData(const uint8_t* data, size_t size)
{
    int ret = 0;

    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        ret = pIptvCas->setPrivateData((void*)pdata, size);
        if (ret != 0) {
            MLOGI("setPrivateData failed, ret =%d", ret);
            return ret;
        }
    }

    return ret;
}

int AmlVMXIptvCas::checkEcmProcess(uint8_t* pBuffer, uint32_t vEcmPid, uint32_t aEcmPid,size_t * nSize)
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
                      ret = pIptvCas->processEcm(0, pid, mEcmTsPacket, TS_PACKET_SIZE);
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

  return ret;
}


int AmlVMXIptvCas::processEcm(const uint8_t* data, size_t size)
{
    int ret = 0;

    //==============FIXME=============
    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        MLOGI("%s, pid=0x%x, size=%zu", __func__, mIptvCasParam.videoPid, size);
        if (size < 188) {
            ret = pIptvCas->processEcm(1, mIptvCasParam.ecmPid[0] ,pdata, size);
        } else {
           checkEcmProcess(pdata, mIptvCasParam.ecmPid[0], mIptvCasParam.ecmPid[1], &size);
        }
    }

    return ret;
}

int AmlVMXIptvCas::processEmm(const uint8_t* data, size_t size)
{
    int ret = 0;

    if (pIptvCas) {
        uint8_t *pdata = const_cast<uint8_t *>(data);
        ret = pIptvCas->processEmm(0, mIptvCasParam.videoPid ,pdata, size);
    }

    return ret;
}

int AmlVMXIptvCas::dscDevOpen(const char *port_addr, int flags)
{
    int r;
#if 0
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
#endif
    return (int)r;
}


int AmlVMXIptvCas::setDscSource()
{
    int ret = 0;
    bool use_hw_multi_demux = false;
#if 0
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
        ret = amsysfs_set_sysfs_str(TSN_PATH, TSN_IPTV);
        if (ret)
            MLOGI("hw multi demux Error ret 0x%x\n", ret);
    }
#endif
#endif
    return ret;
}

}
