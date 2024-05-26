/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * bl31 route to kernel with spi functions moudle header
 *
 * dfx_bl31_route_handler.h
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

#ifndef __BB_KLOG_H__
#define __BB_KLOG_H__
extern asmlinkage void fiq_dump(struct pt_regs *regs, unsigned int esr);
extern void dfx_mntn_inform(void);
#endif