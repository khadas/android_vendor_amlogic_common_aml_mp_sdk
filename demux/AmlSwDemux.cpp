/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_AmlSwDemux"
#include <utils/Log.h>
#include "AmlSwDemux.h"
#include <utils/AmlMpUtils.h>
#include <map>
#include <utils/AmlMpEventLooper.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpMessage.h>
#ifdef ANDROID
#include <media/stagefright/foundation/ADebug.h>
#endif
#include <utils/AmlMpEventHandlerReflector.h>
#include <utils/AmlMpBitReader.h>
#include <inttypes.h>
#include <Aml_MP/Aml_MP.h>

namespace aml_mp {

#define ERROR_SIZE (-1)
#define ERROR_CRC  (-2)
#define MY_LOGV(x, y) \
    do { unsigned tmp = y; ALOGV(x, tmp); } while (0)
#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

const size_t kTSPacketSize = 188;

class SwTsParser: public AmlDemuxBase::ITsParser
{
public:
    SwTsParser(const std::function<SectionCallback>& cb);
    ~SwTsParser();
    int feedTs(const uint8_t* buffer, size_t size) override;
    void reset() override;
    int addPSISection(int pid, bool checkCRC) override;
    int getPSISectionData(int pid) override;
    void removePSISection(int pid) override;

private:
    struct PSISection;
    struct Program;

    void parseAdaptationField(AmlMpBitReader *br, unsigned PID);
    int parseTS(AmlMpBitReader *br);
    void parseProgramAssociationTable(AmlMpBitReader *br);
    int parsePID(AmlMpBitReader* br, unsigned PID, unsigned continuity_counter, unsigned payload_unit_start_indicator);
    int programMapPID() const {return mProgramMapPID;}

    void initCrcTable();
    uint32_t crc32(const uint8_t *p_start, size_t length);

    std::vector<sptr<Program> > mPrograms;
    int mPcrPid = 0x1FFF;
    size_t mNumTSPacketsParsed;

    uint32_t mCrcTable[256];
    std::map<unsigned, sptr<PSISection> > mPSISections;
    unsigned mProgramMapPID = 0x1FFF;

private:
    SwTsParser(const SwTsParser&);
    SwTsParser& operator= (const SwTsParser&) = delete;
};

struct SwTsParser::PSISection : public AmlMpRefBase {
    PSISection(int pid, SwTsParser* tsParser);

    int append(const void *data, size_t size);
    void clear();

    bool isComplete() const;
    bool isEmpty() const;

    const uint8_t *data() const;
    size_t size() const;
    void setRange(size_t offset, size_t length);
    size_t sectionLength() const;
    const sptr<AmlMpBuffer> rawBuffer() const {return mBuffer;}
    bool isChanged() const {return mChanged;}
    bool needCheckVersionChange();
    int sectionVersion() const;

    bool parse(int PID, unsigned continuity_counter,
                   unsigned payload_unit_start_indicator,
                   AmlMpBitReader *br);
    int parseSection(AmlMpBitReader* br);

protected:
    virtual ~PSISection();

private:
    sptr<AmlMpBuffer> mBuffer;
    int mPid = 0x1FFF;
    SwTsParser* mTsParser = nullptr;
    int32_t mExpectedContinuityCounter = -1;
    bool mPayloadStarted = false;

    mutable bool mGuessed = false;
    std::map<int, int> mSectionVersions;
    bool mChanged = false;

    DISALLOW_EVIL_CONSTRUCTORS(PSISection);
};

struct SwTsParser::Program : public AmlMpRefBase {
    Program(SwTsParser *parser, unsigned programNumber, unsigned programMapPID)
    : mParser(parser)
    , mProgramNumber(programNumber)
    , mProgramMapPID(programMapPID)
    {

    }

