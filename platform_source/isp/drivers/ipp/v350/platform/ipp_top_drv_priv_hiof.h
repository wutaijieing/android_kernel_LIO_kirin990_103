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

#ifndef __IPP_TOP_DRV_PRIV_H_HIOF_
#define __IPP_TOP_DRV_PRIV_H_HIOF_

/* Define the union U_HIOF_CRG_CFG0 */
union u_hiof_crg_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_clken        : 1  ; /* [0] */
		unsigned int    rsv_94            : 15  ; /* [15:1] */
		unsigned int    hiof_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_95            : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_CRG_CFG1 */
union u_hiof_crg_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_96        : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_MEM_CFG */
union u_hiof_mem_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_97           : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_IRQ_REG0 */
union u_hiof_irq_reg0 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_irq_clr : 13  ; /* [12:0] */
		unsigned int    rsv_98       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_IRQ_REG1 */
union u_hiof_irq_reg1 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_irq_mask : 24  ; /* [23:0] */
		unsigned int    rsv_99        : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_IRQ_REG2 */
union u_hiof_irq_reg2 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_100        : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_IRQ_REG3 */
union u_hiof_irq_reg3 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_irq_state_mask : 24  ; /* [23:0] */
		unsigned int    rsv_101             : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_IRQ_REG4 */
union u_hiof_irq_reg4 {
	/* Define the struct bits */
	struct {
		unsigned int    hiof_irq_state_raw : 24  ; /* [23:0] */
		unsigned int    rsv_102            : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

#endif // __IPP_TOP_DRV_PRIV_H_HIOF_