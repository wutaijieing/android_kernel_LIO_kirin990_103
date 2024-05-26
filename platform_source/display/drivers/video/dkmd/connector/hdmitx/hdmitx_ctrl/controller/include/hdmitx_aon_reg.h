/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver hdmi controller aon reg header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_AON_REG_H__
#define __HDMITX_AON_REG_H__

/* tx_aon module field info */
#define REG_TX_HW_INFO     0x0000
#define reg_tx_hw_year(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_TX_HW_YEAR_M   (0xffff << 16)
#define reg_tx_hw_month(x) (((x) & 0xff) << 8) /* [15:8] */
#define REG_TX_HW_MONTH_M  (0xff << 8)
#define reg_tx_hw_day(x)   (((x) & 0xff) << 0) /* [7:0] */
#define REG_TX_HW_DAY_M    (0xff << 0)

#define REG_TX_HW_VERSION     0x0004
#define reg_tx_rtl_version(x) (((x) & 0xffff) << 16) /* [23:16] */
#define REG_TX_RTL_VERSION_M  (0xffff << 16)
#define reg_tx_drv_version(x) (((x) & 0xff) << 8) /* [15:8] */
#define REG_TX_DRV_VERSION_M  (0xff << 8)
#define reg_tx_reg_version(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_TX_REG_VERSION_M  (0xff << 0)

#define REG_TX_HW_FEATURE 0x0008

#define REG_TX_AON_RST_CTRL         0x001C   /* Reserved register */
#define reg_tx_aon_soft_arst_req(x) (((x) & 1) << 0) /* [0] */
#define REG_TX_AON_SOFT_ARST_REQ_M  (1 << 0)

#define REG_HOTPLUG_ST_CFG      0x0020
#define reg_osc_div_cnt(x)      (((x) & 0xfff) << 4) /* [15:4] */
#define REG_OSC_DIV_CNT_M       (0xfff << 4)
#define reg_hpd_soft_value(x)   (((x) & 1) << 3) /* [3] */
#define REG_HPD_SOFT_VALUE_M    (1 << 3)
#define reg_hpd_polarity_ctl(x) (((x) & 1) << 1) /* [2] */
#define REG_HPD_POLARITY_CTL_M  (1 << 1)
#define reg_hpd_override_en(x)  (((x) & 1) << 1) /* [1] */
#define REG_HPD_OVERRIDE_EN_M   (1 << 1)
#define reg_hpd_fillter_en(x)   (((x) & 1) << 0) /* [0] */
#define REG_HPD_FILLTER_EN_M    (1 << 0)

#define REG_HOTPLUG_FILLTER_CFG 0x0024
#define reg_hpd_low_reshold(x)  (((x) & 0xffff) << 16) /* [31:16] */
#define REG_HPD_LOW_RESHOLD_M   (0xffff << 16)
#define reg_hpd_high_reshold(x) (((x) & 0xffff) << 0) /* [15:0] */
#define REG_HPD_HIGH_RESHOLD_M  (0xffff << 0)

#define REG_TX_AON_STATE     0x0028
#define reg_phy_rx_sense(x)  (((x) & 1) << 1) /* [1] */
#define REG_PHY_RX_SENSE_M   (1 << 1)
#define reg_hotplug_state(x) (((x) & 1) << 0) /* [0] */
#define REG_HOTPLUG_STATE_M  (1 << 0)

