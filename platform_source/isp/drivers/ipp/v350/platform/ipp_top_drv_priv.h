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

#ifndef __IPP_TOP_DRV_PRIV_H_TOP_
#define __IPP_TOP_DRV_PRIV_H_TOP_

#include "ipp_top_drv_priv_arfeature.h"
#include "ipp_top_drv_priv_cmdlst.h"
#include "ipp_top_drv_priv_cmp.h"
#include "ipp_top_drv_priv_enh.h"
#include "ipp_top_drv_priv_gf.h"
#include "ipp_top_drv_priv_hiof.h"
#include "ipp_top_drv_priv_mc.h"
#include "ipp_top_drv_priv_rdr.h"

/* Define the union U_DMA_CRG_CFG0 */
union u_dma_crg_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_0             : 16  ; /* [15:0] */
		unsigned int    dlock_and_dbg_clr : 1  ; /* [16] */
		unsigned int    rsv_1             : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DMA_CRG_CFG1 */
union u_dma_crg_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_clken     : 1  ; /* [0] */
		unsigned int    tbu_cvdr_clken : 1  ; /* [1] */
		unsigned int    rsv_2          : 1  ; /* [2] */
		unsigned int    cmdlst_clken   : 1  ; /* [3] */
		unsigned int    dpm_clken      : 1  ; /* [4] */
		unsigned int    rsv_3          : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DMA_CRG_CFG2 */
union u_dma_crg_cfg2 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_soft_rst     : 1  ; /* [0] */
		unsigned int    tbu_cvdr_soft_rst : 1  ; /* [1] */
		unsigned int    rsv_4             : 1  ; /* [2] */
		unsigned int    cmdlst_soft_rst   : 1  ; /* [3] */
		unsigned int    dpm_soft_rst      : 1  ; /* [4] */
		unsigned int    rsv_5             : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG0 */
union u_cvdr_irq_reg0 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_clr : 8  ; /* [7:0] */
		unsigned int    rsv_6        : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG2 */
union u_cvdr_irq_reg2 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_mask : 8  ; /* [7:0] */
		unsigned int    rsv_7         : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG3 */
union u_cvdr_irq_reg3 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_state_raw : 8  ; /* [7:0] */
		unsigned int    rsv_8              : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG4 */
union u_cvdr_irq_reg4 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_state_mask : 8  ; /* [7:0] */
		unsigned int    rsv_9               : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_MEM_CFG0 */
union u_cvdr_mem_cfg0 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    mem_ctrl_sp      : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_MEM_CFG1 */
union u_cvdr_mem_cfg1 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_10      : 3  ; /* [2:0] */
		unsigned int    mem_ctrl_tp : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_MEM_CTRL_SPUA */
