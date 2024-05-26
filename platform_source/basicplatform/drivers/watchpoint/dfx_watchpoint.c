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
#include "dfx_watchpoint.h"
#include "../mntn/mntn_log.h"
#include <platform_include/basicplatform/linux/util.h>

#include <linux/init.h> /* Needed for the macros */
#include <linux/kallsyms.h>
#include <linux/kernel.h> /* Needed for KERN_ERR */
#include <linux/module.h> /* Needed by all modules */
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/version.h>

#include <asm/dfx_arch_watchpoint.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>
#include <linux/hw_breakpoint.h>
#include <linux/kthread.h>
#include <linux/perf_event.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <linux/ctype.h>
#include <linux/kallsyms.h>

#include <securec.h>
#include <linux/io.h>

/* Variable */
static struct wp_struct wp_info[MAX_WP_NUM];
static struct wp_cfg_set wp_set;
static struct wp_cfg_pub wp_pub;
static int wp_info_num;

#ifdef CONFIG_DFX_DEBUG_FS
static int wp_whitelist_num;
static int watchpt_para_set = SET_NONE;
#endif

static LIST_HEAD(wlist_head);
static DEFINE_MUTEX(watchpt_para_lock);
static bool is_show_expected = true;
static u64 g_dfx_wpt_addr = 0;
static char g_dfx_wpt_var[WP_STR_LEN] = "undef";

void dfx_watchpt_show_expected(int is_show)
{
	is_show_expected = (is_show == 0) ? false : true;
}

static int wp_find_pe(const void *ibp_addr, struct perf_event *__percpu **ppe)
{
	int idx;

	for (idx = 0; idx < MAX_WP_NUM; idx++) {
		if (wp_info[idx].set.wp_addr == (uintptr_t)ibp_addr) {
			if (ppe != NULL)
				*ppe = wp_info[idx].wp_pe;
			break;
		}
	}

	if (idx == MAX_WP_NUM) {
		mlog_e("cannot find the addr[0x%pK]\n", ibp_addr);
		return -EWPNOMTCH;
	}

	return idx;
}

static int wp_add_pe(int idx, struct wp_cfg_set *ibp_attr, struct perf_event *__percpu *pe)
{
	/* register ensure the num bigger than MAX_WP_NUM */
	wp_info_num++;
	if (wp_info_num > MAX_WP_NUM) {
		mlog_e("wp_info_num[%d] overflowed\n", wp_info_num);
		return -EWPOVFL;
	}

	wp_info[idx].set.wp_addr = ibp_attr->wp_addr;
	wp_info[idx].set.store.wp_addr_byte = ibp_attr->store.wp_addr_byte;
	wp_info[idx].set.store.wp_addr_mask = ibp_attr->store.wp_addr_mask;
	wp_info[idx].set.store.wp_rw_type = ibp_attr->store.wp_rw_type;
	wp_info[idx].wp_pe = pe;

	strlcpy(wp_info[idx].set.store.addr_str, ibp_attr->store.addr_str,
		ADDR_STR_LEN);

	return 0;
}

