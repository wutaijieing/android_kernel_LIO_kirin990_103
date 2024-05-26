/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver hdmi tx controller header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_CTRL_REG_H__
#define __HDMITX_CTRL_REG_H__

/* tx_ctrl module field info */
#define REG_TX_CHANNEL_SEL    0x0000
#define reg_uart_tx_sel(x)    (((x) & 1) << 3) /* [3] */
#define REG_UART_TX_SEL_M     (1 << 3)
#define reg_flbk_sel(x)       (((x) & 1) << 2) /* [2] */
#define REG_FLBK_SEL_M        (1 << 2)
#define reg_rx_frl_sel(x)     (((x) & 1) << 1) /* [1] */
#define REG_RX_FRL_SEL_M      (1 << 1)
#define reg_vid_bypass_sel(x) (((x) & 1) << 0) /* [0] */
#define REG_VID_BYPASS_SEL_M  (1 << 0)

#define REG_CSC_DITHER_DFIR_EN   0x0004
#define reg_vdp2hdmi_yuv_mode(x) (((x) & 1) << 4) /* [4] */
#define REG_VDP2HDMI_YUV_MODE_M  (1 << 4)
#define reg_dfir_422_en(x)       (((x) & 1) << 3) /* [3] */
#define REG_DFIR_422_EN_M        (1 << 3)
#define reg_dfir_420_en(x)       (((x) & 1) << 2) /* [2] */
#define REG_DFIR_420_EN_M        (1 << 2)
#define reg_dither_chn_en(x)     (((x) & 1) << 1) /* [1] */
#define REG_DITHER_CHN_EN_M      (1 << 1)
#define reg_csc_chn_en(x)        (((x) & 1) << 0) /* [0] */
#define REG_CSC_CHN_EN_M         (1 << 0)

#define REG_DSC_DIV_UPDATA        0x0008
#define reg_dsc_div_digital_en(x) (((x) & 1) << 31) /* [31] */
#define REG_DSC_DIV_DIGITAL_EN_M  (1 << 31)
#define reg_dsc_div_up(x)         (((x) & 0xffffff) << 0) /* [23:0] */
#define REG_DSC_DIV_UP_M          (0xffffff << 0)

#define REG_DSC_DIV_DOWNDATA 0x000C
#define reg_dsc_div_down(x)  (((x) & 0xffffff) << 0) /* [23:0] */
#define REG_DSC_DIV_DOWN_M   (0xffffff << 0)

#define REG_TX_PWD_RST_CTRL           0x0010
#define reg_clk_test_out_sel(x)       (((x) & 1) << 23) /* [23] */
#define REG_CLK_TEST_OUT_SEL_M        (1 << 23)
#define reg_clk_frl_source_sel(x)     (((x) & 1) << 22) /* [22] */
#define REG_CLK_FRL_SOURCE_SEL_M      (1 << 22)
#define reg_clk_tmds_source_sel(x)    (((x) & 1) << 21) /* [21] */
#define REG_CLK_TMDS_SOURCE_SEL_M     (1 << 21)
#define reg_audpath_acr_clk_sel(x)    (((x) & 1) << 20) /* [20] */
#define REG_AUDPATH_ACR_CLK_SEL_M     (1 << 20)
#define reg_dsc_clk_sel(x)            (((x) & 1) << 19) /* [19] */
#define REG_DSC_CLK_SEL_M             (1 << 19)
#define reg_aud_clk_gen_sel(x)        (((x) & 1) << 18) /* [18] */
#define REG_AUD_CLK_GEN_SEL_M         (1 << 18)
#define reg_vidpath_clk_sel(x)        (((x) & 1) << 17) /* [17] */
#define REG_VIDPATH_CLK_SEL_M         (1 << 17)
#define reg_vidpath_dout_clk_sel(x)   (((x) & 1) << 16) /* [16] */
#define REG_VIDPATH_DOUT_CLK_SEL_M    (1 << 16)
#define reg_tx_csc_dither_srst_req(x) (((x) & 1) << 13) /* [13] */
#define REG_TX_CSC_DITHER_SRST_REQ_M  (1 << 13)
#define reg_frl_fast_arst_req(x)      (((x) & 1) << 12) /* [12] */
#define REG_FRL_FAST_ARST_REQ_M       (1 << 12)
#define reg_frl_soft_arst_req(x)      (((x) & 1) << 11) /* [11] */
#define REG_FRL_SOFT_ARST_REQ_M       (1 << 11)
#define reg_tx_mcu_srst_req(x)        (((x) & 1) << 10) /* [10] */
#define REG_TX_MCU_SRST_REQ_M         (1 << 10)
#define reg_tx_hdcp2x_srst_req(x)     (((x) & 1) << 9) /* [9] */
#define REG_TX_HDCP2X_SRST_REQ_M      (1 << 9)
#define reg_tx_afifo_srst_req(x)      (((x) & 1) << 8) /* [8] */
#define REG_TX_AFIFO_SRST_REQ_M       (1 << 8)
#define reg_tx_acr_srst_req(x)        (((x) & 1) << 7) /* [7] */
#define REG_TX_ACR_SRST_REQ_M         (1 << 7)
#define reg_tx_aud_srst_req(x)        (((x) & 1) << 6) /* [6] */
#define REG_TX_AUD_SRST_REQ_M         (1 << 6)
#define reg_tx_phy_srst_req(x)        (((x) & 1) << 5) /* [5] */
#define REG_TX_PHY_SRST_REQ_M         (1 << 5)
#define reg_tx_hdcp1x_srst_req(x)     (((x) & 1) << 4) /* [4] */
#define REG_TX_HDCP1X_SRST_REQ_M      (1 << 4)
#define reg_tx_hdmi_srst_req(x)       (((x) & 1) << 3) /* [3] */
#define REG_TX_HDMI_SRST_REQ_M        (1 << 3)
#define reg_tx_vid_srst_req(x)        (((x) & 1) << 2) /* [2] */
#define REG_TX_VID_SRST_REQ_M         (1 << 2)
#define reg_tx_sys_srst_req(x)        (((x) & 1) << 1) /* [1] */
#define REG_TX_SYS_SRST_REQ_M         (1 << 1)
#define reg_tx_pwd_srst_req(x)        (((x) & 1) << 0) /* [0] */
#define REG_TX_PWD_SRST_REQ_M         (1 << 0)

