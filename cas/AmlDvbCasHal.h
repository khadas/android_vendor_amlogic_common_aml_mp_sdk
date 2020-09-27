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

#ifndef _AML_MP_DVB_CAS_HAL_H_
#define _AML_MP_DVB_CAS_HAL_H_

#include <Aml_MP/Cas.h>
#include <utils/AmlMpHandle.h>
#include "am_cas.h"

namespace aml_mp {

class AmlDvbCasHal : public AmlMpHandle
{
public:
    AmlDvbCasHal();
    ~AmlDvbCasHal();

    int registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData);

    int startDescrambling(Aml_MP_CASServiceInfo* serviceInfo);
    int stopDescrambling();
    int updateDescramblingPid(int oldStreamPid, int newStreamPid);

    int startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo);
    int stopDVRRecord();

    int setDVRReplayPreParam(struct Aml_MP_CASPreParam* params);
    int startDVRReplay(Aml_MP_CASCryptoParams* cryptoParams);
    int stopDVRReplay();

    int DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams);
    int DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams);

    AML_MP_HANDLE createSecmem(Aml_MP_CASServiceType, void** pSecBuf, uint32_t* size);
    int destroySecmem(AML_MP_HANDLE secMem);

private:
    CasSession mCasSession = 0;

private:
    AmlDvbCasHal(const AmlDvbCasHal&) = delete;
    AmlDvbCasHal& operator= (const AmlDvbCasHal&) = delete;
};

}

#endif
