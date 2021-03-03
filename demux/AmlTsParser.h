/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_TS_PARSER_H_
#define _AML_TS_PARSER_H_

#include <utils/AmlMpRefBase.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <Aml_MP/Common.h>
#include <demux/AmlDemuxBase.h>

namespace aml_mp {
class AmlDemuxBase;

typedef enum {
    SCRAMBLE_ALGO_CSA,
    SCRAMBLE_ALGO_AES,
    SCRAMBLE_ALGO_INVALID,
    SCRAMBLE_ALGO_NONE
} SCRAMBLE_ALGO_t;

typedef enum {
    SCRAMBLE_MODE_ECB,
    SCRAMBLE_MODE_CBC,
    SCRAMBLE_MODE_INVALID
} SCRAMBLE_MODE_t;

typedef enum {
    SCRAMBLE_ALIGNMENT_LEFT,
    SCRAMBLE_ALIGNMENT_RIGHT,
    SCRAMBLE_ALIGNMENT_INVALID
} SCRAMBLE_ALIGNMENT_t;

typedef enum {
    TYPE_AUDIO,
    TYPE_VIDEO,
    TYPE_SUBTITLE,
    TYPE_INVALID
} STREAM_TYPE_t;

typedef struct SCRAMBLE_INFO_s {
    SCRAMBLE_ALGO_t             algo;
    SCRAMBLE_MODE_t             mode;
    SCRAMBLE_ALIGNMENT_t        alignment;
    uint8_t                     has_iv_value;
    uint8_t                     iv_value_data[16];
} SCRAMBLE_INFO_t;

struct ProgramInfo : public AmlMpRefBase
{
    int programNumber               = -1;
    int pmtPid                      = AML_MP_INVALID_PID;
    int caSystemId                  = -1;
    int emmPid                      = AML_MP_INVALID_PID;
    bool scrambled                  = false;
    SCRAMBLE_INFO_t scrambleInfo{};
    int serviceIndex                = 0;
    int serviceNum                  = 0;
    int ecmPid[3]{AML_MP_INVALID_PID};
#define ECM_INDEX_AUDIO 0
#define ECM_INDEX_VIDEO 1
#define ECM_INDEX_SUB   2
    Aml_MP_CodecID audioCodec    = AML_MP_CODEC_UNKNOWN;
    Aml_MP_CodecID videoCodec    = AML_MP_CODEC_UNKNOWN;
    Aml_MP_CodecID subtitleCodec = AML_MP_CODEC_UNKNOWN;
    int audioPid                    = AML_MP_INVALID_PID;
    int videoPid                    = AML_MP_INVALID_PID;
    int subtitlePid                 = AML_MP_INVALID_PID;
    int compositionPageId;
    int ancillaryPageId;

public:
    bool isComplete() const {
        bool hasEcmPid = false;
        for (size_t i = 0; i < 3; ++i) {
            if (ecmPid[i] != AML_MP_INVALID_PID) {
                hasEcmPid = true;
                break;
            }
        }

        bool hasEmmPid = emmPid != AML_MP_INVALID_PID;

        return (audioPid != AML_MP_INVALID_PID ||
               videoPid != AML_MP_INVALID_PID) &&
               (!scrambled || (hasEcmPid || hasEmmPid));
    }
};

class Parser : public AmlMpRefBase
{
public:
    Parser(Aml_MP_DemuxId demuxId, int programNumber, bool isHardwareSource);
    ~Parser();
    int open();
    int close();
    int wait();
    void signalQuit();
    sptr<ProgramInfo> getProgramInfo() const;
    Aml_MP_DemuxId getDemuxId() const {
        return mDemuxId;
    }
    virtual int writeData(const uint8_t* buffer, size_t size);

private:
    struct Section {
        Section(const uint8_t* buffer_, size_t size_)
        : buffer(buffer_)
        , size(size_)
        {
        }

        const uint8_t* data() const {
            return buffer + bufferIndex;
        }

        size_t dataSize() const {
            return size;
        }

        bool empty() const {
            return size == 0;
        }

        const uint8_t* advance(int offset) {
            bufferIndex += offset;
            size -= offset;

            return buffer + bufferIndex;
        }

    private:
        const uint8_t* buffer = nullptr;
        int size = 0;
        int bufferIndex = 0;
    };

    struct SectionFilterContext : public AmlMpRefBase {
    public:
        SectionFilterContext(int pid)
        : mPid(pid)
        {

        }

        ~SectionFilterContext() {
        }

        int mPid;
        void* channel;
        void* filter;
    };

    struct PATSection {
        int programNumber;
        int pmtPid;
    };

    static const int kMaxStreamsInPMT = 5;
    struct PMTStream {
        int streamPid;
        int streamType;
        int ecmPid={AML_MP_INVALID_PID}; //es info
        int descriptorTags[10];
        int descriptorCount = 0;

        int compositionPageId{}; //dvb subtitle
        int ancillaryPageId{}; //dvb subtitle
    };

    struct PMTSection {
        int pcrPid = 0x1FFF;

        bool scrambled = false;
        int caSystemId = -1;
        int ecmPid = 0x1FFF;
        int scrambleAlgorithm = -1;
        SCRAMBLE_INFO_t scrambleInfo{};

        int streamCount = 0;
        PMTStream streams[kMaxStreamsInPMT];
    };

    struct CATSection {
        int caSystemId = -1;
        int emmPid= 0x1FFF;
    };

    int addSectionFilter(int pid, Aml_MP_Demux_SectionFilterCb cb);
    int removeSectionFilter(int pid);
    void clearAllSectionFilters();
    void notifyParseDone_l();

    static int patCb(size_t size, const uint8_t* data, void* userData);
    static int pmtCb(size_t size, const uint8_t* data, void* userData);
    static int catCb(size_t size, const uint8_t* data, void* userData);

    void onPatParsed(const std::vector<PATSection>& results);
    void onPmtParsed(const PMTSection& results);
    void onCatParsed(const CATSection& results);

private:
    int mProgramNumber = -1;
    int mProgramMapPid = -1;
    bool mIsHardwareSource = false;
    Aml_MP_DemuxId mDemuxId = AML_MP_HW_DEMUX_ID_0;

    sptr<AmlDemuxBase> mDemux;
    sptr<ProgramInfo> mProgramInfo;

    mutable std::mutex mLock;
    std::condition_variable mCond;
    bool mParseDone = false;
    std::map<int, sptr<SectionFilterContext>> mSectionFilters;  //pid, section

    std::atomic_bool mRequestQuit{false};

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
};

}







#endif