static void wp_delete_pe(int idx)
{
	/* delete the cooresponding addr */
	wp_info[idx].set.wp_addr = 0L;
	wp_info[idx].set.store.wp_addr_byte = 0;
	wp_info[idx].set.store.wp_addr_mask = 0;
	wp_info[idx].set.store.wp_rw_type = 0;
	wp_info[idx].wp_pe = NULL;

	wp_info_num--;

	if (wp_info_num < 0) {
		mlog_e("wp_info_num underflowed\n");
		wp_info_num = 0;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	/*
 * check whether the addr in the PC function range
 * @data : the addr to compare
 * @pc : stackframe pc
 *
 * Returns TRUE if in range, FALSE is not in range.
 * PC is the instruction accessed the data address
 * It belongs to a function, it's a stack of pc.
 */
static bool compare_address(void *data,unsigned long pc)
{
	struct watchpoint_cmp_addr *cmpaddr_p = data;
	unsigned long size = 0;
	char str[KSYM_NAME_LEN] = {0};
	const char *name = NULL;

	if (cmpaddr_p == NULL) {
		mlog_e("addr to compare is NULL\n");
		return WP_FALSE;
	}

	name = kallsyms_lookup(pc, &size, NULL, NULL, str);
	if (name == NULL) {
		mlog_e("lookup err, size=%lu, pc=0x%lx", size, pc);
		return WP_FALSE;
	}

	if (cmpaddr_p->ref_addr == 0) {
		mlog_n("[0x%lx] <%s>", pc, name);
		return WP_FALSE;
	}

	if ((pc >= cmpaddr_p->ref_addr) && (pc < cmpaddr_p->ref_addr + size)) {
		cmpaddr_p->is_caller_in_stack = WP_TRUE;
		return WP_TRUE;
	}

	return WP_FALSE;
}
#else
/*
 * check whether the addr in the PC function range
 * @frame : stackframe
 * @data : the addr to compare
 *
 * Returns TRUE if in range, FALSE is not in range.
 * PC is the instruction accessed the data address
 * It belongs to a function, it's a stack of pc.
 */
static int compare_address(struct stackframe *frame, void *data)
{
	struct watchpoint_cmp_addr *cmpaddr_p = data;
	unsigned long size = 0;
	char str[KSYM_NAME_LEN] = {0};
	const char *name = NULL;

	if (frame == NULL || cmpaddr_p == NULL) {
		mlog_e("stackframe or addr to compare is NULL\n");
		return WP_FALSE;
	}

	name = kallsyms_lookup(frame->pc, &size, NULL, NULL, str);
	if (name == NULL) {
		mlog_e("lookup err, size=%lu, pc=0x%lx", size, frame->pc);
		return WP_FALSE;
	}

	if (cmpaddr_p->ref_addr == 0) {
		mlog_n("[0x%lx] <%s>", frame->pc, name);
		return WP_FALSE;
	}

	if ((frame->pc >= cmpaddr_p->ref_addr) && (frame->pc < cmpaddr_p->ref_addr + size)) {
		cmpaddr_p->is_caller_in_stack = WP_TRUE;
		return WP_TRUE;
	}

	return WP_FALSE;
}
#endif
/*
 * to check the caller who bring about watchpoint exception.
 * @regs : the pointer of registers
 * @caller : the caller which is set by watchpoint user. this is as a reference.
 *
 * Return value. 0:the reference caller is not in the exception stack.
 *              1:the reference caller is in the exception stack.
 * When watchpoint exception occurs ,we want to check the function which
 * bring about this exception .
 * If the function is equal to our reference caller, then the exception process
 * will be exit ,kernel still run normally.
 * else kernel will be panic or the stack information will be dump out.
 *
 * this function is supply to watchpoint user as a call back function.
 */
static int wp_check_caller(const struct pt_regs *regs, const void *caller)
{
	u64 fp;
	struct stackframe frame;
	struct watchpoint_cmp_addr cmpaddr;

	if (regs == NULL) {
		mlog_e("pointer of registers is NULL\n");
		return WP_FALSE;
	}
	mlog_i("regs=0x%pK, caller=0x%pK", regs, caller);

	fp = regs->regs[WP_FP_OFFSET];
	if (fp == 0) {
		mlog_e("frame pointer is NULL, so can't recognise caller");
		return 0;
	}
	(void)memset_s(&frame, sizeof(frame), 0, sizeof(frame));
	frame.fp = regs->regs[WP_FP_OFFSET];
	frame.pc = regs->pc;
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	frame.sp = regs->sp;
#endif

	cmpaddr.ref_addr = (uintptr_t)caller;
	cmpaddr.is_caller_in_stack = WP_FALSE;
#if (KERNEL_VERSION(4, 4, 0) > LINUX_VERSION_CODE)
	walk_stackframe(&frame, compare_address, &cmpaddr);
#else
	walk_stackframe(current, &frame, compare_address, &cmpaddr);
#endif
	if (cmpaddr.is_caller_in_stack) {
		mlog_i("caller is in the stack");
		return WP_TRUE;
	}

	return WP_FALSE;
}

/*
 * watchpoint callback handler
 * @bp : perf_event
 * @data : perf_sample_data, not used
 * @regs : ptrace regs
 *
 * Tne function is to check whitelist and do the corresponding act.
 */
static void wp_handler(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs)
{
	u64 addr;
	u64 trigger;
	int is_in_stack;

	(void)(data);

	if (bp == NULL) {
		mlog_e("pointer of perf event is NULL\n");
		return;
	}
	addr = bp->attr.bp_addr;
	trigger = bp->hw.info.trigger;

	/* check white list */
	is_in_stack = watchpt_wlist_check(regs);
	if (is_in_stack) {
		/* do nothing */
		if (is_show_expected) {
			mlog_n("a watchpoint triggered, trigger=0x%llx, addr=0x%llx", trigger, addr);
			mlog_n("the expected caller");
		}
	} else {
		/* dump stack and run as usual */
		mlog_n("a watchpoint triggered, trigger=0x%llx, addr=0x%llx", trigger, addr);
		mlog_n("the unexpected caller");

		/* trigger type */
		if (watchpt_get_tg_type() == TG_TYPE_DUMP) {
			/* walk stack */
			wp_check_caller(regs, NULL);
			dump_stack();
		} else {
			die("Watchpoint", regs, 0);
		}
	}
}

/*
 * register a watchpoint
 * @i_attr : the param to register
 *
 * Returns 0 for success, other value for fail.
 */
static int wp_register(struct wp_cfg_set *i_attr)
{
	struct perf_event *__percpu *pe = NULL;
	struct perf_event_attr attr;
	int idx;
	int ret;

	mlog_e("Begin, addr=0x%llx, byte=%u, mask=%u, type=%u\n",
			i_attr->wp_addr, i_attr->store.wp_addr_byte,
			i_attr->store.wp_addr_mask, i_attr->store.wp_rw_type);

	/* init one watchpoint for startup watch */
	hw_breakpoint_init(&attr);
	attr.bp_addr = i_attr->wp_addr;
	attr.bp_len = i_attr->store.wp_addr_byte;
	attr.bp_addr_mask = i_attr->store.wp_addr_mask;
	attr.bp_type = i_attr->store.wp_rw_type;

	pe = register_wide_hw_breakpoint(&attr, wp_handler, NULL);
	if (IS_ERR(pe)) {
		ret = (int)PTR_ERR(pe);
		mlog_e("register failed, addr=0x%llx, ret = %d\n",
				i_attr->wp_addr, ret);
	} else {
		/* find an empty space and record pe in array */
		idx = wp_find_pe(NULL, NULL);
		if (idx >= 0) {
			ret = wp_add_pe(idx, i_attr, pe);
		} else {
			mlog_e("find space failed, addr=0x%llx, idx=%d\n", attr.bp_addr, idx);
			ret = -EWPNOSPC;
		}
	}

	mlog_i("End, ret = %d\n", ret);
	return ret;
}

/*
 * unregister a watchpoint
 * @i_addr : the addr to unregister a watchpoint
 *
 * Returns negative for error, 0 for ok.
 */
static int wp_unregister(const void *i_addr)
{
	int idx;
	int ret = 0;
	struct perf_event *__percpu *pe = NULL;

	mlog_i("Begin, addr=0x%pK", i_addr);

	if (wp_info_num == 0) {
		mlog_e("wp_info_num is zero");
		return -EWPNOWP;
	}

	/* find pe according to addr */
	idx = wp_find_pe(i_addr, &pe);
	if (idx >= 0) {
		unregister_wide_hw_breakpoint(pe);

		/* delete the cooresponding addr */
		wp_delete_pe(idx);
	} else {
		mlog_e("error, addr=0x%pK", i_addr);
		ret = -EWPNOADDR;
	}

	mlog_i("End");
	return ret;
}

/*
 * register a watchpoint
 * @addr : the addr to watch
 * @byte : the addr byte to watch, 1 2 4 or 8 is available
 * @mask : 0 for the max length of 8 byte
 *             3~31 for a range calculated in power of 2
 *                     for example 5 is for a range of 32.
 * @access : read or write to the addr range
 *              HW_BREAKPOINT_R(1) HW_BREAKPOINT_W(2) or rw(3)
 *
 * Returns negative for error, 0 or positive is ok.
 * Monitor range is decided by mask and byte.
 * If range <= 8, set mask=0, range is decided by byte.
 * If range > 8, set byte=8.
 */
int watchpoint_enable(u64 addr, unsigned char byte, unsigned char mask, unsigned char access)
{
	struct wp_cfg_set l_wp_set;

	l_wp_set.wp_addr = addr;
	l_wp_set.store.wp_addr_byte = byte;
	l_wp_set.store.wp_addr_mask = mask;
	l_wp_set.store.wp_rw_type = access;

	return wp_register(&l_wp_set);
}
EXPORT_SYMBOL_GPL(watchpoint_enable);

/*
 * unregister a watchpoint
 * @addr : the addr watched
 *
 * Returns negative for error, 0 for ok.
 */
int watchpoint_disable(const void *addr)
{
	return wp_unregister(addr);
}
EXPORT_SYMBOL_GPL(watchpoint_disable);


#ifdef CONFIG_DFX_DEBUG_FS
/*
 * reads the user input string
 *
 * The return and parameter is same to simple_write_to_buffer().
 *
 */
static ssize_t watchpt_from_user(char *kbuf, size_t kbuf_size, loff_t *ppos, const char __user *ubuf, size_t ubuf_cnt)
{
	ssize_t write;

	write = simple_write_to_buffer(kbuf, kbuf_size - 1, ppos, ubuf, ubuf_cnt);

	if (write < 0)
		return write;

	kbuf[write] = 0;

	return write;
}
#endif

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * parse value to string
 * @i_access : input, access value
 * @o_access_str : output, buffer to hold the string
 * @i_access_str_len : input, size of o_access_str
 *
 * Returns the length of access string parsed
 */
static u32 watchpt_access_parse_to_string(u8 i_access, char *o_access_str, u32 i_access_str_len)
{
	u32 ilen = i_access_str_len;
	u32 olen = 0;
	int ret;

	if (memset_s(o_access_str, ilen, 0, ilen) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return 0;
	}
	if ((i_access & ACCESS_BIT_RW) == 0) {
		/* no access attr */
		ret = snprintf_s(o_access_str, ilen, ilen - 1, ACCESS_STR_NONE);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_access_str += ret;

		ret = snprintf_s(o_access_str, ilen, ilen - 1, "\n");
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_access_str += ret;

		return strlen(ACCESS_STR_NONE) + 1;
	}

	if (i_access & ACCESS_BIT_READ) {
		ret = snprintf_s(o_access_str, ilen, ilen - 1, ACCESS_STR_READ);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_access_str += ret;
		olen++;
	}

	if (i_access & ACCESS_BIT_WRITE) {
		ret = snprintf_s(o_access_str, ilen, ilen - 1, ACCESS_STR_WRITE);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_access_str += ret;
		olen++;
	}

	ret = snprintf_s(o_access_str, ilen, ilen - 1, "\n");
	if (unlikely(ret < 0)) {
		mlog_e("snprintf_s ret %d!\n", ret);
		return 0;
	}
	o_access_str += ret;
	olen++;
	return olen;
}
#endif

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * access read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_access_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	u32 len;
	ssize_t cnt;
	char watchpt_para_access_str[ACCESS_STR_LEN] = {0};

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);
	len = watchpt_access_parse_to_string(wp_set.store.wp_rw_type, watchpt_para_access_str, ACCESS_STR_LEN);
	mutex_unlock(&watchpt_para_lock);

	cnt = simple_read_from_buffer(ubuf, count, ppos, watchpt_para_access_str, len);

	return cnt;
}

