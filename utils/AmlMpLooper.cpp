//
// Copyright 2010 The Android Open Source Project
//
// A looper implementation based on epoll().
//
#define LOG_TAG "AmlMpLooper"

//#define LOG_NDEBUG 0

// Debugs poll and wake interactions.
#define DEBUG_POLL_AND_WAKE 0

// Debugs callback registration and invocation.
#define DEBUG_CALLBACKS 0

#include "AmlMpLooper.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <chrono>
#define KEEP_ALOGX
#include "AmlMpLog.h"

namespace aml_mp {
inline int64_t getNowUs()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return now;
}

int toMillisecondTimeoutDelay(int64_t referenceTime, int64_t timeoutTime)
{
    int64_t timeoutDelayMillis;

    if (timeoutTime > referenceTime) {
        uint64_t timeoutDelay = uint64_t(timeoutTime - referenceTime);
        if (timeoutDelay > uint64_t(INT32_MAX - 1) * 1000000LL) {
            timeoutDelayMillis = -1;
        } else {
            timeoutDelayMillis = (timeoutDelay + 999LL) / 1000LL;
        }
    } else {
        timeoutDelayMillis = 0;
    }

    return (int)timeoutDelayMillis;
}

// --- WeakMessageHandler ---

WeakMessageHandler::WeakMessageHandler(const wptr<MessageHandler>& handler) :
        mHandler(handler) {
}

WeakMessageHandler::~WeakMessageHandler() {
}

void WeakMessageHandler::handleMessage(const Message& message) {
    sptr<MessageHandler> handler = mHandler.promote();
    if (handler != nullptr) {
        handler->handleMessage(message);
    }
}


// --- SimpleLooperCallback ---

SimpleLooperCallback::SimpleLooperCallback(Looper_callbackFunc callback) :
        mCallback(callback) {
}

SimpleLooperCallback::~SimpleLooperCallback() {
}

int SimpleLooperCallback::handleEvent(int fd, int events, void* data) {
    return mCallback(fd, events, data);
}


// --- Looper ---

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;

static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSKey = 0;

Looper::Looper(bool allowNonCallbacks)
    : mAllowNonCallbacks(allowNonCallbacks),
      mSendingMessage(false),
      mPolling(false),
      mEpollRebuildRequired(false),
      mNextRequestSeq(0),
      mResponseIndex(0),
      mNextMessageUptime(LLONG_MAX) {
    mWakeEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (mWakeEventFd < 0) {
        ALOGE("Could not make wake event fd: %s", strerror(errno));
    }

    std::lock_guard<std::mutex> _l(mLock);
    rebuildEpollLocked();
}

Looper::~Looper() {
}

void Looper::initTLSKey() {
    int error = pthread_key_create(&gTLSKey, threadDestructor);
    LOG_ALWAYS_FATAL_IF(error != 0, "Could not allocate TLS key: %s", strerror(error));
}

void Looper::threadDestructor(void *st) {
    Looper* const self = static_cast<Looper*>(st);
    if (self != nullptr) {
        self->decStrong((void*)threadDestructor);
    }
}

void Looper::setForThread(const sptr<Looper>& looper) {
    sptr<Looper> old = getForThread(); // also has side-effect of initializing TLS

    if (looper != nullptr) {
        looper->incStrong((void*)threadDestructor);
    }

    pthread_setspecific(gTLSKey, looper.get());

    if (old != nullptr) {
        old->decStrong((void*)threadDestructor);
    }
}

sptr<Looper> Looper::getForThread() {
    int result = pthread_once(& gTLSOnce, initTLSKey);
    LOG_ALWAYS_FATAL_IF(result != 0, "pthread_once failed");

    return (Looper*)pthread_getspecific(gTLSKey);
}

sptr<Looper> Looper::prepare(int opts) {
    bool allowNonCallbacks = opts & PREPARE_ALLOW_NON_CALLBACKS;
    sptr<Looper> looper = Looper::getForThread();
    if (looper == nullptr) {
        looper = new Looper(allowNonCallbacks);
        Looper::setForThread(looper);
    }
    if (looper->getAllowNonCallbacks() != allowNonCallbacks) {
        ALOGW("Looper already prepared for this thread with a different value for the "
                "LOOPER_PREPARE_ALLOW_NON_CALLBACKS option.");
    }
    return looper;
}

bool Looper::getAllowNonCallbacks() const {
    return mAllowNonCallbacks;
}

