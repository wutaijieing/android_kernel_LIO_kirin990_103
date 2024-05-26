/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the hw_rscan_utils.h - get current run mode, eng or user
 * Create: 2016-06-18
 */

#ifndef _HW_RSCAN_UTILS_H_
#define _HW_RSCAN_UTILS_H_

#include <linux/buffer_head.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/ratelimit.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "hw_rscan_interface.h"
#include "securec.h"

#define EOF (-1)
#define LINE_LEN_SHORT 200

#define var_not_used(variable)  do { (void)(variable); } while (0)

/*
 * the length of list of rproc is limited to 840 byte,
 * otherwise, upload contents will be empty
 */
#define RPROC_VALUE_LEN_MAX  840

#define RSCAN_UNINIT    0
#define RSCAN_INIT      1

#define RO_SECURE       0
#define RO_NORMAL       1

/*
 * A convenient interface for error log print, Root Scan Log Error.
 * and the print rate is limited
 * used like below,
 * rs_log_error("hw_rscan_utils", "xxx %s xxx", yyy)
 */
#define RSCAN_ERROR "[Error]"
#define rs_log_error(tag, fmt, args...) \
	pr_err_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_ERROR, \
				tag, __func__, ##args)

/*
 * A convenient interface for warning log print, Root Scan Log warning.
 * and the print rate is limited
 * used like below,
 * rs_log_warning("hw_rscan_utils", "xxx %s xxx", yyy)
 */
#define RSCAN_WARNING "[WARNING]"
#define rs_log_warning(tag, fmt, args...) \
	pr_warn_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_WARNING, \
				tag, __func__, ##args)

/*
 * A convenient interface for trace log print, Root Scan Log trace.
 * and the print rate is limited
 * used like below,
 * rs_log_trace("hw_rscan_utils", "xxx %s xxx", yyy)
 */
#define RSCAN_TRACE "[TRACE]"
#define rs_log_trace(tag, fmt, args...) \
	pr_info_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_TRACE, \
				tag, __func__, ##args)

#ifdef CONFIG_HW_ROOT_SCAN_ENG_DEBUG
/*
 * A convenient interface for debug log print, Root Scan Log debug.
 * and the print rate is limited
 * used like below,
 * rs_log_debug("hw_rscan_utils", "xxx %s xxx", yyy)
 */
#define RSCAN_DEBUG "[DEBUG]"
#define rs_log_debug(tag, fmt, args...) \
	printk_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_DEBUG, \
				tag, __func__, ##args)
#else
#define rs_log_debug(tag, fmt, args...) no_printk(fmt, ##args)
#endif

bool get_xom_enable(void);

#endif