/*
 * parse string to value
 * @o_attr : output, the pointer to hold the attr
 * @i_str : input, the buffer to hold the string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_access_parse(u8 *o_attr, char *i_str, u32 i_str_len)
{
	size_t i;
	size_t len;
	char chr;
	char *str = NULL;

	/* skip spaces */
	str = strstrip(i_str);

	len = strlen(str);
	if ((len <= 0) || (len > ACCESS_CHR_NUM)) {
		/* length error */
		*o_attr = ACCESS_BIT_NONE;
		return;
	}

	if (len == ACCESS_CHR_NUM) {
		if (str[0] == str[1]) {
			/* same char */
			*o_attr = ACCESS_BIT_NONE;
			return;
		}
	}

	*o_attr = 0;
	for (i = 0; i < len; i++) {
		chr = str[i];
		if (chr == ACCESS_CHR_READ) {
			*o_attr |= ACCESS_BIT_READ;
		} else if (chr == ACCESS_CHR_WRITE) {
			*o_attr |= ACCESS_BIT_WRITE;
		} else {
			/* unexpected, set to 0 */
			*o_attr = ACCESS_BIT_NONE;
			break;
		}
	}
}

/*
 * access write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_access_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[ACCESS_STR_LEN] = {0};
	const size_t buffer_size = ACCESS_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);
		watchpt_access_parse(&wp_set.store.wp_rw_type, buffer, buffer_size);
		mutex_unlock(&watchpt_para_lock);
	}

	return cnt;
}

static const struct file_operations watchpt_access_fops = {
	.read = watchpt_access_read,
	.write = watchpt_access_write,
};
#endif

static u8 watchpt_get_tg_type(void)
{
	return wp_pub.wp_tg_type;
}

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * parse value to string
 * @i_attr : input, access value
 * @o_str : output, buffer to hold the string
 * @i_str_len : input, size of o_str
 *
 * Returns the length of string parsed
 */
static u32 watchpt_tg_type_parse_to_string(char i_attr, char *o_str, u32 i_str_len)
{
	u32 ilen = i_str_len;
	u32 olen;
	int ret;

	if (memset_s(o_str, ilen, 0, ilen) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return 0;
	}
	switch (i_attr) {
	case TG_TYPE_PAINC:
		strlcpy(o_str, TG_TYPE_STR_PANIC, ilen);
		break;
	case TG_TYPE_DUMP:
		strlcpy(o_str, TG_TYPE_STR_DUMP, ilen);
		break;
	default:
		strlcpy(o_str, TG_TYPE_STR_NONE, ilen);
		break;
	}

	olen = strlen(o_str);
	if (olen >= (ilen - 1)) {
		mlog_e("[%s:%d]no space left\n", __func__, __LINE__);
		return olen;
	}
	ret = strncat_s(o_str, ilen, "\n", 1);
	if (ret != EOK) {
		mlog_e("[%s:%d]strncat_s ret %d!\n", __func__, __LINE__, ret);
		return 0;
	}
	olen = strlen(o_str);
	return olen;
}