void Looper::rebuildEpollLocked() {
    // Close old epoll instance if we have one.
    if (mEpollFd >= 0) {
#if DEBUG_CALLBACKS
        ALOGD("%p ~ rebuildEpollLocked - rebuilding epoll set", this);
#endif
        ::close(mEpollFd);
        mEpollFd = -1;
    }

    // Allocate the new epoll instance and register the wake pipe.
    mEpollFd = (epoll_create1(EPOLL_CLOEXEC));
    LOG_ALWAYS_FATAL_IF(mEpollFd < 0, "Could not create epoll instance: %s", strerror(errno));

    struct epoll_event eventItem;
    memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mWakeEventFd;
    int result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeEventFd, &eventItem);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not add wake event fd to epoll instance: %s",
                        strerror(errno));

    for (size_t i = 0; i < mRequests.size(); i++) {
        const Request& request = mRequests.at(i);
        struct epoll_event eventItem;
        request.initEventItem(&eventItem);

        int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, request.fd, &eventItem);
        if (epollResult < 0) {
            ALOGE("Error adding epoll events for fd %d while rebuilding epoll set: %s",
                  request.fd, strerror(errno));
        }
    }
}

void Looper::scheduleEpollRebuildLocked() {
    if (!mEpollRebuildRequired) {
#if DEBUG_CALLBACKS
        ALOGD("%p ~ scheduleEpollRebuildLocked - scheduling epoll set rebuild", this);
#endif
        mEpollRebuildRequired = true;
        wake();
    }
}

int Looper::pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    int result = 0;
    for (;;) {
        while (mResponseIndex < mResponses.size()) {
            const Response& response = mResponses.at(mResponseIndex++);
            int ident = response.request.ident;
            if (ident >= 0) {
                int fd = response.request.fd;
                int events = response.events;
                void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE
                ALOGD("%p ~ pollOnce - returning signalled identifier %d: "
                        "fd=%d, events=0x%x, data=%p",
                        this, ident, fd, events, data);
#endif
                if (outFd != nullptr) *outFd = fd;
                if (outEvents != nullptr) *outEvents = events;
                if (outData != nullptr) *outData = data;
                return ident;
            }
        }

        if (result != 0) {
#if DEBUG_POLL_AND_WAKE
            ALOGD("%p ~ pollOnce - returning result %d", this, result);
#endif
            if (outFd != nullptr) *outFd = 0;
            if (outEvents != nullptr) *outEvents = 0;
            if (outData != nullptr) *outData = nullptr;
            return result;
        }

        result = pollInner(timeoutMillis);
    }
}

int Looper::pollInner(int timeoutMillis) {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
#endif

    // Adjust the timeout based on when the next message is due.
    if (timeoutMillis != 0 && mNextMessageUptime != LLONG_MAX) {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        int messageTimeoutMillis = toMillisecondTimeoutDelay(now, mNextMessageUptime);
        if (messageTimeoutMillis >= 0
                && (timeoutMillis < 0 || messageTimeoutMillis < timeoutMillis)) {
            timeoutMillis = messageTimeoutMillis;
        }
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - next message in %" PRId64 "ns, adjusted timeout: timeoutMillis=%d",
                this, mNextMessageUptime - now, timeoutMillis);
#endif
    }

    // Poll.
    int result = POLL_WAKE;
    mResponses.clear();
    mResponseIndex = 0;

    // We are about to idle.
    mPolling = true;

    struct epoll_event eventItems[EPOLL_MAX_EVENTS];
    int eventCount = epoll_wait(mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);

    // No longer idling.
    mPolling = false;

    // Acquire lock.
    mLock.lock();

    // Rebuild epoll set if needed.
    if (mEpollRebuildRequired) {
        mEpollRebuildRequired = false;
        rebuildEpollLocked();
        goto Done;
    }

    // Check for poll error.
    if (eventCount < 0) {
        if (errno == EINTR) {
            goto Done;
        }
        ALOGW("Poll failed with an unexpected error: %s", strerror(errno));
        result = POLL_ERROR;
        goto Done;
    }

    // Check for poll timeout.
    if (eventCount == 0) {
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - timeout", this);
#endif
        result = POLL_TIMEOUT;
        goto Done;
    }

    // Handle all events.
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - handling events from %d fds", this, eventCount);
#endif

    for (int i = 0; i < eventCount; i++) {
        int fd = eventItems[i].data.fd;
        uint32_t epollEvents = eventItems[i].events;
        if (fd == mWakeEventFd) {
            if (epollEvents & EPOLLIN) {
                awoken();
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on wake event fd.", epollEvents);
            }
        } else {
            auto requestIndex = mRequests.find(fd);
            if (requestIndex != mRequests.end()) {
                int events = 0;
                if (epollEvents & EPOLLIN) events |= EVENT_INPUT;
                if (epollEvents & EPOLLOUT) events |= EVENT_OUTPUT;
                if (epollEvents & EPOLLERR) events |= EVENT_ERROR;
                if (epollEvents & EPOLLHUP) events |= EVENT_HANGUP;
                pushResponse(events, requestIndex->second);
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on fd %d that is "
                        "no longer registered.", epollEvents, fd);
            }
        }
    }
