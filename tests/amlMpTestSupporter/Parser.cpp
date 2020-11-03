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
#include "Parser.h"
#include <media/stagefright/foundation/ADebug.h>
#include <vector>
#include <utils/AmlMpUtils.h>

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

///////////////////////////////////////////////////////////////////////////////
Parser::Parser(int programNumber, bool isHardwareSource)
: mProgramNumber(programNumber)
, mIsHardwareSource(isHardwareSource)
, mProgramInfo(new ProgramInfo)
{

}

Parser::~Parser()
{
    close();
}

int Parser::open()
{
    bool isHardwareDemux = false;
    if (mIsHardwareSource) {
        isHardwareDemux = true;
        mDemuxId = AML_MP_HW_DEMUX_ID_0;
    }

    mDemux = AmlDemuxBase::create(isHardwareDemux);
    if (mDemux == nullptr) {
        ALOGE("create demux failed!");
        return -1;
    }

    int ret = mDemux->open(mIsHardwareSource, mDemuxId);
    if (ret < 0) {
        ALOGE("demux open failed!");
        return -1;
    }

    ret = mDemux->start();
    if (ret < 0) {
        ALOGE("demux start failed!");
        return -1;
    }

    addSectionFilter(0, patCb);
    addSectionFilter(1, catCb);

    return 0;
}

int Parser::close()
{
    clearAllSectionFilters();

    if (mDemux != nullptr) {
        mDemux->stop();
        mDemux->close();
        mDemux.clear();
    }

    return 0;
}

int Parser::wait()
{
    std::unique_lock<std::mutex> l(mLock);
    mCond.wait(l, [this] {
        return mParseDone || mRequestQuit;
    });

    return 0;
}

void Parser::signalQuit()
{
    mRequestQuit = true;

    std::unique_lock<std::mutex> l(mLock);
    mCond.notify_all();
}

sp<ProgramInfo> Parser::getProgramInfo() const
{
    std::lock_guard<std::mutex> _l(mLock);
    sp<ProgramInfo> info = mProgramInfo;

    if (info && info->isComplete()) {
        return info;
    }

    return nullptr;
}

int Parser::writeData(const uint8_t* buffer, size_t size)
{
    int wlen = 0;
    //ALOGV("writeData:%p, size:%d", buffer, size);

    sp<AmlDemuxBase> demux = mDemux;
    wlen = demux->feedTs(buffer, size);

    int ret = ISourceReceiver::writeData(buffer, size);

    return ret;
}