    unsigned number() const { return mProgramNumber; }
    void updateProgramMapPID(unsigned programMapPID) {
        mProgramMapPID = programMapPID;
    }

private:
    SwTsParser *mParser __unused;
    unsigned mProgramNumber;
    unsigned mProgramMapPID;

private:
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
AmlSwDemux::AmlSwDemux()
: mRemainingBytesBuffer(new AmlMpBuffer(kTSPacketSize))
{
}

AmlSwDemux::~AmlSwDemux()
{
    close();
}

int AmlSwDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId)
{
    if (isHardwareSource) {
        ALOGE("swdemux don't support hw source!!!");
        return -1;
    }

    if (mTsParser == nullptr) {
        mTsParser = new SwTsParser([this](int pid, const sptr<AmlMpBuffer>& data, int version) {
            return notifyData(pid, data, version);
        });
    }

    return 0;
}

int AmlSwDemux::close()
{
    flush();

    if (mLooper != nullptr) {
        mLooper->unregisterHandler(mHandler->id());
        mLooper->stop();
        mLooper.clear();
    }

    return 0;
}

int AmlSwDemux::start()
{
    if (mHandler == nullptr) {
        mHandler = new AmlMpEventHandlerReflector<AmlSwDemux>(this);
    }

    if (mLooper == nullptr) {
        mLooper = new AmlMpEventLooper;
        mLooper->setName("swDemux");
        mLooper->registerHandler(mHandler);
        int ret = mLooper->start();
        ALOGI("start swDemux looper, ret = %d", ret);
    }

    return 0;
}

int AmlSwDemux::stop()
{
    if (mStopped) {
        return 0;
    }

    mStopped = true;

    return 0;
}

int AmlSwDemux::flush()
{
    ++mBufferGeneration;

    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatFlush, mHandler);

    sptr<AmlMpMessage> response;
    msg->postAndAwaitResponse(&response);
    MLOG();

    return 0;
}

int AmlSwDemux::feedTs(const uint8_t* buffer, size_t size)
{
    if (mStopped) {
        return 0;
    }

    sptr<AmlMpBuffer> data = new AmlMpBuffer(size);
    memcpy(data->base(), buffer, size);
    data->setRange(0, size);

    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatFeedData, mHandler);
    msg->setBuffer("buffer", data);
    msg->setInt32("generation", mBufferGeneration);
    msg->post();

    return 0;
}

int AmlSwDemux::addPSISection(int pid, bool checkCRC)
{
    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatAddPid, mHandler);
    msg->setInt32("pid", pid);
    msg->setInt32("checkCRC", checkCRC);
    msg->post();

    return 0;
}

int AmlSwDemux::removePSISection(int pid)
{
    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatRemovePid, mHandler);
    msg->setInt32("pid", pid);
    msg->post();

    return 0;
}

bool AmlSwDemux::isStopped() const
{
    return mStopped.load(std::memory_order_relaxed);
}

void AmlSwDemux::onMessageReceived(const sptr<AmlMpMessage>& msg)
{
    switch (msg->what()) {
    case kWhatFeedData:
    {
        sptr<AmlMpBuffer> data;
        int32_t generation;
        CHECK(msg->findBuffer("buffer", &data));
        CHECK(msg->findInt32("generation", &generation));
        if (generation != mBufferGeneration) {
            ALOGW("kWhatFeedData break, %d %d", generation, mBufferGeneration.load());
            break;
        }

        onFeedData(data);
    }
    break;

    case kWhatAddPid:
    {
        int pid = AML_MP_INVALID_PID;
        msg->findInt32("pid", &pid);
        int checkCRC = 0;
        msg->findInt32("checkCRC", &checkCRC);
        onAddFilterPid(pid, checkCRC);
    }
    break;

    case kWhatRemovePid:
    {
        int pid = AML_MP_INVALID_PID;
        msg->findInt32("pid", &pid);
        onRemoveFilterPid(pid);
    }
    break;

    case kWhatFlush:
    {
        onFlush();

        sptr<AReplyToken> replyID;
        CHECK(msg->senderAwaitsResponse(&replyID));
        sptr<AmlMpMessage> response = new AmlMpMessage;
        response->postReply(replyID);
    }
    break;

    default:
#ifdef ANDROID
        TRESPASS();
#endif
        break;
    }
}

