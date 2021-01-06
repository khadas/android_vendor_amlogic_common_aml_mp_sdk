/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <utils/AmlMpRefBase.h>
#include <mutex>
#include <Aml_MP/Common.h>

namespace aml_mp {
struct ISourceReceiver : virtual public AmlMpRefBase {
    ~ISourceReceiver() {}
    void linkNextReceiver(const sptr<ISourceReceiver>& receiver) {
        mNextReceiver = receiver;
    }

    virtual int writeData(const uint8_t* buffer, size_t size) {
        if (mNextReceiver) {
            mNextReceiver->writeData(buffer, size);
        }

        return size;
    }

protected:
    sptr<ISourceReceiver> mNextReceiver;
};

class Source : public AmlMpRefBase {
public:
    enum Flags : uint32_t {
        kIsMemorySource         = 1 << 0,
        kIsHardwareSource       = 1 << 1,
        kIsDVRSource            = 1 << 2,
    };

    static sptr<Source> create(const char* url);
    virtual ~Source();

    virtual int initCheck() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int restart() {
        return 0;
    }

    virtual void signalQuit() = 0;

    Aml_MP_DemuxId getDemuxId() const {
        return mDemuxId;
    }

    int getProgramNumber() const {
        return mProgramNumber;
    }

    uint32_t getFlags() const {
        return mFlags;
    }

    void addSourceReceiver(const sptr<ISourceReceiver>& receiver) {
        std::lock_guard<std::mutex> _l(mLock);
        mReceiver = receiver;
    }

    void removeSourceReceiver(const sptr<ISourceReceiver>& receiver) {
        std::lock_guard<std::mutex> _l(mLock);
        mReceiver = receiver;
        if (mReceiver == receiver) {
            mReceiver = nullptr;
        }
    }

    sptr<ISourceReceiver> sourceReceiver() const {
        std::lock_guard<std::mutex> _l(mLock);
        return mReceiver;
    }

protected:
    Source(Aml_MP_DemuxId demuxId, int programNumber, uint32_t flags);
    Aml_MP_DemuxId mDemuxId = AML_MP_HW_DEMUX_ID_0;
    int mProgramNumber;
    uint32_t mFlags;

private:
    void parseArguments(const std::string& argument, int* demuxId, int* programNumber);
    mutable std::mutex mLock;
    sptr<ISourceReceiver> mReceiver;

    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;
};

class DVRSource : public Source {
public:
    DVRSource(Aml_MP_DemuxId demuxId, int programNumber, uint32_t flags)
    : Source(demuxId, programNumber, flags) {}
    ~DVRSource() {}
    int initCheck() {return 0;}
    int start() {return 0;}
    int stop() { return 0; }
    void signalQuit() {}

private:
    DVRSource(const DVRSource&) = delete;
    DVRSource& operator=(const DVRSource&) = delete;
};


}


#endif
