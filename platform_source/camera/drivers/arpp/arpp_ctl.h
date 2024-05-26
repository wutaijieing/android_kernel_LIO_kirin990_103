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
#ifndef _ARPP_CTL_H_
#define _ARPP_CTL_H_

#include <linux/types.h>

struct buf_base {
	int fd;
	uint64_t size;
	uint64_t prot;
};

struct hwacc_ini_info {
	struct buf_base cmdlist_buf;
	struct buf_base inout_buf;
	struct buf_base swap_buf;
	struct buf_base free_buf;
	struct buf_base out_buf;
};

struct arpp_lba_info {
	uint32_t num_kf;
	uint32_t num_ff;
	uint32_t num_mp;
	uint32_t num_edge;
	uint32_t num_edge_idp_dc;
	uint32_t num_edge_idp_fc;
	uint32_t num_edge_idp_sc;
	uint32_t num_edge_idp_nc;
	uint32_t num_edge_kf;
	uint32_t num_param;
	uint32_t num_eff_param;
	uint32_t num_residual;
	uint32_t num_max_iter;
	uint32_t jac_scl;
	uint32_t norm_step;
	uint32_t max_norm_step;
	uint32_t max_invld_step;
	uint32_t num_res_blk;
	uint32_t num_param_blk;
	uint32_t max_retries;
	uint32_t res;
	uint32_t num_edge_relpos;
	/* 2 elements represent low address and high address */
	uint32_t init_prv_loss[2];
	uint32_t init_bias_loss[2];
	uint32_t init_idp_loss[2];
	uint32_t grad_tol[2];
	uint32_t param_tol[2];
	uint32_t lose_func_tol[2];
	uint32_t min_rel_dec[2];
	uint32_t min_trust_rgn_rad[2];
	uint32_t init_radius[2];
	uint32_t max_radius[2];
	uint32_t min_diagonal[2];
	uint32_t max_diagonal[2];
	uint32_t diagonal_factor[2];
	uint32_t fixed_cost_val[2];
	/* Camera Intrinsics */
	uint32_t calib_int[8];
	/* Camera Extrinsics */
	uint32_t calib_ext[24];
	uint32_t param_offset;
	uint32_t idp_dc_offset;
	uint32_t idp_fc_offset;
	uint32_t idp_sc_offset;
	uint32_t idp_nc_offset;
	uint32_t prv_offset;
	uint32_t bias_offset;
	uint32_t edge_vertex_prvb_offset;
	uint32_t edge_vertex_idp_dc_offset;
	uint32_t edge_vertex_idp_fc_offset;
	uint32_t edge_vertex_idp_sc_offset;
	uint32_t edge_vertex_idp_nc_offset;
	uint32_t jbr_offset_dc_offset;
	uint32_t jbr_offset_fc_offset;
	uint32_t jbr_offset_sc_offset;
	uint32_t jbr_offset_nc_offset;
	uint32_t jbr_offset2_fc_offset;
	uint32_t jbr_offset1_sc_offset;
	uint32_t jbr_offset1_nc_offset;
	uint32_t jbr_offset2_nc_offset;
	uint32_t is_const_offset;
	uint32_t offset_param_offset;
	uint32_t eff_offset_param_offset;
	uint32_t msk_hbl_offset;
	uint32_t mp_stat_fc_offset;
	uint32_t mp_stat_sc_offset;
	uint32_t mp_stat_nc_offset;
	uint32_t edge_vertex_relpos_offset;
	uint32_t param_prior_offset;
	uint32_t param_vel_offset;
	uint32_t param_grav_offset;
	uint32_t param_relpos_offset;
	uint32_t param_abspos_offset;
	uint32_t m2_dc_pcb0_offset;
	uint32_t m2_fc_pcb0_offset;
	uint32_t m2_sc_pcb0_offset;
	uint32_t m2_nc_pcb0_offset;
	uint32_t m2_dc_pcbi_offset;
	uint32_t m2_fc_pcbi_offset;
	uint32_t m2_sc_pcbi_offset;
	uint32_t m2_nc_pcbi_offset;
	uint32_t m2_dc_rcb0_offset;
	uint32_t m2_fc_rcb0_offset;
	uint32_t m2_sc_rcb0_offset;
	uint32_t m2_nc_rcb0_offset;
	uint32_t m2_dc_rcbi_offset;
	uint32_t m2_fc_rcbi_offset;
	uint32_t m2_sc_rcbi_offset;
	uint32_t m2_nc_rcbi_offset;
};

#define ARPP_CTRL_MAGIC             'R'
#define ARPP_CTRL_MAX_NR            200
#define ARPP_IOCTL_POWER_UP         _IO(ARPP_CTRL_MAGIC, 0xA0)
#define ARPP_IOCTL_POWER_DOWN       _IO(ARPP_CTRL_MAGIC, 0xA1)
#define ARPP_IOCTL_LOAD_ENGINE      _IOW(ARPP_CTRL_MAGIC, 0xA2, struct hwacc_ini_info)
#define ARPP_IOCTL_UNLOAD_ENGINE    _IO(ARPP_CTRL_MAGIC, 0xA3)
#define ARPP_IOCTL_DO_LBA           _IOW(ARPP_CTRL_MAGIC, 0xA4, struct arpp_lba_info)
#define ARPP_IOCTL_SET_CLK_LEVEL    _IOW(ARPP_CTRL_MAGIC, 0xA5, int)

#endif /* _ARPP_CTL_H_ */
