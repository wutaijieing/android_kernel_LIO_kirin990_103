/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver hdmi trainning reg header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_LINK_TRAINING_REG_H__
#define __HDMITX_LINK_TRAINING_REG_H__

/* training_cfg module field info */
#define reg_flt_mcu_pram(s)   (0x0000 + 0x4 * (s))       /* FRL training MCU program space */
#define reg_flt_mcu_pram_f(x) (((x) & 0xffffffff) << 0) /* [31:0] */
#define REG_FLT_MCU_PRAM_M    (0xffffffff << 0)

#define reg_flt_mcu_dram(s)   (0x8000 + 0x4 * (s)) /* FRL training MCU data space */
#define reg_flt_mcu_dram_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_FLT_MCU_DRAM_M    (0xff << 0)

#define REG_FRL_TRN_START      0x9000 /* FRL configuration MCU training begins */
#define reg_frl_trn_start_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_TRN_START_M    (0x1 << 0)

#define REG_FRL_TRN_STOP      0x9004 /* FRL configuration MCU training stop */
#define reg_frl_trn_stop_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_TRN_STOP_M    (0x1 << 0)

#define REG_FRL_TRN_MODE      0x9008 /* FRL configuration training mode */
#define reg_frl_trn_mode_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_TRN_MODE_M    (0x1 << 0)

#define REG_FRL_MCU_UART_CTRL     0x900C /* MCU UART control register */
#define reg_mcu_uart_stop_sel(x)  (((x) & 0x1) << 14)  /* [14] */
#define REG_MCU_UART_STOP_SEL_M   (0x1 << 14)
#define reg_mcu_uart_rate_sel(x)  (((x) & 0x3) << 12)  /* [13:12] */
#define REG_MCU_UART_RATE_SEL_M   (0x3 << 12)
#define reg_mcu_uart_band_unit(x) (((x) & 0xfff) << 0) /* [11:0] */
#define REG_MCU_UART_BAND_UNIT_M  (0xfff << 0)

#define REG_FRL_MCU_TRN_RD_BACK_EN 0x9010 /* MCU training readback Sink configuration enable */
#define reg_mcu_trn_rd_back_en(x)  (((x) & 0x1) << 0) /* [0] */
#define REG_MCU_TRN_RD_BACK_EN_M   (0x1 << 0)

#define REG_FRL_MCU_NO_TIMEOUT_EN 0x9014 /* MCU training is Polling enabled */
#define reg_mcu_trn_poll_en(x)    (((x) & 0x1) << 0) /* [0] */
#define REG_MCU_TRN_POLL_EN_M     (0x1 << 0)

#define REG_FRL_POLLING_TIMER_THD 0x9018 /* FRL training polling timer threshold configuration */
#define reg_frl_poll_timer_thd(x) (((x) & 0xffffff) << 0) /* [0] */
#define REG_FRL_POLL_TIMER_THD_M  (0xffffff << 0)

#define REG_FRL_POLLING_TIMER_START 0x901C /* FRL training starts regularly */
#define reg_frl_poll_timer_start(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_POLL_TIMER_START_M  (0x1 << 0)

#define REG_FRL_TRN_INT_MASK                0x9020 /* FRL training interrupt mask enable */
#define reg_frl_mcu_trn_finish_int_mask(x)  (((x) & 0x1) << 3) /* [3] */
#define REG_FRL_MCU_TRN_FINISH_INT_MASK_M   (0x1 << 3)
#define reg_frl_ncu_trn_success_int_mask(x) (((x) & 0x1) << 2) /* [2] */
#define REG_FRL_NCU_TRN_SUCCESS_INT_MASK_M  (0x1 << 2)
#define reg_frl_mcu_trn_fail_int_mask(x)    (((x) & 0x1) << 1) /* [1] */
#define REG_FRL_MCU_TRN_FAIL_INT_MASK_M     (0x1 << 1)
#define reg_frl_poll_timer_int_mask(x)      (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_POLL_TIMER_INT_MASK_M       (0x1 << 0)

#define REG_FRL_TRN_INT_RAW                0x9024 /* FRL Training Interrupt Status Register */
#define reg_frl_mcu_trn_fininsh_int_raw(x) (((x) & 0x1) << 3) /* [3] */
#define REG_FRL_MCU_TRN_FININSH_INT_RAW_M  (0x1 << 3)
#define reg_frl_mcu_scuccess_int_raw(x)    (((x) & 0x1) << 2) /* [2] */
#define REG_FRL_MCU_SCUCCESS_INT_RAW_M     (0x1 << 2)
#define reg_frl_mcu_trn_fail_int_raw(x)    (((x) & 0x1) << 1) /* [1] */
#define REG_FRL_MCU_TRN_FAIL_INT_RAW_M     (0x1 << 1)
#define reg_frl_poll_timer_int_raw(x)      (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_POLL_TIMER_INT_RAW_M       (0x1 << 0)

