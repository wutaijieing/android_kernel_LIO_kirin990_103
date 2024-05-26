// ****************************************************************
// Copyright    Copyright (c) 2019- Hisilicon Technologies CO., Ltd.
// File name    ipp.h
// Description:
//
// Version	   1.0
// Date		   2019-05-13
// ****************************************************************/

#ifndef _IPP_CS_H_
#define _IPP_CS_H_

#include <linux/types.h>
#include <linux/stddef.h>

#define ISP_IPP_OK	 0
#define ISP_IPP_ERR	(-1)
#define CVDR_ALIGN_BYTES			16	// bytes

#define MAX_CPE_STRIPE_NUM			25 //max orb = 22;need review 
#define MAX_VBK_STRIPE_NUM			2
#define MAX_GF_STRIPE_NUM			15
#define ISP_IPP_CLK    0
#define IPP_SIZE_MAX   8192

enum HISP_CPE_REG_TYPE {
	CPE_TOP = 0,
	CMDLIST_REG  = 1,
	CVDR_REG = 2,
	GF_REG = 3,
	ORB_REG = 4,
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

#define FLAG_LOG_DEBUG  0

extern unsigned int kmsgcat_mask;

#define D(fmt, args...) \
	do { \
		if (kmsgcat_mask & DEBUG_BIT) \
			printk("[ispcpe][%s]\n" fmt, __func__, ##args); \
	} while (0)

#define I(fmt, args...) \
	do { \
		if (kmsgcat_mask & INFO_BIT) \
			printk("[ispcpe][%s]\n" fmt, __func__, ##args); \
	} while (0)
#define E(fmt, args...) \
	do { \
		if (kmsgcat_mask & ERROR_BIT) \
			printk("[ispcpe][%s]\n" fmt, __func__, ##args); \
	} while (0)

#ifndef HIPP_ALIGN_DOWN
#define HIPP_ALIGN_DOWN(val, al)  ((unsigned int)(val) & ~((al) - 1))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(val, al) (((unsigned int)(val) + ((al) - 1)) & ~((al) - 1))
#endif

#ifndef ALIGN_LONG_UP
#define ALIGN_LONG_UP(val, al)	(((unsigned long long)(val) \
	+ (((unsigned long long)(al)) - 1)) & ~(((unsigned long long)(al)) - 1))
#endif

#ifndef MAX
#define MAX(x, y)			 ((x) > (y) ? (x) : (y))
#endif

#define loge_if(x) {\
	if (x) {\
		pr_err("'%s' failed", #x); \
	} \
}

#define loge_if_ret(x) \
   {\
      if (x) \
      {\
          pr_err("'%s' failed", #x); \
          return 1; \
      } \
   }

enum pix_format_e {
	PIXEL_FMT_IPP_Y8 = 0,
	PIXEL_FMT_IPP_1PF8 = 1,
	PIXEL_FMT_IPP_2PF8 = 2,
	PIXEL_FMT_IPP_3PF8 = 3,
	PIXEL_FMT_IPP_D32 = 4,
	PIXEL_FMT_IPP_D48 = 5,
	PIXEL_FMT_IPP_D64 = 6,
	PIXEL_FMT_IPP_D128 = 7,
	PIXEL_FMT_IPP_MAX,
};

struct ipp_stream_t {
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int buffer;
	enum pix_format_e format;
};

#define JPG_SUBSYS_BASE_ADDR		0xE9300000

#define JPG_TOP_OFFSET			0x00004000
#define JPG_CMDLST_OFFSET		0x00005000
#define JPG_CVDR_OFFSET			0x00006000
#define JPG_SMMU_MASTER_OFFSET		0x00008000
#define JPG_VBK_OFFSET			0x00009000
#define JPG_GF_OFFSET			0x0000A000
#define JPG_ANF_OFFSET			0x0000B000
#define JPG_ORB_OFFSET			0x0000C000
#define JPG_REORDER_OFFSET		0x0000E000
#define JPG_COMPARE_OFFSET		0x0000F000
#define JPG_ORB_ENH_OFFSET		0x00010000
#define JPG_MC_OFFSET			0x00011000

#define IPP_BASE_ADDR_TOP	(JPG_SUBSYS_BASE_ADDR + JPG_TOP_OFFSET)
#define IPP_BASE_ADDR_CMDLST	(JPG_SUBSYS_BASE_ADDR + JPG_CMDLST_OFFSET)
#define IPP_BASE_ADDR_CVDR	(JPG_SUBSYS_BASE_ADDR + JPG_CVDR_OFFSET)
#define IPP_BASE_ADDR_SMMU_MASTER \
				(JPG_SUBSYS_BASE_ADDR + JPG_SMMU_MASTER_OFFSET)
#define IPP_BASE_ADDR_VBK	(JPG_SUBSYS_BASE_ADDR + JPG_VBK_OFFSET)
#define IPP_BASE_ADDR_GF	(JPG_SUBSYS_BASE_ADDR + JPG_GF_OFFSET)
#define IPP_BASE_ADDR_ANF	(JPG_SUBSYS_BASE_ADDR + JPG_ANF_OFFSET)
#define IPP_BASE_ADDR_ORB	(JPG_SUBSYS_BASE_ADDR + JPG_ORB_OFFSET)
#define IPP_BASE_ADDR_REORDER	(JPG_SUBSYS_BASE_ADDR + JPG_REORDER_OFFSET)
#define IPP_BASE_ADDR_COMPARE	(JPG_SUBSYS_BASE_ADDR + JPG_COMPARE_OFFSET)
#define IPP_BASE_ADDR_ORB_ENH	(JPG_SUBSYS_BASE_ADDR + JPG_ORB_ENH_OFFSET)
#define IPP_BASE_ADDR_MC	(JPG_SUBSYS_BASE_ADDR + JPG_MC_OFFSET)

#define IPP_BASE_ADDR_PAC	0xF8800000

void *isp_fstcma_alloc(dma_addr_t *dma_handle, size_t size, gfp_t flag);
void isp_fstcma_free(void *va, dma_addr_t dma_handle, size_t size);

enum ipp_clk_rate_index_e {
	CLK_RATE_INDEX_GF = 0,
	CLK_RATE_INDEX_ORB = 1,
	CLK_RATE_INDEX_MATCHER = 2,
	CLK_RATE_INDEX_VBK = 3,
	CLK_RATE_INDEX_MC = 4,
	CLK_RATE_INDEX_MAX
};

enum hipp_refs_type_e {
	REFS_TYPE_GF = 0,
	REFS_TYP_ORB = 1,
	REFS_TYP_MAX
};

int atfhipp_smmu_enable(unsigned int mode);
int atfhipp_smmu_disable(unsigned int mode);
int atfhipp_smmu_smrx(unsigned int swid, unsigned int len, unsigned int sid,
	unsigned int ssid, unsigned int mode);

int atfhipp_smmu_pref(unsigned int swid, unsigned int len);
int hipp_core_powerup(unsigned int devtype);
int hipp_core_powerdn(unsigned int devtype);
int hipp_set_rate(unsigned int devtype, unsigned int clktype);
void relax_ipp_wakelock(void);

#endif /* _IPP_CS_H_ */

/* ************************************** END ***************************** */