void AmlSwDemux::onFeedData(const sptr<AmlMpBuffer>& entry)
{
    int err = 0;

    if (mRemainingBytesBuffer->size() > 0) {
        //CHECK_LE(mRemainingBytesBuffer->size(), kTSPacketSize);
        size_t paddingSize = kTSPacketSize - mRemainingBytesBuffer->size();
        size_t copySize = paddingSize < entry->size() ? paddingSize : entry->size();

        memcpy(mRemainingBytesBuffer->data() + mRemainingBytesBuffer->size(), entry->data(), copySize);
        mRemainingBytesBuffer->setRange(mRemainingBytesBuffer->offset(), mRemainingBytesBuffer->size() + copySize);
        entry->setRange(entry->offset() + copySize, entry->size() - copySize);

        if (mRemainingBytesBuffer->size() == kTSPacketSize) {
            err = mTsParser->feedTs(mRemainingBytesBuffer->data(), kTSPacketSize);
            if (err != 0) {
                const uint8_t* p = mRemainingBytesBuffer->data();
                ALOGI("%d %lld, feedTSPacket err:%d, %#x, %#x, %#x, %#x, %#x", __LINE__, mOutBufferCount.load(), err,
                        p[0], p[1], p[2], p[3], p[4]);
            }
            mRemainingBytesBuffer->setRange(0, 0);
        }
    }

    while (entry->size() > 0) {
        if (entry->size() < kTSPacketSize) {
            if (*entry->data() == 0x47) {
                memcpy(mRemainingBytesBuffer->data(), entry->data(), entry->size());
                mRemainingBytesBuffer->setRange(mRemainingBytesBuffer->offset(), mRemainingBytesBuffer->size() + entry->size());
            } else {
                ALOGW("left data doesn't start with 0x47, discard it!");
            }
            break;
        }

        if (*entry->data() != 0x47) {
            ALOGV("mOutBufferCount:%lld, entry start bytes:%#x, offset:%d, size:%d",
                  mOutBufferCount.load(), *entry->data(), entry->offset(), entry->size());
            const uint8_t* p = entry->data();
            ALOGV("entry bytes: %#x %#x %#x %#x %#x", p[0], p[1], p[2], p[3], p[4]);

            err = resync(entry);
            if (err < 0) {
                ALOGE("resync ts buffer failed! %d, size:%zu", err, entry->size());
                return;
            } else {
                ALOGV("resync add offset:%d", err);
                continue;
            }
        }

        err = mTsParser->feedTs(entry->data(), kTSPacketSize);
        if (err != 0) {
            ALOGE("%d feedTSPacket failed, err:%d", __LINE__, err);
        }
        entry->setRange(entry->offset() + kTSPacketSize, entry->size() - kTSPacketSize);
    }
}

int AmlSwDemux::resync(const sptr<AmlMpBuffer>& buffer)
{
    const uint8_t* p = buffer->data();
    size_t size = buffer->size();

    if (p == nullptr || size < kTSPacketSize)
        return -1;

    bool synced = false;
    size_t offset = 0;
    while (offset < size) {
        if (p[offset] != 0x47) {
            ++offset;
            continue;
        }

        if (offset + kTSPacketSize < size) {
            if (p[offset + kTSPacketSize] != 0x47) {
                ++offset;
                ALOGV("next ts packet header check failed, try sync again...");
                continue;
            }
        }

        synced = true;
        break;
    }

    if (synced) {
        buffer->setRange(buffer->offset() + offset, buffer->size() - offset);
        return offset;
    }

    return -1;
}

