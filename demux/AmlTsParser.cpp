/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_Parser"
#include <utils/Log.h>
#include "AmlTsParser.h"
#include <media/stagefright/foundation/ADebug.h>
#include <vector>
#include <utils/AmlMpUtils.h>

static const char* mName = LOG_TAG;

namespace aml_mp {
///////////////////////////////////////////////////////////////////////////////
struct StreamType {
    int esStreamType;
    Aml_MP_StreamType mpStreamType;
    Aml_MP_CodecID codecId;
};

static const StreamType g_streamTypes[] = {
    {0x01, AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_MPEG12},
    {0x02, AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_MPEG12},
    {0x03, AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_MP3},
    {0x04, AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_MP3},
    {0x0f, AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_AAC},
    {0x10, AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_MPEG4},
    {0x11, AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_LATM},
    {0x1b, AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_H264},
    {0x24, AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_HEVC},
    {0x81, AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_AC3},
    {0x82, AML_MP_STREAM_TYPE_SUBTITLE, AML_MP_SUBTITLE_CODEC_SCTE27},
    {0, AML_MP_STREAM_TYPE_UNKNOWN, AML_MP_CODEC_UNKNOWN},
};

static const StreamType g_descTypes[] = {
    { 0x6a, AML_MP_STREAM_TYPE_AUDIO,    AML_MP_AUDIO_CODEC_AC3          }, /* AC-3 descriptor */
    { 0x7a, AML_MP_STREAM_TYPE_AUDIO,    AML_MP_AUDIO_CODEC_EAC3         }, /* E-AC-3 descriptor */
    { 0x7b, AML_MP_STREAM_TYPE_AUDIO,    AML_MP_AUDIO_CODEC_DTS          },
    { 0x56, AML_MP_STREAM_TYPE_SUBTITLE, AML_MP_SUBTITLE_CODEC_TELETEXT },
    { 0x59, AML_MP_STREAM_TYPE_SUBTITLE, AML_MP_SUBTITLE_CODEC_DVB }, /* subtitling descriptor */
    { 0, AML_MP_STREAM_TYPE_UNKNOWN, AML_MP_CODEC_UNKNOWN},
};

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
static const StreamType g_identifierTypes[] = {
    { MKTAG('A', 'C', '-', '3'), AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_AC3},
    { MKTAG('E', 'A', 'C', '3'), AML_MP_STREAM_TYPE_AUDIO, AML_MP_AUDIO_CODEC_EAC3},
    { MKTAG('H', 'E', 'V', 'C'), AML_MP_STREAM_TYPE_VIDEO, AML_MP_VIDEO_CODEC_HEVC},
    {0, AML_MP_STREAM_TYPE_UNKNOWN, AML_MP_CODEC_UNKNOWN},
};

static const struct StreamType* getStreamTypeInfo(int esStreamType, const StreamType* table = g_streamTypes)
{
    const struct StreamType* result = nullptr;
    const StreamType *s = table;

    for (; s->esStreamType; s++) {
        if (esStreamType == s->esStreamType) {
            result = s;
            break;
        }
    }

    return result;
}

void ProgramInfo::debugLog() const
{
    MLOGI("ProgramInfo: programNumber=%d, pid=%d", programNumber, pmtPid);
    for (auto it : videoStreams) {
        MLOGI("ProgramInfo: videoStream: pid:%d, codecId:%d", it.pid, it.codecId);
    }
    for (auto it : audioStreams) {
        MLOGI("ProgramInfo: audioStream: pid:%d, codecId:%d", it.pid, it.codecId);
    }
    for (auto it : subtitleStreams) {
        MLOGI("ProgramInfo: subtitleStream: pid:%d, codecId:%d", it.pid, it.codecId);
    }
    if(scrambled) {
        MLOGI("ProgramInfo: is scrambled, caSystemId:0x%04X, ecmPid:0x%04X, privateDataLength:%d", caSystemId, ecmPid[0], privateDataLength);
        std::string privateDataHex;
        char hex[3];
        for(int i = 0; i < privateDataLength; i++){
            snprintf(hex, sizeof(hex), "%02X", privateData[i]);
            privateDataHex.append(hex);
            privateDataHex.append(" ");
        }
        MLOGI("ProgramInfo: privateData: %s", privateDataHex.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
Parser::Parser(Aml_MP_DemuxId demuxId, bool isHardwareSource, bool isHardwareDemux)
: mDemuxId(demuxId)
, mIsHardwareSource(isHardwareSource)
, mIsHardwareDemux(isHardwareDemux)
, mProgramInfo(new ProgramInfo)
{

}

Parser::~Parser()
{
    close();
}

int Parser::open()
{
    bool isHardwareDemux = mIsHardwareDemux;
    if (mIsHardwareSource) {
        isHardwareDemux = true;
    }

    mDemux = AmlDemuxBase::create(isHardwareDemux);
    if (mDemux == nullptr) {
        MLOGE("create demux failed!");
        return -1;
    }

    int ret = mDemux->open(mIsHardwareSource, mDemuxId);
    if (ret < 0) {
        MLOGE("demux open failed!");
        return -1;
    }

    ret = mDemux->start();
    if (ret < 0) {
        MLOGE("demux start failed!");
        return -1;
    }

    addSectionFilter(0, patCb);
    addSectionFilter(1, catCb);

    return 0;
}

void Parser::setProgram(int programNumber)
{
    std::lock_guard<std::mutex> _l(mLock);
    mProgramNumber = programNumber;
}

void Parser::setProgram(int vPid, int aPid)
{
    std::lock_guard<std::mutex> _l(mLock);
    mVPid = vPid;
    mAPid = aPid;
}

bool Parser::hasProgramHint_l() const
{
    return mProgramNumber >= 0 || mVPid != AML_MP_INVALID_PID || mAPid != AML_MP_INVALID_PID;
}

void Parser::setEventCallback(const std::function<ProgramEventCallback>& cb)
{
    std::lock_guard<std::mutex> _l(mLock);
    mCb = cb;
}

int Parser::close()
{
    clearAllSectionFilters();
    sptr<AmlDemuxBase> dmxTemp;
    {
        std::lock_guard<std::mutex> _l(mLock);
        if (mDemux != nullptr) {
            dmxTemp = mDemux;
            mDemux.clear();
        }
    }
    if (dmxTemp != nullptr) {
        dmxTemp->stop();
        dmxTemp->close();
    }

    MLOGI("%s:%d, end", __FUNCTION__, __LINE__);
    return 0;
}

int Parser::wait()
{
    std::unique_lock<std::mutex> l(mLock);
    bool ret = mCond.wait_for(l, std::chrono::seconds(3), [this] {
        return mParseDone || mRequestQuit;
    });

    return ret ? 0 : -1;
}

void Parser::signalQuit()
{
    mRequestQuit = true;

    std::unique_lock<std::mutex> l(mLock);
    mCond.notify_all();
}

sptr<ProgramInfo> Parser::getProgramInfo() const
{
    std::lock_guard<std::mutex> _l(mLock);
    sptr<ProgramInfo> info = mProgramInfo;

    if (info && info->isComplete()) {
        return info;
    }

    return nullptr;
}

int Parser::writeData(const uint8_t* buffer, size_t size)
{
    int wlen = -1;
    //MLOGV("writeData:%p, size:%d", buffer, size);
    std::lock_guard<std::mutex> _l(mLock);
    sptr<AmlDemuxBase> demux = mDemux;
    if(demux){
        wlen = demux->feedTs(buffer, size);
    }

    return wlen;
}

int Parser::patCb(int pid, size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    MLOGI("pat cb, size:%d", size);
    Section section(data, size);
    const uint8_t* p = section.data();
    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    CHECK(section_syntax_indicator == 1);
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    MLOGI("section_length = %d, size:%d", section_length, size);

    p = section.advance(3);
    //int transport_stream_id = p[0]<<8 | p[1];
    p = section.advance(5);
    int numPrograms = (section.dataSize() - 4)/4;

    std::vector<PATSection> results;
    for (int i = 0; i < numPrograms; ++i) {
        int program_number = p[0]<<8 | p[1];

        if (program_number != 0) {
            int program_map_PID = (p[2]&0x01F) << 8 | p[3];
			MLOGI("programNumber:%d, program_map_PID = %d\n", program_number, program_map_PID);
            results.push_back({program_number, program_map_PID});
        } else {
            //int network_PID = (p[2]&0x1F) << 8 | p[3];
        }

        p = section.advance(4);
    }

    if (parser) {
        parser->onPatParsed(results);
    }

    return 0;
}

int Parser::pmtCb(int pid, size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    MLOGI("pmt cb, pid:%d, size:%d", pid, size);
    Section section(data, size);
    const uint8_t* p = section.data();
    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    if (section_syntax_indicator != 1) {
        MLOGE("pmt section_syntax_indicator CHECK failed!");
        return -1;
    }
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    MLOGI("section_length = %d, size:%d", section_length, size);

    PMTSection results;
    results.pmtPid = pid;

    p = section.advance(3);
    int programNumber = p[0]<<8 | p[1];
    results.programNumber = programNumber;
    //check version_number and current_next_indicator to skip same pmt
    results.version_number = p[2] & 0x3E;
    results.current_next_indicator = p[2] & 0x01;
    //MLOGI("pmt cb, version_number:%d, current_next_indicator:%d", results.version_number, results.current_next_indicator);
    if (results.current_next_indicator == 0) {
        //MLOGI("just skip this pmt, because the current_next_indicator is zero");
        return 0;
    }
    if (parser) {
        // check version_number is same
        auto it = parser->mPidPmtMap.find(pid);
        if (it != parser->mPidPmtMap.end()) {
            if (results.version_number == it->second.version_number) {
                //MLOGI("just skip this pmt, because the version_number(%d) is same", results.version_number);
                return 0;
            }
        }
    }

    p = section.advance(5);
    int pcr_pid = (p[0]&0x1F)<<8 | p[1];
    results.pcrPid = pcr_pid;
    int program_info_length = (p[2]&0x0F) << 8 | p[3];
    CHECK(program_info_length < 1024);
    p = section.advance(4);

    results.privateDataLength = 0;
    if (program_info_length > 0) {
        int descriptorsRemaining = program_info_length;
        const uint8_t* p2 = p;
        int count = 0;
        while (descriptorsRemaining >= 2) {
            int descriptor_tag = p2[0];
            int descriptor_length = p2[1];
            switch (descriptor_tag) {
            case 0x09:
            {
                int ca_system_id = p2[2]<<8 | p2[3];
                int ecm_pid = (p2[4]&0x1F)<<8 | p2[5];
                MLOGI("ca_system_id:%#x, ecm_pid:%#x, count:%d, descriptor_length:%d\n", ca_system_id, ecm_pid, ++count, descriptor_length);

                results.scrambled = true;
                results.caSystemId = ca_system_id;
                results.ecmPid = ecm_pid;
                results.privateDataLength = descriptor_length - 4;
                memcpy(results.privateData, &p2[6], results.privateDataLength);
            }
            break;

            case 0x65:
            {
                int scrambleAlgorithm = p[2];
                MLOGI("scrambleAlgorithm:%d", scrambleAlgorithm);

                results.scrambleAlgorithm = scrambleAlgorithm;
            }
            break;

            default:
                MLOGI("descriptor_tag:%#x", descriptor_tag);
                break;
            }

            p2 += 2 + descriptor_length;
            descriptorsRemaining -= 2 + descriptor_length;
        }
    }

    //skip program info
    p = section.advance(program_info_length);
    int infoBytesRemaining = section.dataSize() - 4;

    while (infoBytesRemaining >= 5) {
        int stream_type = p[0];
        int elementary_pid = (p[1]&0x1F) << 8 | p[2];
        int es_info_length = (p[3]&0x0F) << 8 | p[4];
        p = section.advance(5);
        infoBytesRemaining -= 5;

        PMTStream esStream;
        esStream.streamType = stream_type;
        esStream.streamPid = elementary_pid;

        if (es_info_length > 0) {
            int descriptorsRemaining = es_info_length;
            const uint8_t* p2 = p;
            int count = 0;
            while (descriptorsRemaining >= 2) {
                int descriptor_tag = p2[0];
                int descriptor_length = p2[1];

                esStream.descriptorTags[esStream.descriptorCount++] = descriptor_tag;

                switch (descriptor_tag) {
                case 0x09:
                {
                    int ca_system_id = p2[2]<<8 | p2[3];
                    int ecm_pid = (p2[4]&0x1F)<<8 | p2[4];
                    MLOGI("streamType:%#x, pid:%d, ca_system_id:%#x, ecm_pid:%#x, count:%d, descriptor_length:%d\n",
                            stream_type, elementary_pid, ca_system_id, ecm_pid, ++count, descriptor_length);
                    if (descriptor_length > 4) {
                        int has_iv = p2[6] & 0x1;
                        int aligned = ( p2[6] & 0x4 ) >> 2;
                        int scramble_mode = ( p2[6] & 0x8 ) >> 3;
                        int algorithm = ( p2[6] & 0xE0 ) >> 5;
                        MLOGI("%s, @@this is ca_private_data, data:%#x, iv:%d, aligned:%d, mode:%d, algo:%d",
                                  __FUNCTION__, p2[6], has_iv, aligned, scramble_mode, algorithm );
                        if ( algorithm == 1 ) {
                            results.scrambleInfo.algo = SCRAMBLE_ALGO_AES;
                            if ( scramble_mode == 0 ) {
                                results.scrambleInfo.mode = SCRAMBLE_MODE_ECB;
                            } else {
                                results.scrambleInfo.mode = SCRAMBLE_MODE_CBC;
                            }
                            results.scrambleInfo.alignment = (SCRAMBLE_ALIGNMENT_t)aligned;
                            results.scrambleInfo.has_iv_value = has_iv;
                            if ( has_iv && descriptor_length > 4 + 16 ) {
                                memcpy(results.scrambleInfo.iv_value_data, &p2[7], 16);
                            }
                        }
                    }

                    results.scrambled = true;
                    results.caSystemId = ca_system_id;

                    esStream.ecmPid = ecm_pid;
                }
                break;

                case 0x59:
                {
                    int language_count = descriptor_length / 8;
                    if (language_count > 0) {
                        esStream.compositionPageId = p2[6] << 8 | p2[7];
                        esStream.ancillaryPageId = p2[7] << 8 | p2[9];
                        MLOGI("compositionPageId:%#x, ancillaryPageId:%#x", esStream.compositionPageId, esStream.ancillaryPageId);
                    }

                }
                break;

                case 0x05:
                {
                    int32_t formatIdentifier = MKTAG(p2[2], p2[3], p2[4], p2[5]);
                    MLOGI("formatIdentifier:%#x, %x %x %x %x", formatIdentifier, p2[2], p2[3], p2[4], p2[5]);
                    esStream.descriptorTags[esStream.descriptorCount-1] = formatIdentifier;
                }
                break;

                default:
                    MLOGI("unhandled stream descriptor_tag:%#x, length:%d", descriptor_tag, descriptor_length);
                    break;
                }

                p2 += 2 + descriptor_length;
                descriptorsRemaining -= 2 + descriptor_length;
            }

            p = section.advance(es_info_length);
            infoBytesRemaining -= es_info_length;
        }

        results.streamCount++;
        results.streams.push_back(esStream);
        MLOGE("programNumber:%d, stream pid:%d, type:%#x\n", programNumber, elementary_pid, stream_type);

    }

    if (parser) {
        parser->onPmtParsed(results);
    }

    return 0;
}


int Parser::catCb(int pid, size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    MLOGI("cat cb, size:%d", size);
    Section section(data, size);
    const uint8_t* p = section.data();

    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    CHECK(section_syntax_indicator == 1);
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    MLOGI("section_length = %d, size:%d", section_length, size);

    p = section.advance(3);

    //skip section header
    p = section.advance(5);

    CATSection results;
    results.catPid = pid;

    int descriptorsRemaining = section.dataSize() - 4;
    int count = 0;
    while (descriptorsRemaining >= 2) {
        int descriptor_tag = p[0];
        int descriptor_length = p[1];
        switch (descriptor_tag) {
        case 0x09:
        {
            int ca_system_id = p[2]<<8 | p[3];
            int emm_pid = (p[4]&0x1F)<<8 | p[5];
            MLOGI("ca_system_id:%#x, emm_pid:%#x, count:%d, descriptor_length:%d\n", ca_system_id, emm_pid, ++count, descriptor_length);

            results.caSystemId = ca_system_id;
            results.emmPid = emm_pid;
        }
        break;

        default:
            MLOGI("CAT descriptor_tag:%#x", descriptor_tag);
            break;
        }

        p = section.advance(2 + descriptor_length);
        descriptorsRemaining -= 2 + descriptor_length;
    }

    if (descriptorsRemaining != 0) {
        MLOGW("descriptorsRemaining = %d\n", descriptorsRemaining);
    }

    if (parser) {
        parser->onCatParsed(results);
    }

    return 0;
}

int Parser::ecmCb(int pid, size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    MLOGI("ecm cb, pid:0x%04X, size:%d", pid, size);
    ECMSection results;
    results.ecmPid = pid;
    results.size = size;
    results.data = new uint8_t[size];
    memcpy(results.data, data, size);

    if (parser) {
        parser->onEcmParsed(results);
    }

    delete[] results.data;

    return 0;
}

void Parser::onPatParsed(const std::vector<PATSection>& results)
{
    if (results.empty()) {
        return;
    }
    removeSectionFilter(0);

    int programCount = 0;
    mPidProgramMap.clear();
    for (auto& p : results) {
        programCount++;
        mPidProgramMap.insert_or_assign(p.pmtPid, p.programNumber);
        addSectionFilter(p.pmtPid, pmtCb);

        std::lock_guard<std::mutex> _l(mLock);
        if (!hasProgramHint_l()) {
            mProgramNumber = p.programNumber;
            MLOGI("set default program number:%d, pmt pid:%d", mProgramNumber, p.pmtPid);
        }
    }

    if (programCount == 0) {
        MLOGI("no valid program found!");
        std::lock_guard<std::mutex> _l(mLock);
        notifyParseDone_l();
    }
}

void Parser::onPmtParsed(const PMTSection& results)
{
    if (results.streamCount == 0) {
        return;
    }

    bool isNewEcm = false;
    bool isProgramSeleted = false;
    bool isNewPmt = false;
    bool isPidChanged = false;
    Aml_MP_PlayerEventPidChangeInfo pidChangeInfo;
    {
        std::lock_guard<std::mutex> _l(mLock);
        if (!hasProgramHint_l()) {
            isProgramSeleted = true;
        } else if (mProgramNumber != -1 && results.programNumber == mProgramNumber) {
            isProgramSeleted = true;
        } else if (mVPid != AML_MP_INVALID_PID || mAPid != AML_MP_INVALID_PID) {
            bool containsAudio = false, containsVideo = false;
            for (PMTStream stream : results.streams) {
                if (mVPid == stream.streamPid) {
                    containsVideo = true;
                } else if (mAPid == stream.streamPid) {
                    containsAudio = true;
                }
            }
            if ((mVPid == AML_MP_INVALID_PID || containsVideo) && (mAPid == AML_MP_INVALID_PID || containsAudio)) {
                isProgramSeleted = true;
            }
        }
        //check is newPmt or updatePmt
        auto it = mPidPmtMap.find(results.pmtPid);
        if (it == mPidPmtMap.end()) {
            // is new pmt
            isNewPmt = true;
            mPidPmtMap.emplace(results.pmtPid, results);
        } else {
            // is pmt updated
            if (isProgramSeleted) {
                // check pid change
                isPidChanged = checkPidChange(it->second, results, &pidChangeInfo);
            }
            mPidPmtMap.erase(results.pmtPid);
            mPidPmtMap.emplace(results.pmtPid, results);
        }
        //check is newEcm
        if (isProgramSeleted && results.scrambled && mEcmPidSet.find(results.ecmPid) == mEcmPidSet.end()) {
            isNewEcm = true;
            mEcmPidSet.emplace(results.ecmPid);
        }
    }

    if (!isProgramSeleted) {
        MLOGI("not program selected");
        return;
    }

    if (isNewEcm && results.scrambled) {
        // filter ecmData
        addSectionFilter(results.ecmPid, ecmCb, false);
    }

    sptr<ProgramInfo> programInfo = mProgramInfo;
    programInfo->programNumber = results.programNumber;
    programInfo->pmtPid = results.pmtPid;
    programInfo->caSystemId = results.caSystemId;
    programInfo->scrambled = results.scrambled;
    programInfo->scrambleInfo = results.scrambleInfo;
    programInfo->serviceIndex = 0;
    programInfo->serviceNum = 0;
    programInfo->ecmPid[ECM_INDEX_AUDIO] = results.ecmPid;
    programInfo->ecmPid[ECM_INDEX_VIDEO] = results.ecmPid;
    programInfo->ecmPid[ECM_INDEX_SUB] = results.ecmPid;
    programInfo->privateDataLength = results.privateDataLength;
    memcpy(programInfo->privateData, results.privateData, results.privateDataLength);


    const struct StreamType* typeInfo;
    for (auto it : results.streams) {
        PMTStream* stream = &it;
        typeInfo = getStreamTypeInfo(stream->streamType);
        if (typeInfo == nullptr) {
            for (size_t j = 0; j < stream->descriptorCount; ++j) {
                typeInfo = getStreamTypeInfo(stream->descriptorTags[j], g_descTypes);
                if (typeInfo != nullptr) {
                    MLOGI("stream pid:%d, found tag:%#x", stream->streamPid, stream->descriptorTags[j]);
                    break;
                }

                typeInfo = getStreamTypeInfo(stream->descriptorTags[j], g_identifierTypes);
                if (typeInfo != nullptr) {
                    MLOGI("identified stream pid:%d, %.4s", stream->streamPid, (char*)&stream->descriptorTags[j]);
                    break;
                }
            }
        }

        if (typeInfo == nullptr) {
            continue;
        }

        switch (typeInfo->mpStreamType) {
        case AML_MP_STREAM_TYPE_AUDIO:
        {
            if (programInfo->audioPid == AML_MP_INVALID_PID) {
                programInfo->audioPid = stream->streamPid;
                programInfo->audioCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_AUDIO] = stream->ecmPid;
                }
            }
            StreamInfo streamInfo;
            streamInfo.type = TYPE_AUDIO;
            streamInfo.pid = stream->streamPid;
            streamInfo.codecId = typeInfo->codecId;
            programInfo->audioStreams.push_back(streamInfo);
            MLOGI("audio pid:%d(%#x), codec:%s", streamInfo.pid, streamInfo.pid, mpCodecId2Str(streamInfo.codecId));
            break;
        }

        case AML_MP_STREAM_TYPE_VIDEO:
        {
            if (programInfo->videoPid == AML_MP_INVALID_PID) {
                programInfo->videoPid = stream->streamPid;
                programInfo->videoCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_VIDEO] = stream->ecmPid;
                }
            }
            StreamInfo streamInfo;
            streamInfo.type = TYPE_VIDEO;
            streamInfo.pid = stream->streamPid;
            streamInfo.codecId = typeInfo->codecId;
            programInfo->videoStreams.push_back(streamInfo);
            MLOGI("video pid:%d(%#x), codec:%s", streamInfo.pid, streamInfo.pid, mpCodecId2Str(streamInfo.codecId));
            break;
        }

        case AML_MP_STREAM_TYPE_SUBTITLE:
        {
            if (programInfo->subtitlePid == AML_MP_INVALID_PID) {
                programInfo->subtitlePid = stream->streamPid;
                programInfo->subtitleCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_SUB] = stream->ecmPid;
                }
                if (programInfo->subtitleCodec == AML_MP_SUBTITLE_CODEC_DVB) {
                    programInfo->compositionPageId = stream->compositionPageId;
                    programInfo->ancillaryPageId = stream->ancillaryPageId;
                }
            }
            StreamInfo streamInfo;
            streamInfo.type = TYPE_SUBTITLE;
            streamInfo.pid = stream->streamPid;
            streamInfo.codecId = typeInfo->codecId;
            if (streamInfo.codecId == AML_MP_SUBTITLE_CODEC_DVB) {
                streamInfo.compositionPageId = stream->compositionPageId;
                streamInfo.ancillaryPageId = stream->ancillaryPageId;
            }
            programInfo->subtitleStreams.push_back(streamInfo);
            MLOGI("subtitle pid:%d(%#x), codec:%s", streamInfo.pid, streamInfo.pid, mpCodecId2Str(streamInfo.codecId));
            break;
        }

