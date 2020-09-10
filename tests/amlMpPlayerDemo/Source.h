#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <utils/RefBase.h>
#include <mutex>

namespace aml_mp {
using namespace android;

struct ISourceReceiver : public RefBase {
    ~ISourceReceiver() {}
    void linkNextReceiver(const sp<ISourceReceiver>& receiver) {
        mNextReceiver = receiver;
    }

    virtual int writeData(const uint8_t* buffer, size_t size) {
        if (mNextReceiver) {
            mNextReceiver->writeData(buffer, size);
        }

        return size;
    }

protected:
    sp<ISourceReceiver> mNextReceiver;
};

class Source : public RefBase {
public:
    enum Flags : uint32_t {
        kIsMemorySource         = 1 << 0,
        kIsHardwareSource       = 1 << 1,
    };

    static sp<Source> create(const char* url);
    virtual ~Source();

    virtual int initCheck() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int restart() {
        return 0;
    }

    virtual void signalQuit() = 0;

    int getProgramNumber() const {
        return mProgramNumber;
    }

    uint32_t getFlags() const {
        return mFlags;
    }

    void addSourceReceiver(const sp<ISourceReceiver>& receiver) {
        std::lock_guard<std::mutex> _l(mLock);
        mReceiver = receiver;
    }

    void removeSourceReceiver(const sp<ISourceReceiver>& receiver) {
        std::lock_guard<std::mutex> _l(mLock);
        mReceiver = receiver;
        if (mReceiver == receiver) {
            mReceiver = nullptr;
        }
    }

    sp<ISourceReceiver> sourceReceiver() const {
        std::lock_guard<std::mutex> _l(mLock);
        return mReceiver;
    }

protected:
    Source(int programNumber, uint32_t flags);
    int mProgramNumber;
    uint32_t mFlags;

private:
    mutable std::mutex mLock;
    sp<ISourceReceiver> mReceiver;

    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;
};


}


#endif
