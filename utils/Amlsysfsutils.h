/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#ifndef AMSYSFS_UTILS_MP_H_
#define AMSYSFS_UTILS_MP_H_

namespace aml_mp {

int amsysfs_set_sysfs_str(const char *path, const char *val);
int amsysfs_get_sysfs_str(const char *path, char *valstr, unsigned size);
int amsysfs_set_sysfs_int(const char *path, int val);
int amsysfs_get_sysfs_int(const char *path);
int amsysfs_set_sysfs_int16(const char *path, int val);
int amsysfs_get_sysfs_int16(const char *path);
unsigned long amsysfs_get_sysfs_ulong(const char *path);

}

#endif

