//
// Copyright 2010 The Android Open Source Project
//
//
#define LOG_TAG "AmlMpPlayerRoster"

#include <utils/AmlMpUtils.h>
#include "AmlMpPlayerRoster.h"
#include "AmlMpLog.h"

static const char* mName = LOG_TAG;

namespace aml_mp {

AmlMpPlayerRoster* AmlMpPlayerRoster::sAmlPlayerRoster = nullptr;

AmlMpPlayerRoster::AmlMpPlayerRoster()
: mPlayers{nullptr}
{

}

AmlMpPlayerRoster::~AmlMpPlayerRoster()
{

}

AmlMpPlayerRoster& AmlMpPlayerRoster::instance()
{
    static std::once_flag s_onceFlag;

    if (sAmlPlayerRoster == nullptr) {
        std::call_once(s_onceFlag, [] {
            sAmlPlayerRoster = new AmlMpPlayerRoster();
        });
    }

    return *sAmlPlayerRoster;
}

int AmlMpPlayerRoster::registerPlayer(void* player)
{
    if (player == nullptr) {
        return -1;
    }

    std::lock_guard<std::mutex> _l(mLock);
    for (size_t i = 0; i < kPlayerInstanceMax; ++i) {
        if (mPlayers[i] == nullptr) {
            mPlayers[i] = player;
            (void)++mPlayerNum;

            //MLOGI("register player id:%d(%p)", i, player);
            return i;
        }
    }

    return -1;
}

void AmlMpPlayerRoster::unregisterPlayer(int id)
{
    std::lock_guard<std::mutex> _l(mLock);

    CHECK(mPlayers[id]);
    mPlayers[id] = nullptr;
    (void)--mPlayerNum;
}

void AmlMpPlayerRoster::signalAmTsPlayerId(int id)
{
    std::lock_guard<std::mutex> _l(mLock);
    mAmtsPlayerId = id;
}

bool AmlMpPlayerRoster::isAmTsPlayerExist() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return mAmtsPlayerId != -1;
}

} // namespace aml_mp
