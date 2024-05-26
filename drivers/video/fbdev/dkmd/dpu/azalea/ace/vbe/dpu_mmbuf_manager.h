/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPU_MMBUF_MANAGER_H
#define DPU_MMBUF_MANAGER_H

#include "dpu_fb.h"

/*******************************************************************************
 *  mmbuf manager struct config
 */
extern struct gen_pool *g_mmbuf_gen_pool;
extern uint32_t mmbuf_max_size;
extern dss_mmbuf_t dss_mmbuf_reserved_info[RESERVED_SERVICE_MAX];
extern struct dss_comm_mmbuf_info g_primary_online_mmbuf[DSS_CHN_MAX_DEFINE];
extern struct dss_comm_mmbuf_info g_external_online_mmbuf[DSS_CHN_MAX_DEFINE];

void dpu_mmbuf_info_init(void);
int dpu_mmbuf_reserved_size_query(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpu_mmbuf_request(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpu_mmbuf_release(struct dpu_fb_data_type *dpufd, const void __user *argp);
int dpu_mmbuf_free_all(struct dpu_fb_data_type *dpufd, const void __user *argp);
void *dpu_mmbuf_init(struct dpu_fb_data_type *dpufd);
void dpu_mmbuf_deinit(struct dpu_fb_data_type *dpufd);
uint32_t dpu_mmbuf_alloc(void *handle, uint32_t size);
void dpu_mmbuf_free(void *handle, uint32_t addr, uint32_t size);
void dpu_mmbuf_info_clear(struct dpu_fb_data_type *dpufd, int idx);
dss_mmbuf_info_t *dpu_mmbuf_info_get(struct dpu_fb_data_type *dpufd, int idx);
void dpu_mmbuf_info_get_online(struct dpu_fb_data_type *dpufd);
void dpu_mmbuf_mmbuf_free_all(struct dpu_fb_data_type *dpufd);
void dpu_check_use_comm_mmbuf(uint32_t display_id,
int *use_comm_mmbuf, dss_mmbuf_t *offline_mmbuf, bool has_rot);
int dpu_rdma_set_mmbuf_base_and_size(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	dss_rect_ltrb_t *afbc_rect, uint32_t *mm_base_0, uint32_t *mm_base_1);

void dpu_mmbuf_sem_pend(void);
void dpu_mmbuf_sem_post(void);

#endif  /* DPU_MIPI_DSI_H */