#define REG_SCDC_FSM_CTRL           0x0014
#define reg_scdc_stall_req(x)       (((x) & 1) << 7) /* [7] */
#define REG_SCDC_STALL_REQ_M        (1 << 7)
#define reg_scdc_hdcp_det_en(x)     (((x) & 1) << 6) /* [6] */
#define REG_SCDC_HDCP_DET_EN_M      (1 << 6)
#define reg_scdc_poll_sel(x)        (((x) & 1) << 5) /* [5] */
#define REG_SCDC_POLL_SEL_M         (1 << 5)
#define reg_scdc_auto_reply_stop(x) (((x) & 1) << 4) /* [4] */
#define REG_SCDC_AUTO_REPLY_STOP_M  (1 << 4)
#define reg_scdc_auto_poll(x)       (((x) & 1) << 3) /* [3] */
#define REG_SCDC_AUTO_POLL_M        (1 << 3)
#define reg_scdc_auto_reply(x)      (((x) & 1) << 2) /* [2] */
#define REG_SCDC_AUTO_REPLY_M       (1 << 2)
#define reg_scdc_access_en(x)       (((x) & 1) << 1) /* [1] */
#define REG_SCDC_ACCESS_EN_M        (1 << 1)
#define reg_scdc_ddcm_abort(x)      (((x) & 1) << 0) /* [0] */
#define REG_SCDC_DDCM_ABORT_M       (1 << 0)

#define REG_SCDC_POLL_TIMER      0x0018
#define reg_scdc_ddcm_speed(x)   (((x) & 0x1ff) << 22) /* [30:22] */
#define REG_SCDC_DDCM_SPEED_M    (0x1ff << 22)
#define reg_scdc_poll_timer_f(x) ((x) & 0x3fffff) /* [21:0] */
#define REG_SCDC_POLL_TIMER_M    (0x3fffff << 0)

#define REG_SCDC_FSM_STATE       0x001C
#define REG_SCDC_RREQ_IN_PROG_M  (1 << 10) /* [10] */
#define reg_scdc_rreq_in_prog(x) (((x) & 1) << 10)
#define REG_SCDC_IN_PROG_M       (1 << 9) /* [9] */
#define reg_scdc_in_prog(x)      (((x) & 1) << 9)
#define REG_SCDC_ACTIVE_M        (1 << 8) /* [8] */
#define reg_scdc_active(x)       (((x) & 1) << 8)
#define REG_SCDC_RREQ_STATE_M    (0xf << 4) /* [7:4] */
#define reg_scdc_rreq_state(x)   (((x) & 0xf) << 4)
#define REG_SCDC_FSM_STATE_M     (0xf << 0) /* [3:0] */
#define reg_scdc_fsm_state_f(x)  (((x) & 0xf) << 0)

