/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver video path reg header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_VIDEO_PATH_REG_H__
#define __HDMITX_VIDEO_PATH_REG_H__

#define MAX_CBAR_PATTERN_NUM 8

/* video_path_reg module field info */
#define REG_TIMING_GEN_CTRL       0x800
#define reg_timing_gen_en_8ppc(x) (((x) & 1) << 16) /* [16] */
#define REG_TIMING_GEN_EN_8PPC_M  (1 << 16)
#define reg_sync_polarity(x)      (((x) & 1) << 8) /* [9:8] */
#define REG_SYNC_POLARITY_M       (1 << 8)
#define reg_timing_sel(x)         (((x) & 0x3f) << 2) /* [7:2] */
#define REG_TIMING_SEL_M          (0x3f << 2)
#define reg_extmode(x)            (((x) & 1) << 1) /* [1] */
#define REG_EXTMODE_M             (1 << 1)
#define reg_timing_gen_en(x)      (((x) & 1) << 0) /* [0] */
#define REG_TIMING_GEN_EN_M       (1 << 0)

#define REG_HSYNC_TIMING_CONFIG0 0x804
#define reg_h_front(x) (((x) & 0xffff) << 16) /* [31:16] */
#define REG_H_FRONT_M  (0xffff << 16)
#define reg_h_sync(x)  ((x) & 0xffff) /* [15:0] */
#define REG_H_SYNC_M   (0xffff << 0)

#define REG_HSYNC_TIMING_CONFIG1 0x808
#define reg_h_back(x)   (((x) & 0xffff) << 16) /* [31:16] */
#define REG_H_BACK_M    (0xffff << 16)
#define reg_h_active(x) ((x) & 0xffff) /* [15:0] */
#define REG_H_ACTIVE_M  (0xffff << 0)

#define REG_VSYNC_TIMING_CONFIG0 0x810
#define reg_v_front(x) (((x) & 0xffff) << 16) /* [31:16] */
#define REG_V_FRONT_M  (0xffff << 16)
#define reg_v_sync(x)  ((x) & 0xffff) /* [15:0] */
#define REG_V_SYNC_M   (0xffff << 0)

#define REG_VSYNC_TIMING_CONFIG1 0x814
#define reg_v_back(x)   (((x) & 0xffff) << 16) /* [31:16] */
#define REG_V_BACK_M    (0xffff << 16)
#define reg_v_active(x) ((x) & 0xffff) /* [15:0] */
#define REG_V_ACTIVE_M  (0xffff << 0)

#define REG_VID_ADAPTER        0x834
#define reg_vid_in_422_mode(x) (((x) & 1) << 1) /* [1] */
#define REG_VID_IN_422_MODE_M  (1 << 1)
#define reg_vid_in_dsc_mode(x) (((x) & 1) << 0) /* [0] */
#define REG_VID_IN_DSC_MODE_M  (1 << 0)

#define REG_PATTERN_GEN_CTRL       0x840
#define reg_tpg_enable_8ppc(x)     (((x) & 1) << 16) /* [16] */
#define REG_TPG_ENABLE_8PPC_M      (1 << 16)
#define reg_cbar_pattern_sel(x)    (((x) & 0x3) << 13) /* [14:13] */
#define REG_CBAR_PATTERN_SEL_M     (0x3 << 13)
#define reg_bar_pattern_extmode(x) (((x) & 1) << 12) /* [12] */
#define REG_BAR_PATTERN_EXTMODE_M  (1 << 12)
#define reg_replace_pattern_en(x)  (((x) & 0x7) << 9) /* [11:9] */
#define REG_REPLACE_PATTERN_EN_M   (0x7 << 9)
#define reg_mask_pattern_en(x)     (((x) & 0x7) << 6) /* [8:6] */
#define REG_MASK_PATTERN_EN_M      (0x7 << 6)
#define reg_square_pattern_en(x)   (((x) & 1) << 5) /* [5] */
#define REG_SQUARE_PATTERN_EN_M    (1 << 5)
#define reg_colorbar_en(x)         (((x) & 1) << 4) /* [4] */
#define REG_COLORBAR_EN_M          (1 << 4)
#define reg_solid_pattern_en(x)    (((x) & 1) << 3) /* [3] */
#define REG_SOLID_PATTERN_EN_M     (1 << 3)
#define reg_tpg_enable(x)          (((x) & 1) << 0) /* [0] */
#define REG_TPG_ENABLE_M           (1 << 0)

