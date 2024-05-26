/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _ARPP_SMMU_H_
#define _ARPP_SMMU_H_

#include <linux/device.h>

#include "arpp_ctl.h"
#include "platform.h"

union smmu_tbu_scr {
	unsigned int value;
	struct {
		unsigned int ns_uarch        : 1;
		unsigned int hazard_dis      : 1;
		unsigned int mtlb_hitmap_dis : 1;
		unsigned int tbu_bypass      : 1;
		unsigned int dummy_unlock_en : 1;
		unsigned int tlb_inv_sel     : 1;
		unsigned int reserved        : 26;
	} reg;
};

union smmu_tbu_cr {
	unsigned int value;
	struct {
		unsigned int tbu_en_req           : 1;
		unsigned int clk_gt_ctrl          : 2;
		unsigned int pref_qos_level_en    : 1;
		unsigned int pref_qos_level       : 4;
		unsigned int max_tok_trans        : 8;
		unsigned int fetchslot_full_level : 6;
		unsigned int reserved             : 2;
		unsigned int prefslot_full_level  : 6;
		unsigned int trans_hazard_size    : 2;
	} reg;
};

union smmu_tbu_crack {
	unsigned int value;
	struct {
		unsigned int tbu_en_ack    : 1;
		unsigned int tbu_connected : 1;
		unsigned int reserved_0    : 6;
		unsigned int tok_trans_gnt : 8;
		unsigned int reserved_1    : 16;
	} reg;
};

union rw_channel_attr {
	unsigned int value;
	struct {
		unsigned int rch_sid        : 8;
		unsigned int reserved       : 4;
		unsigned int rch_ssid       : 8;
		unsigned int rch_ssidv      : 1;
		unsigned int reserved_0     : 3;
		unsigned int rch_sc_hint    : 4;
		unsigned int rch_sc_hint_en : 1;
		unsigned int rch_ptl_as_ful : 1;
		unsigned int reserved_1     : 2;
	} reg;
};

struct mapped_buffer {
	struct buf_base base_buf;
	unsigned long iova;
	unsigned long mapped_size;
};

struct lba_buffer_info {
	struct mapped_buffer inout_buf;
	struct mapped_buffer swap_buf;
	struct mapped_buffer free_buf;
	struct mapped_buffer out_buf;
};

int smmu_enable(struct arpp_device *arpp_dev);
void smmu_disable(struct arpp_device *arpp_dev);
int map_buffer_from_user(struct device *dev,
	struct lba_buffer_info *buf_addr);
void unmap_buffer_from_user(struct device *dev,
	struct lba_buffer_info *buf_addr);
int map_cmdlist_buffer(struct device *dev,
	struct mapped_buffer *cmdlist_buf);
void unmap_cmdlist_buffer(struct device *dev,
	struct mapped_buffer *cmdlist_buf);

#endif /* _ARPP_SMMU_H_ */
