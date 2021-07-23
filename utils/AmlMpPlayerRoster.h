/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_PLAYER_ROSTER_H_
#define _AML_MP_PLAYER_ROSTER_H_
#include <mutex>
#include <condition_variable>

namespace aml_mp {

struct AmlMpPlayerRoster
{
    static constexpr int kPlayerInstanceMax = 9;

    static AmlMpPlayerRoster& instance();
    int registerPlayer(void* player);
    void unregisterPlayer(int id);
    void signalAmTsPlayerId(int id);
    bool isAmTsPlayerExist() const;

private:
    static AmlMpPlayerRoster* sAmlPlayerRoster;
    mutable std::mutex mLock;
    std::condition_variable mcond;
    void* mPlayers[kPlayerInstanceMax];
    int mPlayerNum = 0;
    int mAmtsPlayerId = -1;

    AmlMpPlayerRoster();
    ~AmlMpPlayerRoster();

    AmlMpPlayerRoster(const AmlMpPlayerRoster&) = delete;
    AmlMpPlayerRoster& operator= (const AmlMpPlayerRoster&) = delete;
};

}
#endif

