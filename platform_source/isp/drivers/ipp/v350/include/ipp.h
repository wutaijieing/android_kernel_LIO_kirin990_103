// ****************************************************************
// Copyright    Copyright (c) 2019- Hisilicon Technologies CO., Ltd.
// File name    ipp.h
// Description:
//
// Date		   2019-05-13
// ****************************************************************/

#ifndef _IPP_CS_H_
#define _IPP_CS_H_

#include <linux/types.h>
#include <linux/stddef.h>

#define ISP_IPP_OK	 0
#define ISP_IPP_ERR	(-1)
#define CVDR_ALIGN_BYTES			16 // bytes
#define PAC_DMA_ALIGN_BYTES			128 // bytes
#define PAC_WIDTH_ALIGN_PIXELS			16 // pixels
#define PAC_HEIGHT_ALIGN_BYTES			8 // bytes

#define MAX_CPE_STRIPE_NUM			25 // max orb = 22;need review
#define MAX_GF_STRIPE_NUM			15
#define ISP_IPP_CLK    0
#define IPP_SIZE_MAX   8192

extern unsigned int arf_updata_wr_addr_flag;

enum HISP_CPE_REG_TYPE {
	CPE_TOP,
	CMDLIST_REG,
	CVDR_REG,
	MC_REG,
	ARF_REG,
	REORDER_REG,
	COMPARE_REG,
	ORB_ENH,
	HIOF_REG,
	MAX_HISP_CPE_REG
};

#define CPE_IRQ0_REG0_OFFSET 0x260
#define CPE_IRQ0_REG4_OFFSET 0x270
#define CPE_IRQ1_REG0_OFFSET 0x274
#define CPE_IRQ1_REG4_OFFSET 0x284
#define CPE_COMP_IRQ_REG_OFFSET 0x288

#define CROP_DEFAULT_VALUE 0x800

int hispcpe_reg_set(unsigned int mode, unsigned int offset, unsigned int value);
unsigned int hispcpe_reg_get(unsigned int mode, unsigned int offset);

#define DEBUG_BIT	(1 << 2)
#define INFO_BIT	(1 << 1)
#define ERROR_BIT	(1 << 0)

#define FLAG_LOG_DEBUG 0

#define LOGLEVEL ((INFO_BIT) | (ERROR_BIT)) /* DEBUG_BIT not set */

