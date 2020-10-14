/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _AML_MP_CAS_H_
#define _AML_MP_CAS_H_

#include "Common.h"

#define MAX_CHAN_COUNT (8)
#define MAX_DATA_LEN (8)
#define AML_MP_MAX_PATH_SIZE    512

/**\brief Service Mode of the program*/
typedef enum {
	AML_MP_CAS_SERVICE_DVB, /**< DTV live playing.*/
	AML_MP_CAS_SERVICE_IPTV /**< IPTV.*/
} Aml_MP_CASServiceMode;

/**\brief Service type of the program*/
typedef enum {
	AML_MP_CAS_SERVICE_LIVE_PLAY,     /**< Live playing.*/
	AML_MP_CAS_SERVICE_PVR_RECORDING, /**< PVR recording.*/
	AML_MP_CAS_SERVICE_PVR_PLAY,      /**< PVR playback.*/
	AML_MP_CAS_SERVICE_TYPE_INVALID,   /**< Invalid type.*/
} Aml_MP_CASServiceType;

typedef struct {
	uint16_t service_id;  /**< The service's index.*/
	uint8_t dmx_dev;      /**< The demux device's index.*/
	uint8_t dsc_dev;      /**< The descrmabler device's index.*/
	uint8_t dvr_dev;      /**< The DVR device's index.*/
	Aml_MP_CASServiceMode serviceMode; /**< Service mode.*/
	Aml_MP_CASServiceType serviceType; /**< Service type.*/
	uint16_t ecm_pid;     /**< ECM's PID.*/
	uint16_t stream_pids[MAX_CHAN_COUNT];  /**< Elementry streams' index.*/
	uint32_t stream_num;  /**< Elementary streams' number.*/
	uint8_t ca_private_data[MAX_DATA_LEN]; /**< Private data.*/
	uint8_t ca_private_data_len;           /**< Private data's length.*/
} Aml_MP_CASServiceInfo;

typedef enum {
    AML_MP_CAS_SECTION_PMT,
    AML_MP_CAS_SECTION_CAT,
    AML_MP_CAS_SECTION_NIT,
}Aml_MP_CASSectionType;

typedef struct {
   Aml_MP_DemuxId           dmxDev;
    uint16_t                serviceId;
    Aml_MP_CASSectionType   sectionType;
} Aml_MP_CASSectionReportAttr;

typedef int (*Aml_MP_CAS_EventCallback)(AML_MP_CASSESSION casSession, const char *json);

typedef enum {
    AML_MP_CAS_ENCRYPT, /**< Encrypt.*/
    AML_MP_CAS_DECRYPT  /**< Decrypt.*/
} Aml_MP_CASCryptoType;

typedef struct {
  Aml_MP_CASCryptoType  type;                        /**< Work type.*/
  char                  filepath[AML_MP_MAX_PATH_SIZE];                   /**< Location of the record file.*/
  int                   segmentId;                      /**< Current segment's index.*/
  int64_t               offset;                          /**< Current offset in the segment file.*/
  Aml_MP_Buffer         inputBuffer;                    /**< Input data buffer.*/
  Aml_MP_Buffer         outputBuffer;                   /**< Output data buffer.*/
  size_t                outputSize;                     /**< Output data size in bytes.*/
} Aml_MP_CASCryptoParams;

typedef struct Aml_MP_CASDVRReplayParams {
      Aml_MP_DemuxId dmxDev;      /**< The demux device's index.*/
} Aml_MP_CASDVRReplayParams;

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// global CAS function begin
int Aml_MP_CAS_Initialize();

int Aml_MP_CAS_Terminate();

int Aml_MP_CAS_IsNeedWholeSection();

int Aml_MP_CAS_IsSystemIdSupported(int caSystemId);

int Aml_MP_CAS_ReportSection(Aml_MP_CASSectionReportAttr* pAttr, uint8_t* data, size_t len);

int Aml_MP_CAS_SetEmmPid(int dmx_dev, uint16_t emmPid);

int Aml_MP_CAS_DVREncrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoPara);

int Aml_MP_CAS_DVRDecrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoPara);
// global CAS function end

int Aml_MP_CAS_OpenSession(AML_MP_CASSESSION* casSession);

int Aml_MP_CAS_CloseSession(AML_MP_CASSESSION casSession);

int Aml_MP_CAS_RegisterEventCallback(AML_MP_CASSESSION casSession, Aml_MP_CAS_EventCallback cb, void* userData);

//for live
int Aml_MP_CAS_StartDescrambling(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo* serviceInfo);

int Aml_MP_CAS_StopDescrambling(AML_MP_CASSESSION casSession);

int Aml_MP_CAS_UpdateDescramblingPid(AML_MP_CASSESSION casSession, int oldStreamPid, int newStreamPid);

//for dvr record
int Aml_MP_CAS_StartDVRRecord(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo *serviceInfo);

int Aml_MP_CAS_StopDVRRecord(AML_MP_CASSESSION casSession);

//for dvr replay
int Aml_MP_CAS_StartDVRReplay(AML_MP_CASSESSION casSession, Aml_MP_CASDVRReplayParams *dvrReplayParams);

int Aml_MP_CAS_StopDVRReplay(AML_MP_CASSESSION casSession);

AML_MP_SECMEM Aml_MP_CAS_CreateSecmem(AML_MP_CASSESSION casSession, Aml_MP_CASServiceType type, void **pSecbuf, uint32_t *size);

int Aml_MP_CAS_DestroySecmem(AML_MP_CASSESSION casSession, AML_MP_SECMEM secMem);

#ifdef __cplusplus
}
#endif
#endif
