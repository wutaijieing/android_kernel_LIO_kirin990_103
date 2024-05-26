/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _EMMC_PARTITION_H_
#define _EMMC_PARTITION_H_

#define BOOT_PARTITION_ENABLE_OFFSET 0x3
#define BOOT_PARTITION_ENABLE_MASK   0x7

void emmc_get_boot_partition_type(struct mmc_card *card);
int emmc_set_boot_partition_type(unsigned int boot_partition_type);

#endif