#define REG_DEBUG0      0x9028 /* DEBUG0 backup */
#define reg_debug0_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUG0_M    (0xff << 0)

#define REG_DEBUG1      0x902C /* DEBUG1 backup */
#define reg_debug1_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUG1_M    (0xff << 0)

#define REG_DEBUGOUT0      0x9030 /* DEBUGOUT0 backup */
#define reg_debugout0_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUGOUT0_M    (0xff << 0)

#define REG_DEBUGOUT1      0x9034 /* DEBUGOUT1 backup */
#define reg_debugout1_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUGOUT1_M    (0xff << 0)

#define REG_FRL_CHAN_SEL      0x9038 /* FRL link selection configuration */
#define reg_frl_chan_sel_f(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_CHAN_SEL_M    (0x1 << 0)

#define REG_FRL_TRN_RATE      0x903C /* FRL training rate configuration */
#define reg_frl_trn_rate_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_FRL_TRN_RATE_M    (0xf << 0)

#define REG_FRL_TRN_FFE_NUMB      0x9040 /* FRL training FFE configuration */
#define reg_frl_trn_ffe_numb_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_FRL_TRN_FFE_NUMB_M    (0xf << 0)

#define REG_FRL_TRN_LTP0     0x9044 /* FRL training Sink ln(0..1)_LTP_req value */
#define reg_lane1_ltp_req(x) (((x) & 0xf) << 4) /* [7:4] */
#define REG_LANE1_LTP_REQ_M  (0xf << 4)
#define reg_lane0_ltp_req(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_LANE0_LTP_REQ_M  (0xf << 0)

#define REG_FRL_TRN_LTP1     0x9048 /* FRL training Sink ln(2..3)_LTP_req value */
#define reg_lane3_ltp_req(x) (((x) & 0xf) << 4) /* [7:4] */
#define REG_LANE3_LTP_REQ_M  (0xf << 4)
#define reg_lane2_ltp_req(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_LANE2_LTP_REQ_M  (0xf << 0)

#define REG_SOURCE_FFE_UP0         0x904C /* SOURCE lane0_lane1 FFE value update during training */
#define reg_source_lane1_ffe_up(x) (((x) & 0xf) << 4) /* [7:4] */
#define REG_SOURCE_LANE1_FFE_UP_M  (0xf << 4)
#define reg_source_lane0_ffe_up(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_SOURCE_LANE0_FFE_UP_M  (0xf << 0)

#define REG_SOURCE_FFE_UP1         0x9050 /* SOURCE lane2_lane3 FFE value update during training */
#define reg_source_lane3_ffe_up(x) (((x) & 0xf) << 4) /* [7:4] */
#define REG_SOURCE_LANE3_FFE_UP_M  (0xf << 4)
#define reg_source_lane2_ffe_up(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_SOURCE_LANE2_FFE_UP_M  (0xf << 0)

#define REG_FRL_MCU_TRN_STATUS      0x9054 /* MCU training status record */
#define reg_frl_mcu_trn_status_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_FRL_MCU_TRN_STATUS_M    (0xf << 0)

#define REG_FRL_MCU_TRN_FAIL_RES      0x9058 /* MCU training failure reason record */
#define reg_frl_mcu_trn_fail_res_f(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_FRL_MCU_TRN_FAIL_RES_M    (0xf << 0)

#define REG_FRL_MCU_EXINT_MASK          0x905C /* MCU External interrupt mask enable */
#define reg_frl_mcu_exint_stop_mask(x)  (((x) & 0x1) << 1) /* [1] */
#define REG_FRL_MCU_EXINT_STOP_MASK_M   (0x1 << 1)
#define reg_frl_mcu_exint_start_mask(x) (((x) & 0x1) << 0) /* [0] */
#define REG_FRL_MCU_EXINT_START_MASK_M  (0x1 << 0)

#define REG_FRL_MCU_EXINT_RAW    0x9060 /* MCU External interrupt status */
#define reg_mcu_stop_int_raw(x)  (((x) & 0x1) << 1) /* [1] */
#define REG_MCU_STOP_INT_RAW_M   (0x1 << 1)
#define reg_mcu_start_int_raw(x) (((x) & 0x1) << 0) /* [0] */
#define REG_MCU_START_INT_RAW_M  (0x1 << 0)