void AmlSwDemux::onFlush()
{
    mOutBufferCount = 0;

    if (mTsParser != nullptr) {
        mTsParser->reset();
    }

    mRemainingBytesBuffer->setRange(0, 0);
}

void AmlSwDemux::onAddFilterPid(int pid, bool checkCRC)
{
    if (mTsParser != nullptr) {
        ALOGI("add section pid:%d(%#x)", pid, pid);
        mTsParser->addPSISection(pid, checkCRC);
    }
}

void AmlSwDemux::onRemoveFilterPid(int pid)
{
    if (mTsParser != nullptr) {
        ALOGI("remove section pid:%d(%#x)", pid, pid);
        mTsParser->removePSISection(pid);
    }
}

///////////////////////////////////////////////////////////////////////////////
SwTsParser::SwTsParser(const std::function<SectionCallback>& cb)
: ITsParser(cb)
{
    MLOG();

    mPSISections.emplace(0 /* PID */, new PSISection(0, this));
    mPSISections.emplace(1 /* PID */, new PSISection(1, this));
    initCrcTable();
}

SwTsParser::~SwTsParser()
{
    MLOG();
}

void SwTsParser::initCrcTable()
{
#if 0
    uint32_t poly = 0x04C11DB7;

    for (int i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? (poly) : 0);
        }
        mCrcTable[i] = crc;
    }
#endif

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t k = 0;
        for (uint32_t j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1) {
            k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
        }

        mCrcTable[i] = k;
    }
}

/**
 * Compute CRC32 checksum for buffer starting at offset start and for length
 * bytes.
 */
uint32_t SwTsParser::crc32(const uint8_t *p_start, size_t length)
{
#if 0
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p;

    for (p = p_start; p < p_start + length; p++) {
        crc = (crc << 8) ^ mCrcTable[((crc >> 24) ^ *p) & 0xFF];
    }

    return crc;
#endif

    uint32_t crc32_reg = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc32_reg = (crc32_reg << 8) ^ mCrcTable[((crc32_reg >> 24) ^ *p_start++) & 0xFF];
    }
    return crc32_reg;
}

int SwTsParser::feedTs(const uint8_t* buffer, size_t size)
{
    //CHECK_EQ(size, kTSPacketSize);

    AmlMpBitReader br(buffer, kTSPacketSize);
    return parseTS(&br);
}

void SwTsParser::reset()
{
    MLOG();

    //for (size_t i = 0; i < mPSISections.size(); ++i) {
        //sptr<PSISection> &p = mPSISections.editValueAt(i);
        //p->clear();
    //}

    for (auto& p : mPSISections) {
        p.second->clear();
    }
}

int SwTsParser::addPSISection(int pid, bool checkCRC)
{
    if (pid == 0x1FFF)
        return -1;

    if (mPSISections.find(pid) == mPSISections.end()) {
        ALOGW("add section pid:%d(%#x)", pid, pid);
        mPSISections.emplace(pid, new PSISection(pid, this));
    }

    return 0;
}

int SwTsParser::getPSISectionData(int pid __unused)
{
    return -1;
}

void SwTsParser::removePSISection(int pid)
{
    if (pid == 0x1FFF)
        return;

    if (mPSISections.find(pid) == mPSISections.end()) {
        return;
    }

    ALOGW("remove section pid:%d(%#x)", pid, pid);
    mPSISections.erase(pid);
}