Done: ;

    // Invoke pending message callbacks.
    mNextMessageUptime = LLONG_MAX;
    while (mMessageEnvelopes.size() != 0) {
        nsecs_t now = getNowUs();
        const MessageEnvelope& messageEnvelope = mMessageEnvelopes.at(0);
        if (messageEnvelope.uptime <= now) {
            // Remove the envelope from the list.
            // We keep a strong reference to the handler until the call to handleMessage
            // finishes.  Then we drop it so that the handler can be deleted *before*
            // we reacquire our lock.
            { // obtain handler
                sptr<MessageHandler> handler = messageEnvelope.handler;
                Message message = messageEnvelope.message;
                mMessageEnvelopes.erase(mMessageEnvelopes.begin());
                mSendingMessage = true;
                mLock.unlock();

#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                ALOGD("%p ~ pollOnce - sending message: handler=%p, what=%d",
                        this, handler.get(), message.what);
#endif
                handler->handleMessage(message);
            } // release handler

            mLock.lock();
            mSendingMessage = false;
            result = POLL_CALLBACK;
        } else {
            // The last message left at the head of the queue determines the next wakeup time.
            mNextMessageUptime = messageEnvelope.uptime;
            break;
        }
    }

    // Release lock.
    mLock.unlock();

    // Invoke all response callbacks.
    for (size_t i = 0; i < mResponses.size(); i++) {
        Response& response = mResponses.at(i);
        if (response.request.ident == POLL_CALLBACK) {
            int fd = response.request.fd;
            int events = response.events;
            void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
            ALOGD("%p ~ pollOnce - invoking fd event callback %p: fd=%d, events=0x%x, data=%p",
                    this, response.request.callback.get(), fd, events, data);
#endif
            // Invoke the callback.  Note that the file descriptor may be closed by
            // the callback (and potentially even reused) before the function returns so
            // we need to be a little careful when removing the file descriptor afterwards.
            int callbackResult = response.request.callback->handleEvent(fd, events, data);
            if (callbackResult == 0) {
                removeFd(fd, response.request.seq);
            }

            // Clear the callback reference in the response structure promptly because we
            // will not clear the response vector itself until the next poll.
            response.request.callback.clear();
            result = POLL_CALLBACK;
        }
    }
    return result;
}

int Looper::pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    if (timeoutMillis <= 0) {
        int result;
        do {
            result = pollOnce(timeoutMillis, outFd, outEvents, outData);
        } while (result == POLL_CALLBACK);
        return result;
    } else {
        nsecs_t endTime = getNowUs()
                + (timeoutMillis * 1000000);

        for (;;) {
            int result = pollOnce(timeoutMillis, outFd, outEvents, outData);
            if (result != POLL_CALLBACK) {
                return result;
            }

            nsecs_t now = getNowUs();
            timeoutMillis = toMillisecondTimeoutDelay(now, endTime);
            if (timeoutMillis == 0) {
                return POLL_TIMEOUT;
            }
        }
    }
}

void Looper::wake() {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ wake", this);
#endif

    uint64_t inc = 1;
    //ssize_t nWrite = TEMP_FAILURE_RETRY(write(mWakeEventFd.get(), &inc, sizeof(uint64_t)));
    ssize_t nWrite = (write(mWakeEventFd, &inc, sizeof(uint64_t)));
    if (nWrite != sizeof(uint64_t)) {
        if (errno != EAGAIN) {
            ALOGE("Could not write wake signal to fd %d (returned %zd): %s",
                             mWakeEventFd, nWrite, strerror(errno));
        }
    }
}

