/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HISI_DISP_GADGETS_H
#define HISI_DISP_GADGETS_H

#include <linux/types.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <soc_dte_define.h>

#include "hisi_disp_debug.h"

#undef ENABLE_BITS
#define ENABLE_BITS(value, bit)    (!!((value) & BIT(bit)))

#undef CLEAR_BITS
#define CLEAR_BITS(value, bit) ((value) &= ~(BIT(bit)))

/*--------------------------------------------------------------------------*/
#ifdef CONFIG_DPU_FB_DUMP_DSS_REG
#define outp32(addr, val) \
	do {\
		writel(val, addr);\
		disp_pr_info("writel(0x%x, 0x%x);\n", val, addr);\
	} while (0)
#else
#define outp32(addr, val) writel(val, addr)
#endif

#define outp16(addr, val)   writew(val, addr)
#define outp8(addr, val)    writeb(val, addr)
#define outp(addr, val)     outp32(addr, val)

#define inp32(addr)         readl(addr)
#define inp16(addr)         readw(addr)
#define inp8(addr)          readb(addr)
#define inp(addr)           inp32(addr)

#define inpw(port)          readw(port)
#define outpw(port, val)    writew(val, port)
#define inpdw(port)         readl(port)
#define outpdw(port, val)   writel(val, port)

/*--------------------------------------------------------------------------*/
#define disp_check_and_return(condition, ret, level, msg, ...) \
	do { \
		if (condition) { \
			disp_pr_##level(msg, ##__VA_ARGS__);\
			return ret; \
		} \
	} while (0)

#define disp_check_and_no_retval(condition, level, msg, ...) \
	do { \
		if (condition) { \
			disp_pr_##level(msg, ##__VA_ARGS__);\
			return; \
		} \
	} while (0)

#define disp_check_with_goto_lable(condition, label, err_msg, ...) \
	do { \
		if (condition) { \
			pr_emerg("[hisifb E]:%s: "err_msg, __func__, ## __VA_ARGS__); \
			goto label; \
		} \
	} while(0)

#define disp_check_and_ret_err(cond, ret, msg, ...) disp_check_and_return(cond, ret, err, msg, ## __VA_ARGS__)
#define disp_check_and_no_retval_err(cond, msg, ...) disp_check_and_no_retval(cond, err, msg, ## __VA_ARGS__)
/*--------------------------------------------------------------------------*/
#define round1(x, y)  ((x) / (y) + ((x) % (y) ? 1 : 0))
#define disp_reduce(x) ((x) > 0 ? ((x) - 1) : (x))

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

/*--------------------------------------------------------------------------*/
void dpu_set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
uint32_t set_bits32(uint32_t old_val, uint32_t val, uint8_t bw, uint8_t bs);

static inline bool is_rgb(enum DPU_FORMAT format)
{
	switch(format) {
	case RGB565:
	case XRGB4444:
	case ARGB4444:
	case XRGB1555:
	case ARGB1555:
	case XRGB8888:
	case ARGB8888:
	case BGR565:
	case XBGR4444:
	case ABGR4444:
	case XBGR1555:
	case ABGR1555:
	case XBGR8888:
	case ABGR8888:
	case RGBG:
	case RGBG_HIDIC:
	case RGBG_IDLEPACK:
	case RGBG_DEBURNIN:
	case RGB_DELTA:
	case RGB_DELTA_HIDIC:
	case RGB_DELTA_IDLEPACK:
	case RGB_DELTA_DEBURNIN:
	case RGB_10BIT:
	case BGR_10BIT:
	case ARGB10101010:
	case XRGB10101010:
	case BGRA1010102:
	case RGBA1010102:
		return true;
	default:
		return false;
	}
}

#define is_yuv(format)   (!is_rgb(format))

static inline bool has_alpha(enum DPU_FORMAT format)
{
	switch (format) {
	case ARGB4444:
	case ABGR4444:
	case ABGR1555:
	case ARGB1555:
	case ARGB8888:
	case ABGR8888:
	case ARGB10101010:
	case BGRA1010102:
	case RGBA1010102:
	case AYUV10101010:
	case YUVA1010102:
	case UYVA1010102:
	case VUYA1010102:
		return true;
	default:
		return false;
	}
}

static inline bool has_40bit(enum DPU_FORMAT format)
{
	switch (format) {
	case ARGB10101010:
	case AYUV10101010:
	case XRGB10101010:
		return true;
	default:
		return false;
	}
}

#endif /* HISI_DISP_GADGETS_H */