int Parser::patCb(size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    ALOGI("pat cb, size:%d", size);
    Section section(data, size);
    const uint8_t* p = section.data();
    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    CHECK(section_syntax_indicator == 1);
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    ALOGI("section_length = %d, size:%d", section_length, size);

    p = section.advance(3);
    //int transport_stream_id = p[0]<<8 | p[1];
    p = section.advance(5);
    int numPrograms = (section.dataSize() - 4)/4;

    std::vector<PATSection> results;
    for (int i = 0; i < numPrograms; ++i) {
        int program_number = p[0]<<4 | p[1];

        if (program_number != 0) {
            int program_map_PID = (p[2]&0x01F) << 8 | p[3];
			ALOGI("programNumber:%d, program_map_PID = %d\n", program_number, program_map_PID);
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

int Parser::pmtCb(size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    ALOGI("pmt cb, size:%d", size);
    Section section(data, size);
    const uint8_t* p = section.data();
    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    CHECK(section_syntax_indicator == 1);
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    ALOGI("section_length = %d, size:%d", section_length, size);

    p = section.advance(3);
    int programNumber = p[0]<<8 | p[1];
    p = section.advance(5);

    PMTSection results;
    PMTStream* esStream = nullptr;

    int pcr_pid = (p[0]&0x1F)<<8 | p[1];
    results.pcrPid = pcr_pid;
    int program_info_length = (p[2]&0x0F) << 8 | p[3];
    CHECK(program_info_length < 1024);
    p = section.advance(4);

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
                ALOGI("ca_system_id:%#x, ecm_pid:%#x, count:%d, descriptor_length:%d\n", ca_system_id, ecm_pid, ++count, descriptor_length);

                results.scrambled = true;
                results.caSystemId = ca_system_id;
                results.ecmPid = ecm_pid;
            }
            break;

            case 0x65:
            {
                int scrambleAlgorithm = p[2];
                ALOGI("scrambleAlgorithm:%d", scrambleAlgorithm);

                results.scrambleAlgorithm = scrambleAlgorithm;
            }
            break;

            default:
                ALOGI("descriptor_tag:%#x", descriptor_tag);
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

        if (results.streamCount >= kMaxStreamsInPMT) {
            ALOGI("exceed max stream count:%d", results.streamCount);
            continue;
        }
        esStream = &results.streams[results.streamCount++];
        esStream->streamType = stream_type;
        esStream->streamPid = elementary_pid;

        if (es_info_length > 0) {
            int descriptorsRemaining = es_info_length;
            const uint8_t* p2 = p;
            int count = 0;
            while (descriptorsRemaining >= 2) {
                int descriptor_tag = p2[0];
                int descriptor_length = p2[1];

                esStream->descriptorTags[esStream->descriptorCount++] = descriptor_tag;

                switch (descriptor_tag) {
                case 0x09:
                {
                    int ca_system_id = p2[2]<<8 | p2[3];
                    int ecm_pid = (p2[4]&0x1F)<<8 | p2[4];
                    ALOGI("streamType:%#x, pid:%d, ca_system_id:%#x, ecm_pid:%#x, count:%d, descriptor_length:%d\n",
                            stream_type, elementary_pid, ca_system_id, ecm_pid, ++count, descriptor_length);
                    if (descriptor_length > 4) {
                        int has_iv = p2[6] & 0x1;
                        int aligned = ( p2[6] & 0x4 ) >> 2;
                        int scramble_mode = ( p2[6] & 0x8 ) >> 3;
                        int algorithm = ( p2[6] & 0xE0 ) >> 5;
                        ALOGI("%s, @@this is ca_private_data, data:%#x, iv:%d, aligned:%d, mode:%d, algo:%d",
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

                    esStream->ecmPid = ecm_pid;
                }
                break;

                case 0x59:
                {
                    int language_count = descriptor_length / 8;
                    if (language_count > 0) {
                        esStream->compositionPageId = p2[6] << 8 | p2[7];
                        esStream->ancillaryPageId = p2[7] << 8 | p2[9];
                        ALOGI("compositionPageId:%#x, ancillaryPageId:%#x", esStream->compositionPageId, esStream->ancillaryPageId);
                    }

                }
                break;

                case 0x05:
                {
                    int32_t formatIdentifier = MKTAG(p2[2], p2[3], p2[4], p2[5]);
                    ALOGI("formatIdentifier:%#x, %x %x %x %x", formatIdentifier, p2[2], p2[3], p2[4], p2[5]);
                    esStream->descriptorTags[esStream->descriptorCount-1] = formatIdentifier;
                }
                break;

                default:
                    ALOGI("unhandled stream descriptor_tag:%#x, length:%d", descriptor_tag, descriptor_length);
                    break;
                }

                p2 += 2 + descriptor_length;
                descriptorsRemaining -= 2 + descriptor_length;
            }

            p = section.advance(es_info_length);
            infoBytesRemaining -= es_info_length;
        }

        ALOGE("programNumber:%d, stream pid:%d, type:%#x\n", programNumber, elementary_pid, stream_type);

    }

    if (parser) {
        parser->onPmtParsed(results);
    }

    return 0;
}


int Parser::catCb(size_t size, const uint8_t* data, void* userData)
{
    Parser* parser = (Parser*)userData;

    ALOGI("cat cb, size:%d", size);
    Section section(data, size);
    const uint8_t* p = section.data();

    //int table_id = p[0];
    int section_syntax_indicator = (p[1]&0x80) >> 7;
    CHECK(section_syntax_indicator == 1);
    int section_length = (p[1] & 0x0F) << 4 | p[2];
    CHECK(section_length <= 4093);
    ALOGI("section_length = %d, size:%d", section_length, size);

    p = section.advance(3);

    //skip section header
    p = section.advance(5);

    CATSection results;

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
            ALOGI("ca_system_id:%#x, emm_pid:%#x, count:%d, descriptor_length:%d\n", ca_system_id, emm_pid, ++count, descriptor_length);

            results.caSystemId = ca_system_id;
            results.emmPid = emm_pid;
        }
        break;

        default:
            ALOGI("CAT descriptor_tag:%#x", descriptor_tag);
            break;
        }

        p = section.advance(2 + descriptor_length);
        descriptorsRemaining -= 2 + descriptor_length;
    }

    if (descriptorsRemaining != 0) {
        ALOGW("descriptorsRemaining = %d\n", descriptorsRemaining);
    }

    if (parser) {
        parser->onCatParsed(results);
    }

    return 0;
}