void SwTsParser::parseAdaptationField(AmlMpBitReader *br, unsigned PID)
{
    unsigned adaptation_field_length = br->getBits(8);

    if (adaptation_field_length > 0) {
        unsigned discontinuity_indicator = br->getBits(1);

        if (discontinuity_indicator) {
            ALOGV("PID 0x%04x: discontinuity_indicator = 1 (!!!)", PID);
        }

        br->skipBits(2);
        unsigned PCR_flag = br->getBits(1);

        size_t numBitsRead = 4;

        if (PCR_flag && (mPcrPid == 0x1FFF || mPcrPid == (int)PID)) {
            br->skipBits(4);
            uint64_t PCR_base = br->getBits(32);
            PCR_base = (PCR_base << 1) | br->getBits(1);

            //PCR_base = recoverPTS(PCR_base, mLastRecoveredPCR);

            br->skipBits(6);
            unsigned PCR_ext = br->getBits(9);

            // The number of bytes from the start of the current
            // MPEG2 transport stream packet up and including
            // the final byte of this PCR_ext field.
            //size_t byteOffsetFromStartOfTSPacket =
                //(188 - br->numBitsLeft() / 8);

            int64_t PCR = PCR_base * 300 + PCR_ext;

            ALOGV("PID 0x%04x: PCR = 0x%016" PRIx64 " (%.2f)",
                  PID, PCR, PCR / 27E6);

            // The number of bytes received by this parser up to and
            // including the final byte of this PCR_ext field.
            //size_t byteOffsetFromStart =
                //mNumTSPacketsParsed * 188 + byteOffsetFromStartOfTSPacket;

            //for (size_t i = 0; i < mPrograms.size(); ++i) {
                //updatePCR(PID, PCR, byteOffsetFromStart);
            //}

            numBitsRead += 52;
        }

        //CHECK_GE(adaptation_field_length * 8, numBitsRead);

        br->skipBits(adaptation_field_length * 8 - numBitsRead);
    }
}

int SwTsParser::parseTS(AmlMpBitReader *br)
{
    ALOGV("---");

    unsigned sync_byte = br->getBits(8);
    if (sync_byte != 0x47u) {
        ALOGE("[error] parseTS: return error as sync_byte=0x%x", sync_byte);
        return -EINVAL;
    }

    if (br->getBits(1)) {  // transport_error_indicator
        // silently ignore.
        return 0;
    }

    unsigned payload_unit_start_indicator = br->getBits(1);
    ALOGV("payload_unit_start_indicator = %u", payload_unit_start_indicator);

    MY_LOGV("transport_priority = %u", br->getBits(1));

    unsigned PID = br->getBits(13);
    ALOGV("PID = 0x%04x", PID);

    MY_LOGV("transport_scrambling_control = %u", br->getBits(2));

    unsigned adaptation_field_control = br->getBits(2);
    ALOGV("adaptation_field_control = %u", adaptation_field_control);

    unsigned continuity_counter = br->getBits(4);
    ALOGV("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    // ALOGI("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        parseAdaptationField(br, PID);
    }

    int err = 0;

    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        err = parsePID(
                br, PID, continuity_counter, payload_unit_start_indicator);
    }

    ++mNumTSPacketsParsed;

    return err;
}

