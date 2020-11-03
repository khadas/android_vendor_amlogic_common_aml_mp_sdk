/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerTest"
#include <utils/Log.h>
#include <gtest/gtest.h>
#include "TestUrlList.h"
#include <AmlMpTestSupporter.h>
#include <pthread.h>
#include <getopt.h>

using namespace aml_mp;

static const int kWaitFirstVfameTimeOutMs = 2 * 1000ll;
static const int kWaitPlayingErrorsMs = 5 * 1000ll;

struct AmlMpPlayerTest : public testing::Test
{
    void SetUp() override {
        ALOGI("SetUp");

    }

    void TearDown() override {
    }

protected:
    void startPlaying(const std::string& url);
    void stopPlaying();
    bool waitFirstVFrameEvent(int timeoutMs = kWaitFirstVfameTimeOutMs);
    bool waitPlayingErrors(int msec= kWaitPlayingErrorsMs);

    void eventCallback(Aml_MP_PlayerEventType event, int64_t param);

    std::string defaultFailureMessage(const std::string& url) const {
        std::stringstream ss;
        ss << "playing " << url << " failed!";
        return ss.str();
    }

    sp<AmlMpTestSupporter> mpPlayer;

protected:
    std::mutex mLock;
    std::condition_variable mCond;
    bool mFirstVFrameDisplayed = false;
    bool mPlayingHaveErrors = false;
};

void AmlMpPlayerTest::startPlaying(const std::string& url)
{
    ALOGI("url:%s", url.c_str());

    mpPlayer = new AmlMpTestSupporter;
    mpPlayer->registerEventCallback([](void* userData, Aml_MP_PlayerEventType event, int64_t param) {
        AmlMpPlayerTest* self = (AmlMpPlayerTest*)userData;
        return self->eventCallback(event, param);
    }, this);
    mpPlayer->setDataSource(url);
    int ret = mpPlayer->prepare();
    ASSERT_EQ(ret, 0) << defaultFailureMessage(url);

    mpPlayer->startPlay();
}

void AmlMpPlayerTest::stopPlaying()
{
    mpPlayer->stop();

    mpPlayer.clear();
    mFirstVFrameDisplayed = false;
    mPlayingHaveErrors = false;
}

bool AmlMpPlayerTest::waitFirstVFrameEvent(int timeoutMs)
{
    std::unique_lock<std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this](){return mFirstVFrameDisplayed;});
}

bool AmlMpPlayerTest::waitPlayingErrors(int timeoutMs)
{
    std::unique_lock<std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this](){return mPlayingHaveErrors;});
}

void AmlMpPlayerTest::eventCallback(Aml_MP_PlayerEventType event, int64_t param)
{
    switch (event) {
    case AML_MP_PLAYER_EVENT_FIRST_FRAME:
    {
        std::unique_lock<std::mutex> _l(mLock);
        mFirstVFrameDisplayed = true;
        mCond.notify_all();
    }
    break;
    }
}


TEST_F(AmlMpPlayerTest, BasicPlaying)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    for (auto& url : urls) {
        startPlaying(url);
        if (mpPlayer->hasVideo()) {
            ASSERT_TRUE(waitFirstVFrameEvent()) << defaultFailureMessage(url);
        }
        ASSERT_FALSE(waitPlayingErrors());
        stopPlaying();
    }
}

int main(int argc, char *argv[])
{
    static const struct option longopts[] = {
        {"dir",      required_argument,       nullptr, 'dir '},
        {nullptr,       no_argument,        nullptr, 0},
    };

    int opt;
    std::string sourceDir;
    while ((opt = getopt_long(argc, argv, "", longopts, nullptr)) != -1) {
        switch (opt) {
        case 'dir ':
            sourceDir = optarg;
            break;
        }
    }

    if (TestUrlList::instance().loadConfig(sourceDir) < 0) {
        TestUrlList::instance().genConfig(sourceDir);
    }

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}


