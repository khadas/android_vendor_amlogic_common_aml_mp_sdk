/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDemuxBase"
#include <utils/Log.h>
#include "AmlHwDemux.h"
#include "AmlSwDemux.h"

namespace aml_mp {

sp<AmlDemuxBase> AmlDemuxBase::create(bool isHardwareDemux)
{
    sp<AmlDemuxBase> demux;

    if (isHardwareDemux) {
        demux = new AmlHwDemux();
    } else {
        demux = new AmlSwDemux();
    }

    return demux;
}


AmlDemuxBase::AmlDemuxBase()
{

}

AmlDemuxBase::~AmlDemuxBase()
{

}


}
