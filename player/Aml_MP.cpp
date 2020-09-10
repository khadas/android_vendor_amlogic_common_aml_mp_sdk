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

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMp"
#include <Aml_MP/Aml_MP.h>
#include <am_cas.h>

///////////////////////////////////////////////////////////////////////////////
#define AML_MP_VERSION_MAJOR   1
#define AML_MP_VERSION_MINOR   0

#define AML_MP_VERSION_INT     AML_MP_VERSION_MAJOR << 16 | AML_MP_VERSION_MINOR
#define AML_MP_VERSION         "AML_MP v" AML_MP_STRINGIFY(AML_MP_VERSION_GLUE(AML_MP_VERSION_MAJOR, AML_MP_VERSION_MINOR))

#define AML_MP_VERSION_GLUE(a, b)   AML_MP_VERSION_DOT(a, b)
#define AML_MP_VERSION_DOT(a, b)   a ## . ## b
#define AML_MP_STRINGIFY(x)    AML_MP_TOSTRING(x)
#define AML_MP_TOSTRING(x)     #x

///////////////////////////////////////////////////////////////////////////////

int Aml_MP_Initialize()
{
    CasHandle casHandle;

#if !defined (__ANDROID_VNDK__)
    AM_CA_Init(&casHandle);
#endif

    return 0;
}

int Aml_MP_GetVersion(const char** versionString)
{
    if (versionString) {
        *versionString = AML_MP_VERSION;
    }

    return AML_MP_VERSION_INT;
}

int Aml_MP_IsCASystemIdSupported(int caSystemId)
{
#if !defined (__ANDROID_VNDK__)
    return AM_CA_IsSystemIdSupported(caSystemId);
#else
    return false;
#endif
}

