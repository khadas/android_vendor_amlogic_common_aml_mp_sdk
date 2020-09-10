#define LOG_TAG "AmlMpPlayerDemo_DvbSource"
#include <utils/Log.h>
#include <Aml_MP/Common.h>
#include "DvbSource.h"
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>


namespace aml_mp {

///////////////////////////////////////////////////////////////////////////////
typedef int (*DvbLockHandler)(int fendFd, const dmd_delivery_t* pDelivery);
static DvbLockHandler getProtoHandler(const char* proto);
static dmd_device_type_t getDeviceType(const char* proto);
static int setFendProp(int fendFd, const struct dtv_properties* prop);
static int lockDvb_T(int fendFd, const dmd_delivery_t* pDelivery);
static int lockDvb_C(int fendFd, const dmd_delivery_t* pDelivery);

static void getDefaultDeliveryConf(dmd_device_type_t type, dmd_delivery_t* delivery)
{
    memset(delivery, 0, sizeof(*delivery));

    switch (type) {
    case DMD_TERRESTRIAL:
        delivery->delivery.terrestrial.dvb_type = DMD_DVBTYPE_DVBT;
        delivery->delivery.terrestrial.desc.dvbt.frequency = 474*1000;
        delivery->delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_8M;
        delivery->delivery.terrestrial.desc.dvbt.transmission_mode = DMD_TRANSMISSION_8K;
        delivery->delivery.terrestrial.desc.dvbt.guard_interval = DMD_GUARD_INTERVAL_1_32;
        break;

    case DMD_CABLE:
        delivery->device_type = DMD_CABLE;
        delivery->delivery.cable.frequency = 474*1000;
        delivery->delivery.cable.symbol_rate = 6875;
        delivery->delivery.cable.modulation = DMD_MOD_64QAM;
        break;

    case DMD_SATELLITE:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
DvbSource::DvbSource(const char* proto, const char* address, int programNumber, uint32_t flags)
: Source(programNumber, flags)
, mProto(proto)
, mAddress(address)
, mDeviceType((dmd_device_type_t)0)
, mDelivery{}
{
    ALOGI("proto:%s, address:%s, programNumber:%d", proto, address, programNumber);
}

DvbSource::~DvbSource()
{
    stop();
}

int DvbSource::initCheck()
{
    if (getProtoHandler(mProto.c_str()) == nullptr) {
        ALOGE("unknown proto:%s", mProto.c_str());
        return -1;
    }

    mDeviceType = getDeviceType(mProto.c_str());

    mDemuxId = std::stoi(mAddress);
    ALOGI("demuxId = %d\n", mDemuxId);

    if (mDemuxId < AML_MP_HW_DEMUX_ID_0 || mDemuxId >= AML_MP_HW_DEMUX_ID_MAX) {
        ALOGE("invalid demux id!");
        return -1;
    }

    if (openFend(mDemuxId) < 0) {
        ALOGE("openFend failed!\n");
        return -1;
    }

    return 0;
}

int DvbSource::start()
{
    getDefaultDeliveryConf(mDeviceType, &mDelivery);

    DvbLockHandler lockHandler = getProtoHandler(mProto.c_str());
    int ret = lockHandler(mFendFd, &mDelivery);
    if (ret < 0) {
        ALOGE("dvb lock (%s) failed!", mProto.c_str());
        return -1;
    }

    FendLockState fendState = FEND_STATE_UNKNOWN;
    while ((fendState = queryFendLockState()) == FEND_STATE_UNKNOWN) {
        if (mRequestQuit) {
            break;
        }

        usleep(10 * 1000);
    }

    ALOGI("start, fendState:%d", fendState);
    if (fendState != FEND_STATE_LOCKED) {
        return -1;
    }

    return 0;
}

int DvbSource::stop()
{
    closeFend();

    return 0;
}

void DvbSource::signalQuit()
{
    mRequestQuit = true;
}

///////////////////////////////////////////////////////////////////////////////
int DvbSource::openFend(int fendIndex)
{
   char fe_name[24];
   struct stat file_status;

   snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", fendIndex);
   if (stat(fe_name, &file_status) == 0) {
       ALOGE("Found FE[%s]\n", fe_name);
   } else {
       ALOGE("No FE found [%s]!", fe_name);
	   return -1;
   }

   if ((mFendFd = open(fe_name, O_RDWR | O_NONBLOCK)) < 0) {
	 ALOGE("Failed to open [%s], errno %d\n", fe_name, errno);
      return -1;
   } else {
      ALOGE("Open %s frontend_fd:%d \n", fe_name, mFendFd);
   }

   return 0;
}

void DvbSource::closeFend()
{
    ALOGI("closeFend! mFendFd:%d", mFendFd);

    if (mFendFd < 0)
        return;

    struct dtv_properties props;
    int cmd_num = 0;
    struct dtv_property p[DTV_IOCTL_MAX_MSGS];
    p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
    p[cmd_num].u.data = SYS_UNDEFINED;
    cmd_num++;

    props.num = cmd_num;
    props.props = (struct dtv_property*)p;
    setFendProp(mFendFd, &props);

    close(mFendFd);
    mFendFd = -1;
}

DvbSource::FendLockState DvbSource::queryFendLockState() const
{
    fe_status_t feStatus;

    if (ioctl(mFendFd, FE_READ_STATUS, &feStatus) < 0) {
        printf("mfendf:%d FE_READ_STATUS errno:%d \n" ,mFendFd, errno);
        return FEND_STATE_ERROR;
    }

    ALOGI("current tuner status=0x%02x \n", feStatus);
   if ((feStatus& FE_HAS_LOCK) != 0) {
       printf("current tuner status [locked]\n");
       return FEND_STATE_LOCKED;
   } else if ((feStatus& FE_TIMEDOUT) != 0) {
        printf("current tuner status [unlocked]\n");
        return FEND_STATE_LOCK_TIMEOUT;
   }

   return FEND_STATE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////
struct ProtoItem {
    const char* proto;
    dmd_device_type_t type;
    int (*fn)(int, const dmd_delivery_t*);
};

static ProtoItem g_protoList[] = {
    {"dvbt", DMD_TERRESTRIAL, &lockDvb_T},
    {"dvbc", DMD_CABLE, &lockDvb_C},
    {nullptr, (dmd_device_type_t)0, nullptr},
};

static DvbLockHandler getProtoHandler(const char* proto)
{
    ProtoItem *item = g_protoList;

    for (; item->proto; item++) {
        if (!strcmp(item->proto, proto)) {
            return item->fn;
        }
    }

    return nullptr;
}

static dmd_device_type_t getDeviceType(const char* proto)
{
    ProtoItem *item = g_protoList;

    for (; item->proto; item++) {
        if (!strcmp(item->proto, proto)) {
            return item->type;
        }
    }

    return (dmd_device_type_t)0;
}

static int setFendProp(int fendFd, const struct dtv_properties *prop)
{
  if (ioctl(fendFd, FE_SET_PROPERTY, prop) == -1) {
     printf("set prop failed>>>>>>>>>>>>.\n");
     return -1;
  }

  return 0;
}

static int lockDvb_T(int fendFd, const dmd_delivery_t * pDelivery)
{
    ALOGI("%s", __FUNCTION__);

   int tmp = 0;
   int cmd_num = 0;
   struct dtv_properties props;
   struct dtv_property p[DTV_IOCTL_MAX_MSGS];

   p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 p[cmd_num].u.data = SYS_DVBT;
   else
	 p[cmd_num].u.data = SYS_DVBT2;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt.frequency * 1000;
   else
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt2.frequency * 1000;

   p[cmd_num].cmd = DTV_FREQUENCY;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.bandwidth;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.bandwidth;
   p[cmd_num].cmd = DTV_BANDWIDTH_HZ;
   switch (tmp) {
   case DMD_BANDWIDTH_10M:
	 p[cmd_num].u.data = 10000000;
	 break;
   case DMD_BANDWIDTH_8M:
	 p[cmd_num].u.data = 8000000;
	 break;
   case DMD_BANDWIDTH_7M:
	 p[cmd_num].u.data = 7000000;
	 break;
   case DMD_BANDWIDTH_6M:
	 p[cmd_num].u.data = 6000000;
	 break;
   case DMD_BANDWIDTH_5M:
	 p[cmd_num].u.data = 5000000;
	 break;
   case DMD_BANDWIDTH_17M:
	 p[cmd_num].u.data = 1712000;
	 break;
   }
   cmd_num++;

   p[cmd_num].cmd = DTV_CODE_RATE_HP;
   p[cmd_num].u.data = FEC_AUTO;
   cmd_num++;

   p[cmd_num].cmd = DTV_CODE_RATE_LP;
   p[cmd_num].u.data = FEC_AUTO;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.transmission_mode;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.transmission_mode;
   if (tmp <= DMD_TRANSMISSION_8K)
	 tmp += -1;
   p[cmd_num].cmd = DTV_TRANSMISSION_MODE;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.guard_interval;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.guard_interval;
   if (tmp <= DMD_GUARD_INTERVAL_1_4)
	 tmp += -1;
   p[cmd_num].cmd = DTV_GUARD_INTERVAL;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT) {
	 p[cmd_num].cmd = DTV_HIERARCHY;
	 p[cmd_num].u.data = HIERARCHY_AUTO;
	 cmd_num++;
   }
   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT2) {
	 p[cmd_num].cmd = DTV_DVBT2_PLP_ID_LEGACY;
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt2.plp_id;
	 cmd_num++;
   }

   p[cmd_num].cmd = DTV_TUNE;
   cmd_num++;
   props.num = cmd_num;
   props.props = (struct dtv_property *)&p;

   return setFendProp(fendFd, &props);
}

static int lockDvb_C(int fendFd, const dmd_delivery_t * pDelivery)
{
    ALOGI("%s", __FUNCTION__);

   int tmp = 0;
   int cmd_num = 0;
   struct dtv_properties props;
   struct dtv_property p[DTV_IOCTL_MAX_MSGS];

   p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
   p[cmd_num].u.data = SYS_DVBC_ANNEX_A;
   cmd_num++;
   p[cmd_num].cmd = DTV_FREQUENCY;
   p[cmd_num].u.data = pDelivery->delivery.cable.frequency * 1000;
   cmd_num++;

   p[cmd_num].cmd = DTV_SYMBOL_RATE;
   p[cmd_num].u.data = pDelivery->delivery.cable.symbol_rate * 1000;
   cmd_num++;

   tmp = pDelivery->delivery.cable.modulation;
   switch (tmp) {
   case DMD_MOD_NONE:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_QPSK:
	 tmp = QPSK;
	 break;
   case DMD_MOD_8PSK:
	 tmp = PSK_8;
	 break;
   case DMD_MOD_QAM:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_4QAM:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_16QAM:
	 tmp = QAM_16;
	 break;
   case DMD_MOD_32QAM:
	 tmp = QAM_32;
	 break;
   case DMD_MOD_64QAM:
	 tmp = QAM_64;
	 break;
   case DMD_MOD_128QAM:
	 tmp = QAM_128;
	 break;
   case DMD_MOD_256QAM:
	 tmp = QAM_256;
	 break;
   case DMD_MOD_BPSK:
   case DMD_MOD_ALL:
	 tmp = QAM_AUTO;
	 break;
   }
   p[cmd_num].cmd = DTV_MODULATION;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   p[cmd_num].cmd = DTV_TUNE;
   cmd_num++;
   props.num = cmd_num;
   props.props = (struct dtv_property *)&p;

   return setFendProp(fendFd, &props);
}

}
