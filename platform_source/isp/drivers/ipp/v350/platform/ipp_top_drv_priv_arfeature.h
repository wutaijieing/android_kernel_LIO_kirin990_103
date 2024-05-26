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

#ifndef __IPP_TOP_DRV_PRIV_H_ARF_
#define __IPP_TOP_DRV_PRIV_H_ARF_

/* Define the union U_ARF_CRG_CFG0 */
union u_arf_crg_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_clken        : 1  ; /* [0] */
		unsigned int    rsv_39           : 15  ; /* [15:1] */
		unsigned int    arf_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_40           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_CRG_CFG1 */
union u_arf_crg_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_41       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_MEM_CFG */
union u_arf_mem_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    arf_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_42          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_IRQ_REG0 */
union u_arf_irq_reg0 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_irq_clr : 30  ; /* [29:0] */
		unsigned int    rsv_43      : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_IRQ_REG1 */
union u_arf_irq_reg1 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_44        : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_IRQ_REG2 */
union u_arf_irq_reg2 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_irq_mask : 30  ; /* [29:0] */
		unsigned int    rsv_45       : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_IRQ_REG3 */
union u_arf_irq_reg3 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_irq_state_raw : 30  ; /* [29:0] */
		unsigned int    rsv_46            : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ARF_IRQ_REG4 */
union u_arf_irq_reg4 {
	/* Define the struct bits */
	struct {
		unsigned int    arf_irq_state_mask : 30  ; /* [29:0] */
		unsigned int    rsv_47             : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

#endif // __IPP_TOP_DRV_PRIV_H_ARF_