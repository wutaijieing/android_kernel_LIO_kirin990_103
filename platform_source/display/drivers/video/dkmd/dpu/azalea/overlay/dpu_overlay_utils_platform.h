/* Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#ifndef DPU_OVERLAY_UTILS_PLATFORM_H
#define DPU_OVERLAY_UTILS_PLATFORM_H

#include "../dpu_fb.h"

void dpufb_dss_disreset(struct dpu_fb_data_type *dpufd);
void dpu_dump_current_info(struct dpu_fb_data_type *dpufd);
void dpu_qos_on(struct dpu_fb_data_type *dpufd);
int dpu_overlay_pan_display(struct dpu_fb_data_type *dpufd);
int dpu_ov_online_play(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpu_ov_offline_play(struct dpu_fb_data_type *dpufd, const void __user *argp);
void dpu_ovl_init(const char __iomem *ovl_base, dss_ovl_t *s_ovl, int ovl_idx);
void dpu_ovl_set_reg(struct dpu_fb_data_type *dpufd, char __iomem *ovl_base, dss_ovl_t *s_ovl, int ovl_idx);
void dpu_ov_set_reg_default_value(struct dpu_fb_data_type *dpufd,
	char __iomem *ovl_base, int ovl_idx);
int dpu_check_pure_layer(struct dpu_fb_data_type *dpufd, dss_overlay_block_t *pov_h_block,
	void __user *argp);
int dpu_check_userdata(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_overlay_block_t *pov_h_block_infos);
int dpu_check_layer_par(struct dpu_fb_data_type *dpufd, dss_layer_t *layer);
int dpu_aif_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block);
void dpu_aif_init(const char __iomem *aif_ch_base,	dss_aif_t *s_aif);
void dpu_aif_ch_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *aif_ch_base, dss_aif_t *s_aif);
int dpu_aif_ch_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_rect_t *wb_dst_rect, dss_wb_layer_t *wb_layer);
int dpu_aif1_ch_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, int ovl_idx);
int dpu_hfbcd_config(struct dpu_fb_data_type *dpufd,
	int ovl_idx, dss_layer_t *layer, struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);
void dpu_hfbcd_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma);
int dpu_hfbce_config(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *layer, dss_rect_t aligned_rect, dss_rect_t *ov_block_rect, bool last_block);
void dpu_hfbce_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *wdma_base, dss_wdma_t *s_wdma);
int dpu_hebce_config(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *layer, dss_rect_t aligned_rect, dss_rect_t *ov_block_rect, bool last_block);
void dpu_hebce_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *wdma_base, dss_wdma_t *s_wdma);
int dpu_hebcd_config(struct dpu_fb_data_type *dpufd, int ovl_idx, dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);
void dpu_hebcd_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma);
int dpu_wb_scl_config(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer);
int dpu_post_scl_load_filter_coef(struct dpu_fb_data_type *dpufd, bool enable_cmdlist,
	char __iomem *scl_lut_base, int coef_lut_idx);
int dpu_post_scf_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
void dpu_csc_init(const char __iomem *csc_base, dss_csc_t *s_csc);
void dpu_csc_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *csc_base, dss_csc_t *s_csc);
int dpu_csc_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	dss_wb_layer_t *wb_layer);
void dpu_mctl_sys_init(const char __iomem *mctl_sys_base, dss_mctl_sys_t *s_mctl_sys);
int dpu_sharpness_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer);
void dpu_post_clip_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_clip_base, dss_post_clip_t *s_post_clip, int chn_idx);
int dpu_ce_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer);
void dpu_secure_layer_check_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
void dpu_rch2_ce_end_handle_func(struct work_struct *work);
void dpu_rch4_ce_end_handle_func(struct work_struct *work);
void dpu_dpp_acm_ce_end_handle_func(struct work_struct *work);
void dpu_sec_mctl_set_regs(struct dpu_fb_data_type *dpufd);
void dpu_drm_layer_online_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req_prev,
	dss_overlay_t *pov_req);
void dpu_drm_layer_online_clear(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req_prev,
	const int *seclist, int list_max);
void dpu_drm_layer_offline_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
void dpu_drm_layer_offline_clear(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
void dpu_mctl_ov_set_ctl_dbg_reg(struct dpu_fb_data_type *dpufd, char __iomem *mctl_base, bool enable_cmdlist);
uint32_t dpu_mif_get_invalid_sel(dss_img_t *img, uint32_t transform, int v_scaling_factor,
	uint8_t is_tile, bool rdma_stretch_enable);
#if !defined(CONFIG_DPU_FB_V320) && !defined(CONFIG_DPU_FB_V330)
int dpu_arsr1p_write_lsc_gain(struct dpu_fb_data_type *dpufd, bool enable_cmdlist,
	char __iomem *addr, const uint32_t **p, int row, int col);
#if !defined(CONFIG_DPU_FB_V501)
int dpu_arsr1p_write_coefs(struct dpu_fb_data_type *dpufd, bool enable_cmdlist,
	char __iomem *addr, const int **p, int row, int col);
#endif
void dpu_post_scf_init(const char __iomem *dss_base, const char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf);
void dpu_post_scf_set_reg(struct dpu_fb_data_type *dpufd, char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf);
#endif

/* arsr2p interface */
void dpu_arsr2p_init(const char __iomem *arsr2p_base, struct dss_arsr2p *s_arsr2p);  /* arsr2p init */
void dpu_arsr2p_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *arsr2p_base, struct dss_arsr2p *s_arsr2p);   /* arsr2p set reg */
void dpu_arsr2p_coef_on(struct dpu_fb_data_type *dpufd, bool enable_cmdlist);  /* lut coef */
int dpu_arsr2p_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	dss_rect_t *aligned_rect, bool rdma_stretch_enable);  /* arsr2p module config */
void dpu_remove_mctl_mutex(struct dpu_fb_data_type *dpufd, int mctl_idx, uint32_t cmdlist_idxs);
void dpu_dpp_acm_gm_set_reg(struct dpu_fb_data_type *dpufd);

