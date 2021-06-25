/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_TEST_SUPPORTER_H_
#define _AML_MP_TEST_SUPPORTER_H_

#include <utils/AmlMpRefBase.h>
#include <utils/AmlMpHandle.h>
#include <string>
#include <thread>
#include <vector>
#include <Aml_MP/Aml_MP.h>

#ifdef ANDROID
struct ANativeWindow;
#endif

namespace aml_mp {
class Source;
class Parser;
class TestModule;
class ParserReceiver;
class Playback;
class DVRRecord;
class DVRPlayback;
struct ProgramInfo;

#ifdef ANDROID
struct NativeUI;
#endif

struct CommandProcessor;

class AmlMpTestSupporter : public AmlMpHandle
{
public:
    enum PlayMode {
        START_ALL_STOP_ALL,
        START_ALL_STOP_SEPARATELY,
        START_SEPARATELY_STOP_ALL,
        START_SEPARATELY_STOP_SEPARATELY,
        START_SEPARATELY_STOP_SEPARATELY_V2,
        START_AUDIO_START_VIDEO,
        START_VIDEO_START_AUDIO,
        PLAY_MODE_MAX,
    };

    struct DisplayParam {
        int x                   = 0;
        int y                   = 0;
        int width               = -1;
        int height              = -1;
        int zorder              = 0;
        int videoMode           = 0;
        ANativeWindow* aNativeWindow = nullptr;
    };

    AmlMpTestSupporter();
    ~AmlMpTestSupporter();
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int setDataSource(const std::string& url);
    int prepare(bool cryptoMode = false);
    void setDisplayParam(const DisplayParam& displayParam);
    int startPlay(PlayMode playMode = START_ALL_STOP_ALL);
    int startRecord();
    int stop();

    bool hasVideo() const;

    int installSignalHandler();
    int fetchAndProcessCommands();
    int setOsdBlank(int blank);

    sptr<TestModule> getPlayback() const;

private:
    int startDVRPlayback();
    bool processCommand(const std::vector<std::string>& args);
    void signalQuit();

    std::string mUrl;
    sptr<Source> mSource;
    sptr<Parser> mParser;
    sptr<ParserReceiver> mParserReceiver;
    sptr<ProgramInfo> mProgramInfo;

    sptr<TestModule> mTestModule;

    sptr<Playback> mPlayback;
    sptr<DVRRecord> mRecorder;
    sptr<DVRPlayback> mDVRPlayback;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    bool mIsDVRPlayback = false;
    bool mCryptoMode = false;
    #ifdef ANDROID
    sptr<NativeUI> mNativeUI;
    #endif
    std::thread mSignalHandleThread;
    sptr<CommandProcessor> mCommandProcessor;

    bool mQuitPending = false;

    PlayMode mPlayMode = START_ALL_STOP_ALL;

    DisplayParam mDisplayParam;

    AmlMpTestSupporter(const AmlMpTestSupporter&) = delete;
    AmlMpTestSupporter& operator=(const AmlMpTestSupporter&) = delete;
};


}






#endif
