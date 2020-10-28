/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef __FEND_H__
#define __FEND_H__
typedef enum
{
  DMD_DOWNLINK_INJECTION,
  DMD_DOWNLINK_CUSTOM,
  DMD_DOWNLINK_Ku_LOW,
  DMD_DOWNLINK_Ku_HIGH,
  DMD_DOWNLINK_C
} dmd_downlink_t;

typedef enum
{
  DMD_LNB_TONE_DEFAULT,
  DMD_LNB_TONE_OFF,
  DMD_LNB_TONE_22KHZ
} dmd_lnb_tone_state_t;

typedef struct
{
  uint_t				   band_start;
  uint_t				   band_end;
  uint_t				   lo;
  dmd_downlink_t		   downlink;
} dmd_satellite_band_t;

typedef enum
{
  DMD_DISEQC_DEFAULT,
  DMD_DISEQC_PORTA,
  DMD_DISEQC_PORTB,
  DMD_DISEQC_PORTC,
  DMD_DISEQC_PORTD,
  DMD_DISEQC_ALL
} dmd_diseqc_port_t;

typedef enum
{
  DMD_LNB_VOLTAGE_OFF = 0,
  DMD_LNB_VOLTAGE_14V = 1,
  DMD_LNB_VOLTAGE_18V = 2
} dmd_lnb_voltage_t;

typedef enum
{
  DMD_PLR_HORIZONTAL      = 0x01,
  DMD_PLR_VERTICAL        = 0x02,
  DMD_PLR_NONE            = 0x03,
  DMD_PLR_CIRCULAR_LEFT   = 0x04,
  DMD_PLR_CIRCULAR_RIGHT  = 0x05
} dmd_polarization_t;

typedef enum
{
  DMD_ROLLOFF_035 = 0x00,
  DMD_ROLLOFF_025 = 0x01,
  DMD_ROLLOFF_020 = 0x02
} dmd_rolloff_t;

typedef enum
{
  DMD_MODSYS_DVBS  = 0x00,
  DMD_MODSYS_DVBS2 = 0x01
} dmd_modulation_system_t;

typedef enum
{
  DMD_FEC_NONE    = 0x0000,
  DMD_FEC_1_2     = 0x0001,
  DMD_FEC_2_3     = 0x0002,
  DMD_FEC_3_4     = 0x0004,
  DMD_FEC_4_5     = 0x0008,
  DMD_FEC_5_6     = 0x0010,
  DMD_FEC_6_7     = 0x0020,
  DMD_FEC_7_8     = 0x0040,
  DMD_FEC_8_9     = 0x0080,
  DMD_FEC_3_5     = 0x0100,
  DMD_FEC_9_10    = 0x0200,
  DMD_FEC_ALL     = 0xFFFF
} dmd_fec_rate_t;

typedef enum
{
  DMD_FEC_OUTER_NONE        = 0x00,
  DMD_FEC_OUTER_NOT_DEFINED = 0x01,
  DMD_FEC_OUTER_RS          = 0x02
} dmd_fec_rate_outer_t;

typedef enum
{
  DMD_MOD_NONE    = 0x0000,
  DMD_MOD_QPSK    = 0x0001,
  DMD_MOD_8PSK    = 0x0002,
  DMD_MOD_QAM     = 0x0004,
  DMD_MOD_4QAM    = 0x0008,
  DMD_MOD_16QAM   = 0x0010,
  DMD_MOD_32QAM   = 0x0020,
  DMD_MOD_64QAM   = 0x0040,
  DMD_MOD_128QAM  = 0x0080,
  DMD_MOD_256QAM  = 0x0100,
  DMD_MOD_BPSK    = 0x0200,
  DMD_MOD_ALL     = 0xFFFF
} dmd_modulation_t;

typedef enum
{
  DMD_CONSTELLATION_NONE    = 0x0000,
  DMD_CONSTELLATION_QPSK    = 0x0001,
  DMD_CONSTELLATION_16QAM   = 0x0002,
  DMD_CONSTELLATION_64QAM   = 0x0004,
  DMD_CONSTELLATION_32QAM   = 0x0008,
  DMD_CONSTELLATION_128QAM  = 0x0010,
  DMD_CONSTELLATION_256QAM  = 0x0020,
  DMD_CONSTELLATION_1024QAM = 0x0040,
  DMD_CONSTELLATION_ALL     = 0xFFFF
} dmd_constellation_t;

typedef enum
{
  DMD_TRANSMISSION_2K   = 0x01,
  DMD_TRANSMISSION_8K   = 0x02,
  DMD_TRANSMISSION_4K   = 0x03,
  DMD_TRANSMISSION_1K   = 0x04,
  DMD_TRANSMISSION_16K  = 0x05,
  DMD_TRANSMISSION_32K  = 0x06
} dmd_transmission_mode_t;

