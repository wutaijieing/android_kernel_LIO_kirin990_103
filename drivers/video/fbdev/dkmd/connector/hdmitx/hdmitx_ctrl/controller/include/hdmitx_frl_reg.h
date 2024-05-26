/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver hdmi frl reg header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_FRL_REG_H__
#define __HDMITX_FRL_REG_H__

/* tx_frl_cfg module field info */
#define REG_TX_FRL_VERSION     0x000 /* FRL Logic Version Update Time Register. */
#define reg_frl_tx_hw_year(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_FRL_TX_HW_YEAR_M   (0xffff << 16)
#define reg_frl_tx_hw_month(x) (((x) & 0xff) << 8)    /* [15:8] */
#define REG_FRL_TX_HW_MONTH_M  (0xff << 8)
#define reg_frl_tx_hw_day(x)   (((x) & 0xff) << 0)    /* [7:0] */
#define REG_FRL_TX_HW_DAY_M    (0xff << 0)

#define REG_TX_FRL_TEST      0x004 /* FRL test register */
#define reg_tx_frl_test_f(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_TX_FRL_TEST_M    (0xffffffff << 0)

#define REG_WORK_EN      0x010 /* Start signal */
#define reg_work_en_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_WORK_EN_M    (0x1 << 0)

#define REG_LINE_START_TIME      0x100 /* Reference Shift time */
#define reg_line_start_time_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_LINE_START_TIME_F_M  (0xffff << 0)

/*
 * The maximum AV data length corresponding to one CMD when writing to Buffer,
 * in units of 3-byte. Can be divisible by 4.
 */
#define REG_AV_MAX_LEN      0x104
#define reg_av_max_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_AV_MAX_LEN_F_M  (0xffff << 0)

#define REG_AV_GAP_LEN      0x108 /* number of Gaps to be inserted between AV slices */
#define reg_av_gap_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_AV_GAP_LEN_F_M  (0xffff << 0)

/*
 * The maximum blank data length corresponding to one CMD when writing to the Buffer.
 * Unit 3-byte. Can be divisible by 4.
 */
#define REG_BLK_MAX_LEN      0x10C
#define reg_blk_max_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_BLK_MAX_LEN_F_M  (0xffff << 0)

/* The number of Gaps to be inserted between BLANK slices, high bit means positive and negative. */
#define REG_BLK_GAP_LEN      0x110
#define reg_blk_gap_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_BLK_GAP_LEN_F_M  (0xffff << 0)

#define REG_HBLANK      0x114 /* Blank length in 3-byte units */
#define reg_hblank_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_HBLANK_F_M  (0xffff << 0)

#define REG_HACTIVE      0x118 /* active Video length in 3-bytes */
#define reg_hactive_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_HACTIVE_F_M  (0xffff << 0)

/*
 * In the extra mode, the maximum blank data length corresponding to one
 * CMD when writing to the Buffer. Unit 3-byte. Can be divisible by 4.
 */
#define REG_EXTRA_BLANK_MAX_LEN      0x11C
#define reg_extra_blank_max_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_EXTRA_BLANK_MAX_LEN_F_M  (0xffff << 0)

#define REG_EXTRA_BLANK_GAP_LEN      0x120 /* In extra mode, the number of gaps to be inserted between BLANK slices. */
#define reg_extra_blank_gap_len_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_EXTRA_BLANK_GAP_LEN_F_M  (0xffff << 0)

#define REG_SBC_CFIFO_STATUS 0x124 /* C fifo status */
#define reg_cfifo_status(x)  (((x) & 0xff) << 0) /* [7:0] */
#define REG_CFIFO_STATUS_M   (0xff << 0)

#define REG_SBC_DFIFO_STATUS 0x128 /* D fifo status */
#define reg_dfifo_status(x)  (((x) & 0xff) << 0) /* [7:0] */
#define REG_DFIFO_STATUS_M   (0xff << 0)

#define REG_VACTIVE      0x12C /* AV line number */
#define reg_vactive_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_VACTIVE_F_M  (0xffff << 0)

