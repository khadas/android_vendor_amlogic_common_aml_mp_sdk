/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AML_MP_CHUNK_FIFO_H_
#define AML_MP_CHUNK_FIFO_H_

#include <mutex>
#include "AmlMpFifo.h"

namespace aml_mp {

struct AmlMpChunkFifo {
public:
    AmlMpChunkFifo();
    void init(size_t maxSize, size_t chunkSize = 1 * 1024 * 1024);
    ~AmlMpChunkFifo();

    size_t get(void* buffer, size_t size);
    size_t put(const void*buffer, size_t size);
    size_t size() const;
    size_t space() const;
    bool empty() const;
    void reset();

private:
    mutable std::mutex mLock;
    char** mChunkTable = nullptr;
    size_t mChunkSize = 0;
    size_t mMaxSize = 0;
    size_t mChunkCount = 0;
    size_t mPutSize = 0;
    size_t mGetSize = 0;

    AmlMpChunkFifo(const AmlMpChunkFifo&) = delete;
    AmlMpChunkFifo& operator= (const AmlMpChunkFifo&) = delete;
};

}

#endif
