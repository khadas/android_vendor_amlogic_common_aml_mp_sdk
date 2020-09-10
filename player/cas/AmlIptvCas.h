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

#include "AmlCasBase.h"
#include <Aml_MP/Common.h>
#include <mutex>

typedef struct _dvb_ca dvb_ca_t;

namespace aml_mp {
struct CasLibWrapper;

class AmlIptvCas : public AmlCasBase
{
public:
    AmlIptvCas(Aml_MP_CASType casType, const Aml_MP_IptvCasParam* param);
    ~AmlIptvCas();
    virtual int openSession() override;
    virtual int closeSession() override;
    virtual int setPrivateData(const uint8_t* data, size_t size) override;
    virtual int processEcm(const uint8_t* data, size_t size) override;
    virtual int processEmm(const uint8_t* data, size_t size) override;

private:
    static CasLibWrapper* sCasLibWrapper;
    static std::once_flag sLoadCasLibFlag;

    Aml_MP_CASType mCasType;
    Aml_MP_IptvCasParam mIptvCasParam;

    dvb_ca_t* mCasHandle = nullptr;

    AmlIptvCas(const AmlIptvCas&) = delete;
    AmlIptvCas& operator= (const AmlIptvCas&) = delete;
};
}
