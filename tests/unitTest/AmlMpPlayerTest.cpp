/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerTest"
#include <utils/AmlMpLog.h>
#include <gtest/gtest.h>
#include "TestUrlList.h"
#include "../amlMpTestSupporter/TestModule.h"
#include <AmlMpTestSupporter.h>
#include <pthread.h>
#include <getopt.h>

using namespace aml_mp;

static const char* mName = LOG_TAG;

static const int kWaitFirstVfameTimeOutMs = 2 * 1000ll;
static const int kWaitPlayingErrorsMs = 5 * 1000ll;

///////////////////////////////////////////////////////////////////////////////
struct AmlMpPlayerTest : public testing::Test
{
    void SetUp() override {
        MLOGI("SetUp");

    }

    void TearDown() override {
    }

protected:
    void runTest(const std::string& url);

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

protected:
    sptr<AmlMpTestSupporter> mpTestSupporter;

    std::mutex mLock;
    std::condition_variable mCond;
    bool mFirstVFrameDisplayed = false;
    bool mPlayingHaveErrors = false;

    AmlMpTestSupporter::PlayMode mPlayMode = AmlMpTestSupporter::START_ALL_STOP_ALL;
};

void AmlMpPlayerTest::runTest(const std::string& url)
{
    startPlaying(url);
    if (mpTestSupporter->hasVideo()) {
        ASSERT_TRUE(waitFirstVFrameEvent()) << defaultFailureMessage(url);
    }
    ASSERT_FALSE(waitPlayingErrors());
    stopPlaying();
}

void AmlMpPlayerTest::startPlaying(const std::string& url)
{
    MLOGI("url:%s", url.c_str());

    if (mpTestSupporter == nullptr) {
        mpTestSupporter = new AmlMpTestSupporter;
        mpTestSupporter->registerEventCallback([](void* userData, Aml_MP_PlayerEventType event, int64_t param) {
            AmlMpPlayerTest* self = (AmlMpPlayerTest*)userData;
            return self->eventCallback(event, param);
        }, this);
    }

    mpTestSupporter->setDataSource(url);
    int ret = mpTestSupporter->prepare();
    ASSERT_EQ(ret, 0) << defaultFailureMessage(url);

    ret = mpTestSupporter->startPlay();
    ASSERT_EQ(ret, 0) << defaultFailureMessage(url);

    if (ret != 0) {
        mPlayingHaveErrors = true;
    }
}

void AmlMpPlayerTest::stopPlaying()
{
    mpTestSupporter->stop();

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
        break;
    }

    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(AmlMpPlayerTest, StartPlayTest1)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_ALL_STOP_ALL;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest2)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_ALL_STOP_SEPARATELY;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest3)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_ALL;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest4)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_SEPARATELY;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest5)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_SEPARATELY_V2;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest6)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_AUDIO_START_VIDEO;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, StartPlayTest7)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();
    mPlayMode = AmlMpTestSupporter::START_VIDEO_START_AUDIO;

    runTest(url);
}

TEST_F(AmlMpPlayerTest, ClearLastFrame)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    std::string url = *urls.begin();

    startPlaying(url);
    if (mpTestSupporter->hasVideo()) {
        ASSERT_TRUE(waitFirstVFrameEvent()) << defaultFailureMessage(url);
    }
    ASSERT_FALSE(waitPlayingErrors());

    sptr<TestModule> testModule = mpTestSupporter->getPlayback();
    AML_MP_PLAYER player = testModule->getCommandHandle();

    int blackOut = 1;
    Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &blackOut);
    ALOGI("cleared last frame!\n");

    stopPlaying();
    ASSERT_FALSE(waitPlayingErrors());
}

TEST_F(AmlMpPlayerTest, ZappingTest)
{
    std::list<std::string> urls;
    if (!TestUrlList::instance().getUrls(test_info_->test_case_name(), &urls)) {
        return;
    }

    for (auto& url : urls) {
        runTest(url);
    }
}

///////////////////////////////////////////////////////////////////////////////
static void usage()
{
    printf("Usage: AmlMpPlayerTest <test_source_dir>\n"
            "   try --help for more details.\n"
            );
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        usage();
        return 0;
    }

    std::string testpath;

    for (size_t i = 1; i < argc;) {
        if (argv[i][0] != '-') {
            testpath = argv[i];

            for (int j = i; j < argc-1; ++j) {
                argv[j] = argv[j+1];
            }
            argc--;
        } else {
            ++i;
        }
    }

    testing::InitGoogleTest(&argc, argv);

    if (!testpath.empty()) {
        printf("testpath:%s, argc:%d\n", testpath.c_str(), argc);
        TestUrlList::instance().initSourceDir(testpath);
    }

    return RUN_ALL_TESTS();
}
