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

#define LOG_TAG "AmlDvbCasHal"
#include <utils/Log.h>
#include "AmlDvbCasHal.h"

#ifndef __ANDROID_VNDK__
#define DMX_DEV_NO (0)

namespace aml_mp {

CasHandle mCas=0;

static AM_RESULT cas_event_cb(CasSession session, char *json)
{
    ALOGI(0, "%s:\n%s", __func__, json);
    return AM_ERROR_SUCCESS;
}

AmlDvbCasHal::AmlDvbCasHal(Aml_MP_CASType casType, const Aml_MP_DvbCasParam* param)
{
    ALOGI("AmlDvbCasHal");
    int ret;
    mEmmPid = param->emmPid;
    mDemuxPid = (int)param->demuxId;
    memset(&cas_para, 0x0, sizeof(AM_CA_ServiceInfo_t));
    cas_para.service_id = param->serviceId;
    cas_para.service_type = SERVICE_LIVE_PLAY;
    cas_para.ecm_pid = param->ecmPid;
    cas_para.stream_pids[0] = param->streamPids[0];
    cas_para.stream_pids[1] = param->streamPids[1];
    cas_para.stream_num = 2;
    cas_para.ca_private_data_len = 0;
    if (!mCas) {
        ret = AM_CA_Init(&mCas);
        ALOGI("CAS init ret = %d\r\n", ret);
    }
    mSecmem_session = AM_CA_CreateSecmem(mCas_session, SERVICE_LIVE_PLAY, NULL, NULL);
    if (!mSecmem_session) {
        ALOGE("CreateSecmem fail");
    }
}

AmlDvbCasHal::~AmlDvbCasHal()
{
    ALOGI("~AmlDvbCasHal");
    //
}

int AmlDvbCasHal::openSession()
{
    int ret =0;
    ALOGI("openSession");
    ret = AM_CA_SetEmmPid(mCas, mDemuxPid, mEmmPid);
    if (ret) {
        ALOGE("CAS set emm PID failed. ret = %d\r\n", ret);
    }
    ret = AM_CA_OpenSession(mCas, &mCas_session);
    if (ret) {
        ALOGE("CAS open session failed. ret = %d\r\n", ret);
        return -1;
    }
    ret = AM_CA_RegisterEventCallback(mCas_session, cas_event_cb);
    if (ret) {
        ALOGE("CAS RegisterEventCallback failed. ret = %d\r\n", ret);
    }

    ret = AM_CA_StartDescrambling(mCas_session, &cas_para);
    if (ret) {
        ALOGE("CAS start descrambling failed. ret = %d\r\n", ret);
        return -1;
    }
    ALOGI("CAS started");
    return 0;
}

int AmlDvbCasHal::closeSession()
{
    ALOGI("closeSession");
    AM_CA_StopDescrambling(mCas_session);
    AM_CA_DestroySecmem(mCas_session,mSecmem_session);
    AM_CA_CloseSession(mCas_session);
    return 0;
}

int AmlDvbCasHal::setPrivateData(const uint8_t* data, size_t size)
{
    return 0;
}

int AmlDvbCasHal::processEcm(const uint8_t* data, size_t size)
{
    return 0;
}

int AmlDvbCasHal::processEmm(const uint8_t* data, size_t size)
{
    return 0;
}

}

#endif