void Looper::awoken() {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ awoken", this);
#endif

    uint64_t counter;
    (read(mWakeEventFd, &counter, sizeof(uint64_t)));
}

void Looper::pushResponse(int events, const Request& request) {
    Response response;
    response.events = events;
    response.request = request;
    mResponses.push_back(response);
}

int Looper::addFd(int fd, int ident, int events, Looper_callbackFunc callback, void* data) {
    return addFd(fd, ident, events, callback ? new SimpleLooperCallback(callback) : nullptr, data);
}

int Looper::addFd(int fd, int ident, int events, const sptr<LooperCallback>& callback, void* data) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ addFd - fd=%d, ident=%d, events=0x%x, callback=%p, data=%p", this, fd, ident,
            events, callback.get(), data);
#endif

    if (!callback.get()) {
        if (! mAllowNonCallbacks) {
            ALOGE("Invalid attempt to set NULL callback but not allowed for this looper.");
            return -1;
        }

        if (ident < 0) {
            ALOGE("Invalid attempt to set NULL callback with ident < 0.");
            return -1;
        }
    } else {
        ident = POLL_CALLBACK;
    }

    { // acquire lock
        std::lock_guard<std::mutex> _l(mLock);

        Request request;
        request.fd = fd;
        request.ident = ident;
        request.events = events;
        request.seq = mNextRequestSeq++;
        request.callback = callback;
        request.data = data;
        if (mNextRequestSeq == -1) mNextRequestSeq = 0; // reserve sequence number -1

        struct epoll_event eventItem;
        request.initEventItem(&eventItem);

        auto requestIndex = mRequests.find(fd);
        if (requestIndex == mRequests.end()) {
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem);
            if (epollResult < 0) {
                ALOGE("Error adding epoll events for fd %d: %s", fd, strerror(errno));
                return -1;
            }
            mRequests.emplace(fd, request);
        } else {
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &eventItem);
            if (epollResult < 0) {
                if (errno == ENOENT) {
                    // Tolerate ENOENT because it means that an older file descriptor was
                    // closed before its callback was unregistered and meanwhile a new
                    // file descriptor with the same number has been created and is now
                    // being registered for the first time.  This error may occur naturally
                    // when a callback has the side-effect of closing the file descriptor
                    // before returning and unregistering itself.  Callback sequence number
                    // checks further ensure that the race is benign.
                    //
                    // Unfortunately due to kernel limitations we need to rebuild the epoll
                    // set from scratch because it may contain an old file handle that we are
                    // now unable to remove since its file descriptor is no longer valid.
                    // No such problem would have occurred if we were using the poll system
                    // call instead, but that approach carries others disadvantages.
#if DEBUG_CALLBACKS
                    ALOGD("%p ~ addFd - EPOLL_CTL_MOD failed due to file descriptor "
                            "being recycled, falling back on EPOLL_CTL_ADD: %s",
                            this, strerror(errno));
#endif
                    epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem);
                    if (epollResult < 0) {
                        ALOGE("Error modifying or adding epoll events for fd %d: %s",
                                fd, strerror(errno));
                        return -1;
                    }
                    scheduleEpollRebuildLocked();
                } else {
                    ALOGE("Error modifying epoll events for fd %d: %s", fd, strerror(errno));
                    return -1;
                }
            }
            mRequests.insert_or_assign(requestIndex, fd, request);
        }
    } // release lock
    return 1;
}

int Looper::removeFd(int fd) {
    return removeFd(fd, -1);
}