#define REG_SOLID_PATTERN_CONFIG 0x844
#define reg_solid_pattern_cr(x)  (((x) & 0x3ff) << 20) /* [29:20] */
#define REG_SOLID_PATTERN_CR_M   (0x3ff << 20)
#define reg_solid_pattern_y(x)   (((x) & 0x3ff) << 10) /* [19:10] */
#define REG_SOLID_PATTERN_Y_M    (0x3ff << 10)
#define reg_solid_pattern_cb(x)  ((x) & 0x3ff)         /* [9:0] */
#define REG_SOLID_PATTERN_CB_M   (0x3ff << 0)

#define REG_MASK_PATTERN_CONFIG 0x848
#define reg_mask_pattern_cr(x)  (((x) & 0x3ff) << 20) /* [29:20] */
#define REG_MASK_PATTERN_CR_M   (0x3ff << 20)
#define reg_mask_pattern_y(x)   (((x) & 0x3ff) << 10) /* [19:10] */
#define REG_MASK_PATTERN_Y_M    (0x3ff << 10)
#define reg_mask_pattern_cb(x)  ((x) & 0x3ff) /* [9:0] */
#define REG_MASK_PATTERN_CB_M   (0x3ff << 0)

#define REG_BAR_EXT_CONFIG    0x84C
#define reg_square_height(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_SQUARE_HEIGHT_M   (0xffff << 16)
#define reg_colorbar_width(x) ((x) & 0xffff) /* [15:0] */
#define REG_COLORBAR_WIDTH_M  (0xffff << 0)

/* self-define this reg to save vic */
#define REG_VIC               0x0850
#define reg_cbar_pattern_a(n) (0x850 + (n) * 0x4)
#define reg_bar_pattern_a(x)  (((x) & 0x3fffffff) << 0) /* [29:0] */
#define REG_BAR_PATTERN_A_M   (0x3fffffff << 0)

#define reg_cbar_pattern_b(n) (0x870 + (n) * 0x4)
#define reg_bar_pattern_b(x)  (((x) & 0x3fffffff) << 0) /* [29:0] */
#define REG_BAR_PATTERN_B_M   (0x3fffffff << 0)

#define REG_FORMAT_DET_CONFIG       0x8B8
#define reg_pixel_cnt_threhold(x)   (((x) & 0xf) << 4) /* [7:4] */
#define REG_PIXEL_CNT_THREHOLD_M    (0xf << 4)
#define reg_fdt_status_clear(x)     (((x) & 1) << 3) /* [3] */
#define REG_FDT_STATUS_CLEAR_M      (1 << 3)
#define reg_vsync_polarity_value(x) (((x) & 1) << 2) /* [2] */
#define REG_VSYNC_POLARITY_VALUE_M  (1 << 2)
#define reg_hsync_polarity_value(x) (((x) & 1) << 1) /* [1] */
#define REG_HSYNC_POLARITY_VALUE_M  (1 << 1)
#define reg_sync_polarity_force(x)  (((x) & 1) << 0) /* [0] */
#define REG_SYNC_POLARITY_FORCE_M   (1 << 0)

#define REG_FDET_STATUS    0x8BC
#define REG_VSYNC_POLARITY (1 << 2) /* [2] */
#define REG_HSYNC_POLARITY (1 << 1) /* [1] */
#define REG_INTERLACED     (1 << 0) /* [0] */

#define REG_FDET_HORI_RES       0x8C0
#define reg_hsync_total_cnt(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_HSYNC_TOTAL_CNT_M   (0xffff << 16)
#define reg_hsync_active_cnt(x) ((x) & 0xffff) /* [15:0] */
#define REG_HSYNC_ACTIVE_CNT_M  (0xffff << 0)

#define REG_FDET_VERT_RES       0x8C4
#define reg_vsync_total_cnt(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_VSYNC_TOTAL_CNT_M   (0xffff << 16)
#define reg_vsync_active_cnt(x) ((x) & 0xffff) /* [15:0] */
#define REG_VSYNC_ACTIVE_CNT_M  (0xffff << 0)