union u_mem_ctrl_spua {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_11        : 3  ; /* [2:0] */
		unsigned int    mem_ctrl_spua : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_IPP_RESERVED */
union u_ipp_reserved {
	/* Define the struct bits */
	struct {
		unsigned int    reserved_ec : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

//==============================================================================
/* Define the global struct */
struct s_ipp_top_regs_type {
	union u_dma_crg_cfg0         dma_crg_cfg0            ; /* 0 */
	union u_dma_crg_cfg1         dma_crg_cfg1            ; /* 4 */
	union u_dma_crg_cfg2         dma_crg_cfg2            ; /* 8 */
	union u_cvdr_irq_reg0        cvdr_irq_reg0           ; /* 10 */
	union u_cvdr_irq_reg2        cvdr_irq_reg2           ; /* 14 */
	union u_cvdr_irq_reg3        cvdr_irq_reg3           ; /* 18 */
	union u_cvdr_irq_reg4        cvdr_irq_reg4           ; /* 1c */
	union u_cvdr_mem_cfg0        cvdr_mem_cfg0           ; /* 20 */
	union u_cvdr_mem_cfg1        cvdr_mem_cfg1           ; /* 24 */
	union u_mem_ctrl_spua        mem_ctrl_spua           ; /* 28 */
	union u_ipp_reserved         ipp_reserved[16]        ; /* 40 */
	union u_gf_crg_cfg0          gf_crg_cfg0             ; /* 200 */
	union u_gf_crg_cfg1          gf_crg_cfg1             ; /* 204 */
	union u_gf_mem_cfg           gf_mem_cfg              ; /* 208 */
	union u_gf_irq_reg0          gf_irq_reg0             ; /* 210 */
	union u_gf_irq_reg1          gf_irq_reg1             ; /* 214 */
	union u_gf_irq_reg2          gf_irq_reg2             ; /* 218 */
	union u_gf_irq_reg3          gf_irq_reg3             ; /* 21c */
	union u_gf_irq_reg4          gf_irq_reg4             ; /* 220 */
	union u_arf_crg_cfg0         arf_crg_cfg0            ; /* 240 */
	union u_arf_crg_cfg1         arf_crg_cfg1            ; /* 244 */
	union u_arf_mem_cfg          arf_mem_cfg             ; /* 248 */
	union u_arf_irq_reg0         arf_irq_reg0            ; /* 24c */
	union u_arf_irq_reg1         arf_irq_reg1            ; /* 250 */
	union u_arf_irq_reg2         arf_irq_reg2            ; /* 254 */
	union u_arf_irq_reg3         arf_irq_reg3            ; /* 258 */
	union u_arf_irq_reg4         arf_irq_reg4            ; /* 25c */
	union u_rdr_crg_cfg0         rdr_crg_cfg0            ; /* 300 */
	union u_rdr_crg_cfg1         rdr_crg_cfg1            ; /* 304 */
	union u_rdr_mem_cfg          rdr_mem_cfg             ; /* 308 */
	union u_rdr_irq_reg0         rdr_irq_reg0            ; /* 310 */
	union u_rdr_irq_reg1         rdr_irq_reg1            ; /* 314 */
	union u_rdr_irq_reg2         rdr_irq_reg2            ; /* 318 */
	union u_rdr_irq_reg3         rdr_irq_reg3            ; /* 31c */
	union u_rdr_irq_reg4         rdr_irq_reg4            ; /* 320 */
	union u_cmp_crg_cfg0         cmp_crg_cfg0            ; /* 330 */
	union u_cmp_crg_cfg1         cmp_crg_cfg1            ; /* 334 */
	union u_cmp_mem_cfg          cmp_mem_cfg             ; /* 338 */
	union u_cmp_irq_reg0         cmp_irq_reg0            ; /* 340 */
	union u_cmp_irq_reg1         cmp_irq_reg1            ; /* 344 */
	union u_cmp_irq_reg2         cmp_irq_reg2            ; /* 348 */
	union u_cmp_irq_reg3         cmp_irq_reg3            ; /* 34c */
	union u_cmp_irq_reg4         cmp_irq_reg4            ; /* 350 */
	union u_enh_crg_cfg0         enh_crg_cfg0            ; /* 360 */
	union u_enh_crg_cfg1         enh_crg_cfg1            ; /* 364 */
	union u_enh_mem_cfg          enh_mem_cfg             ; /* 368 */
	union u_enh_vpb_cfg          enh_vpb_cfg             ; /* 36c */
	union u_enh_irq_reg0         enh_irq_reg0            ; /* 380 */
	union u_enh_irq_reg1         enh_irq_reg1            ; /* 384 */
	union u_enh_irq_reg2         enh_irq_reg2            ; /* 388 */
	union u_enh_irq_reg3         enh_irq_reg3            ; /* 38c */
	union u_enh_irq_reg4         enh_irq_reg4            ; /* 390 */
	union u_hiof_crg_cfg0        hiof_crg_cfg0           ; /* 4e0 */
	union u_hiof_crg_cfg1        hiof_crg_cfg1           ; /* 4e4 */
	union u_hiof_mem_cfg         hiof_mem_cfg            ; /* 4e8 */
	union u_hiof_irq_reg0        hiof_irq_reg0           ; /* 4ec */
	union u_hiof_irq_reg1        hiof_irq_reg1           ; /* 4f0 */
	union u_hiof_irq_reg2        hiof_irq_reg2           ; /* 4f4 */
	union u_hiof_irq_reg3        hiof_irq_reg3           ; /* 4f8 */
	union u_hiof_irq_reg4        hiof_irq_reg4           ; /* 4fc */
	union u_cmdlst_ctrl_map      cmdlst_ctrl_map[16]     ; /* 500 */
	union u_cmdlst_ctrl_pm       cmdlst_ctrl_pm[16]      ; /* 504 */
	union u_cmdlst_irq_clr       cmdlst_irq_clr[6]       ; /* 600 */
	union u_cmdlst_irq_msk       cmdlst_irq_msk[6]       ; /* 604 */
	union u_cmdlst_irq_state_raw cmdlst_irq_state_raw[6] ; /* 608 */
	union u_cmdlst_irq_state_msk cmdlst_irq_state_msk[6] ; /* 60c */
	union u_mc_crg_cfg0          mc_crg_cfg0             ; /* 700 */
	union u_mc_crg_cfg1          mc_crg_cfg1             ; /* 704 */
	union u_mc_mem_cfg           mc_mem_cfg              ; /* 708 */
	union u_mc_irq_reg0          mc_irq_reg0             ; /* 70c */
	union u_mc_irq_reg1          mc_irq_reg1             ; /* 710 */
	union u_mc_irq_reg2          mc_irq_reg2             ; /* 714 */
	union u_mc_irq_reg3          mc_irq_reg3             ; /* 718 */
	union u_mc_irq_reg4          mc_irq_reg4             ; /* 71c */
};

#endif // __IPP_TOP_DRV_PRIV_H_TOP_