/*
 * trigger type read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_tg_type_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	u32 len;
	ssize_t cnt;
	char tg_type_str[TG_TYPE_STR_LEN] = {0};

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);
	len = watchpt_tg_type_parse_to_string(wp_pub.wp_tg_type, tg_type_str, TG_TYPE_STR_LEN);
	mutex_unlock(&watchpt_para_lock);

	cnt = simple_read_from_buffer(ubuf, count, ppos, tg_type_str, len);
	return cnt;
}

/*
 * parse string to value
 * @o_attr : output, the pointer to hold the attr
 * @i_str : input, the buffer to hold the string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_tg_type_parse(u8 *o_attr, char *i_str, u32 i_str_len)
{
	char *str = NULL;

	/* skip spaces */
	str = strstrip(i_str);
	if (strncmp(str, TG_TYPE_STR_PANIC, strlen(TG_TYPE_STR_PANIC) + 1) == 0)
		*o_attr = TG_TYPE_PAINC;
	else if (strncmp(str, TG_TYPE_STR_DUMP, strlen(TG_TYPE_STR_DUMP) + 1) == 0)
		*o_attr = TG_TYPE_DUMP;
	else
		*o_attr = TG_TYPE_PAINC;
}

/*
 * trigger type write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_tg_type_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[TG_TYPE_STR_LEN];
	const size_t buffer_size = TG_TYPE_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);
		watchpt_tg_type_parse(&wp_pub.wp_tg_type, buffer, buffer_size);
		mutex_unlock(&watchpt_para_lock);
	}

	return cnt;
}

static const struct file_operations watchpt_ttype_fops = {
	.read = watchpt_tg_type_read,
	.write = watchpt_tg_type_write,
};

/*
 * parse value to string
 * @i_attr : input, access value
 * @o_str : output, buffer to hold the string
 * @i_str_len : input, size of o_str
 *
 * Returns the length of string parsed
 */
static u32 watchpt_addr_byte_parse_to_string(char i_attr, char *o_str, u32 i_str_len)
{
	u32 ilen = i_str_len;
	u32 olen;
	int ret;

	if (memset_s(o_str, ilen, 0, ilen) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return 0;
	}
	switch (i_attr) {
	case ADDR_BYTE_1:
		ret = snprintf_s(o_str, ilen, ilen - 1, ADDR_BYTE_STR_1);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_str += ret;

		olen = strlen(ADDR_BYTE_STR_1);
		break;
	case ADDR_BYTE_2:
		ret = snprintf_s(o_str, ilen, ilen - 1, ADDR_BYTE_STR_2);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_str += ret;

		olen = strlen(ADDR_BYTE_STR_2);
		break;
	case ADDR_BYTE_4:
		ret = snprintf_s(o_str, ilen, ilen - 1, ADDR_BYTE_STR_4);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_str += ret;

		olen = strlen(ADDR_BYTE_STR_4);
		break;
	case ADDR_BYTE_8:
		ret = snprintf_s(o_str, ilen, ilen - 1, ADDR_BYTE_STR_8);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_str += ret;

		olen = strlen(ADDR_BYTE_STR_8);
		break;
	default:
		ret = snprintf_s(o_str, ilen, ilen - 1, ADDR_BYTE_STR_N);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
		o_str += ret;

		olen = strlen(ADDR_BYTE_STR_N);
		break;
	}

	ret = snprintf_s(o_str, ilen, ilen - 1, "\n");
	if (unlikely(ret < 0)) {
		mlog_e("snprintf_s ret %d!\n", ret);
		return 0;
	}
	o_str += ret;
	olen++;

	return olen;
}

/*
 * addr byte read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_byte_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	u32 len;
	ssize_t cnt;
	char watchpt_para_len_str[ADDR_BYTE_STR_LEN] = {0};

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);
	len = watchpt_addr_byte_parse_to_string(wp_set.store.wp_addr_byte, watchpt_para_len_str, ADDR_BYTE_STR_LEN);
	mutex_unlock(&watchpt_para_lock);

	cnt = simple_read_from_buffer(ubuf, count, ppos, watchpt_para_len_str, len);
	return cnt;
}

/*
 * parse string to value
 * @o_attr : output, the pointer to hold the attr
 * @i_str : input, the buffer to hold the string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_addr_byte_parse(u8 *o_attr, char *i_str, u32 i_str_len)
{
	char *str = NULL;

	/* skip spaces */
	str = strstrip(i_str);
	if (strncmp(str, ADDR_BYTE_STR_1, strlen(ADDR_BYTE_STR_1) + 1) == 0)
		*o_attr = ADDR_BYTE_1;
	else if (strncmp(str, ADDR_BYTE_STR_2, strlen(ADDR_BYTE_STR_2) + 1) == 0)
		*o_attr = ADDR_BYTE_2;
	else if (strncmp(str, ADDR_BYTE_STR_4, strlen(ADDR_BYTE_STR_4) + 1) == 0)
		*o_attr = ADDR_BYTE_4;
	else if (strncmp(str, ADDR_BYTE_STR_8, strlen(ADDR_BYTE_STR_8) + 1) == 0)
		*o_attr = ADDR_BYTE_8;
	else
		*o_attr = ADDR_BYTE_NONE;
}

/*
 * addr byte write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_byte_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[ADDR_BYTE_STR_LEN] = {0};
	const size_t buffer_size = ADDR_BYTE_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);
		watchpt_addr_byte_parse(&wp_set.store.wp_addr_byte, buffer, buffer_size);
		mutex_unlock(&watchpt_para_lock);
	}

	return cnt;
}

static const struct file_operations watchpt_addr_byte_fops = {
	.read = watchpt_addr_byte_read,
	.write = watchpt_addr_byte_write,
};

/*
 * parse value to string
 * @i_attr : input, access value
 * @o_str : output, buffer to hold the string
 * @i_str_len : input, size of o_str
 *
 * Returns the length of string parsed
 */
