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

#include "AmlCasBase.h"
#include "am_cas.h"

namespace aml_mp {
class AmlDvbCasHal : public AmlCasBase
{
public:
    AmlDvbCasHal(Aml_MP_CASType casType, const Aml_MP_DvbCasParam* param);
    ~AmlDvbCasHal();
    virtual int openSession() override;
    virtual int closeSession() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;

private:
    AmlDvbCasHal(const AmlDvbCasHal&) = delete;
    AmlDvbCasHal& operator= (const AmlDvbCasHal&) = delete;
    CasSession mCas_session;
    SecMemHandle mSecmem_session;
    int mEmmPid;
    int mDemuxPid;
    AM_CA_ServiceInfo_t cas_para;
};

}

#endif
