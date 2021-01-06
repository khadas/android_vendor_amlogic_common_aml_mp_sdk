/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "Parser.h"
#include "TestModule.h"
#include <Aml_MP/Aml_MP.h>

namespace aml_mp {
class DVRRecord : public TestModule, public ISourceReceiver
{
public:
    DVRRecord(bool cryptoMode, Aml_MP_DemuxId demuxId, const sptr<ProgramInfo>& programInfo);
    ~DVRRecord();

    int start();
    int stop();
    void signalQuit();

protected:
    virtual const Command* getCommandTable() const override;
    virtual void* getCommandHandle() const override;

private:
    int initDVREncryptRecord(Aml_MP_DVRRecorderEncryptParams& encryptParams);
    int uninitDVREncryptRecord();

private:
    const bool mCryptoMode;
    const Aml_MP_DemuxId mDemuxId;
    const sptr<ProgramInfo> mProgramInfo;
    AML_MP_DVRRECORDER mRecorder = AML_MP_INVALID_HANDLE;
    Aml_MP_DVRRecorderEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    AML_MP_CASSESSION mCasSession = nullptr;
    AML_MP_SECMEM mSecMem = nullptr;

private:
    DVRRecord(const DVRRecord&) = delete;
    DVRRecord& operator=(const DVRRecord&) = delete;
};

}
