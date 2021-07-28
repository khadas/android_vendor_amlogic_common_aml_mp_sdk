/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpChunkFifo"
#include <utils/AmlMpLog.h>
#include "AmlMpChunkFifo.h"
#include <algorithm>
#include <cassert>
#include <string.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

AmlMpChunkFifo::AmlMpChunkFifo()
{
}

void AmlMpChunkFifo::init(size_t maxSize, size_t chunkSize)
{
    mMaxSize = roundUpPowerOfTwo(maxSize);
    mChunkSize = roundUpPowerOfTwo(chunkSize);
    mChunkCount = mMaxSize / mChunkSize;

    MLOGI("maxSize = %u, mChunkCount = %u", maxSize, mChunkCount);

    mChunkTable = new char*[mChunkCount]{};

    for (int i = 0; i < mChunkCount; ++i) {
        if (mChunkTable[i] != nullptr) {
            MLOGE("ERROR! mChunkTable pointer:%p, i:%d", mChunkTable[i], i);
        }
    }
}

AmlMpChunkFifo::~AmlMpChunkFifo()
{
    if (mChunkTable) {
        for (size_t i = 0; i < mChunkCount; ++i) {
            if (!mChunkTable[i]) {
                break;
            }

            delete[] mChunkTable[i];
        }

        delete[] mChunkTable;
    }
}

size_t AmlMpChunkFifo::put(const void* buffer, size_t size)
{
    std::unique_lock<std::mutex> _l(mLock);
    size = std::min(size, mMaxSize-mPutSize+mGetSize);
    size_t total = size;

    size_t len = 0;
    while (size) {
        int index = (mPutSize / mChunkSize) % mChunkCount;
        int offset = mPutSize % mChunkSize;
        char* f = mChunkTable[index];
        if (f == nullptr) {
            assert(offset == 0);
            f = mChunkTable[index] = new char[mChunkSize];
        }
        len = std::min(size, mChunkSize-offset);
        memcpy(f+offset, buffer, len);
        size -= len;
        buffer = (char*)buffer + len;
        mPutSize += len;
    }

    return total - size;
}

size_t AmlMpChunkFifo::get(void* buffer, size_t size)
{
    std::unique_lock<std::mutex> _l(mLock);
    size = std::min(size, mPutSize - mGetSize);
    size_t total = size;

    size_t len = 0;
    while (size) {
        int index = (mGetSize/mChunkSize) % mChunkCount;
        int offset = mGetSize % mChunkSize;
        char* f = mChunkTable[index];
        if (f == nullptr) {
            MLOGE("ERROR, chunk buffer is NULL, index:%d, size:%d", index, size);
            break;
        }
        len = std::min(size, mChunkSize-offset);
        memcpy(buffer, f+offset, len);
        size -= len;
        buffer = (char*)buffer + len;
        mGetSize += len;
    }

    return total - size;
}

size_t AmlMpChunkFifo::size() const
{
    std::unique_lock<std::mutex> _l(mLock);
    return mPutSize - mGetSize;
}

size_t AmlMpChunkFifo::space() const
{
    std::unique_lock<std::mutex> _l(mLock);
    return mMaxSize - mPutSize + mGetSize;
}

bool AmlMpChunkFifo::empty() const
{
    std::unique_lock<std::mutex> _l(mLock);
    return mPutSize == mGetSize;
}

void AmlMpChunkFifo::reset()
{
    std::unique_lock<std::mutex> _l(mLock);
    mPutSize = mGetSize = 0;
}


}
