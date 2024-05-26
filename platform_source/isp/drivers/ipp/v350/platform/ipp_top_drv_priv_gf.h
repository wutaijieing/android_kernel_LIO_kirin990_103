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

#ifndef __IPP_TOP_DRV_PRIV_H_GF_
#define __IPP_TOP_DRV_PRIV_H_GF_

/* Define the union U_GF_CRG_CFG0 */
union u_gf_crg_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_clken        : 1  ; /* [0] */
		unsigned int    rsv_30          : 15  ; /* [15:1] */
		unsigned int    gf_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_31          : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_CRG_CFG1 */
union u_gf_crg_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_32      : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_MEM_CFG */
union u_gf_mem_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    gf_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_33         : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG0 */
union u_gf_irq_reg0 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_clr : 13  ; /* [12:0] */
		unsigned int    rsv_34     : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG1 */
union u_gf_irq_reg1 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_35       : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG2 */
union u_gf_irq_reg2 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_mask : 13  ; /* [12:0] */
		unsigned int    rsv_36      : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG3 */
union u_gf_irq_reg3 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_state_mask : 13  ; /* [12:0] */
		unsigned int    rsv_37            : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG4 */
union u_gf_irq_reg4 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_state_raw : 13  ; /* [12:0] */
		unsigned int    rsv_38           : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

#endif // __IPP_TOP_DRV_PRIV_H_GF_