#define REG_TX_AON_INTR_MASK   0x0030
#define reg_aon_intr_mask13(x) (((x) & 1) << 13) /* [13] */
#define REG_AON_INTR_MASK13_M  (1 << 13)
#define reg_aon_intr_mask12(x) (((x) & 1) << 12) /* [12] */
#define REG_AON_INTR_MASK12_M  (1 << 12)
#define reg_aon_intr_mask11(x) (((x) & 1) << 11) /* [11] */
#define REG_AON_INTR_MASK11_M  (1 << 11)
#define reg_aon_intr_mask10(x) (((x) & 1) << 10) /* [10] */
#define REG_AON_INTR_MASK10_M  (1 << 10)
#define reg_aon_intr_mask9(x)  (((x) & 1) << 9)  /* [9] */
#define REG_AON_INTR_MASK9_M   (1 << 9)
#define reg_aon_intr_mask8(x)  (((x) & 1) << 8)  /* [8] */
#define REG_AON_INTR_MASK8_M   (1 << 8)
#define reg_aon_intr_mask7(x)  (((x) & 1) << 7)  /* [7] */
#define REG_AON_INTR_MASK7_M   (1 << 7)
#define reg_aon_intr_mask6(x)  (((x) & 1) << 6)  /* [6] */
#define REG_AON_INTR_MASK6_M   (1 << 6)
#define reg_aon_intr_mask5(x)  (((x) & 1) << 5)  /* [5] */
#define REG_AON_INTR_MASK5_M   (1 << 5)
#define reg_aon_intr_mask4(x)  (((x) & 1) << 4)  /* [4] */
#define REG_AON_INTR_MASK4_M   (1 << 4)
#define reg_aon_intr_mask1(x)  (((x) & 1) << 1)  /* [1] */
#define REG_AON_INTR_MASK1_M   (1 << 1)
#define reg_aon_intr_mask0(x)  (((x) & 1) << 0)  /* [0] */
#define REG_AON_INTR_MASK0_M   (1 << 0)

#define REG_TX_AON_INTR_STATE  0x0034
#define reg_aon_intr_stat13(x) (((x) & 1) << 13) /* [13] */
#define REG_AON_INTR_STAT13_M  (1 << 13)
#define reg_aon_intr_stat12(x) (((x) & 1) << 12) /* [12] */
#define REG_AON_INTR_STAT12_M  (1 << 12)
#define reg_aon_intr_stat11(x) (((x) & 1) << 11) /* [11] */
#define REG_AON_INTR_STAT11_M  (1 << 11)
#define reg_aon_intr_stat10(x) (((x) & 1) << 10) /* [10] */
#define REG_AON_INTR_STAT10_M  (1 << 10)
#define reg_aon_intr_stat9(x)  (((x) & 1) << 9)  /* [9] */
#define REG_AON_INTR_STAT9_M   (1 << 9)
#define reg_aon_intr_stat8(x)  (((x) & 1) << 8)  /* [8] */
#define REG_AON_INTR_STAT8_M   (1 << 8)
#define reg_aon_intr_stat7(x)  (((x) & 1) << 7)  /* [7] */
#define REG_AON_INTR_STAT7_M   (1 << 7)
#define reg_aon_intr_stat6(x)  (((x) & 1) << 6)  /* [6] */
#define REG_AON_INTR_STAT6_M   (1 << 6)
#define reg_aon_intr_stat5(x)  (((x) & 1) << 5)  /* [5] */
#define REG_AON_INTR_STAT5_M   (1 << 5)
#define reg_aon_intr_stat4(x)  (((x) & 1) << 4)  /* [4] */
#define REG_AON_INTR_STAT4_M   (1 << 4)
#define reg_aon_intr_stat1(x)  (((x) & 1) << 1)  /* [1] */
#define REG_AON_INTR_STAT1_M   (1 << 1)
#define reg_aon_intr_stat0(x)  (((x) & 1) << 0)  /* [0] */
#define REG_AON_INTR_STAT0_M   (1 << 0)

/* DDC module regs */
#define REG_DDC_MST_CTRL      0x0 /* 0x40 */
#define reg_ddc_speed_cnt(x)  (((x) & 0xff) << 4) /* [12:4] */
#define REG_DDC_SPEED_CNT_M   (0xff << 4)
#define reg_dcc_man_en(x)     (((x) & 1) << 1) /* [1] */
#define REG_DCC_MAN_EN_M      (1 << 1)
#define reg_ddc_aon_access(x) (((x) & 1) << 0) /* [0] */
#define REG_DDC_AON_ACCESS_M  (1 << 0)

#define REG_DDC_FIFO_RDATA       0x4 /* 0x44 */
#define reg_ddc_fifo_data_out(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DDC_FIFO_DATA_OUT_M  (0xff << 0)

#define REG_DDC_FIFO_WDATA      0x8 /* 0x48 */
#define reg_ddc_fifo_data_in(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DDC_FIFO_DATA_IN_M  (0xff << 0)