static void watchpt_addr_mask_parse_to_string(char i_attr, char *o_str, u32 *io_str_len)
{
	u32 ilen = *io_str_len;
	u32 olen;
	char *buf = o_str;
	int ret;

	mlog_i("Begin, i_attr=%d, len=%u\n", i_attr, ilen);

	if (memset_s(buf, ilen, 0, ilen) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return;
	}
	if (i_attr > ADDR_MASK_MAX) {
		ret = snprintf_s(buf, ilen, ilen - 1, ADDR_MASK_STR_N);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return;
		}
		buf += ret;
	} else {
		ret = snprintf_s(buf, ilen, ilen - 1, "%d", i_attr);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return;
		}
		buf += ret;
	}

	ret = snprintf_s(buf, ilen, ilen - 1, "\n");
	if (unlikely(ret < 0)) {
		mlog_e("snprintf_s ret %d!\n", ret);
		return;
	}
	buf += ret;

	olen = strlen(o_str);

	*io_str_len = olen;

	mlog_i("End, olen=%u, o_str=<%s>\n", olen, o_str);
}

/*
 * addr mask read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_mask_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	u32 len = ADDR_MASK_STR_LEN;
	char watchpt_para_addr_mask_str[ADDR_MASK_STR_LEN] = {0};
	ssize_t cnt;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);
	watchpt_addr_mask_parse_to_string(wp_set.store.wp_addr_mask, watchpt_para_addr_mask_str, &len);
	mutex_unlock(&watchpt_para_lock);

	mlog_i("str=%s, len=%u\n", watchpt_para_addr_mask_str, len);

	cnt = simple_read_from_buffer(ubuf, count, ppos, watchpt_para_addr_mask_str, len);

	return cnt;
}

/*
 * parse string to value
 * @o_attr : output, the pointer to hold the attr
 * @i_str : input, the buffer to hold the string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_addr_mask_parse(u8 *o_attr, char *i_str, u32 i_str_len)
{
	u32 attr;
	char *str = NULL;

	/* skip spaces */
	str = strstrip(i_str);
	if (kstrtou32(str, DECIMAL_BASE, &attr))
		*o_attr = ADDR_MASK_MAX + 1;
	else
		*o_attr = attr;

	mlog_i("str=%s, attr=%u\n", str, attr);
}

/*
 * addr mask write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_mask_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[ADDR_MASK_STR_LEN] = {0};
	const size_t buffer_size = ADDR_MASK_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);
		watchpt_addr_mask_parse(&wp_set.store.wp_addr_mask, buffer, buffer_size);
		mutex_unlock(&watchpt_para_lock);
	}

	return cnt;
}


static const struct file_operations watchpt_addr_mask_fops = {
	.read = watchpt_addr_mask_read,
	.write = watchpt_addr_mask_write,
};

/*
 * check the addr type
 * @i_str : input, the buffer to hold the addr string
 * @i_str_len : input, the size of i_str
 *
 * Returns the addr type
 *
 * The i_str may be variable name, struct name or a virtual_addr.
 * The variable name and struct name can add a offset, in form of vname+offset.
 */
static int watchpt_check_addr_type(const char *i_str, u32 i_str_len)
{
	const char *str = i_str;

	if (str == NULL)
		return -1;
	if (strncmp(str, "0x", STR_LEN_OFFSET) == 0)
		return ADDR_TYPE_VADDR;
	if (strncmp(str, "#", 1) == 0)
		return ADDR_TYPE_NONE;
	if (strnchr(str, i_str_len, '+'))
		return ADDR_TYPE_VNAME_PLUS;

	return ADDR_TYPE_VNAME;
}

/*
 * parse the intput addr string to the addr value and astr
 * @o_attr : output, the pointer to hold the addr value
 * @o_astr : output, the pointer to hold the addr string
 * @i_astr_len : input, the size of o_astr
 * @i_str : input, the buffer to hold the string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_addr_parse(u64 *o_attr, char *o_astr, u32 i_astr_len, char *i_str, u32 i_str_len)
{
	char *name = NULL;
	char *poffset = NULL;
	u32 offset = 0;
	u32 name_len;
	u64 addr;
	int type;

	/* skip spaces */
	name = strstrip(i_str);
	name_len = strlen(name);
	name_len = name_len > i_str_len ? i_str_len : name_len;

	/* set output str first */
	strlcpy(o_astr, name, i_astr_len);

	type = watchpt_check_addr_type(name, name_len);
	switch (type) {
	case ADDR_TYPE_VADDR:
		if (kstrtou64(name, HEXADECIMAL_BASE, &addr))
			goto parse_error;
		break;
	case ADDR_TYPE_VNAME_PLUS:
		/* get offset */
		poffset = strnchr(name, i_str_len, '+');
		if (kstrtou32(poffset + 1, DECIMAL_BASE, &offset))
			goto parse_error;

		/* set + to 0 */
		*poffset = 0;

		/* fall-through */
	case ADDR_TYPE_VNAME:
		addr = kallsyms_lookup_name(name);
		if (addr == 0) {
			mlog_e("name<%s> not found\n", name);
			goto parse_error;
		}
		break;
	default:
		goto parse_error;
	}

	/* set output attr */
	*o_attr = addr + offset;

	mlog_i("o_str<%s>, *o_attr=%llu, addr=%llx, offset=%u\n", o_astr, *o_attr, addr, offset);

	return;

parse_error:
	*o_attr = ADDR_NONE;
	strlcpy(o_astr, ADDR_STR_NONE, i_astr_len);
}

/*
 * addr write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[WP_STR_LEN] = {0};
	const size_t buffer_size = WP_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mlog_i("Begin, cnt=%lu, pos=%lld\n", cnt, *ppos);

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);

		watchpt_addr_parse(&wp_set.wp_addr, wp_set.store.addr_str, WP_STR_LEN, buffer, WP_STR_LEN);

		mutex_unlock(&watchpt_para_lock);
	}

	mlog_i("End, cnt=%lu, pos=%lld, writen=%ld\n", cnt, *ppos, write);
	return cnt;
}

/*
 * parse value to string
 * @i_attr : input, the struct hold the addr
 * @o_str : output, buffer to hold the string
 * @i_str_len : input, size of o_str
 *
 * Returns the length of string parsed
 */