#define REG_SYNC_POLAR        0x134 /* Line sync polarity */
#define reg_hsync_polar_en(x) (((x) & 0x1) << 3) /* [3] */
#define REG_HSYNC_POLAR_EN_M  (0x1 << 3)
#define reg_hsync_polar(x)    (((x) & 0x1) << 2) /* [2] */
#define REG_HSYNC_POLAR_M     (0x1 << 2)
#define reg_vsync_polar_en(x) (((x) & 0x1) << 1) /* [1] */
#define REG_VSYNC_POLAR_EN_M  (0x1 << 1)
#define reg_vsync_polar(x)    (((x) & 0x1) << 0) /* [0] */
#define REG_VSYNC_POLAR_M     (0x1 << 0)

#define REG_EXTRA_MODE_EN      0x138 /* Extra mode enable */
#define reg_extra_mode_en_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_EXTRA_MODE_EN_F_M  (0x1 << 0)

#define REG_SBC_INT_STATUS      0x140 /* Interrupt status of the SBC module */
#define reg_sbc_int_status_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_SBC_INT_STATUS_F_M  (0xf << 0)

#define REG_VBLANK1_LINES      0x144 /* VBLANK1 lines */
#define reg_vblank1_lines_f(x) (((x) & 0x3ff) << 0) /* [9:0] */
#define REG_VBLANK1_LINES_F_M  (0x3ff << 0)

#define REG_LINE_START_HSYNC_EN      0x148 /* Reference Shift time Use line synchronization enable */
#define reg_line_start_hsync_en_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_LINE_START_HSYNC_EN_F_M  (0x1 << 0)

#define REG_EXTRA_NO_GAP_FLAG      0x150 /* Special format does not require the insertion of the GAP package flag */
#define reg_extra_no_gap_flag_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_EXTRA_NO_GAP_FLAG_F_M  (0x1 << 0)

#define REG_RESERVED_REG0 0x154 /* Keep DFX register 0 */
#define REG_RESERVED_REG1 0x158 /* Keep DFX register 1 */
#define REG_RESERVED_REG2 0x15C /* Keep DFX register 2 */
#define REG_RESERVED_REG3 0x160 /* Keep DFX register 3 */
#define REG_RESERVED_REG4 0x164 /* Keep DFX register 4 */

#define reg_reserved_reg(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_RESERVED_REG_M  (0xffffffff << 0)

#define REG_HTOTAL_ERR_INFO      0x168 /* HTOTAL statistical error information register */
#define reg_htotal_err_info_f(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_HTOTAL_ERR_INFO_F_M  (0xffffffff << 0)

#define REG_PP_STATUS      0x16C /* Tx_frl_pp module status register */
#define reg_pp_status_f(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_PP_STATUS_F_M  (0xffffffff << 0)

#define REG_MANUAL_SB_GEN      0x170 /* SB structure generation control */
#define reg_manual_sb_gen_f(x) (((x) & 0x3) << 0) /* [1:0] */
#define REG_MANUAL_SB_GEN_F_M  (0x3 << 0)

#define REG_IN_CRC0        0x180 /* Input CRC0 */
#define reg_tx2apb_crc0(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_TX2APB_CRC0_M  (0xffffffff << 0)

#define REG_IN_CRC1        0x184 /* Input CRC1 */
#define reg_tx2apb_crc1(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_TX2APB_CRC1_M  (0xffffffff << 0)

#define REG_PFIFO_BASE_THRESHOLD      0x1A0 /* Alternative flow control fifo base value */
#define reg_pfifo_base_threshold_f(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_PFIFO_BASE_THRESHOLD_M    (0xffff << 0)

#define REG_PFIFO_LINE_THRESHOLD    0x1A4 /* Alternative flow control fifo waterline */
#define reg_pfifo_up_threshold(x)   (((x) & 0xffff) << 16) /* [31:16] */
#define REG_PFIFO_UP_THRESHOLD_M    (0xffff << 16)
#define reg_pfifo_down_threshold(x) (((x) & 0xffff) << 0)  /* [15:0] */
#define REG_PFIFO_DOWN_THRESHOLD_M  (0xffff << 0)