#define REG_SCDC_FLAG_BTYE     0x0020
#define REG_SCDC_FLAG_BYTE1_M  (0xff << 8) /* [15:8] */
#define reg_scdc_flag_byte1(x) (((x) & 0xff) << 8)
#define REG_SCDC_FLAG_BYTE0_M  (0xff << 0) /* [7:0] */
#define reg_scdc_flag_byte0(x) (((x) & 0xff) << 0)

#define REG_CLIP_Y_RANGE  0x0024
#define reg_clip_y_max(x) (((x) & 0xfff) << 16) /* [27:16] */
#define REG_CLIP_Y_MAX_M  (0xfff << 16)
#define reg_clip_y_min(x) (((x) & 0xfff) << 0) /* [11:0] */
#define REG_CLIP_Y_MIN_M  (0xfff << 0)

#define REG_CLIP_C_RANGE  0x0028
#define reg_clip_c_max(x) (((x) & 0xfff) << 16) /* [27:16] */
#define REG_CLIP_C_MAX_M  (0xfff << 16)
#define reg_clip_c_min(x) (((x) & 0xfff) << 0) /* [11:0] */
#define REG_CLIP_C_MIN_M  (0xfff << 0)

#define REG_RAS_MEM_MODE           0x002C
#define reg_ras_t16_shcsu_wtsel(x) (((x) & 0x3) << 18) /* [19:18] */
#define REG_RAS_T16_SHCSU_WTSEL_M  (0x3 << 18)
#define reg_ras_t16_shcsu_rtsel(x) (((x) & 0x3) << 16) /* [17:16] */
#define REG_RAS_T16_SHCSU_RTSEL_M  (0x3 << 16)
#define reg_ras_t16_wtsel(x)       (((x) & 0x3) << 14) /* [15:14] */
#define REG_RAS_T16_WTSEL_M        (0x3 << 14)
#define reg_ras_t16_rtsel(x)       (((x) & 0x3) << 12) /* [13:12] */
#define REG_RAS_T16_RTSEL_M        (0x3 << 12)
#define reg_ras_mem_ret1n(x)       (((x) & 1) << 6) /* [6] */
#define REG_RAS_MEM_RET1N_M        (1 << 6)
#define reg_ras_mem_emaw(x)        (((x) & 0x3) << 4) /* [5:4] */
#define REG_RAS_MEM_EMAW_M         (0x3 << 4)
#define reg_ras_mem_ema(x)         (((x) & 0x7) << 1) /* [3:1] */
#define REG_RAS_MEM_EMA_M          (0x7 << 1)
#define reg_ras_mem_gt_en(x)       (((x) & 1) << 0) /* [0] */
#define REG_RAS_MEM_GT_EN_M        (1 << 0)

#define REG_RFS_MEM_MODE     0x0030
#define reg_rfs_t16_wtsel(x) (((x) & 0x3) << 14) /* [15:14] */
#define REG_RFS_T16_WTSEL_M  (0x3 << 14)
#define reg_rfs_t16_rtsel(x) (((x) & 0x3) << 12) /* [13:12] */
#define REG_RFS_T16_RTSEL_M  (0x3 << 12)
#define reg_rfs_mem_ret1n(x) (((x) & 1) << 6) /* [6] */
#define REG_RFS_MEM_RET1N_M  (1 << 6)
#define reg_rfs_mem_emaw(x)  (((x) & 0x3) << 4) /* [5:4] */
#define REG_RFS_MEM_EMAW_M   (0x3 << 4)
#define reg_rfs_mem_ema(x)   (((x) & 0x7) << 1) /* [3:1] */
#define REG_RFS_MEM_EMA_M    (0x7 << 1)
#define reg_rfs_mem_gt_en(x) (((x) & 1) << 0) /* [0] */
#define REG_RFS_MEM_GT_EN_M  (1 << 0)

