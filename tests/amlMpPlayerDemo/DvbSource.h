/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _DVB_SOURCE_H_
#define _DVB_SOURCE_H_

#include "Source.h"
#include <string>

extern "C" {
#include <linux/dvb/frontend.h>
#include "fend.h"
}

namespace aml_mp {
class DvbSource : public Source
{
public:
    DvbSource(const char* proto, const char* address, int programNumber, uint32_t flags);
    ~DvbSource();

    virtual int initCheck() override;
    virtual int start() override;
    virtual int stop() override;
    virtual void signalQuit() override;

private:
    enum FendLockState {
        FEND_STATE_UNKNOWN,
        FEND_STATE_LOCKED,
        FEND_STATE_LOCK_TIMEOUT,
        FEND_STATE_ERROR,
    };

	int openFend(int fendIndex);
	void closeFend();
    FendLockState queryFendLockState() const;

    std::string mProto;
    std::string mAddress;
    dmd_device_type_t mDeviceType;
    dmd_delivery_t mDelivery;
    int mDemuxId = -1;

	int mFendFd = -1;

    std::atomic_bool mRequestQuit = false;

    DvbSource(const DvbSource&) = delete;
    DvbSource& operator= (const DvbSource&) = delete;
};

}

#endif
