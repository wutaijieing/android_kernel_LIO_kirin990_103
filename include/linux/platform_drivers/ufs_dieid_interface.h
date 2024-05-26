/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * ufs-dieid_interface.h
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _UFS_DIEID_INTERFACE_H_
#define _UFS_DIEID_INTERFACE_H_

#ifdef CONFIG_PLATFORM_DIEID
#ifdef CONFIG_SCSI_UFS_HI1861_VCMD
int hufs_get_dieid(char *dieid, unsigned int len);
#else
static inline int hufs_get_dieid(char *dieid, unsigned int len)
{
	return -1;
}
#endif
#endif
#endif