int Looper::removeFd(int fd, int seq) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeFd - fd=%d, seq=%d", this, fd, seq);
#endif

    { // acquire lock
        std::lock_guard<std::mutex> _l(mLock);
        auto requestIndex = mRequests.find(fd);
        if (requestIndex == mRequests.end()) {
            return 0;
        }

        // Check the sequence number if one was given.
        if (seq != -1 && requestIndex->second.seq != seq) {
#if DEBUG_CALLBACKS
            ALOGD("%p ~ removeFd - sequence number mismatch, oldSeq=%d",
                    this, mRequests.valueAt(requestIndex).seq);
#endif
            return 0;
        }

        // Always remove the FD from the request map even if an error occurs while
        // updating the epoll set so that we avoid accidentally leaking callbacks.
        mRequests.erase(requestIndex);

        int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
        if (epollResult < 0) {
            if (seq != -1 && (errno == EBADF || errno == ENOENT)) {
                // Tolerate EBADF or ENOENT when the sequence number is known because it
                // means that the file descriptor was closed before its callback was
                // unregistered.  This error may occur naturally when a callback has the
                // side-effect of closing the file descriptor before returning and
                // unregistering itself.
                //
                // Unfortunately due to kernel limitations we need to rebuild the epoll
                // set from scratch because it may contain an old file handle that we are
                // now unable to remove since its file descriptor is no longer valid.
                // No such problem would have occurred if we were using the poll system
                // call instead, but that approach carries others disadvantages.
#if DEBUG_CALLBACKS
                ALOGD("%p ~ removeFd - EPOLL_CTL_DEL failed due to file descriptor "
                        "being closed: %s", this, strerror(errno));
#endif
                scheduleEpollRebuildLocked();
            } else {
                // Some other error occurred.  This is really weird because it means
                // our list of callbacks got out of sync with the epoll set somehow.
                // We defensively rebuild the epoll set to avoid getting spurious
                // notifications with nowhere to go.
                ALOGE("Error removing epoll events for fd %d: %s", fd, strerror(errno));
                scheduleEpollRebuildLocked();
                return -1;
            }
        }
    } // release lock
    return 1;
}

void Looper::sendMessage(const sptr<MessageHandler>& handler, const Message& message) {
    nsecs_t now = getNowUs();
    sendMessageAtTime(now, handler, message);
}

void Looper::sendMessageDelayed(nsecs_t uptimeDelay, const sptr<MessageHandler>& handler,
        const Message& message) {
    nsecs_t now = getNowUs();
    sendMessageAtTime(now + uptimeDelay, handler, message);
}

void Looper::sendMessageAtTime(nsecs_t uptime, const sptr<MessageHandler>& handler,
        const Message& message) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ sendMessageAtTime - uptime=%" PRId64 ", handler=%p, what=%d",
            this, uptime, handler.get(), message.what);
#endif

    size_t i = 0;
    { // acquire lock
        std::lock_guard<std::mutex> _l(mLock);

        size_t messageCount = mMessageEnvelopes.size();
        auto it = mMessageEnvelopes.begin();
        while (i < messageCount && uptime >= mMessageEnvelopes.at(i).uptime) {
            i += 1;
            ++it;
        }

        MessageEnvelope messageEnvelope(uptime, handler, message);
        mMessageEnvelopes.insert(it, messageEnvelope);

        // Optimization: If the Looper is currently sending a message, then we can skip
        // the call to wake() because the next thing the Looper will do after processing
        // messages is to decide when the next wakeup time should be.  In fact, it does
        // not even matter whether this code is running on the Looper thread.
        if (mSendingMessage) {
            return;
        }
    } // release lock

    // Wake the poll loop only when we enqueue a new message at the head.
    if (i == 0) {
        wake();
    }
}

void Looper::removeMessages(const sptr<MessageHandler>& handler) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p", this, handler.get());
#endif

    { // acquire lock
        std::lock_guard<std::mutex> _l(mLock);

        for (size_t i = mMessageEnvelopes.size(); i != 0; ) {
            const MessageEnvelope& messageEnvelope = mMessageEnvelopes.at(--i);
            if (messageEnvelope.handler == handler) {
                mMessageEnvelopes.at(i);
            }
        }
    } // release lock
}

void Looper::removeMessages(const sptr<MessageHandler>& handler, int what) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p, what=%d", this, handler.get(), what);
#endif

    { // acquire lock
        std::lock_guard<std::mutex> _l(mLock);

        for (auto i = mMessageEnvelopes.end(); i != mMessageEnvelopes.begin(); ) {
            const MessageEnvelope& messageEnvelope = *--i;
            if (messageEnvelope.handler == handler
                    && messageEnvelope.message.what == what) {
                mMessageEnvelopes.erase(i);
            }
        }
    } // release lock
}

bool Looper::isPolling() const {
    return mPolling;
}

void Looper::Request::initEventItem(struct epoll_event* eventItem) const {
    int epollEvents = 0;
    if (events & EVENT_INPUT) epollEvents |= EPOLLIN;
    if (events & EVENT_OUTPUT) epollEvents |= EPOLLOUT;

    memset(eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
    eventItem->events = epollEvents;
    eventItem->data.fd = fd;
}

MessageHandler::~MessageHandler() { }

LooperCallback::~LooperCallback() { }

} // namespace aml_mp