static u32 watchpt_addr_parse_to_string(struct wp_cfg_set *i_attr, char *o_str, u32 i_str_len)
{
	u32 alen;
	u32 olen;
	int type;
	char *astr = NULL;
	int ret;

	if (memset_s(o_str, i_str_len, 0, i_str_len) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return 0;
	}

	astr = i_attr->store.addr_str;
	alen = strlen(astr);

	type = watchpt_check_addr_type(astr, alen);
	if ((type == ADDR_TYPE_VNAME_PLUS) || (type == ADDR_TYPE_VNAME)) {
		ret = snprintf_s(o_str, i_str_len - STR_LEN_OFFSET, i_str_len - STR_LEN_OFFSET - 1,
				"%s[0x%llx]", astr, i_attr->wp_addr);
		if (unlikely(ret < 0)) {
			mlog_e("snprintf_s ret %d!\n", ret);
			return 0;
		}
	} else {
		strlcpy(o_str, astr, i_str_len - STR_LEN_OFFSET);
	}

	olen = strlen(o_str);
	if (olen >= (i_str_len - 1)) {
		mlog_e("[%s:%d]no space left\n", __func__, __LINE__);
		return olen;
	}
	ret = strncat_s(o_str, i_str_len, "\n", 1);
	if (ret != EOK) {
		mlog_e("[%s:%d]strncat_s ret %d!\n", __func__, __LINE__, ret);
		return 0;
	}
	olen = strlen(o_str);
	return olen;
}

/*
 * addr read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_addr_read(struct file *filp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	u32 len;
	ssize_t count;
	char watchpt_para_addr_str[WP_STR_LEN] = {0};

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);
	len = watchpt_addr_parse_to_string(&wp_set, watchpt_para_addr_str, WP_STR_LEN);
	mutex_unlock(&watchpt_para_lock);

	count = simple_read_from_buffer(ubuf, cnt, ppos, watchpt_para_addr_str, len);

	return count;
}

static const struct file_operations watchpt_addr_fops = {
	.read = watchpt_addr_read,
	.write = watchpt_addr_write,
};
#endif

/*
 * check whether the white list function is in stack
 * @regs : pt regs
 *
 * Returns 0: not in stack, 1: in stack.
 */
static int watchpt_wlist_check(const struct pt_regs *regs)
{
	struct wp_white_llist *pwlist = NULL;
	struct list_head *pos = NULL;
	int in_stack;

	list_for_each(pos, &wlist_head) {
		/* get the white list info */
		pwlist = list_entry(pos, struct wp_white_llist, node);

		in_stack = wp_check_caller(regs, (void *)(uintptr_t)pwlist->func_addr);
		if (in_stack) {
			mlog_i("in stack, func_name=<%s>\n", pwlist->func_name);
			break;
		}
	}

	return in_stack;
}

#ifdef CONFIG_DFX_DEBUG_FS
/* clear all the white list function */
static void watchpt_wlist_clear(void)
{
	struct wp_white_llist *pwlist = NULL;
	struct list_head *pos = NULL;
	struct list_head *next = NULL;

	/* clear all */
	list_for_each_safe(pos, next, &wlist_head) {
		/* get the white list info */
		pwlist = list_entry(pos, struct wp_white_llist, node);

		if (pwlist->func_name != NULL) {
			kfree(pwlist->func_name);
			pwlist->func_name = NULL;
		}
		pwlist->func_addr = 0L;

		list_del(pos);

		kfree(pwlist);
		pwlist = NULL;
	}
}

/*
 * add the addr to the white list
 * @name : the function name
 * @addr : the function address
 *
 * Returns 0: add ok, negative for add failed.
 */
static int watchpt_wlist_add(const char *name, u64 addr)
{
	struct wp_white_llist *pwlist = NULL;
	size_t len;

	pwlist = kzalloc(sizeof(*pwlist), GFP_KERNEL);
	if (pwlist == NULL) {
		mlog_e("kmalloc failed\n");
		return -ENOMEM;
	}

	len = strlen(name);
	len++;
	len = len > FUNC_NAME_LEN ? FUNC_NAME_LEN : len;

	pwlist->func_name = kzalloc(len, GFP_KERNEL);
	if (pwlist->func_name == NULL) {
		mlog_e("kmalloc failed\n");
		kfree(pwlist);
		pwlist = NULL;
		return -ENOMEM;
	}
	/* name */
	strlcpy(pwlist->func_name, name, len);
	/* addr */
	pwlist->func_addr = addr;

	list_add(&pwlist->node, &wlist_head);

	return 0;
}

/*
 * parse string and add to the white list
 * @i_str : input, the buffer to hold the function name string
 * @i_str_len : input, the size of i_str
 */
static void watchpt_wlist_parse(char *i_str, u32 i_str_len)
{
	char *name = NULL;
	u32 name_len;
	u64 addr;

	/* skip spaces */
	name = strstrip(i_str);
	name_len = strlen(name);
	name_len = name_len > i_str_len ? i_str_len : name_len;
	mlog_i("name=<%s>, len=%u, num=%d\n", name, name_len, wp_whitelist_num);

	if (name_len == 0) {
		watchpt_wlist_clear();
		wp_whitelist_num = 0;
		return;
	}

	if (wp_whitelist_num >= WHITE_LIST_MAX_NUM) {
		mlog_e("reach max number %d\n", wp_whitelist_num);
		return;
	}

	/* check validation */
	addr = kallsyms_lookup_name(name);
	if (addr == 0) {
		mlog_e("name<%s> not found\n", name);
		return;
	}

	/* save */
	if (watchpt_wlist_add(name, addr) == 0)
		wp_whitelist_num++;

	mlog_i("name<%s>, addr=0x%llx, num=%d\n", name, addr, wp_whitelist_num);
}

/*
 * white list write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_wlist_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	ssize_t write;
	char buffer[WP_STR_LEN] = {0};
	const size_t buffer_size = WP_STR_LEN;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	write = watchpt_from_user(buffer, buffer_size, ppos, ubuf, cnt);
	if (write > 0) {
		mutex_lock(&watchpt_para_lock);
		watchpt_wlist_parse(buffer, buffer_size);
		mutex_unlock(&watchpt_para_lock);
	}

	return cnt;
}

/*
 * parse value to string
 * @o_str : output, buffer to hold the string
 * @io_str_len : input/output, size of o_str/length of o_str
 */
static void watchpt_wlist_parse_to_string(char *o_str, u32 *io_str_len)
{
	struct wp_white_llist *pwlist = NULL;
	struct list_head *pos = NULL;
	u32 ilen = *io_str_len;
	char *buf = o_str;
	int ret;

	if (memset_s(buf, ilen, 0, ilen) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return;
	}
	list_for_each(pos, &wlist_head) {
		/* get the white list info */
		pwlist = list_entry(pos, struct wp_white_llist, node);

		if (pwlist->func_name != NULL) {
			ret = snprintf_s(buf, ilen, ilen - 1, "%s\n", pwlist->func_name);
			if (unlikely(ret < 0)) {
				mlog_e("snprintf_s ret %d!\n", ret);
				return;
			}
			buf += ret;
		}
	}

	*io_str_len = strlen(o_str);
}

