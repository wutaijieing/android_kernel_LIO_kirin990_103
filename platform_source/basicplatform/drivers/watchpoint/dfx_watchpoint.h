/*
 *
 * Copyright (c) 2010-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DFX_WATCHPOINT_H
#define __DFX_WATCHPOINT_H

#include <linux/debugfs.h>

#define MERROR_LEVEL    1
#define MWARNING_LEVEL  0
#define MNOTICE_LEVEL   1
#define MINFO_LEVEL     0
#define MDEBUG_LEVEL    0

#define WP_BUFF_LEN         16
#define ACCESS_CHR_NUM      2
#define STR_LEN_OFFSET      2
#define DECIMAL_BASE        10
#define HEXADECIMAL_BASE    16
#define WP_FP_OFFSET        29

#define MLOG_TAG        "watchpoint"

#define EWPNONE         300 /* None */
#define EWPNOMTCH       301 /* No Match */
#define EWPNOSPC        302 /* No Space */
#define EWPNOADDR       303 /* Not Found the Addr */
#define EWPNOWP         304 /* No WP registered */
#define EWPOVFL         305 /* No Space, overflowed */

#define WP_FALSE        0
#define WP_TRUE         1

#define WP_FILE_MODE        0660

#define MAX_WP_NUM           4
#define WP_DBG_SIZE_PAGE     2048
#define WP_STR_LEN           128

#define ACCESS_STR_LEN      8
#define ACCESS_BIT_NONE     0
#define ACCESS_BIT_READ     HW_BREAKPOINT_R
#define ACCESS_BIT_WRITE    HW_BREAKPOINT_W
#define ACCESS_BIT_RW       (ACCESS_BIT_READ | ACCESS_BIT_WRITE)
#define ACCESS_STR_READ     "r"
#define ACCESS_STR_WRITE    "w"
#define ACCESS_CHR_READ     'r'
#define ACCESS_CHR_WRITE    'w'
#define ACCESS_STR_NONE     "none"

#define TG_TYPE_PAINC            0
#define TG_TYPE_DUMP             1
#define TG_TYPE_NONE             9
#define TG_TYPE_STR_LEN          8
#define TG_TYPE_STR_PANIC        "panic"
#define TG_TYPE_STR_DUMP         "dump"
#define TG_TYPE_STR_NONE         "none"

#define ADDR_BYTE_NONE      0
#define ADDR_BYTE_1         HW_BREAKPOINT_LEN_1
#define ADDR_BYTE_2         HW_BREAKPOINT_LEN_2
#define ADDR_BYTE_4         HW_BREAKPOINT_LEN_4
#define ADDR_BYTE_8         HW_BREAKPOINT_LEN_8
#define ADDR_BYTE_STR_LEN   8
#define ADDR_BYTE_STR_1     "1"
#define ADDR_BYTE_STR_2     "2"
#define ADDR_BYTE_STR_4     "4"
#define ADDR_BYTE_STR_8     "8"
#define ADDR_BYTE_STR_N     "none"

#define ADDR_MASK_0         0
#define ADDR_MASK_MAX       31
#define ADDR_MASK_STR_LEN   8
#define ADDR_MASK_STR_N     "none"

#define ADDR_NONE           1UL
#define ADDR_STR_NONE       "#none"
#define ADDR_STR_LEN        128

#define ADDR_TYPE_VADDR          0
#define ADDR_TYPE_VNAME          1
#define ADDR_TYPE_VNAME_PLUS     2
#define ADDR_TYPE_NONE           3

#define WHITE_LIST_MAX_NUM       16

#define STAT_STR_LEN             512

#define SET_DIS          0UL
#define SET_EN           1UL
#define SET_STR_OK       "ok"
#define SET_STR_FAIL     "fail"
#define SET_STR_NONE     "none"
#define SET_OK           0
#define SET_NONE         (-EWPNONE)
#define SET_STR_LEN      16

#define FUNC_NAME_LEN    128

#define STORE_FILE       "/data/watchpt.cfg"


struct watchpoint_cmp_addr {
	u64 ref_addr;
	int is_caller_in_stack;
};

/*
 * struct wp_white_list - servers for save the user valid input
 * @fname: function name
 * @faddr: function address
 */
struct wp_white_llist {
	struct list_head node;
	char *func_name;
	u64 func_addr;
};

/*
 * struct wp_cfg_store - used in store
 * @addr_str: holds the user input in form of hex, var_name or var_name+decimal
 * @rw_type: read(0x01), write(0x02)
 * @addr_mask: 3~31
 * @len: 1,2,4,8
 * parameters when config:addr, access, addr_mask, length
 * transfer when set
 * transfer when transfer:addr_str, access, addr_mask, length
 */
struct wp_cfg_store {
	u8 wp_addr_byte;
	u8 wp_rw_type;
	u8 wp_addr_mask;
	char addr_str[ADDR_STR_LEN];
};

/* used in set */
struct wp_cfg_set {
	u64 wp_addr;
	struct wp_cfg_store store;
};

/* public cofiguration */
struct wp_cfg_pub {
	u8 wp_tg_type;
};

/* used in register */
struct wp_struct {
	struct perf_event *__percpu *wp_pe;
	struct wp_cfg_set set;
};

static u8 watchpt_get_tg_type(void);
static int watchpt_wlist_check(const struct pt_regs *regs);

#endif /* __DFX_WATCHPOINT_H */