        case AML_MP_STREAM_TYPE_AD:
            break;

        default:
            break;
        }
    }

    if (isNewPmt) {
        if (mCb && mProgramInfo->isComplete()) {
            mCb(ProgramEventType::EVENT_PROGRAM_PARSED, mProgramInfo->pmtPid, mProgramInfo->programNumber, mProgramInfo.get());
        }
    } else {
        if (mCb) {
            mCb(ProgramEventType::EVENT_AV_PID_CHANGED, results.pmtPid, results.programNumber, (void *)&pidChangeInfo);
        }
    }

    if (mProgramInfo->isComplete()) {
        std::lock_guard<std::mutex> _l(mLock);
        notifyParseDone_l();
    }
}

void Parser::onCatParsed(const CATSection& results)
{
    mProgramInfo->scrambled = true;
    mProgramInfo->caSystemId = results.caSystemId;
    mProgramInfo->emmPid = results.emmPid;

    if (mCb && mProgramInfo->isComplete()) {
        mCb(ProgramEventType::EVENT_PROGRAM_PARSED, mProgramInfo->pmtPid, mProgramInfo->programNumber, mProgramInfo.get());
    }

    if (mProgramInfo->isComplete()) {
        std::lock_guard<std::mutex> _l(mLock);
        notifyParseDone_l();
    }
}