#define REG_LM_IN_AC0_CMD     0x200 /* Indirect access to issue commands */
#define reg_in_ac0_addr(x)    (((x) & 0xffff) << 16) /* [31:16] */
#define REG_IN_AC0_ADDR_M     (0xffff << 16)
#define reg_in_ac0_number(x)  (((x) & 0xff) << 8)    /* [15:8] */
#define REG_IN_AC0_NUMBER_M   (0xff << 8)
#define reg_protect_number(x) (((x) & 0xf) << 4)     /* [7:4] */
#define REG_PROTECT_NUMBER_M  (0xf << 4)
#define reg_command(x)        (((x) & 0x1) << 0)     /* [0] */
#define REG_COMMAND_M         (0x1 << 0)

#define REG_LM_CODE_RFD        0x204 /* RFD initial value */
#define reg_lm_ptn_rsv_en(x)   (((x) & 0x1) << 31) /* [31] */
#define REG_LM_PTN_RSV_EN_M    (0x1 << 31)
#define reg_trn_ptn_rsv_set(x) (((x) & 0x1) << 30) /* [30] */
#define REG_TRN_PTN_RSV_SET_M  (0x1 << 30)
#define reg_code_rfd_init3(x)  (((x) & 0xf) << 24) /* [27:24] */
#define REG_CODE_RFD_INIT3_M   (0xf << 24)
#define reg_code_rfd_init2(x)  (((x) & 0xf) << 16) /* [19:16] */
#define REG_CODE_RFD_INIT2_M   (0xf << 16)
#define reg_code_rfd_init1(x)  (((x) & 0xf) << 8)  /* [11:8] */
#define REG_CODE_RFD_INIT1_M   (0xf << 8)
#define reg_code_rfd_init0(x)  (((x) & 0xf) << 0)  /* [3:0] */
#define REG_CODE_RFD_INIT0_M   (0xf << 0)

#define REG_LM_IN_AC0_RDATA 0x208 /* Indirect access returns read data */
#define reg_in_ac0_rdata(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_IN_AC0_RDATA_M  (0xffffffff << 0)

#define REG_LM_IN_AC0_WDATA 0x20C /* Indirect access to write data */
#define reg_in_ac0_wdata(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_IN_AC0_WDATA_M  (0xffffffff << 0)

#define REG_LM_SRC_EN        0x210 /* Scrambling enable register */
#define reg_ram_config_en(x) (((x) & 0x1) << 1) /* [1] */
#define REG_RAM_CONFIG_EN_M  (0x1 << 1)
#define reg_scram_en(x)      (((x) & 0x1) << 0) /* [0] */
#define REG_SCRAM_EN_M       (0x1 << 0)

#define REG_LM_SRC_INIT0        0x214 /* Scrambling initial value register 0 */
#define reg_lane1_init_value(x) (((x) & 0xffff) << 16) /* [31:16] */
#define REG_LANE1_INIT_VALUE_M  (0xffff << 16)
#define reg_lane0_init_value(x) (((x) & 0xffff) << 0)  /* [15:0] */
#define REG_LANE0_INIT_VALUE_M  (0xffff << 0)

#define REG_LM_SRC_INIT1        0x218 /* Scrambling initial value register 1 */
#define reg_lane3_init_value(x) (((x) & 0xffff) << 16) /* [31:16] */
#define REG_LANE3_INIT_VALUE_M  (0xffff << 16)
#define reg_lane2_init_value(x) (((x) & 0xffff) << 0)  /* [15:0] */
#define REG_LANE2_INIT_VALUE_M  (0xffff << 0)

#define REG_LM_COMMAND_EN 0x21C /* lm Register configuration enable */
#define reg_command_en(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_COMMAND_EN_M  (0xffff << 0)

#define REG_RS_SPLIT_SB_CNT      0x300 /* Non-complete super block count input by RS module */
#define reg_rs_split_sb_cnt_f(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_RS_SPLIT_SB_CNT_F_M  (0xffffffff << 0)

#define REG_FRL_CLK_DET_STABLE 0x500 /* Frl clock detection status */
#define reg_frl_clk_stable(x)  (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_CLK_STABLE_M   (0x1 << 0)

#endif /* __HAL_HDMITX_FRL_REG_H__ */