#define REG_DDC_DATA_CNT         0xC /* 0x4c */
#define reg_ddc_data_out_cnt(x)  (((x) & 0x3ff) << 8) /* [17:8] */
#define REG_DDC_DATA_OUT_CNT_M   (0x3ff << 8)
#define reg_fifo_data_cnt(x)     (((x) & 0x1f) << 0)  /* [4:0] */
#define REG_DDC_FIFO_DATA_CNT_M  (0x1f << 0)

#define REG_DDC_SLAVE_CFG       0x10 /* 0x50 */
#define reg_ddc_slave_seg(x)    (((x) & 0xff) << 16) /* [23:16] */
#define REG_DDC_SLAVE_SEG_M     (0xff << 16)
#define reg_ddc_slave_offset(x) (((x) & 0xff) << 8) /* [15:8] */
#define REG_DDC_SLAVE_OFFSET_M  (0xff << 8)
#define reg_ddc_slave_addr(x)   (((x) & 0xff) << 0) /* [7:0] */
#define REG_DDC_SLAVE_ADDR_M    (0xff << 0)

#define REG_DDC_MST_STATE         0x14 /* 0x54 */
#define reg_ddc_fifo_full(x)      (((x) & 1) << 7) /* [7] */
#define REG_DDC_FIFO_FULL_M       (1 << 7)
#define reg_ddc_fifo_half_full(x) (((x) & 1) << 6) /* [6] */
#define REG_DDC_FIFO_HALF_FULL_M  (1 << 6)
#define reg_ddc_fifo_empty(x)     (((x) & 1) << 5) /* [5] */
#define REG_DDC_FIFO_EMPTY_M      (1 << 5)
#define reg_ddc_fifo_rd_in_use(x) (((x) & 1) << 4) /* [4] */
#define REG_DDC_FIFO_RD_IN_USE_M  (1 << 4)
#define reg_ddc_fifo_wr_in_use(x) (((x) & 1) << 3) /* [3] */
#define REG_DDC_FIFO_WR_IN_USE_M  (1 << 3)
#define reg_ddc_i2c_in_prog(x)    (((x) & 1) << 2) /* [2] */
#define REG_DDC_I2C_IN_PROG_M     (1 << 2)
#define reg_ddc_i2c_bus_low(x)    (((x) & 1) << 1) /* [1] */
#define REG_DDC_I2C_BUS_LOW_M     (1 << 1)
#define reg_ddc_i2c_no_ack(x)     (((x) & 1) << 0) /* [0] */
#define REG_DDC_I2C_NO_ACK_M      (1 << 0)

#define REG_DDC_MST_CMD      0x18 /* 0x58 */
#define reg_ddc_mst_cmd_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_DDC_MST_CMD_M    (0xf << 0)

#define REG_DDC_MAN_CTRL   0x1C /* 0x5c */
#define reg_ddc_sda_oen(x) (((x) & 1) << 3) /* [3] */
#define REG_DDC_SDA_OEN_M  (1 << 3)
#define reg_ddc_scl_oen(x) (((x) & 1) << 2) /* [2] */
#define REG_DDC_SCL_OEN_M  (1 << 2)
#define reg_ddc_sda_st(x)  (((x) & 1) << 1) /* [1] */
#define REG_DDC_SDA_ST_M   (1 << 1)
#define reg_ddc_scl_st(x)  (((x) & 1) << 0) /* [0] */
#define REG_DDC_SCL_ST_M   (1 << 0)

#define REG_DDC_STATE_CLR      0x20 /* 0x60 */
#define reg_ddc_clr_no_ack(x)  (((x) & 1) << 1) /* [1] */
#define REG_DDC_CLR_NO_ACK_M   (1 << 1)
#define reg_ddc_clr_bus_low(x) (((x) & 1) << 0) /* [0] */
#define REG_DDC_CLR_BUS_LOW_M  (1 << 0)

#define REG_CFG_CEC_SEL      0x0064
#define reg_cfg_cec_sel_f(x) (((x) & 1) << 0) /* [0] */
#define REG_CFG_CEC_SEL_F_M  (1 << 0)

#endif
