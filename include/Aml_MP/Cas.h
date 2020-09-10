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

#include <Aml_MP/Common.h>

typedef void (*Aml_MP_CAS_EventCallback)(int event, void* privateData, void* userData);

///////////////////////////////////////////////////////////////////////////////
int Aml_MP_CAS_CreatePlugin(Aml_MP_CASType casType, AML_MP_HANDLE* handle);

int Aml_MP_CAS_DestroyPlugin(AML_MP_HANDLE handle);

int Aml_MP_CAS_OpenSession(AML_MP_HANDLE handle);

int Aml_MP_CAS_CloseSession(AML_MP_HANDLE handle);

int Aml_MP_CAS_SetPrivateData(AML_MP_HANDLE handle, const uint8_t* data, size_t size);

int Aml_MP_CAS_ProcessEcm(AML_MP_HANDLE handle, const uint8_t* data, size_t size);

int Aml_MP_CAS_ProcessEmm(AML_MP_HANDLE handle, const uint8_t* data, size_t size);

int Aml_MP_CAS_RegisterEventCallback(AML_MP_HANDLE handle, Aml_MP_CAS_EventCallback cb, void* userData);

#endif