#if defined(CONFIG_DPU_FB_V320) || defined(CONFIG_DPU_FB_V330)
void dpu_post_scf_set_reg(struct dpu_fb_data_type *dpufd, char __iomem *post_scf_base, dss_scl_t *s_post_scf);
void dpu_post_scf_init(char __iomem *dss_base, const char __iomem *post_scf_base, dss_scl_t *s_post_scf);
#endif
#ifndef CONFIG_DPU_FB_V410
/* CONFIG_SH_AOD_ENABLE */
void clear_xcc_table(struct dpu_fb_data_type *dpufd);
void restore_xcc_table(struct dpu_fb_data_type *dpufd);
#endif

void dpufb_masklayer_backlight_notify_handler(struct work_struct *work);

#if defined(CONFIG_DPU_FB_V510) || defined(CONFIG_DPU_FB_V350) || \
	defined(CONFIG_DPU_FB_V320) || defined(CONFIG_DPU_FB_V600) || \
	defined(CONFIG_DPU_FB_V345) || defined(CONFIG_DPU_FB_V346) || \
	defined(CONFIG_DPU_FB_V360)
void dpu_arsr_post_coef_on(struct dpu_fb_data_type *dpufd);
#endif

/* This api will pause send data to LCD which will freeze the display, be careful to use it */
int dpu_online_play_bypass(struct dpu_fb_data_type *dpufd, const void __user *argp);
bool dpu_online_play_bypass_set(struct dpu_fb_data_type *dpufd, int bypass);
bool dpu_online_play_bypass_check(struct dpu_fb_data_type *dpufd);
int dpu_get_release_and_retire_fence(struct dpu_fb_data_type *dpufd, void __user *argp);
void dpufb_mask_layer_backlight_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req_prev,
	dss_overlay_t *pov_req, bool *masklayer_maxbacklight_flag);
void dpufb_dc_backlight_config(struct dpu_fb_data_type *dpufd);
int dpufb_wait_for_mipi_resource_available(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);


#endif /* DPU_OVERLAY_UTILS_PLATFORM_H */
