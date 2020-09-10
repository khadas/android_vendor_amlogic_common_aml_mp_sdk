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

#ifndef _AML_MP_CAS_BASE_H_
#define _AML_MP_CAS_BASE_H_

#include <Aml_MP/Common.h>
#include <Aml_MP/Cas.h>
#include <utils/RefBase.h>

namespace aml_mp {
using android::RefBase;
using android::sp;

class AmlCasBase : public RefBase
{
public:
    static sp<AmlCasBase> create(Aml_MP_InputSourceType inputType, const Aml_MP_CASParams* params);
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
