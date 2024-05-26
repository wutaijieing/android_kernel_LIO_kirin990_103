// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  ipp_top_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2013/3/10
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.2
// History       :  xxx 2020/04/15 14:54:24 Create file
// ******************************************************************************

#ifndef __IPP_TOP_DRV_PRIV_H_CMDLST_
#define __IPP_TOP_DRV_PRIV_H_CMDLST_

/* Define the union U_CMDLST_CTRL_MAP */
union u_cmdlst_ctrl_map {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_ctrl_chn : 4  ; /* [3:0] */
		unsigned int    rsv_103         : 28  ; /* [31:4] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_CTRL_PM */
union u_cmdlst_ctrl_pm {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_post_mask : 2  ; /* [1:0] */
		unsigned int    rsv_104          : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_CLR */
union u_cmdlst_irq_clr {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_clr : 16  ; /* [15:0] */
		unsigned int    rsv_105        : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_MSK */
union u_cmdlst_irq_msk {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_mask : 16  ; /* [15:0] */
		unsigned int    rsv_106         : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_STATE_RAW */
union u_cmdlst_irq_state_raw {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_state_raw : 16  ; /* [15:0] */
		unsigned int    rsv_107              : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_STATE_MSK */
union u_cmdlst_irq_state_msk {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_state_mask : 16  ; /* [15:0] */
		unsigned int    rsv_108               : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

#endif // __IPP_TOP_DRV_PRIV_H_CMDLST_
