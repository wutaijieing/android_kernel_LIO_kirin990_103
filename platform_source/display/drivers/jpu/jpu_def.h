/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpeg jpu def
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef JPU_DEF_H
#define JPU_DEF_H

#include <linux/printk.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <asm/io.h>

#define JPU_TAG "JPU_LOG"
#define PR_LOG_TAG  JPU_TAG

extern uint32_t g_jpu_msg_level;

/*
 * Message printing priorities:
 * LEVEL 0 KERN_EMERG (highest priority)
 * LEVEL 1 KERN_ALERT
 * LEVEL 2 KERN_CRIT
 * LEVEL 3 KERN_ERR
 * LEVEL 4 KERN_WARNING
 * LEVEL 5 KERN_NOTICE
 * LEVEL 6 KERN_INFO
 * LEVEL 7 KERN_DEBUG (Lowest priority)
 */
#define jpu_emerg(msg, ...) \
	do { if (g_jpu_msg_level > 0) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_alert(msg, ...) \
	do { if (g_jpu_msg_level > 1) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_crit(msg, ...) \
	do { if (g_jpu_msg_level > 2) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_err(msg, ...) \
	do { if (g_jpu_msg_level > 3) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_warning(msg, ...) \
	do { if (g_jpu_msg_level > 4) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_notice(msg, ...) \
	do { if (g_jpu_msg_level > 5) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_info(msg, ...) \
	do { if (g_jpu_msg_level > 6) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)
#define jpu_debug(msg, ...) \
	do { if (g_jpu_msg_level > 7) \
		pr_info("%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)

#ifdef CONFIG_HISI_JPU_DUMP_REG
#define outp32(addr, val) \
	do { \
		writel(val, addr); \
		pr_info("writel(0x%x, 0x%x);\n", val, addr); \
	} while (0)
#else
#define outp32(addr, val) writel(val, addr)
#endif

#define inp32(addr) readl(addr)

#endif /* JPU_DEF_H */
