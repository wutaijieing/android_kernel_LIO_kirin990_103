/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef DPU_FB_ADAPT_H
#define DPU_FB_ADAPT_H

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define sys_open  ksys_open
#define sys_close ksys_close
#define sys_lseek ksys_lseek
#define sys_read  ksys_read
#endif

#endif /* DPU_FB_ADAPT_H */
