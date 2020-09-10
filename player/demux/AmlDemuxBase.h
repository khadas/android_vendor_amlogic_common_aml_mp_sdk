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

#ifndef _AML_MP_DEMUX_BASE_H_
#define _AML_MP_DEMUX_BASE_H_

#include <Aml_MP/Common.h>
#include <utils/RefBase.h>

namespace aml_mp {
using android::RefBase;
using android::sp;

///////////////////////////////////////////////////////////////////////////////
typedef int (*Aml_MP_Demux_SectionFilterCb)(size_t size, const uint8_t* data, void* userData);

class AmlDemuxBase : public RefBase
{
public:
    static sp<AmlDemuxBase> create(bool isHardwareDemux);
    virtual ~AmlDemuxBase();

    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId = AML_MP_DEMUX_ID_DEFAULT) = 0;
    virtual int close() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int flush() = 0;
    virtual AML_MP_HANDLE createChannel(int pid) = 0;
    virtual int destroyChannel(AML_MP_HANDLE channel) = 0;
    virtual int openChannel(AML_MP_HANDLE channel) = 0;
    virtual int closeChannel(AML_MP_HANDLE channel) = 0;
    virtual AML_MP_HANDLE createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData) = 0;
    virtual int destroyFilter(AML_MP_HANDLE filter) = 0;
    virtual int attachFilter(AML_MP_HANDLE filter, AML_MP_HANDLE channel) = 0;
    virtual int detachFilter(AML_MP_HANDLE filter, AML_MP_HANDLE channel) = 0;
    virtual int feedTs(const uint8_t* buffer, size_t size) {
        return size;
    }

protected:
    AmlDemuxBase();

private:
    AmlDemuxBase(const AmlDemuxBase&) = delete;
    AmlDemuxBase& operator= (const AmlDemuxBase&) = delete;
};

}





#endif
