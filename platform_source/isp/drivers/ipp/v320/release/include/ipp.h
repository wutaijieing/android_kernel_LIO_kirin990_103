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

enum HISP_CPE_REG_TYPE {
	CPE_TOP = 0,
	CMDLIST_REG  = 1,
	CVDR_REG = 2,
	GF_REG = 3,
	ORB_REG = 4,
	MAX_HISP_CPE_REG
};

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
