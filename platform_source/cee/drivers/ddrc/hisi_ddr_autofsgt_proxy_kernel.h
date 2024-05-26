/*
 * hisi_ddr_autofsgt_proxy_kernel.h
 *
 * autofsgt proxy kernel for ddr
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef __HISI_DDR_AUTOFSGT_PROXY_KERNEL_H__
#define __HISI_DDR_AUTOFSGT_PROXY_KERNEL_H__

#include <ddr_define.h>

int ddr_autofsgt_ctrl(unsigned int client, unsigned int cmd);

#endif /* __HISI_DDR_AUTOFSGT_PROXY_KERNEL_H__ */
