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

#ifndef __IPP_TOP_DRV_PRIV_H_ENH_
#define __IPP_TOP_DRV_PRIV_H_ENH_

/* Define the union U_ENH_CRG_CFG0 */
union u_enh_crg_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_clken        : 1  ; /* [0] */
		unsigned int    rsv_66           : 15  ; /* [15:1] */
		unsigned int    enh_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_67           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_CRG_CFG1 */
union u_enh_crg_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_68       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_MEM_CFG */
union u_enh_mem_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    enh_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_69          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_VPB_CFG */
union u_enh_vpb_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    enh_vpbg_en : 2  ; /* [1:0] */
		unsigned int    rsv_70      : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG0 */
union u_enh_irq_reg0 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_clr : 10  ; /* [9:0] */
		unsigned int    rsv_71      : 22  ; /* [31:10] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG1 */
union u_enh_irq_reg1 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_72        : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG2 */
union u_enh_irq_reg2 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_mask : 10  ; /* [9:0] */
		unsigned int    rsv_73       : 22  ; /* [31:10] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG3 */
union u_enh_irq_reg3 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_state_raw : 10  ; /* [9:0] */
		unsigned int    rsv_74            : 22  ; /* [31:10] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG4 */
union u_enh_irq_reg4 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_state_mask : 10  ; /* [9:0] */
		unsigned int    rsv_75             : 22  ; /* [31:10] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

#endif // __IPP_TOP_DRV_PRIV_H_ENH_