#define REG_RFT_MEM_MODE        0x0034
#define reg_rft_t16_kp(x)       (((x) & 0x7) << 16) /* [18:16] */
#define REG_RFT_T16_KP_M        (0x7 << 16)
#define reg_rft_t16_wct(x)      (((x) & 0x3) << 14) /* [15:14] */
#define REG_RFT_T16_WCT_M       (0x3 << 14)
#define reg_rft_t16_rct(x)      (((x) & 0x3) << 12) /* [13:12] */
#define REG_RFT_T16_RCT_M       (0x3 << 12)
#define reg_rft_mem_colldisn(x) (((x) & 1) << 9) /* [9] */
#define REG_RFT_MEM_COLLDISN_M  (1 << 9)
#define reg_rft_mem_ret1n(x)    (((x) & 1) << 8) /* [8] */
#define REG_RFT_MEM_RET1N_M     (1 << 8)
#define reg_rft_mem_emab(x)     (((x) & 0x7) << 5) /* [7:5] */
#define REG_RFT_MEM_EMAB_M      (0x7 << 5)
#define reg_rft_mem_emasa(x)    (((x) & 1) << 4) /* [4] */
#define REG_RFT_MEM_EMASA_M     (1 << 4)
#define reg_rft_mem_emaa(x)     (((x) & 0x7) << 1) /* [3:1] */
#define REG_RFT_MEM_EMAA_M      (0x7 << 1)
#define reg_rft_mem_gt_en(x)    (((x) & 1) << 0) /* [0] */
#define REG_RFT_MEM_GT_EN_M     (1 << 0)

/* DDC module regs */
#define REG_PWD_FIFO_RDATA       0x0 /* 0x38 */
#define reg_pwd_fifo_data_out(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_PWD_FIFO_DATA_OUT_M  (0xff << 0)

#define REG_PWD_FIFO_WDATA      0x4 /* 0x3C */
#define reg_pwd_fifo_data_in(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_PWD_FIFO_DATA_IN_M  (0xff << 0)

#define REG_PWD_DATA_CNT         0x8 /* 0x40 */
#define reg_pwd_data_out_cnt(x)  (((x) & 0x3ff) << 8) /* [17:8] */
#define REG_PWD_DATA_OUT_CNT_M   (0x3ff << 8)
#define reg_pwd_fifo_data_cnt(x) (((x) & 0x1f) << 0) /* [4:0] */
#define REG_PWD_FIFO_DATA_CNT_M  (0x1f << 0)

#define REG_PWD_SLAVE_CFG       0xC /* 0x44 */
#define reg_pwd_slave_seg(x)    (((x) & 0xff) << 16) /* [23:16] */
#define REG_PWD_SLAVE_SEG_M     (0xff << 16)
#define reg_pwd_slave_offset(x) (((x) & 0xff) << 8) /* [15:8] */
#define REG_PWD_SLAVE_OFFSET_M  (0xff << 8)
#define reg_pwd_slave_addr(x)   (((x) & 0xff) << 0) /* [7:0] */
#define REG_PWD_SLAVE_ADDR_M    (0xff << 0)

#define REG_PWD_MST_STATE         0x10 /* 0x48 */
#define reg_pwd_fifo_full(x)      (((x) & 1) << 7) /* [7] */
#define REG_PWD_FIFO_FULL_M       (1 << 7)
#define reg_pwd_fifo_half_full(x) (((x) & 1) << 6) /* [6] */
#define REG_PWD_FIFO_HALF_FULL_M  (1 << 6)
#define reg_pwd_fifo_empty(x)     (((x) & 1) << 5) /* [5] */
#define REG_PWD_FIFO_EMPTY_M      (1 << 5)
#define reg_pwd_fifo_rd_in_use(x) (((x) & 1) << 4) /* [4] */
#define REG_PWD_FIFO_RD_IN_USE_M  (1 << 4)
#define reg_pwd_fifo_wr_in_use(x) (((x) & 1) << 3) /* [3] */
#define REG_PWD_FIFO_WR_IN_USE_M  (1 << 3)
#define reg_pwd_i2c_in_prog(x)    (((x) & 1) << 2) /* [2] */
#define REG_PWD_I2C_IN_PROG_M     (1 << 2)
#define reg_pwd_i2c_bus_low(x)    (((x) & 1) << 1) /* [1] */
#define REG_PWD_I2C_BUS_LOW_M     (1 << 1)
#define reg_pwd_i2c_no_ack(x)     (((x) & 1) << 0) /* [0] */
#define REG_PWD_I2C_NO_ACK_M      (1 << 0)

#define REG_PWD_MST_CMD      0x14 /* 0x4C */
#define reg_pwd_mst_cmd_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_PWD_MST_CMD_M    (0xf << 0)

#define REG_PWD_MST_CLR        0x18 /* 0x50 */
#define reg_pwd_clr_no_ack(x)  (((x) & 1) << 1) /* [1] */
#define REG_PWD_CLR_NO_ACK_M   (1 << 1)
#define reg_pwd_clr_bus_low(x) (((x) & 1) << 0) /* [0] */
#define REG_PWD_CLR_BUS_LOW_M  (1 << 0)