void Parser::onEcmParsed(const ECMSection& results){
    if (mCb) {
        mCb(ProgramEventType::EVENT_ECM_DATA_PARSED, results.ecmPid, results.size, results.data);
    }
}

bool Parser::checkPidChange(PMTSection oldPmt, PMTSection newPmt, Aml_MP_PlayerEventPidChangeInfo* pidChangeInfo)
{
    std::set<int> oldPidSet;
    std::set<int> newPidSet;
    std::set<int> unChangedPidSet;
    for (PMTStream pmtStream : oldPmt.streams) {
        oldPidSet.insert(pmtStream.streamPid);
    }
    for (PMTStream pmtStream : newPmt.streams) {
        newPidSet.insert(pmtStream.streamPid);
    }
    for (int pid : oldPidSet) {
        if (newPidSet.find(pid) != newPidSet.end()) {
            unChangedPidSet.insert(pid);
        }
    }
    for (int pid : unChangedPidSet) {
        oldPidSet.erase(pid);
        newPidSet.erase(pid);
    }
    bool isPidChange =false;
    if (!oldPidSet.empty()) {
        pidChangeInfo->oldStreamPid = *oldPidSet.begin();
        isPidChange = true;
    }
    if (!newPidSet.empty()) {
        pidChangeInfo->newStreamPid = *newPidSet.begin();
        isPidChange = true;
    }
    pidChangeInfo->programPid = oldPmt.pmtPid;
    pidChangeInfo->programNumber = oldPmt.programNumber;
    return isPidChange;
}