void SwTsParser::parseProgramAssociationTable(AmlMpBitReader *br)
{
    unsigned table_id = br->getBits(8);
    ALOGV("  table_id = %u", table_id);
    if (table_id != 0x00u) {
        ALOGE("PAT data error!");
        return ;
    }
    unsigned section_syntax_indictor = br->getBits(1);
    ALOGV("  section_syntax_indictor = %u", section_syntax_indictor);
    //CHECK_EQ(section_syntax_indictor, 1u);

    //CHECK_EQ(br->getBits(1), 0u);
    (br->getBits(1), 0u);
    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    ALOGV("  section_length = %u", section_length);
    //CHECK_EQ(section_length & 0xc00, 0u);

    MY_LOGV("  transport_stream_id = %u", br->getBits(16));
    MY_LOGV("  reserved = %u", br->getBits(2));
    MY_LOGV("  version_number = %u", br->getBits(5));
    MY_LOGV("  current_next_indicator = %u", br->getBits(1));
    MY_LOGV("  section_number = %u", br->getBits(8));
    MY_LOGV("  last_section_number = %u", br->getBits(8));

    size_t numProgramBytes = (section_length - 5 /* header */ - 4 /* crc */);
    //CHECK_EQ((numProgramBytes % 4), 0u);

    for (size_t i = 0; i < numProgramBytes / 4; ++i) {
        unsigned program_number = br->getBits(16);
        ALOGV("    program_number = %u", program_number);

        MY_LOGV("    reserved = %u", br->getBits(3));

        if (program_number == 0) {
            MY_LOGV("    network_PID = 0x%04x", br->getBits(13));
        } else {
            unsigned programMapPID = br->getBits(13);

            ALOGV("    program_map_PID = 0x%04x, %d", programMapPID, programMapPID);

            if (mProgramMapPID == 0x1FFF) {
                ALOGE("first update programMapPID:%d", programMapPID);
            }
            mProgramMapPID = programMapPID;

            bool found = false;
            for (size_t index = 0; index < mPrograms.size(); ++index) {
                const sptr<Program> &program = mPrograms.at(index);

                if (program->number() == program_number) {
                    program->updateProgramMapPID(programMapPID);
                    found = true;
                    break;
                }
            }

            if (!found) {
                mPrograms.push_back(
                        new Program(this, program_number, programMapPID));
            }

            if (mPSISections.find(programMapPID) == mPSISections.end()) {
                mPSISections.emplace(programMapPID, new PSISection(programMapPID, this));
            }
        }
    }

    MY_LOGV("  CRC = 0x%08x", br->getBits(32));
}

int SwTsParser::parsePID(
        AmlMpBitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator) {
    auto sectionIndex = mPSISections.find(PID);

    if (sectionIndex != mPSISections.end()) {
        sptr<PSISection> section = mPSISections.at(PID);

        if (!section->parse(PID, continuity_counter, payload_unit_start_indicator, br)) {
            ALOGW("pre parse failed!!!! PID = %d", PID);
            return 0;
        }

        CHECK((br->numBitsLeft() % 8) == 0);
        int err = section->append(br->data(), br->numBitsLeft() / 8);

        if (err != 0) {
            ALOGW("section append data %d size failed!", br->numBitsLeft()/8);
            return err;
        }

        if (!section->isComplete()) {
            return 0;
        }

        AmlMpBitReader sectionBits2(section->data(), section->size());
        bool notifyListener = true;

        err = section->parseSection(&sectionBits2);
        if (err != ERROR_SIZE) {
            section->setRange(0, section->sectionLength());
        }

        if (err != 0) {
            notifyListener = false;

            ALOGE("parse failed:%d, section pid:%d, buffer size:%d, section length:%d", err, PID,
                    section->size(), section->sectionLength());
#if 0
            //
            ALOGE("Dump data:");
            hexdump(section->data(), std::min(section->size(), (size_t)184));

            char path[50];
            sprintf(path, "/data/crc_err_%d.bin", PID);
            int fd = ::open(path, O_RDONLY);
            if (fd < 0) {
                ALOGI("dump to %s", path);
                FILE* fp = fopen(path, "w");
                if (fp == NULL) {
                    ALOGE("open %s failed!", path);
                } else {
                    fwrite(section->data(), 1, section->size(), fp);
                    fclose(fp);
                }
            } else {
                ::close(fd);
            }
            //
#endif

#if 0
            if (err == ERROR_CRC) {
                notifyListener = true;
                ALOGW("crc error ignored!");
            }
#endif

        } else {
            ALOGV("section pid:%d, set range:%d", PID, section->sectionLength());
            //section->setRange(0, section->sectionLength());
        }

        if (section != NULL) {
            if (notifyListener) {
                int version = section->needCheckVersionChange() ? section->sectionVersion() : -1;
                mSectionCallback(PID, section->rawBuffer(), version);
            }

            //section->clear();
        }


        AmlMpBitReader sectionBits(section->data(), section->size());

        if (PID == 0) {
            parseProgramAssociationTable(&sectionBits);
        } else {
#if 0
            bool handled = false;
            for (size_t i = 0; i < mPrograms.size(); ++i) {
                status_t err;
                if (!mPrograms.editItemAt(i)->parsePSISection(
                            PID, &sectionBits, &err)) {
                    continue;
                }

                if (err != 0) {
                    return err;
                }

                handled = true;
                break;
            }

            if (!handled) {
                //mPSISections.removeItem(PID);
                //section.clear();

            }
#endif
        }

        if (section != NULL) {
            section->clear();
        }

        return 0;
    }

//don't parse PES
#if 0
    bool handled = false;
    for (size_t i = 0; i < mPrograms.size(); ++i) {
        status_t err;
        if (mPrograms.editItemAt(i)->parsePID(
                    PID, continuity_counter, payload_unit_start_indicator,
                    br, &err)) {
            if (err != 0) {
                return err;
            }

            handled = true;
            break;
        }
    }

    if (!handled) {
        ALOGV("PID 0x%04x not handled.", PID);
    }
#endif

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
static inline uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

SwTsParser::PSISection::PSISection(int pid, SwTsParser* tsParser)
: mPid(pid)
, mTsParser(tsParser)
{
}

SwTsParser::PSISection::~PSISection() {
}

int SwTsParser::PSISection::append(const void *data, size_t size) {
    if (mBuffer == NULL || mBuffer->size() + size > mBuffer->capacity()) {
        size_t newCapacity =
            (mBuffer == NULL) ? size : mBuffer->capacity() + size;

        newCapacity = (newCapacity + 1023) & ~1023;

        sptr<AmlMpBuffer> newBuffer = new AmlMpBuffer(newCapacity);

        if (mBuffer != NULL) {
            memcpy(newBuffer->data(), mBuffer->data(), mBuffer->size());
            newBuffer->setRange(0, mBuffer->size());
        } else {
            newBuffer->setRange(0, 0);
        }

        mBuffer = newBuffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), data, size);
    mBuffer->setRange(0, mBuffer->size() + size);

    return 0;
}