#define REG_DDC_MST_ARB_CTRL     0x2C /* 0x64 */
#define reg_auto_abort_en(x)     (((x) & 1) << 1) /* [1] */
#define REG_AUTO_ABORT_EN_M      (1 << 1)
#define reg_cpu_ddc_force_req(x) (((x) & 1) << 0) /* [0] */
#define REG_CPU_DDC_FORCE_REQ_M  (1 << 0)

#define REG_DDC_MST_ARB_REQ 0x30 /* 0x68 */
#define reg_cpu_ddc_req(x)  (((x) & 1) << 0) /* [0] */
#define REG_CPU_DDC_REQ_M   (1 << 0)

#define REG_DDC_MST_ARB_ACK    0x34 /* 0x6C */
#define REG_CPU_DDC_REQ_ACK_M  (1 << 0) /* [0] */
#define reg_cpu_ddc_req_ack(x) (((x) & 1) << 0)

#define REG_DDC_MST_ARB_STATE 0x0070
#define reg_ddc_arb_state(x)  (((x) & 0xff) << 0) /* [8:0] */
#define REG_DDC_ARB_STATE_M   (0xff << 0)

#define REG_DDC_MST_DLY_CFG      0x0074
#define reg_scdcm_cmd_dly_cnt(x) (((x) & 0xffff) << 16) /* [31:16] */
#define REG_SCDCM_CMD_DLY_CNT_M  (0xffff << 16)
#define reg_ddcm_cmd_dly_cnt(x)  ((x) & 0xffff) /* [15:0] */
#define REG_DDCM_CMD_DLY_CNT_M   (0xffff << 0)

#define REG_RAS_MEM_MODE_H 0x0078
#define reg_s14_ras_rm(x)  (((x) & 0xf) << 4) /* [7:4] */
#define REG_S14_RAS_RM_M   (0xf << 4)
#define reg_s14_ras_rme(x) (((x) & 1) << 0) /* [1] */
#define REG_S14_RAS_RME_M  (1 << 0)

#define REG_RFS_MEM_MODE_H 0x007C
#define reg_s14_rfs_rm(x)  (((x) & 0xf) << 4) /* [7:4] */
#define REG_S14_RFS_RM_M   (0xf << 4)
#define reg_s14_rfs_rme(x) (((x) & 1) << 0) /* [1] */
#define REG_S14_RFS_RME_M  (1 << 0)

#define REG_RFT_MEM_MODE_H      0x0080
#define reg_s14_rft_mem_rmb(x)  (((x) & 0xf) << 8) /* [11:8] */
#define REG_S14_RFT_MEM_RMB_M   (0xf << 8)
#define reg_s14_rft_mem_rma(x)  (((x) & 0xf) << 4) /* [7:4] */
#define REG_S14_RFT_MEM_RMA_M   (0xf << 4)
#define reg_s14_rft_mem_rmeb(x) (((x) & 1) << 3) /* [3] */
#define REG_S14_RFT_MEM_RMEB_M  (1 << 3)
#define reg_s14_rft_mem_rmea(x) (((x) & 1) << 2) /* [2] */
#define REG_S14_RFT_MEM_RMEA_M  (1 << 2)

#define REG_DSC_DET_HTOTAL      0x0084
#define reg_dsc_det_en(x)       (((x) & 1) << 31) /* [31] */
#define REG_DSC_DET_EN_M        (1 << 31)
#define reg_dsc_det_htotal_f(x) (((x) & 0x3fff) << 0) /* [13:0] */
#define REG_DSC_DET_HTOTAL_M    (0x3fff << 0)

#define REG_DSC_DET_HCTOTAL      0x0088
#define reg_dsc_det_hctotal_f(x) (((x) & 0x3fff) << 0) /* [13:0] */
#define REG_DSC_DET_HCTOTAL_M    (0x3fff << 0)

#define REG_DSC_DET_PXL_CNT_H 0x008C

#define REG_DSC_DET_PXL_CNT_L 0x0090

#define REG_DSC_DET_DSC_CNT_H 0x0094

#define REG_DSC_DET_DSC_CNT_L 0x0098

#define REG_DSC_DET_MAX_DIFF 0x009C

#define REG_TX_PWD_INTR_STATE      0x0100
#define REG_TX_PWD_INTR_STATE_M    (1 << 0) /* [0] */
#define reg_tx_pwd_intr_state_f(x) (((x) & 1) << 0)

