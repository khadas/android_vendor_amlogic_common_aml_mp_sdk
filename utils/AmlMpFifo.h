/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_FIFO_H_
#define _AML_MP_FIFO_H_

#include <utils/Log.h>
#include <stdint.h>
#include <mutex>

namespace aml_mp {

class AmlMpFifo
{
public:
    explicit AmlMpFifo(size_t size) {
        --size;
        for (size_t i = 1; i < sizeof(size) * 8; i <<= 1) {
            size |= size >> i;
        }
        ++size;

        mBuffer = (uint8_t*)calloc(size, 1);
        mSize = size;
        ALOG(LOG_INFO, "AmlMpFifo", "Fifo_Size:%#x", mSize);
    }

    ~AmlMpFifo() {
        if (mBuffer) {
            free(mBuffer);
            mBuffer = nullptr;
        }
    }

    size_t get(void* buffer, size_t size) {
        std::unique_lock<std::mutex> _l(mLock);

        size_t len = std::min(size, in - out);
        size_t l = std::min(len, mSize - (out & mSize-1));
        memcpy(buffer, mBuffer + (out & mSize-1), l);
        memcpy((uint8_t*)buffer + l, mBuffer, len-l);

        out += len;
        return len;
    }

    size_t put(const void* buffer, size_t size) {
        std::unique_lock<std::mutex> _l(mLock);

        size_t len = std::min(size, mSize - in + out);
        size_t l = std::min(len, mSize - (in & mSize-1));
        memcpy(mBuffer + (in & mSize-1), buffer, l);
        memcpy(mBuffer, (uint8_t*)buffer + l, len-l);

        in += len;
        return len;
    }

    size_t size() const {
        std::unique_lock<std::mutex> _l(mLock);

        return in - out;
    }

    size_t space() const {
        std::unique_lock<std::mutex> _l(mLock);

        return mSize - (in - out);
    }

    bool empty() const {
        std::unique_lock<std::mutex> _l(mLock);

        return in == out;
    }

    void reset() {
        std::unique_lock<std::mutex> _l(mLock);
        in = out = 0;
    }

private:
    mutable std::mutex mLock;
    size_t mSize = 0;
    uint8_t* mBuffer = nullptr;
    size_t in = 0;
    size_t out = 0;

    AmlMpFifo(const AmlMpFifo&) = delete;
    AmlMpFifo& operator= (const AmlMpFifo&) = delete;
};



}





#endif
