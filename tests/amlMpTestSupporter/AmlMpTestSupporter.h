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
#include <string>
#include <thread>
#include <vector>
#include <Aml_MP/Aml_MP.h>

struct ANativeWindow;

namespace aml_mp {
class Source;
class Parser;
class TestModule;
class Playback;
struct ProgramInfo;

struct NativeUI;
struct CommandProcessor;

class AmlMpTestSupporter : public AmlMpRefBase
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

    AmlMpTestSupporter();
    ~AmlMpTestSupporter();
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int setDataSource(const std::string& url);
    int prepare();
    int startPlay(PlayMode playMode = START_ALL_STOP_ALL);
    int stop();
    bool hasVideo() const;

    int installSignalHandler();
    int fetchAndProcessCommands();

private:
    bool processCommand(const std::vector<std::string>& args);
    void signalQuit();

    int startPlayback(Aml_MP_DemuxId demuxId, Aml_MP_InputSourceType sourceType);

    std::string mUrl;
    sptr<Source> mSource;
    sptr<Parser> mParser;
    sptr<ProgramInfo> mProgramInfo;

    sptr<TestModule> mTestModule;

    sptr<Playback> mPlayback;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    sptr<NativeUI> mNativeUI;
    std::thread mSignalHandleThread;
    sptr<CommandProcessor> mCommandProcessor;

    bool mQuitPending = false;

    PlayMode mPlayMode = START_ALL_STOP_ALL;

    AmlMpTestSupporter(const AmlMpTestSupporter&) = delete;
    AmlMpTestSupporter& operator=(const AmlMpTestSupporter&) = delete;
};


}






#endif