typedef enum
{
  DMD_BANDWIDTH_8M    = 0x01,
  DMD_BANDWIDTH_7M    = 0x02,
  DMD_BANDWIDTH_6M    = 0x03,
  DMD_BANDWIDTH_5M    = 0x04,
  DMD_BANDWIDTH_10M   = 0x05,
  DMD_BANDWIDTH_17M   = 0x06
} dmd_bandwidth_t;

typedef enum
{
  DMD_GUARD_INTERVAL_1_32    = 0x01,
  DMD_GUARD_INTERVAL_1_16    = 0x02,
  DMD_GUARD_INTERVAL_1_8     = 0x03,
  DMD_GUARD_INTERVAL_1_4     = 0x04,
  DMD_GUARD_INTERVAL_1_128   = 0x05,
  DMD_GUARD_INTERVAL_19_128  = 0x06,
  DMD_GUARD_INTERVAL_19_256  = 0x07
} dmd_guard_interval_t;

typedef enum
{
  DMD_HIERARCHY_NONE = 0x01,
  DMD_HIERARCHY_1    = 0x02,
  DMD_HIERARCHY_2    = 0x03,
  DMD_HIERARCHY_4    = 0x04
} dmd_hierarchy_t;

typedef enum
{
  DMD_HIERARCHY_HP = 0x00,
  DMD_HIERARCHY_LP = 0x01
} dmd_hierarchy_hplp_t;

typedef enum
{
  DMD_PLP_COMMON,
  DMD_PLP_DATA1,
  DMD_PLP_DATA2
} dmd_plp_type_t;

typedef enum
{
  DMD_DVBTYPE_DVBT,
  DMD_DVBTYPE_DVBT2
} dmd_terrestrial_dvbtype_t;

typedef enum
{
   DMD_SATELLITE   = 0x0100,
   DMD_CABLE 	   = 0x0200,
   DMD_TERRESTRIAL = 0x0300,
} dmd_device_type_t;

typedef enum
{
   TUNER_STATE_LOCKED,
   TUNER_STATE_TIMEOUT,
   TUNER_STATE_UNKNOW
} dmd_tuner_event_t;


typedef struct
{
  uint_t                  frequency;
  uint_t                  symbol_rate;
  dmd_modulation_system_t modulation_system;
  dmd_polarization_t      polarization;
  dmd_modulation_t        modulation;
  dmd_fec_rate_t          fec_rate;
  dmd_rolloff_t           roll_off;
  dmd_lnb_tone_state_t    lnb_tone_state;
  dmd_diseqc_port_t       diseqc_port;
  dmd_satellite_band_t    band;
} dmd_satellite_desc_t;

typedef struct
{
  uint_t               frequency;         /* frequency in KHz */
  dmd_fec_rate_t       fec_rate;          /* select FEC rate(s) */
  dmd_fec_rate_outer_t fec_rate_outer;    /* select FEC outer */
  dmd_modulation_t     modulation;        /* select modulation scheme */
  uint_t               symbol_rate;       /* symbol rate in KBAUD */
} dmd_cable_desc_t;

typedef struct
{
  uint_t                   frequency;
  dmd_constellation_t      constellation;
  dmd_hierarchy_t          hierarchy;
  dmd_hierarchy_hplp_t     hp_lp;
  dmd_fec_rate_t           HP_code_rate;
  dmd_fec_rate_t           LP_code_rate;
  dmd_guard_interval_t     guard_interval;
  dmd_transmission_mode_t  transmission_mode;
  dmd_bandwidth_t          bandwidth;
} dmd_terrestrial_dvbt_desc_t;

typedef struct
{
  uint_t                   frequency;
  uint_t                   plp_id;
  dmd_guard_interval_t     guard_interval;
  dmd_transmission_mode_t  transmission_mode;
  uint_t                   T2_system_id;
  dmd_bandwidth_t          bandwidth;
} dmd_terrestrial_dvbt2_desc_t;

typedef struct
{
  dmd_terrestrial_dvbtype_t dvb_type;
  union
  {
    dmd_terrestrial_dvbt_desc_t  dvbt;
    dmd_terrestrial_dvbt2_desc_t dvbt2;
  } desc;
} dmd_terrestrial_desc_t;


typedef struct
{
   dmd_device_type_t	device_type;
   union
   {
      dmd_satellite_desc_t	satellite;
      dmd_cable_desc_t		cable;
      dmd_terrestrial_desc_t terrestrial;
   } delivery;
} dmd_delivery_t;

int open_fend(int fe_idx, int *fe_id);
int dmd_lock_t(int fe_fd, const dmd_delivery_t * pDelivery);
int dmd_lock_c(int fe_fd, const dmd_delivery_t * pDelivery);
int dmd_lock_s(int fe_fd, const dmd_delivery_t * pDelivery, dmd_lnb_tone_state_t tone_state, dmd_lnb_voltage_t vol);
dmd_tuner_event_t get_dmd_lock_status(int frontend_fd);
int close_fend(int fe_id);
#endif
