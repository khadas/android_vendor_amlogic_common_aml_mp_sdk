/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_HANDLE_H_
#define _AML_MP_HANDLE_H_

#include <utils/Log.h>
#include <utils/RefBase.h>
#include <Aml_MP/Common.h>

namespace aml_mp {
using android::sp;
using android::RefBase;

struct AmlMpHandle;


struct AmlMpHandle : public RefBase
{
    AmlMpHandle()
    : mMagicID(kMagicID)
    {
    }

    ~AmlMpHandle() {
        mMagicID = ~kMagicID;
    }

    operator bool() const {
        return mMagicID == kMagicID;
    }

private:
    static constexpr uint32_t kMagicID = 0x5A6B7C8D;
    uint32_t mMagicID;

    AmlMpHandle(const AmlMpHandle&) = delete;
    AmlMpHandle& operator= (const AmlMpHandle&) = delete;
};

static inline void* aml_handle_cast(const sp<AmlMpHandle>& amlMpHandle)
{
    if (!(amlMpHandle && *amlMpHandle)) {
        if (amlMpHandle) {
            ALOG(LOG_INFO, "AmlMpHandle", "[%d] AmlMpHandle CHECK failed!", __LINE__);
        }

        return AML_MP_INVALID_HANDLE;
    }

    return amlMpHandle.get();
}

template <typename T>
sp<T> aml_handle_cast(void* h)
{
    AmlMpHandle* amlMpHandle = static_cast<AmlMpHandle*>(h);
    if (!(amlMpHandle && *amlMpHandle)) {
        if (amlMpHandle) {
            ALOG(LOG_INFO, "AmlMpHandle", "[%d] AmlMpHandle CHECK failed!", __LINE__);
        }

        return nullptr;
    }

    T* t = static_cast<T*>(h);
    return sp<T>(t);
}

}

#endif