#define REG_PWD_SUB_INTR_STATE      0x0104
#define reg_txmeta_intr_state_f(x)  (((x) & 1) << 7) /* [7] */
#define REG_TXMETA_INTR_STATE_M     (1 << 7)
#define reg_txfrl_intr_state(x)     (((x) & 1) << 6) /* [6] */
#define REG_TXFRL_INTR_STATE_M      (1 << 6)
#define reg_hdcp2x_intr_state(x)    (((x) & 1) << 5) /* [5] */
#define REG_HDCP2X_INTR_STATE_M     (1 << 5)
#define reg_txhdcp_intr_state(x)    (((x) & 1) << 4) /* [4] */
#define REG_TXHDCP_INTR_STATE_M     (1 << 4)
#define reg_txhdmi_intr_state_f(x)  (((x) & 1) << 3) /* [3] */
#define REG_TXHDMI_INTR_STATE_M     (1 << 3)
#define reg_audpath_intr_state_f(x) (((x) & 1) << 2) /* [2] */
#define REG_AUDPATH_INTR_STATE_M    (1 << 2)
#define reg_vidpath_intr_state_f(x) (((x) & 1) << 1) /* [1] */
#define REG_VIDPATH_INTR_STATE_M    (1 << 1)
#define reg_tx_sys_intr_state(x)    (((x) & 1) << 0) /* [0] */
#define REG_TX_SYS_INTR_STATE_M     (1 << 0)

#define REG_PWD_SUB_INTR_MASK      0x0108
#define reg_txmeta_intr_mask_f(x)  (((x) & 1) << 7) /* [7] */
#define REG_TXMETA_INTR_MASK_M     (1 << 7)
#define reg_txfrl_intr_mask_f(x)   (((x) & 1) << 6) /* [6] */
#define REG_TXFRL_INTR_MASK_M      (1 << 6)
#define reg_hdcp2x_intr_mask(x)    (((x) & 1) << 5) /* [5] */
#define REG_HDCP2X_INTR_MASK_M     (1 << 5)
#define reg_txhdcp_intr_mask(x)    (((x) & 1) << 4) /* [4] */
#define REG_TXHDCP_INTR_MASK_M     (1 << 4)
#define reg_txhdmi_intr_mask_f(x)  (((x) & 1) << 3) /* [3] */
#define REG_TXHDMI_INTR_MASK_M     (1 << 3)
#define reg_audpath_intr_mask_f(x) (((x) & 1) << 2) /* [2] */
#define REG_AUDPATH_INTR_MASK_M    (1 << 2)
#define reg_vidpath_intr_mask_f(x) (((x) & 1) << 1) /* [1] */
#define REG_VIDPATH_INTR_MASK_M    (1 << 1)
#define reg_tx_sys_intr_mask(x)    (((x) & 1) << 0) /* [0] */
#define REG_TX_SYS_INTR_MASK_M     (1 << 0)

#define REG_TXSYS_INTR_STATE      0x010C
#define reg_tx_sys_intr_state5(x) (((x) & 1) << 5) /* [5] */
#define REG_TX_SYS_INTR_STATE5_M  (1 << 5)
#define reg_tx_sys_intr_state4(x) (((x) & 1) << 4) /* [4] */
#define REG_TX_SYS_INTR_STATE4_M  (1 << 4)
#define reg_tx_sys_intr_state3(x) (((x) & 1) << 3) /* [3] */
#define REG_TX_SYS_INTR_STATE3_M  (1 << 3)
#define reg_tx_sys_intr_state2(x) (((x) & 1) << 2) /* [2] */
#define REG_TX_SYS_INTR_STATE2_M  (1 << 2)
#define reg_tx_sys_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_TX_SYS_INTR_STATE1_M  (1 << 1)
#define reg_tx_sys_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_TX_SYS_INTR_STATE0_M  (1 << 0)

#define REG_TXSYS_INTR_MASK      0x0110
#define reg_tx_sys_intr_mask5(x) (((x) & 1) << 5) /* [5] */
#define REG_TX_SYS_INTR_MASK5_M  (1 << 5)
#define reg_tx_sys_intr_mask4(x) (((x) & 1) << 4) /* [4] */
#define REG_TX_SYS_INTR_MASK4_M  (1 << 4)
#define reg_tx_sys_intr_mask3(x) (((x) & 1) << 3) /* [3] */
#define REG_TX_SYS_INTR_MASK3_M  (1 << 3)
#define reg_tx_sys_intr_mask2(x) (((x) & 1) << 2) /* [2] */
#define REG_TX_SYS_INTR_MASK2_M  (1 << 2)
#define reg_tx_sys_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_TX_SYS_INTR_MASK1_M  (1 << 1)
#define reg_tx_sys_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_TX_SYS_INTR_MASK0_M  (1 << 0)