/*
 * white list read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_wlist_read(struct file *filp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	u32 len;
	char *buf = NULL;
	ssize_t count;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	buf = kzalloc(WP_DBG_SIZE_PAGE, GFP_KERNEL);
	if (buf == NULL) {
		mlog_e("kmalloc failed\n");
		return -1;
	}

	mutex_lock(&watchpt_para_lock);

	len = WP_DBG_SIZE_PAGE;
	watchpt_wlist_parse_to_string(buf, &len);

	mutex_unlock(&watchpt_para_lock);

	mlog_i("buf=%s, len=%u\n", buf, len);

	count = simple_read_from_buffer(ubuf, cnt, ppos, buf, len);

	kfree(buf);
	buf = NULL;

	return count;
}

static const struct file_operations watchpt_white_list_fops = {
	.read = watchpt_wlist_read,
	.write = watchpt_wlist_write,
};

/*
 * parse the state info
 * @o_str : output, the buffer to hold state info
 * @i_str_len : input, the size of i_str
 */
static void watchpt_stat_parse(char *o_str, u32 i_str_len)
{
	u64 val[WP_BUFF_LEN] = {0};
	u64 ctl[WP_BUFF_LEN] = {0};
	int i;
	int num;
	int num_en = 0;
	struct arch_hw_breakpoint_ctrl ctrl;
	int ret;

	if (memset_s(o_str, i_str_len, 0, i_str_len) != EOK) {
		mlog_e("[%s:%d]memset_s fail!\n", __func__, __LINE__);
		return;
	}
	num = hw_breakpoint_slots(TYPE_DATA);

	arch_read_wvr(val, WP_BUFF_LEN);
	arch_read_wcr(ctl, WP_BUFF_LEN);

	for (i = 0; i < num; i++) {
		/* check enable */
		decode_ctrl_reg(ctl[i], &ctrl);
		if (ctrl.enabled) {
			/* value */
			ret = snprintf_s(o_str, i_str_len, i_str_len - 1, "wvr[%d]: 0x%llx\n", i, val[i]);
			if (unlikely(ret < 0)) {
				mlog_e("snprintf_s ret %d!\n", ret);
				return;
			}
			o_str += ret;

			/* control */
			ret = snprintf_s(o_str, i_str_len, i_str_len - 1,
					"wcr[%d]: byte=0x%x, mask=%u, access=%c%c\n",
					i, ctrl.len, ctrl.mask,
					ctrl.type & HW_BREAKPOINT_R ? 'r' : 'o',
					ctrl.type & HW_BREAKPOINT_W ? 'w' : 'o');
			if (unlikely(ret < 0)) {
				mlog_e("snprintf_s ret %d!\n", ret);
				return;
			}
			o_str += ret;

			num_en++;
		}
	}
	ret = snprintf_s(o_str, i_str_len, i_str_len - 1, "watchpoint enabled %d, total %d\n", num_en, num);
	if (unlikely(ret < 0)) {
		mlog_e("snprintf_s ret %d!\n", ret);
		return;
	}
	o_str += ret;
}

/*
 * state read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @count : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_stat_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	ssize_t cnt;
	char kbuf[STAT_STR_LEN] = {0};

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);

	watchpt_stat_parse(kbuf, STAT_STR_LEN);
	mlog_i("len=%lu, str=%s\n", strlen(kbuf), kbuf);

	mutex_unlock(&watchpt_para_lock);

	cnt = simple_read_from_buffer(ubuf, count, ppos, kbuf, strlen(kbuf));

	return cnt;
}

static const struct file_operations watchpt_stat_fops = {
	.read = watchpt_stat_read,
};

/*
 * addr read for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read to
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 */
static ssize_t watchpt_set_read(struct file *filp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[SET_STR_LEN] = {0};
	char *pbuf = buf;
	char *para = NULL;
	size_t read;
	int ret = 0;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	mutex_lock(&watchpt_para_lock);

	if (watchpt_para_set >= 0)
		para = SET_STR_OK;
	else if (watchpt_para_set == SET_NONE)
		para = SET_STR_NONE;
	else
		para = SET_STR_FAIL;

	ret = snprintf_s(pbuf, SET_STR_LEN, SET_STR_LEN - 1, "%s\n", para);
	if (unlikely(ret < 0)) {
		mlog_e("snprintf_s ret %d!\n", ret);
		mutex_unlock(&watchpt_para_lock);
		return 0;
	}
	pbuf += ret;

	mutex_unlock(&watchpt_para_lock);

	read = pbuf - buf;

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, read);
}

/*
 * set write for user space
 * @filp : file pointer, not used
 * @ubuf : the user space buffer to read from
 * @cnt : the maximum number of bytes to read
 * @ppos : the current position in the buffer
 *
 * Returns the number of bytes read.
 * echo 1 : register the watchpoint
 * echo 0 : unregister the watchpoint
 * cat : show the set result
 */
static ssize_t watchpt_set_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	u64 val;
	int ret;

	if (ubuf == NULL || ppos == NULL)
		return 0;

	ret = kstrtoul_from_user(ubuf, cnt, DECIMAL_BASE, (unsigned long *)&val);
	if (ret) {
		watchpt_para_set = SET_NONE;
		return ret;
	}

	mutex_lock(&watchpt_para_lock);
	if (val == SET_EN)
		watchpt_para_set = wp_register(&wp_set);
	else if (val == SET_DIS)
		watchpt_para_set = wp_unregister((void *)(uintptr_t)wp_set.wp_addr);
	else
		watchpt_para_set = SET_NONE;

	mutex_unlock(&watchpt_para_lock);

	(*ppos)++;

	return cnt;
}

static const struct file_operations watchpt_set_fops = {
	.read = watchpt_set_read,
	.write = watchpt_set_write,
};
#endif

/*
 * initilize the param
 * @pset : the wp_cfg_set struct pointer
 * @ppub : the wp_cfg_pub struct pointer
 */