void SwTsParser::PSISection::clear() {
    if (mBuffer != NULL) {
        mBuffer->setRange(0, 0);
    }

    mChanged = false;
}

bool SwTsParser::PSISection::isComplete() const {
    if (mBuffer == NULL || mBuffer->size() < 3) {
        return false;
    }

    unsigned sectionLength = U16_AT(mBuffer->data() + 1) & 0xfff;
    if (sectionLength > 4093) {
        ALOGE("section size error:%d", sectionLength);
        return true;
    }

    return mBuffer->size() >= sectionLength + 3;
}

bool SwTsParser::PSISection::isEmpty() const {
    return mBuffer == NULL || mBuffer->size() == 0;
}

const uint8_t *SwTsParser::PSISection::data() const {
    return mBuffer == NULL ? NULL : mBuffer->data();
}

size_t SwTsParser::PSISection::size() const {
    return mBuffer == NULL ? 0 : mBuffer->size();
}

void SwTsParser::PSISection::setRange(size_t offset, size_t length)
{
    if (mBuffer == nullptr || mBuffer->capacity() < length) {
        ALOGE("section setRange failed! length:%d", length);
        return;
    }

    mBuffer->setRange(offset, length);
}

size_t SwTsParser::PSISection::sectionLength() const {
    if (mBuffer == NULL || mBuffer->size() < 3) {
        return 0;
    }

    unsigned sectionLength = U16_AT(mBuffer->data() + 1) & 0xfff;
    return sectionLength + 3;
}

bool SwTsParser::PSISection::needCheckVersionChange()
{
    int pid = mPid;
    int programMapPID = mTsParser->programMapPID();

    return pid == 0 || pid == 1 || (programMapPID!=0x1FFF && programMapPID==pid);
}