#define REG_TXHDMI_INTR_STATE     0x0124
#define reg_txhdmi_intr_state6(x) (((x) & 1) << 6) /* [6] */
#define REG_TXHDMI_INTR_STATE6_M  (1 << 6)
#define reg_txhdmi_intr_state5(x) (((x) & 1) << 5) /* [5] */
#define REG_TXHDMI_INTR_STATE5_M  (1 << 5)
#define reg_txhdmi_intr_state4(x) (((x) & 1) << 4) /* [4] */
#define REG_TXHDMI_INTR_STATE4_M  (1 << 4)
#define reg_txhdmi_intr_state3(x) (((x) & 1) << 3) /* [3] */
#define REG_TXHDMI_INTR_STATE3_M  (1 << 3)
#define reg_txhdmi_intr_state2(x) (((x) & 1) << 2) /* [2] */
#define REG_TXHDMI_INTR_STATE2_M  (1 << 2)
#define reg_txhdmi_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXHDMI_INTR_STATE1_M  (1 << 1)
#define reg_txhdmi_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXHDMI_INTR_STATE0_M  (1 << 0)

#define REG_TXHDMI_INTR_MASK     0x0128
#define reg_txhdmi_intr_mask6(x) (((x) & 1) << 6) /* [6] */
#define REG_TXHDMI_INTR_MASK6_M  (1 << 6)
#define reg_txhdmi_intr_mask5(x) (((x) & 1) << 5) /* [5] */
#define REG_TXHDMI_INTR_MASK5_M  (1 << 5)
#define reg_txhdmi_intr_mask4(x) (((x) & 1) << 4) /* [4] */
#define REG_TXHDMI_INTR_MASK4_M  (1 << 4)
#define reg_txhdmi_intr_mask3(x) (((x) & 1) << 3) /* [3] */
#define REG_TXHDMI_INTR_MASK3_M  (1 << 3)
#define reg_txhdmi_intr_mask2(x) (((x) & 1) << 2) /* [2] */
#define REG_TXHDMI_INTR_MASK2_M  (1 << 2)
#define reg_txhdmi_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXHDMI_INTR_MASK1_M  (1 << 1)
#define reg_txhdmi_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXHDMI_INTR_MASK0_M  (1 << 0)

#define REG_HDCP_INTR_STATE     0x012C
#define reg_hdcp_intr_state5(x) (((x) & 1) << 5) /* [5] */
#define REG_HDCP_INTR_STATE5_M  (1 << 5)
#define reg_hdcp_intr_state4(x) (((x) & 1) << 4) /* [4] */
#define REG_HDCP_INTR_STATE4_M  (1 << 4)
#define reg_hdcp_intr_state3(x) (((x) & 1) << 3) /* [3] */
#define REG_HDCP_INTR_STATE3_M  (1 << 3)
#define reg_hdcp_intr_state2(x) (((x) & 1) << 2) /* [2] */
#define REG_HDCP_INTR_STATE2_M  (1 << 2)
#define reg_hdcp_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_HDCP_INTR_STATE1_M  (1 << 1)
#define reg_hdcp_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_HDCP_INTR_STATE0_M  (1 << 0)

#define REG_HDCP_INTR_MASK     0x0130
#define reg_hdcp_intr_mask5(x) (((x) & 1) << 5) /* [5] */
#define REG_HDCP_INTR_MASK5_M  (1 << 5)
#define reg_hdcp_intr_mask4(x) (((x) & 1) << 4) /* [4] */
#define REG_HDCP_INTR_MASK4_M  (1 << 4)
#define reg_hdcp_intr_mask3(x) (((x) & 1) << 3) /* [3] */
#define REG_HDCP_INTR_MASK3_M  (1 << 3)
#define reg_hdcp_intr_mask2(x) (((x) & 1) << 2) /* [2] */
#define REG_HDCP_INTR_MASK2_M  (1 << 2)
#define reg_hdcp_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_HDCP_INTR_MASK1_M  (1 << 1)
#define reg_hdcp_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_HDCP_INTR_MASK0_M  (1 << 0)

#define REG_TXFRLINTR_STATE      0x0134
#define reg_txfrl_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXFRL_INTR_STATE1_M  (1 << 1)
#define reg_txfrl_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXFRL_INTR_STATE0_M  (1 << 0)

#define REG_TXFRL_INTR_MASK     0x0138
#define reg_txfrl_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXFRL_INTR_MASK1_M  (1 << 1)
#define reg_txfrl_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXFRL_INTR_MASK0_M  (1 << 0)