///////////////////////////////////////////////////////////////////////////////
int Parser::addSectionFilter(int pid, Aml_MP_Demux_SectionFilterCb cb, bool checkCRC)
{
    int ret = 0;

    sptr<SectionFilterContext> context = new SectionFilterContext(pid);
    if (context == nullptr) {
        return -1;
    }

    context->channel = mDemux->createChannel(pid, checkCRC);
    ret = mDemux->openChannel(context->channel);
    if (ret < 0) {
        MLOGE("open channel pid:%d failed!", pid);
        return ret;
    }

    ret = mDemux->openChannel(context->channel);
    if (ret < 0) {
        MLOGE("open channel pid:%d failed!", pid);
        return ret;
    }

    context->filter = mDemux->createFilter(cb, this);
    if (ret < 0) {
        MLOGE("create filter for pid:%d failed!", pid);
        return ret;
    }

    ret = mDemux->attachFilter(context->filter, context->channel);
    if (ret < 0) {
        MLOGE("attach filter for pid:%d failed!", pid);
        return ret;
    }

    {
        std::lock_guard<std::mutex> _l(mLock);
        mSectionFilters.emplace(pid, context);
    }

    return ret;
}

int Parser::removeSectionFilter(int pid)
{
    std::lock_guard<std::mutex> _l(mLock);
    sptr<SectionFilterContext> context;

    auto it = mSectionFilters.find(pid);
    if (it == mSectionFilters.end()) {
        return 0;
    }
    context = it->second;

    if (context->filter != AML_MP_INVALID_HANDLE) {
        mDemux->detachFilter(context->filter, context->channel);
        mDemux->destroyFilter(context->filter);
        context->filter = AML_MP_INVALID_HANDLE;
    }

    if (context->channel != AML_MP_INVALID_HANDLE) {
        mDemux->closeChannel(context->channel);
        mDemux->destroyChannel(context->channel);
        context->channel = AML_MP_INVALID_HANDLE;
    }

    int ret = mSectionFilters.erase(context->mPid);
    MLOGI("pid:%d, %d sections removed!", pid, ret);

    return 0;
}

void Parser::clearAllSectionFilters()
{
    std::lock_guard<std::mutex> _l(mLock);
    for (auto p : mSectionFilters) {
        sptr<SectionFilterContext> context = p.second;
        if (context->filter != AML_MP_INVALID_HANDLE) {
            mDemux->detachFilter(context->filter, context->channel);
            mDemux->destroyFilter(context->filter);
            context->filter = AML_MP_INVALID_HANDLE;
        }

        if (context->channel != AML_MP_INVALID_HANDLE) {
            mDemux->closeChannel(context->channel);
            mDemux->destroyChannel(context->channel);
            context->channel = AML_MP_INVALID_HANDLE;
        }
    }

    mSectionFilters.clear();
}

void Parser::notifyParseDone_l()
{
    mParseDone = true;
    mCond.notify_all();
}


}