static void watchpt_para_init(struct wp_cfg_set *pset, struct wp_cfg_pub *ppub)
{
	(void)memset_s(pset, sizeof(struct wp_cfg_set), 0, sizeof(struct wp_cfg_set));

	pset->wp_addr = ADDR_NONE;
	pset->store.wp_rw_type = ACCESS_BIT_WRITE;
	pset->store.wp_addr_mask = ADDR_MASK_0;
	pset->store.wp_addr_byte = ADDR_BYTE_8;
	strlcpy(pset->store.addr_str, ADDR_STR_NONE, ADDR_STR_LEN);

	/* trigger type */
	ppub->wp_tg_type = TG_TYPE_PAINC;
}

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * create a file in the debugfs filesystem
 * @name: a pointer to a string containing the name of the file to create.
 * @parent: a pointer to the parent dentry for this file.  This should be a
 *          directory dentry if set.  If this paramater is NULL, then the
 *          file will be created in the root of the debugfs filesystem.
 * @fops: a pointer to a struct file_operations that should be used for
 *        this file.
 *
 * Returns a pointer to a dentry if it succeeds, NULL for fail.
 */
struct dentry *wp_create_file(const char *name, struct dentry *parent, const struct file_operations *fops)
{
	struct dentry *ret = NULL;

	if (name == NULL || parent == NULL || fops == NULL)
		return NULL;

	ret = debugfs_create_file(name, WP_FILE_MODE, parent, NULL, fops);
	if (ret == NULL)
		mlog_e("Could not create debugfs '%s' entry\n", name);

	return ret;
}
#endif

/*
 * the watchpoint debugfs interface init function
 *
 * Returns 0 for success, negative for fail.
 */
static int watchpt_debugfs_init(void)
{
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *df_dir = NULL;

	df_dir = debugfs_create_dir("watchpt", NULL);
	if (df_dir == NULL) {
		mlog_e("Could not create debugfs watchpt dir\n");
		return -ENOMEM;
	}

	/* addr */
	if (wp_create_file("addr", df_dir, &watchpt_addr_fops) == NULL)
		goto error;

	/* byte */
	if (wp_create_file("addr_byte", df_dir, &watchpt_addr_byte_fops) == NULL)
		goto error;

	/* mask */
	if (wp_create_file("addr_mask", df_dir, &watchpt_addr_mask_fops) == NULL)
		goto error;

	/* access */
	if (wp_create_file("access", df_dir, &watchpt_access_fops) == NULL)
		goto error;

	/* set */
	if (wp_create_file("set", df_dir, &watchpt_set_fops) == NULL)
		goto error;

	/* status */
	if (wp_create_file("stat", df_dir, &watchpt_stat_fops) == NULL)
		goto error;

	/* white list */
	if (wp_create_file("white_list", df_dir, &watchpt_white_list_fops) == NULL)
		goto error;

	/* trigger type */
	if (wp_create_file("tg_type", df_dir, &watchpt_ttype_fops) == NULL)
		goto error;

	return 0;

error:
	debugfs_remove(df_dir);
	df_dir = NULL;

	return -ENOMEM;
#else
	return -ENOMEM;
#endif
}

static int __init early_parse_wpt_addr_cmdline(char *hbp_addr_cmdline)
{
	u64 addr;
	int rc;

	if (hbp_addr_cmdline == NULL) {
		mlog_e("[%s:%d]: hbp_addr_cmdline is null\n", __func__, __LINE__);
		return -1;
	}

	rc = kstrtoull(hbp_addr_cmdline, 0, &addr);
	if (rc)
		return rc;

	g_dfx_wpt_addr = addr;

	return 0;
}

early_param("breakpoint_addr", early_parse_wpt_addr_cmdline);

static int __init early_parse_wpt_var_cmdline(char *hbp_var_cmdline)
{
	if (hbp_var_cmdline == NULL) {
		mlog_e("[%s:%d]: hbp_var_cmdline is null\n", __func__, __LINE__);
		return -1;
	}

	(void)memset_s(g_dfx_wpt_var, WP_STR_LEN, 0x0, WP_STR_LEN);

	if (memcpy_s(g_dfx_wpt_var, WP_STR_LEN,
	    hbp_var_cmdline, WP_STR_LEN - 1) != 0) {
		mlog_e("[%s:%d]: memcpy_s err\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

early_param("breakpoint_var", early_parse_wpt_var_cmdline);

static u64 dfx_get_addr_from_cmdline(u64 phy_addr,char *strbuff)
{
	u64 ret;
	void __iomem *vir_addr = NULL;

	if (strncmp(strbuff, "undef", sizeof("undef")) == 0){
		if(phy_addr == 0)
			return 0;

		vir_addr = ioremap(phy_addr, sizeof(unsigned long));
		if (!vir_addr) {
			mlog_e("vir_addr is NULL\n");
			return 0;
		}

		ret = (u64)vir_addr;
		iounmap(vir_addr);
	}
	else{
		ret = (u64)kallsyms_lookup_name(strbuff);
		if(ret == 0)
			mlog_e("%s:%d  can't find syms\n", __func__,__LINE__);
	}

	return ret;
}

static void watchpt_set_by_addr(u64 addr)
{
	wp_set.wp_addr = addr;
	/* trigger type */
	wp_pub.wp_tg_type = TG_TYPE_DUMP;
	wp_register(&wp_set);
}

/*
 * the watchpoint module init function
 *
 * Returns 0 for success, negative for fail.
 */
static int __init dfx_watchpoint_module_init(void)
{
	u64 vir_addr = 0;
	/* NVE Switch */
	if (check_mntn_switch(MNTN_WATCHPOINT_EN) == 0) {
		/* not enabled */
		mlog_i("MNTN_WATCHPOINT_EN is Not Enabled\n");
		return 0;
	}

	/* init parameter */
	watchpt_para_init(&wp_set, &wp_pub);

	/* init debugfs interface */
	if (watchpt_debugfs_init()) {
		mlog_e("debugfs_init failed\n");
		return -ENOMEM;
	}

	vir_addr = dfx_get_addr_from_cmdline(g_dfx_wpt_addr,g_dfx_wpt_var);
	if(vir_addr != 0)
		watchpt_set_by_addr(vir_addr);

	return 0;
}

static void __exit dfx_watchpoint_module_exit(void)
{
}

module_init(dfx_watchpoint_module_init);
module_exit(dfx_watchpoint_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("dfx watchpoint");