void Parser::onPatParsed(const std::vector<PATSection>& results)
{
    if (results.empty()) {
        return;
    }

    for (auto& p : results) {
        if (mProgramNumber < 0 || mProgramNumber == p.programNumber) {
            mProgramNumber = p.programNumber;
            mProgramMapPid = p.pmtPid;

            ALOGI("set mProgramNumber:%d, mProgramMapPid:%d", mProgramNumber, mProgramMapPid);

            break;
        }
    }

    if (mProgramNumber > 0) {
        removeSectionFilter(0);

        addSectionFilter(mProgramMapPid, pmtCb);
    } else {
        ALOGI("no valid program found!");
        std::lock_guard<std::mutex> _l(mLock);
        notifyParseDone_l();
    }
}

void Parser::onPmtParsed(const PMTSection& results)
{
    if (results.streamCount == 0) {
        return;
    }

    std::lock_guard<std::mutex> _l(mLock);

    sp<ProgramInfo> programInfo = mProgramInfo;
    programInfo->programNumber = mProgramNumber;
    programInfo->pmtPid = mProgramMapPid;
    programInfo->caSystemId = results.caSystemId;
    programInfo->scrambled = results.scrambled;
    programInfo->scrambleInfo = results.scrambleInfo;
    programInfo->serviceIndex = 0;
    programInfo->serviceNum = 0;
    programInfo->ecmPid[ECM_INDEX_AUDIO] = results.ecmPid;
    programInfo->ecmPid[ECM_INDEX_VIDEO] = results.ecmPid;
    programInfo->ecmPid[ECM_INDEX_SUB] = results.ecmPid;


    const struct StreamType* typeInfo;
    for (int i = 0; i < results.streamCount; ++i) {
        const PMTStream *stream = results.streams + i;
        typeInfo = getStreamTypeInfo(stream->streamType);
        if (typeInfo == nullptr) {
            for (size_t j = 0; j < stream->descriptorCount; ++j) {
                typeInfo = getStreamTypeInfo(stream->descriptorTags[j], g_descTypes);
                if (typeInfo != nullptr) {
                    ALOGI("stream pid:%d, found tag:%#x", stream->streamPid, stream->descriptorTags[j]);
                    break;
                }

                typeInfo = getStreamTypeInfo(stream->descriptorTags[j], g_identifierTypes);
                if (typeInfo != nullptr) {
                    ALOGI("identified stream pid:%d, %.4s", stream->streamPid, (char*)&stream->descriptorTags[j]);
                    break;
                }
            }
        }

        if (typeInfo == nullptr) {
            continue;
        }

        switch (typeInfo->mpStreamType) {
        case AML_MP_STREAM_TYPE_AUDIO:
            if (programInfo->audioPid == AML_MP_INVALID_PID) {
                programInfo->audioPid = stream->streamPid;
                programInfo->audioCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_AUDIO] = stream->ecmPid;
                }
                ALOGI("audio pid:%d(%#x), codec:%s", programInfo->audioPid, programInfo->audioPid, mpCodecId2Str(programInfo->audioCodec));
            }
            break;

        case AML_MP_STREAM_TYPE_VIDEO:
            if (programInfo->videoPid == AML_MP_INVALID_PID) {
                programInfo->videoPid = stream->streamPid;
                programInfo->videoCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_VIDEO] = stream->ecmPid;
                }
                ALOGI("video pid:%d(%#x), codec:%s", programInfo->videoPid, programInfo->videoPid, mpCodecId2Str(programInfo->videoCodec));
            }
            break;

        case AML_MP_STREAM_TYPE_SUBTITLE:
            if (programInfo->subtitlePid == AML_MP_INVALID_PID) {
                programInfo->subtitlePid = stream->streamPid;
                programInfo->subtitleCodec = typeInfo->codecId;
                if (stream->ecmPid != AML_MP_INVALID_PID) {
                    programInfo->ecmPid[ECM_INDEX_SUB] = stream->ecmPid;
                }
                ALOGI("subtitle pid:%d(%#x), codec:%s", programInfo->subtitlePid, programInfo->subtitlePid, mpCodecId2Str(programInfo->subtitleCodec));
                if (programInfo->subtitleCodec == AML_MP_SUBTITLE_CODEC_DVB) {
                    programInfo->compositionPageId = stream->compositionPageId;
                    programInfo->ancillaryPageId = stream->ancillaryPageId;
                }
            }
            break;

        case AML_MP_STREAM_TYPE_AD:
            break;

        default:
            break;
        }
    }

    if (programInfo->isComplete()) {
        notifyParseDone_l();
    }
}