int SwTsParser::PSISection::sectionVersion() const
{
    auto index = mSectionVersions.find(0);
    if (index == mSectionVersions.end())
        return -1;

    int version = mSectionVersions.at(0);
    return version;
}

bool SwTsParser::PSISection::parse(int PID, unsigned continuity_counter,
                   unsigned payload_unit_start_indicator,
                   AmlMpBitReader *br)
{
    (void)PID;
    (void)continuity_counter;
#if 0
    if (mExpectedContinuityCounter >= 0 && (unsigned)mExpectedContinuityCounter != continuity_counter) {
        ALOGW("section discontinuity on stream pid 0x%04x(%d)", PID, PID);

        mPayloadStarted = false;
        mBuffer->setRange(0, 0);
        mExpectedContinuityCounter = -1;

        if (!payload_unit_start_indicator) {
            return false;
        }
    }

    mExpectedContinuityCounter = (continuity_counter + 1) & 0x0f;
#endif

    if (payload_unit_start_indicator) {
        // Otherwise we run the danger of receiving the trailing bytes
        // of a section packet that we never saw the start of and assuming
        // we have a a complete section packet.
        if (!isEmpty()) {
            ALOGW("parsePID encounters payload_unit_start_indicator when section is not empty");
            clear();
        }

        unsigned skip = br->getBits(8);
        if (skip > 0) {
            ALOGW("skip %d bytes!", skip);
        }
        br->skipBits(skip * 8);

        mPayloadStarted = true;
    }

    if (!mPayloadStarted) {
        return false;
    }

    return true;
}

int SwTsParser::PSISection::parseSection(AmlMpBitReader* br)
{
    unsigned table_id = br->getBits(8);
    ALOGV("  table_id = %u", table_id);

    unsigned section_syntax_indicator = br->getBits(1);
    ALOGV("  section_syntax_indicator = %u", section_syntax_indicator);

    unsigned private_indicator =  br->getBits(1);

    if ((section_syntax_indicator ^ private_indicator) != 1) {
        ALOGW_IF(!mGuessed, "HAVE DSM-CC data!!!");
        mGuessed = true;
    }

    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    ALOGV("  section_length = %u", section_length);
    //CHECK_EQ(section_length & 0xc00, 0u);
    //CHECK_LE(section_length, 4093u);
    if (section_length > 4093) {
        return ERROR_SIZE;
    }

    if (section_syntax_indicator) {
        br->skipBits(18);
        int versionNumber = br->getBits(5);
        MY_LOGV("  version_number = %u", versionNumber);
        //MY_LOGV("  version_number = %u", br->getBits(5));
        MY_LOGV("  current_next_indicator = %u", br->getBits(1));
        int sectionNumber = br->getBits(8);
        MY_LOGV("  section_number = %u", sectionNumber);
        MY_LOGV("  last_section_number = %u", br->getBits(8));

        if (needCheckVersionChange()) {
            auto index = mSectionVersions.find(sectionNumber);
            if (index == mSectionVersions.end()) {
                mChanged = true;
                mSectionVersions.emplace(sectionNumber, versionNumber);
                ALOGE("section FIRST changed, PID:%d, sectionNumber:%d, versionNumber:%d", mPid, sectionNumber, versionNumber);
            } else {
                int &lastVersionNumber = mSectionVersions.at(sectionNumber);
                if (lastVersionNumber != versionNumber) {
                    ALOGE("section changed, PID:%d, sectionNumber:%d, versionNumber:%d, lastVersionNumber:%d", mPid, sectionNumber, versionNumber, lastVersionNumber);
                    mChanged = true;
                    lastVersionNumber = versionNumber;
                }
            }
        }

        uint32_t crc = mTsParser->crc32(mBuffer->data(), section_length + 3);
        if (crc != 0) {
            ALOGE("crc error: %#x", crc);
            return ERROR_CRC;
        }
    }

    return 0;
}


}