#define REG_SEC_INTR_STATE          0x0160
#define reg_pwd_sec_intr_state_f(x) (((x) & 1) << 1) /* [1] */
#define REG_PWD_SEC_INTR_STATE_M    (1 << 1)
#define reg_mcu_sec_intr_state(x)   (((x) & 1) << 0) /* [0] */
#define REG_MCU_SEC_INTR_STATE_M    (1 << 0)

#define REG_SEC_INTR_MASK          0x0164
#define reg_pwd_sec_intr_mask_f(x) (((x) & 1) << 1) /* [1] */
#define REG_PWD_SEC_INTR_MASK_M    (1 << 1)
#define reg_mcu_sec_intr_mask(x)   (((x) & 1) << 0) /* [0] */
#define REG_MCU_SEC_INTR_MASK_M    (1 << 0)

#define REG_PWD_SEC_INTR_STATE         0x0168
#define reg_pwd_pwd_sec_intr_state3(x) (((x) & 1) << 3) /* [3] */
#define REG_PWD_PWD_SEC_INTR_STATE3_M  (1 << 3)
#define reg_pwd_pwd_sec_intr_state2(x) (((x) & 1) << 2) /* [2] */
#define REG_PWD_PWD_SEC_INTR_STATE2_M  (1 << 2)
#define reg_pwd_pwd_sec_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_PWD_PWD_SEC_INTR_STATE1_M  (1 << 1)
#define reg_pwd_pwd_sec_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_PWD_PWD_SEC_INTR_STATE0_M  (1 << 0)

#define REG_PWD_SEC_INTR_MASK     0x016C
#define reg_pwd_sec_intr_mask3(x) (((x) & 1) << 3) /* [3] */
#define REG_PWD_SEC_INTR_MASK3_M  (1 << 3)
#define reg_pwd_sec_intr_mask2(x) (((x) & 1) << 2) /* [2] */
#define REG_PWD_SEC_INTR_MASK2_M  (1 << 2)
#define reg_pwd_sec_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_PWD_SEC_INTR_MASK1_M  (1 << 1)
#define reg_pwd_sec_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_PWD_SEC_INTR_MASK0_M  (1 << 0)

#define REG_TXMETA_INTR_MASK     0x0184
#define reg_txmeta_intr_mask6(x) (((x) & 1) << 6) /* [6] */
#define REG_TXMETA_INTR_MASK6_M  (1 << 6)
#define reg_txmeta_intr_mask5(x) (((x) & 1) << 5) /* [5] */
#define REG_TXMETA_INTR_MASK5_M  (1 << 5)
#define reg_txmeta_intr_mask4(x) (((x) & 1) << 4) /* [4] */
#define REG_TXMETA_INTR_MASK4_M  (1 << 4)
#define reg_txmeta_intr_mask3(x) (((x) & 1) << 3) /* [3] */
#define REG_TXMETA_INTR_MASK3_M  (1 << 3)
#define reg_txmeta_intr_mask2(x) (((x) & 1) << 2) /* [2] */
#define REG_TXMETA_INTR_MASK2_M  (1 << 2)
#define reg_txmeta_intr_mask1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXMETA_INTR_MASK1_M  (1 << 1)
#define reg_txmeta_intr_mask0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXMETA_INTR_MASK0_M  (1 << 0)

#define REG_TXMETA_INTR_STATE     0x0188
#define reg_txmeta_intr_state6(x) (((x) & 1) << 6) /* [6] */
#define REG_TXMETA_INTR_STATE6_M  (1 << 6)
#define reg_txmeta_intr_state5(x) (((x) & 1) << 5) /* [5] */
#define REG_TXMETA_INTR_STATE5_M  (1 << 5)
#define reg_txmeta_intr_state4(x) (((x) & 1) << 4) /* [4] */
#define REG_TXMETA_INTR_STATE4_M  (1 << 4)
#define reg_txmeta_intr_state3(x) (((x) & 1) << 3) /* [3] */
#define REG_TXMETA_INTR_STATE3_M  (1 << 3)
#define reg_txmeta_intr_state2(x) (((x) & 1) << 2) /* [2] */
#define REG_TXMETA_INTR_STATE2_M  (1 << 2)
#define reg_txmeta_intr_state1(x) (((x) & 1) << 1) /* [1] */
#define REG_TXMETA_INTR_STATE1_M  (1 << 1)
#define reg_txmeta_intr_state0(x) (((x) & 1) << 0) /* [0] */
#define REG_TXMETA_INTR_STATE0_M  (1 << 0)

#endif