#define REG_PXL_CAP_CTRL       0x90C
#define reg_cap_stat_error(x)  (((x) & 1) << 6) /* [6] */
#define REG_CAP_STAT_ERROR_M   (1 << 6)
#define reg_cap_stat_busy(x)   (((x) & 1) << 5) /* [5] */
#define REG_CAP_STAT_BUSY_M    (1 << 5)
#define reg_cap_stat_done(x)   (((x) & 1) << 4) /* [4] */
#define REG_CAP_STAT_DONE_M    (1 << 4)
#define reg_show_point_en(x)   (((x) & 1) << 2) /* [2] */
#define REG_SHOW_POINT_EN_M    (1 << 2)
#define reg_soft_trigger_en(x) (((x) & 1) << 1) /* [1] */
#define REG_SOFT_TRIGGER_EN_M  (1 << 1)
#define reg_auto_trigger_en(x) (((x) & 1) << 0) /* [0] */
#define REG_AUTO_TRIGGER_EN_M  (1 << 0)

#define REG_PXL_CAP_POSITION      0x910
#define reg_cap_line_position(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_CAP_LINE_POSITION_M   (0xffff << 16)
#define reg_cap_pixel_position(x) ((x) & 0xffff) /* [15:0] */
#define REG_CAP_PIXEL_POSITION_M  (0xffff << 0)

#define REG_CAP_Y_VALUE        0x914
#define reg_capture_y_value(x) ((x) & 0xfff) /* [11:0] */
#define REG_CAPTURE_Y_VALUE_M  (0xfff << 0)

#define REG_CAP_CB_VALUE        0x918
#define reg_capture_cb_value(x) ((x) & 0xfff) /* [11:0] */

#define REG_CAP_CR_VALUE        0x91C
#define reg_capture_cr_value(x) ((x) & 0xfff) /* [11:0] */
#define REG_CAPTURE_CR_VALUE_M  (0xfff << 0)

#define REG_VIDEO_DMUX_CTRL 0x9A8
#define reg_inver_sync(x)   (((x) & 0xf) << 25) /* [28:25] */
#define REG_INVER_SYNC_M    (0xf << 25)
#define REG_DE_M            (0x1 << 28)

#define REG_TX_PACK_FIFO_CTRL   0xA00
#define reg_fifo_delay_cnt(x)   (((x) & 0xff) << 8) /* [15:8] */
#define REG_FIFO_DELAY_CNT_M    (0xff << 8)
#define reg_tmds_delay_sel(x)   (((x) & 1) << 6) /* [6] */
#define REG_TMDS_DELAY_SEL_M    (1 << 6)
#define reg_ext_tmds_para(x)    (((x) & 1) << 5) /* [5] */
#define REG_EXT_TMDS_PARA_M     (1 << 5)
#define reg_clock_det_en(x)     (((x) & 1) << 4) /* [4] */
#define REG_CLOCK_DET_EN_M      (1 << 4)
#define reg_fifo_manu_rst(x)    (((x) & 1) << 3) /* [3] */
#define REG_FIFO_MANU_RST_M     (1 << 3)
#define reg_fifo_auto_rst_en(x) (((x) & 1) << 2) /* [2] */
#define REG_FIFO_AUTO_RST_EN_M  (1 << 2)
#define reg_pack_mode(x)        (((x) & 0x3) << 0) /* [1:0] */
#define REG_TMDS_PACK_MODE_M    (0x3 << 0)

#define REG_TX_PACK_FIFO_ST   0xA04
#define REG_TMDS_FIFO_WFULL   (1 << 6) /* [6] */
#define REG_TMDS_FIFO_WAFULL  (1 << 5) /* [5] */
#define REG_TMDS_FIFO_WERROR  (1 << 4) /* [4] */
#define REG_TMDS_FIFO_REMPTY  (1 << 3) /* [3] */
#define REG_TMDS_FIFO_RAEMPTY (1 << 2) /* [2] */
#define REG_TMDS_FIFO_RERROR  (1 << 1) /* [1] */
#define REG_PCLK2TCLK_STABLE  (1 << 0) /* [0] */

#define REG_PCLK_REFER_CNT    0xA08
#define reg_pclk_refer_cnt(x) ((x) & 0x3ffff) /* [17:0] */
#define REG_PCLK_REFER_CNT_M  (0x3ffff << 0)

#define REG_TCLK_LOWER_THRESHOLD    0xA0C
#define reg_tcnt_lower_threshold(x) ((x) & 0x3ffff) /* [17:0] */
#define REG_TCNT_LOWER_THRESHOLD_M  (0x3ffff << 0)

#define REG_TCLK_UPPER_THRESHOLD    0xA10
#define reg_tcnt_upper_threshold(x) ((x) & 0x3ffff) /* [17:0] */
#define REG_TCNT_UPPER_THRESHOLD_M  (0x3ffff << 0)

#endif