#define logd(fmt, args...) \
	do { if (LOGLEVEL & DEBUG_BIT) printk("[ispcpe][%s,%d]" fmt "\n", __func__, __LINE__, ##args); } while (0)

#define logi(fmt, args...) \
	do { if (LOGLEVEL & INFO_BIT) printk("[ispcpe][%s,%d]" fmt "\n", __func__, __LINE__, ##args); } while (0)

#define loge(fmt, args...) \
	do { if (LOGLEVEL & ERROR_BIT) printk("[ispcpe][%s,%d]" fmt "\n", __func__, __LINE__, ##args); } while (0)

#ifndef align_down
#define align_down(val, al)  ((unsigned int)(val) & ~((al) - 1))
#endif
#ifndef align_up
#define align_up(val, al) (((unsigned int)(val) + ((al) - 1)) & ~((al) - 1))
#endif
#ifndef align_long_up
#define align_long_up(val, al)	(((unsigned long long)(val) \
	+ (((unsigned long long)(al)) - 1)) & ~(((unsigned long long)(al)) - 1))
#endif

#define loge_if(x) {\
	if (x) {\
		pr_err("'%s' failed\n", #x); \
	} \
}

#define loge_if_ret(x) {\
		if (x) {\
			pr_err("'%s' failed\n", #x); return ISP_IPP_ERR; \
		} \
	}

#define loge_if_ret_einval(x) {\
		if (x) {\
			pr_err("'%s' failed\n", #x); return -EINVAL; \
		} \
	}

#define loge_if_ret_efault(x) {\
		if (x) {\
			pr_err("'%s' failed\n", #x); return -EFAULT; \
		} \
	}

#define dataflow_check_para(x){\
	if(NULL == (x)) {\
	    loge("Failed: NULL parameters!"); return ISP_IPP_ERR; \
	}\
}

#define loge_if_null(x){\
	if(NULL == (x)) {\
	    loge("Failed: NULL parameters!"); return ISP_IPP_ERR; \
	}\
}

enum pix_format_e {
	PIXEL_FMT_IPP_Y8 = 0,
	PIXEL_FMT_IPP_1PF8 = 1,
	PIXEL_FMT_IPP_2PF8 = 2,
	PIXEL_FMT_IPP_3PF8 = 3,
	PIXEL_FMT_IPP_1PF10 = 4,
	PIXEL_FMT_IPP_2PF14 = 5,
	PIXEL_FMT_IPP_D32 = 6,
	PIXEL_FMT_IPP_D48 = 7,
	PIXEL_FMT_IPP_D64 = 8,
	PIXEL_FMT_IPP_D128 = 9,
	PIXEL_FMT_IPP_MAX,
};

struct ipp_stream_t {
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int buffer;
	enum pix_format_e format;
};

#define JPG_SUBSYS_BASE_ADDR		0xE8C00000

#define JPG_TOP_OFFSET			0x00004000
#define JPG_CMDLST_OFFSET		0x00005000
#define JPG_CVDR_OFFSET			0x00006000
#define JPG_MC_OFFSET			0x00009000
#define JPG_GF_OFFSET			0x0000A000
#define JPG_ARF_OFFSET			0x0000C000
#define JPG_REORDER_OFFSET		0x0000E000
#define JPG_COMPARE_OFFSET		0x0000F000
#define JPG_ORB_ENH_OFFSET		0x00010000
#define JPG_HIOF_OFFSET			0x00011000

#define IPP_BASE_ADDR_TOP	(JPG_SUBSYS_BASE_ADDR + JPG_TOP_OFFSET)
#define IPP_BASE_ADDR_CMDLST	(JPG_SUBSYS_BASE_ADDR + JPG_CMDLST_OFFSET)
#define IPP_BASE_ADDR_CVDR	(JPG_SUBSYS_BASE_ADDR + JPG_CVDR_OFFSET)

#define IPP_BASE_ADDR_HIOF	(JPG_SUBSYS_BASE_ADDR + JPG_HIOF_OFFSET)
#define IPP_BASE_ADDR_GF	(JPG_SUBSYS_BASE_ADDR + JPG_GF_OFFSET)
#define IPP_BASE_ADDR_ARF	(JPG_SUBSYS_BASE_ADDR + JPG_ARF_OFFSET)
#define IPP_BASE_ADDR_REORDER	(JPG_SUBSYS_BASE_ADDR + JPG_REORDER_OFFSET)
#define IPP_BASE_ADDR_COMPARE	(JPG_SUBSYS_BASE_ADDR + JPG_COMPARE_OFFSET)
#define IPP_BASE_ADDR_ORB_ENH	(JPG_SUBSYS_BASE_ADDR + JPG_ORB_ENH_OFFSET)
#define IPP_BASE_ADDR_MC	(JPG_SUBSYS_BASE_ADDR + JPG_MC_OFFSET)

#define PCIE_IPP_IRQ_TAG    0

enum mem_attr_type_e {
	MMU_READ = 0,
	MMU_WRITE = 1,
	MMU_EXEC = 2,
	MMU_SEC = 3,
	MMU_CACHE = 4,
	MMU_DEVICE = 5,
	MMU_INV = 6,
	MAX_MMUATTR
};

enum ipp_clk_rate_index_e {
	CLK_RATE_INDEX_GF = 0,
	CLK_RATE_INDEX_ARFEATURE = 1,
	CLK_RATE_INDEX_MATCHER = 2,
	CLK_RATE_INDEX_HIOF = 3,
	CLK_RATE_INDEX_MC = 4,
	CLK_RATE_INDEX_MAX
};

enum hipp_refs_type_e {
	REFS_TYP_ORCM = 0,
	REFS_TYP_HIOF = 1,
	REFS_TYP_MAX
};

// Macro for CMP UT
#define  IPP_CMP_UT_TEST		(1)

// This part is used to print DIO during the UT test.
#define  IPP_UT_DEBUG		(0)
#if IPP_UT_DEBUG
#define UT_REG_ADDR	IPP_BASE_ADDR_COMPARE // Start address of the current module
#define IPP_MAX_REG_OFFSET	0x28 // Maximum offset address of the current module
#define MAX_DUMP_STRIPE_CNT	10
extern void set_dump_register_init(unsigned int addr, unsigned int offset, unsigned int input_fmt);
extern void get_register_info(unsigned int reg, unsigned int val);
extern void get_register_incr_info(unsigned int reg, unsigned int reg_num);
extern void get_register_incr_data(unsigned int data);
extern void ut_dump_register(unsigned int stripe_cnt);
#endif

#endif /* _IPP_CS_H_ */

/* ************************************** END ***************************** */
