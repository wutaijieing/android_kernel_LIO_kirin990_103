// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  cmdlst_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2013/3/10
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.2
// History       :  xxx 2020/04/02 14:01:19 Create file
// ******************************************************************************

#ifndef __CMDLST_DRV_PRIV_H__
#define __CMDLST_DRV_PRIV_H__

/* Define the union U_CMDLST_ID */
union u_cmdlst_id {
	struct {
		unsigned int    ip_id : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_VERSION */
union u_version {
	struct {
		unsigned int    ip_version : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_ST_LD */
union u_st_ld {
	struct {
		unsigned int    store_load : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CFG */
union u_cfg {
	struct {
		unsigned int    rsv_0                : 1  ; /* [0] */
		unsigned int    slowdown_nrt_channel : 2  ; /* [2:1] */
		unsigned int    rsv_1                : 29  ; /* [31:3] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_START_ADDR */
union u_start_addr {
	struct {
		unsigned int    start_addr : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_END_ADDR */
union u_end_addr {
	struct {
		unsigned int    end_addr : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CHECK_ERROR_STATUS */
union u_check_error_status {
	struct {
		unsigned int    check_error_status : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_VCD_TRACE */
union u_vcd_trace {
	struct {
		unsigned int    vcd_out_select : 3  ; /* [2:0] */
		unsigned int    rsv_2          : 29  ; /* [31:3] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_0 */
union u_debug_0 {
	struct {
		unsigned int    dbg_fifo_nb_elem  : 3  ; /* [2:0] */
		unsigned int    rsv_3             : 1  ; /* [3] */
		unsigned int    dbg_lb_master_fsm : 4  ; /* [7:4] */
		unsigned int    dbg_vp_wr_fsm     : 2  ; /* [9:8] */
		unsigned int    rsv_4             : 2  ; /* [11:10] */
		unsigned int    dbg_arb_fsm       : 1  ; /* [12] */
		unsigned int    rsv_5             : 19  ; /* [31:13] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_1 */
union u_debug_1 {
	struct {
		unsigned int    last_cmd_exec : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_2 */
union u_debug_2 {
	struct {
		unsigned int    dbg_sw_start : 16  ; /* [15:0] */
		unsigned int    dbg_hw_start : 16  ; /* [31:16] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_3 */
union u_debug_3 {
	struct {
		unsigned int    dbg_sw_start : 16  ; /* [15:0] */
		unsigned int    dbg_hw_start : 16  ; /* [31:16] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_4 */
union u_debug_4 {
	struct {
		unsigned int    dbg_fsm_ch_0 : 4  ; /* [3:0] */
		unsigned int    dbg_fsm_ch_1 : 4  ; /* [7:4] */
		unsigned int    dbg_fsm_ch_2 : 4  ; /* [11:8] */
		unsigned int    dbg_fsm_ch_3 : 4  ; /* [15:12] */
		unsigned int    dbg_fsm_ch_4 : 4  ; /* [19:16] */
		unsigned int    dbg_fsm_ch_5 : 4  ; /* [23:20] */
		unsigned int    dbg_fsm_ch_6 : 4  ; /* [27:24] */
		unsigned int    dbg_fsm_ch_7 : 4  ; /* [31:28] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CH_CFG */
union u_ch_cfg {
	struct {
		unsigned int    active_token_nbr        : 7  ; /* [6:0] */
		unsigned int    active_token_nbr_enable : 1  ; /* [7] */
		unsigned int    nrt_channel             : 1  ; /* [8] */
		unsigned int    rsv_6                   : 23  ; /* [31:9] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CHECK_ERROR_PTR */
union u_check_error_ptr {
	struct {
		unsigned int    check_error_ptr : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CHECK_ERROR_TSK */
union u_check_error_tsk {
	struct {
		unsigned int    check_error_task : 8  ; /* [7:0] */
		unsigned int    rsv_7            : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_CHECK_ERROR_CMD */
union u_check_error_cmd {
	/* Define the struct bits */
	struct {
		unsigned int    check_error_cmd : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_BRANCH */
union u_sw_branch {
	struct {
		unsigned int    branching : 1  ; /* [0] */
		unsigned int    rsv_8     : 31  ; /* [31:1] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_LAST_EXEC_PTR */
union u_last_exec_ptr {
	struct {
		unsigned int    last_exec_ptr : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_LAST_EXEC_INFO */
union u_last_exec_info {
	struct {
		unsigned int    last_exec_task : 8  ; /* [7:0] */
		unsigned int    rsv_9          : 24  ; /* [31:8] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CH_MNGR */
union u_sw_ch_mngr {
	struct {
		unsigned int    sw_task     : 8  ; /* [7:0] */
		unsigned int    sw_resource : 23  ; /* [30:8] */
		unsigned int    sw_priority : 1  ; /* [31] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CVDR_RD_ADDR */
union u_sw_cvdr_rd_addr {
	struct {
		unsigned int    reserved_sw_0      : 2  ; /* [1:0] */
		unsigned int    sw_cvdr_rd_address : 20  ; /* [21:2] */
		unsigned int    reserved_sw_1      : 2  ; /* [23:22] */
		unsigned int    sw_cvdr_rd_size    : 2  ; /* [25:24] */
		unsigned int    reserved_sw_2      : 6  ; /* [31:26] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CVDR_RD_DATA_0 */
union u_sw_cvdr_rd_data_0 {
	struct {
		unsigned int    sw_vprd_pixel_format    : 5  ; /* [4:0] */
		unsigned int    sw_vprd_pixel_expansion : 1  ; /* [5] */
		unsigned int    sw_vprd_allocated_du    : 5  ; /* [10:6] */
		unsigned int    sw_vprd_mono_mode       : 1  ; /* [11] */
		unsigned int    sw_reserved_0           : 1  ; /* [12] */
		unsigned int    sw_vprd_last_page       : 19  ; /* [31:13] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CVDR_RD_DATA_1 */
union u_sw_cvdr_rd_data_1 {
	struct {
		unsigned int    sw_vprd_line_size           : 15  ; /* [14:0] */
		unsigned int    sw_reserved_2               : 1  ; /* [15] */
		unsigned int    sw_vprd_horizontal_blanking : 8  ; /* [23:16] */
		unsigned int    sw_reserved_1               : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CVDR_RD_DATA_2 */
union u_sw_cvdr_rd_data_2 {
	struct {
		unsigned int    sw_vprd_frame_size        : 15  ; /* [14:0] */
		unsigned int    sw_reserved_4             : 1  ; /* [15] */
		unsigned int    sw_vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    sw_reserved_3             : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_SW_CVDR_RD_DATA_3 */
union u_sw_cvdr_rd_data_3 {
	struct {
		unsigned int    sw_reserved_5           : 2  ; /* [1:0] */
		unsigned int    sw_vprd_axi_frame_start : 30  ; /* [31:2] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CH_MNGR */
union u_hw_ch_mngr {
	struct {
		unsigned int    hw_task     : 8  ; /* [7:0] */
		unsigned int    hw_resource : 23  ; /* [30:8] */
		unsigned int    hw_priority : 1  ; /* [31] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CVDR_RD_ADDR */
union u_hw_cvdr_rd_addr {
	struct {
		unsigned int    reserved_hw_0      : 2  ; /* [1:0] */
		unsigned int    hw_cvdr_rd_address : 20  ; /* [21:2] */
		unsigned int    reserved_hw_1      : 2  ; /* [23:22] */
		unsigned int    hw_cvdr_rd_size    : 2  ; /* [25:24] */
		unsigned int    reserved_hw_2      : 6  ; /* [31:26] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CVDR_RD_DATA_0 */
union u_hw_cvdr_rd_data_0 {
	struct {
		unsigned int    hw_vprd_pixel_format    : 5  ; /* [4:0] */
		unsigned int    hw_vprd_pixel_expansion : 1  ; /* [5] */
		unsigned int    hw_vprd_allocated_du    : 5  ; /* [10:6] */
		unsigned int    hw_vprd_mono_mode       : 1  ; /* [11] */
		unsigned int    hw_reserved_0           : 1  ; /* [12] */
		unsigned int    hw_vprd_last_page       : 19  ; /* [31:13] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CVDR_RD_DATA_1 */
union u_hw_cvdr_rd_data_1 {
	struct {
		unsigned int    hw_vprd_line_size           : 15  ; /* [14:0] */
		unsigned int    hw_reserved_2               : 1  ; /* [15] */
		unsigned int    hw_vprd_horizontal_blanking : 8  ; /* [23:16] */
		unsigned int    hw_reserved_1               : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CVDR_RD_DATA_2 */
union u_hw_cvdr_rd_data_2 {
	struct {
		unsigned int    hw_vprd_frame_size        : 15  ; /* [14:0] */
		unsigned int    hw_reserved_4             : 1  ; /* [15] */
		unsigned int    hw_vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    hw_reserved_3             : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_HW_CVDR_RD_DATA_3 */
union u_hw_cvdr_rd_data_3 {
	struct {
		unsigned int    hw_reserved_5           : 2  ; /* [1:0] */
		unsigned int    hw_vprd_axi_frame_start : 30  ; /* [31:2] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CMD_CFG */
union u_cmd_cfg {
	struct {
		unsigned int    wait_eop : 1  ; /* [0] */
		unsigned int    rsv_10   : 31  ; /* [31:1] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_TOKEN_CFG */
union u_token_cfg {
	struct {
		unsigned int    token_mode     : 1  ; /* [0] */
		unsigned int    rsv_11         : 7  ; /* [7:1] */
		unsigned int    link_channel   : 5  ; /* [12:8] */
		unsigned int    rsv_12         : 3  ; /* [15:13] */
		unsigned int    link_token_nbr : 4  ; /* [19:16] */
		unsigned int    rsv_13         : 12  ; /* [31:20] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_EOS */
union u_eos {
	struct {
		unsigned int    line_nbr     : 12  ; /* [11:0] */
		unsigned int    rsv_14       : 12  ; /* [23:12] */
		unsigned int    line_ovl_nbr : 7  ; /* [30:24] */
		unsigned int    rsv_15       : 1  ; /* [31] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_CHECK_CTRL */
union u_check_ctrl {
	struct {
		unsigned int    check_enable : 1  ; /* [0] */
		unsigned int    rsv_16       : 31  ; /* [31:1] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_VP_WR_FLUSH */
union u_vp_wr_flush {
	struct {
		unsigned int    vp_wr_flush : 1  ; /* [0] */
		unsigned int    rsv_17      : 31  ; /* [31:1] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_EOS */
union u_debug_eos {
	struct {
		unsigned int    eos_dbg : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_CHANNEL */
union u_debug_channel {
	struct {
		unsigned int    dbg_fsm_ch   : 4  ; /* [3:0] */
		unsigned int    dbg_sw_start : 1  ; /* [4] */
		unsigned int    rsv_18       : 3  ; /* [7:5] */
		unsigned int    dbg_hw_start : 1  ; /* [8] */
		unsigned int    rsv_19       : 23  ; /* [31:9] */
	} bits;