void Parser::onCatParsed(const CATSection& results)
{
    std::lock_guard<std::mutex> _l(mLock);

    mProgramInfo->scrambled = true;
    mProgramInfo->caSystemId = results.caSystemId;
    mProgramInfo->emmPid = results.emmPid;

    if (mProgramInfo->isComplete()) {
        notifyParseDone_l();
    }
}

///////////////////////////////////////////////////////////////////////////////
int Parser::addSectionFilter(int pid, Aml_MP_Demux_SectionFilterCb cb)
{
    int ret = 0;

    sp<SectionFilterContext> context = new SectionFilterContext(pid);
    if (context == nullptr) {
        return -1;
    }

    context->channel = mDemux->createChannel(pid);
    ret = mDemux->openChannel(context->channel);
    if (ret < 0) {
        ALOGE("open channel pid:%d failed!", pid);
        return ret;
    }

    ret = mDemux->openChannel(context->channel);
    if (ret < 0) {
        ALOGE("open channel pid:%d failed!", pid);
        return ret;
    }

    context->filter = mDemux->createFilter(cb, this);
    if (ret < 0) {
        ALOGE("create filter for pid:%d failed!", pid);
        return ret;
    }

    ret = mDemux->attachFilter(context->filter, context->channel);
    if (ret < 0) {
        ALOGE("attach filter for pid:%d failed!", pid);
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
    sp<SectionFilterContext> context;

    {
        std::lock_guard<std::mutex> _l(mLock);
        auto it = mSectionFilters.find(pid);
        context = it->second;
    }

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

    {
        std::lock_guard<std::mutex> _l(mLock);
        int ret = mSectionFilters.erase(context->mPid);
        ALOGI("pid:%d, %d sections removed!", pid, ret);
    }

    return 0;
}

void Parser::clearAllSectionFilters()
{
    std::lock_guard<std::mutex> _l(mLock);
    for (auto p : mSectionFilters) {
        sp<SectionFilterContext> context = p.second;
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