#define REG_FRL_MCU_TRN_CNT 0x9064 /* MCU training count */
#define reg_mcu_trn_cnt(x)  (((x) & 0xf) << 0) /* [3:0] */
#define REG_MCU_TRN_CNT_M   (0xf << 0)

#define REG_DEBUG2      0x9068 /* DEBUG register 2 */
#define reg_debug2_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUG2_M    (0xff << 0)

#define REG_DEBUG3      0x906C /* DEBUG register 3 */
#define reg_debug3_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUG3_M    (0xff << 0)

#define REG_MCU_DDC_SLAVE    0x9070 /* DDC slave Base address */
#define reg_mcu2ddc_slave(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_MCU2DDC_SLAVE_M  (0xff << 0)

#define REG_MCU_DDC_OFFSET    0x9074 /* DDC slave Offset address */
#define reg_mcu2ddc_offset(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_MCU2DDC_OFFSET_M  (0xff << 0)

#define REG_MCU_DDC_SEGMENT    0x9078 /* DDC slave segment value */
#define reg_mcu2ddc_segment(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_MCU2DDC_SEGMENT_M  (0xff << 0)

#define REG_MCU_DDC_BYTE_LB    0x907C /* DDC slave Send data byte number is lower 8bit */
#define reg_mcu2ddc_byte_lb(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_MCU2DDC_BYTE_LB_M  (0xff << 0)

#define REG_MCU_DDC_BYTE_HB    0x9080 /* DDC slave Send data byte high 2bit */
#define reg_mcu2ddc_byte_hb(x) (((x) & 0x3) << 0) /* [1:0] */
#define REG_MCU2DDC_BYTE_HB_M  (0x3 << 0)

#define REG_MCU_DDC_WDATA    0x9084 /* DDC FIFO write data */
#define reg_mcu2ddc_wdata(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_MCU2DDC_WDATA_M  (0xff << 0)

#define REG_MCU_DDC_RD    0x9088 /* DDC FIFO read enable */
#define reg_mcu2ddc_rd(x) (((x) & 0x1) << 0) /* [0] */
#define REG_MCU2DDC_RD_M  (0x1 << 0)

#define REG_MCU_DDC_CMD    0x908C /* DDC master Instruction configuration */
#define reg_mcu2ddc_cmd(x) (((x) & 0xf) << 0) /* [3:0] */
#define REG_MCU2DDC_CMD_M  (0xf << 0)

#define REG_MCU_DDC_REQ    0x9090 /* DDC Read request */
#define reg_mcu2ddc_req(x) (((x) & 0x1) << 0) /* [0] */
#define REG_MCU2DDC_REQ_M  (0x1 << 0)

#define REG_MCU_DDC_CLR            0x9094 /* DDC Clear status */
#define reg_mcu2ddc_clr_no_ack(x)  (((x) & 0x1) << 1) /* [1] */
#define REG_MCU2DDC_CLR_NO_ACK_M   (0x1 << 1)
#define reg_mcu2ddc_clr_bus_low(x) (((x) & 0x1) << 0) /* [0] */
#define REG_MCU2DDC_CLR_BUS_LOW_M  (0x1 << 0)

#define REG_DDC_MCU_FIFO_CNT    0x9098 /* DDC FIFO CNT count */
#define reg_ddc2mcu_fifo_cnt(x) (((x) & 0xf) << 0) /* [4:0] */
#define REG_DDC2MCU_FIFO_CNT_M  (0xf << 0)

#define REG_DDC_MCU_FIFO_RDATA    0x909C /* DDC FIFO reading data */
#define reg_ddc2mcu_fifo_rdata(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DDC2MCU_FIFO_RDATA_M  (0xff << 0)

#define REG_DDC_MCU_STATUS    0x90A0 /* DDC input status */
#define reg_ddc2mcu_status(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DDC2MCU_STATUS_M  (0xff << 0)

#define REG_DDC_MCU_REQ_ACK    0x90A4 /* DDC master Return ack status */
#define reg_ddc2mcu_req_ack(x) (((x) & 0x1) << 0) /* [0] */
#define REG_DDC2MCU_REQ_ACK_M  (0x1 << 0)

#define REG_DEBUGOUT2      0x90A8 /* DEBUGOUT Register 2 */
#define reg_debugout2_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUGOUT2_F_M  (0xff << 0)

#define REG_DEBUGOUT3      0x90AC /* DEBUGOUT Register 3 */
#define reg_debugout3_f(x) (((x) & 0xff) << 0) /* [7:0] */
#define REG_DEBUGOUT3_F_M  (0xff << 0)

#endif /* __HAL_HDMITX_LINK_TRAINING_REG_H__ */