	unsigned int    u32;
};

struct s_cmdlst_regs_type {
	union u_cmdlst_id                 id                    ; /* 0 */
	union u_version            version               ; /* 4 */
	union u_st_ld              st_ld                 ; /* 10 */
	union u_cfg                cfg                   ; /* 2C */
	union u_start_addr         start_addr            ; /* 30 */
	union u_end_addr           end_addr              ; /* 34 */
	union u_check_error_status check_error_status    ; /* 40 */
	union u_vcd_trace          vcd_trace             ; /* 44 */
	union u_debug_0            debug_0               ; /* 48 */
	union u_debug_1            debug_1               ; /* 4C */
	union u_debug_2            debug_2               ; /* 50 */
	union u_debug_3            debug_3               ; /* 54 */
	union u_debug_4            debug_4[4]            ; /* 58 */
	union u_ch_cfg             ch_cfg[24]            ; /* 80 */
	union u_check_error_ptr    check_error_ptr[24]   ; /* 84 */
	union u_check_error_tsk    check_error_tsk[24]   ; /* 88 */
	union u_check_error_cmd    check_error_cmd[24]   ; /* 8C */
	union u_sw_branch          sw_branch[24]         ; /* 90 */
	union u_last_exec_ptr      last_exec_ptr[24]     ; /* 94 */
	union u_last_exec_info     last_exec_info[24]    ; /* 98 */
	union u_sw_ch_mngr         sw_ch_mngr[24]        ; /* 9C */
	union u_sw_cvdr_rd_addr    sw_cvdr_rd_addr[24]   ; /* A0 */
	union u_sw_cvdr_rd_data_0  sw_cvdr_rd_data_0[24] ; /* A4 */
	union u_sw_cvdr_rd_data_1  sw_cvdr_rd_data_1[24] ; /* A8 */
	union u_sw_cvdr_rd_data_2  sw_cvdr_rd_data_2[24] ; /* AC */
	union u_sw_cvdr_rd_data_3  sw_cvdr_rd_data_3[24] ; /* B0 */
	union u_hw_ch_mngr         hw_ch_mngr[24]        ; /* B4 */
	union u_hw_cvdr_rd_addr    hw_cvdr_rd_addr[24]   ; /* B8 */
	union u_hw_cvdr_rd_data_0  hw_cvdr_rd_data_0[24] ; /* BC */
	union u_hw_cvdr_rd_data_1  hw_cvdr_rd_data_1[24] ; /* C0 */
	union u_hw_cvdr_rd_data_2  hw_cvdr_rd_data_2[24] ; /* C4 */
	union u_hw_cvdr_rd_data_3  hw_cvdr_rd_data_3[24] ; /* C8 */
	union u_cmd_cfg            cmd_cfg[24]           ; /* CC */
	union u_token_cfg          token_cfg[24]         ; /* D0 */
	union u_eos                eos[24]               ; /* D4 */
	union u_check_ctrl         check_ctrl[24]        ; /* DC */
	union u_vp_wr_flush        vp_wr_flush[24]       ; /* E8 */
	union u_debug_eos          debug_eos[24]         ; /* EC */
	union u_debug_channel      debug_channel[24]     ; /* F0 */
};

#endif // __CMDLST_DRV_PRIV_H__
