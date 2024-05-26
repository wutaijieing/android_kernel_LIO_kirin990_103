/*
 * dbg.h
 *
 * This is for log intf.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef VFMW_DEBUG
#define VFMW_DEBUG
#include "vcodec_types.h"
#ifdef ENV_SOS_KERNEL
#include "tee_log.h"
#endif

typedef enum {
	PRN_FATAL = 0,  /* 0. fatal error */
	PRN_ERROR,      /* 1. normal error */
	PRN_WARN,       /* 2. warnning message */
	PRN_DBG,        /* 3. debug message */
	PRN_VDMREG,     /* 4. VDM register info */
	PRN_DNMSG,      /* 5. down message info */
	PRN_RPMSG,      /* 6. repair message info */
	PRN_UPMSG,      /* 7. up message info */
	PRN_STREAM,     /* 8. stream info */
	PRN_CTRL,       /* 9. control info */
	PRN_SCD_REGMSG, /* 10. SCD register info */
	PRN_REF,        /* 11. reference pictures info */
	PRN_SEQ,        /* 12. sequence syntax info */
	PRN_PIC,        /* 13. picture syntax info */
	PRN_SLICE,      /* 14. slice syntax info */
	PRN_SEI,        /* 15. SEI syntax info */
	PRN_KFUNC,      /* 16. trace key function timestamp info */
	PRN_REV_17,
	PRN_REV_18,
	PRN_REV_19,
	PRN_REV_20,
	PRN_STM_BLK,    /* 21. stm module block info */
	PRN_SYN_BLK,    /* 22. syn module block info */
	PRN_DEC_BLK,    /* 23. dec module block info */
	PRN_PTS,        /* 24. pts info */
	PRN_REV_25,
	PRN_REV_26,
	PRN_REV_27,
	PRN_REV_28,
	PRN_REV_29,
	PRN_REV_30,
	PRN_ALWS = 31   /* 31. unlimit print level */
} print_type;

#define VDH_REG_DUMP_DIR  "/data/misc/media/"
#define DUMP_BUFFER_LEN 256

#define DEFAULT_PRINT_ENABLE (1 << PRN_FATAL | 1 << PRN_ERROR | 1 << PRN_WARN)
#define default_print_check(type) ((DEFAULT_PRINT_ENABLE & (1 << type)) != 0)

#ifndef ENV_SOS_KERNEL
#define dprint_linux_kernel(type, fmt, arg...) \
	do { \
		if ((type == PRN_ALWS) || default_print_check(type)) { \
			printk(KERN_ALERT "VDEC: <%s: %d>"fmt, __func__, __LINE__, ##arg); \
		} \
	} while (0)

#define dprint_vfmw_nothing(type, fmt, arg...) ({ do {} while (0); 0; })

#ifdef ENV_ARMLINUX_KERNEL
#define dprint(type, fmt, arg...) dprint_linux_kernel(type, fmt, ##arg)
#else
#define dprint(type, fmt, arg...) dprint_vfmw_nothing(type, fmt, ##arg)
#endif
#else
#define dprint_sos_kernel(type, fmt, arg...) \
	do { \
		if ((type == PRN_ALWS) || default_print_check(type)) { \
			uart_printf_func("[%s]: [%d] ", __func__, __LINE__); \
			uart_printf_func(fmt, ##arg); \
		} \
	} while (0)

#define dprint(type, fmt, arg...)  dprint_sos_kernel(type, fmt, ##arg)
#endif

#define vfmw_assert(cond) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "assert error: condition %s not match.\n", #cond); \
			return; \
		} \
	} while (0)

#define vfmw_assert_prnt(cond, fmt, arg...) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, fmt, ##arg); \
			return; \
		} \
	} while (0)

#define vfmw_assert_goto(cond, tag) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "assert error: condition %s not match.\n", #cond); \
			goto tag; \
		} \
	} while (0)

#define vfmw_assert_goto_prnt(cond, tag, fmt, arg...) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, fmt, ##arg); \
			goto tag; \
		} \
	} while (0)

#define vfmw_assert_ret(cond, ret) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "assert error: condition %s not match.\n", #cond); \
			return ret; \
		} \
	} while (0)

#define vfmw_assert_ret_prnt(cond, ret, fmt, arg...) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, fmt, ##arg); \
			return ret; \
		} \
	} while (0)

#endif
