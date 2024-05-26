/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#ifndef DPUFE_MMBUF_MANAGER_H
#define DPUFE_MMBUF_MANAGER_H

#include "dpufe.h"

/*
 *  mmbuf manager struct config
 */
int dpufe_mmbuf_request(struct fb_info *info, void __user *argp);
int dpufe_mmbuf_release(struct fb_info *info, const void __user *argp);
int dpufe_mmbuf_free_all(struct fb_info *info, const void __user *argp);
void *dpufe_mmbuf_init(struct dpufe_data_type *dfd);
void dpufe_mmbuf_deinit(struct dpufe_data_type *dfd);
uint32_t dpufe_mmbuf_alloc(void *handle, uint32_t size);
void dpufe_mmbuf_free(void *handle, uint32_t addr, uint32_t size);
void dpufe_mmbuf_mmbuf_free_all(struct dpufe_data_type *dfd);
void dpufe_mmbuf_sem_pend(void);
void dpufe_mmbuf_sem_post(void);
#endif
