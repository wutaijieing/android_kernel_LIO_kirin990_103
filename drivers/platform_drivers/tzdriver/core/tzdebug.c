/*
 * tzdebug.c
 *
 * for tzdriver debug
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "tzdebug.h"
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <stdarg.h>
#include <linux/mm.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <securec.h>
#include <asm/io.h>
#include "tc_ns_log.h"
#include "tc_ns_client.h"
#include "tc_client_driver.h"
#include "teek_ns_client.h"
#include "smc_smp.h"
#include "teek_client_constants.h"
#include "mailbox_mempool.h"
#include "tlogger.h"
#include "cmdmonitor.h"
#include "session_manager.h"
#include "static_ion_mem.h"

#define DEBUG_OPT_LEN 128

#define TA_MEMSTAT_ALL 1

static struct dentry *g_tz_dbg_dentry;

typedef void (*tzdebug_opt_func)(const char *param);

struct opt_ops {
	char *name;
	tzdebug_opt_func func;
};

static DEFINE_MUTEX(g_meminfo_lock);
static struct tee_mem g_tee_meminfo;
static void tzmemdump(const char *param);
static int send_dump_mem(int flag, int history, const struct tee_mem *statmem)
{
	struct tc_ns_smc_cmd smc_cmd = { {0}, 0 };
	struct mb_cmd_pack *mb_pack = NULL;
	int ret = 0;

	if (!statmem) {
		tloge("statmem is NULL\n");
		return -EINVAL;
	}
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack)
		return -ENOMEM;

	smc_cmd.cmd_id = GLOBAL_CMD_ID_DUMP_MEMINFO;
	smc_cmd.cmd_type = CMD_TYPE_GLOBAL;
	mb_pack->operation.paramtypes = teec_param_types(
		TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
	mb_pack->operation.params[0].memref.buffer = virt_to_phys(statmem);
	mb_pack->operation.params[0].memref.size = sizeof(*statmem);
	mb_pack->operation.buffer_h_addr[0] =
		(uint64_t)virt_to_phys(statmem) >> ADDR_TRANS_NUM;
	mb_pack->operation.params[1].value.a = flag;
	mb_pack->operation.params[1].value.b = history;
	smc_cmd.operation_phys =
		(unsigned int)virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys =
		(uint64_t)virt_to_phys(&mb_pack->operation) >> ADDR_TRANS_NUM;

	if (tc_ns_smc(&smc_cmd)) {
		ret = -EPERM;
		tloge("send dump mem failed\n");
	}
	tz_log_write();
	mailbox_free(mb_pack);
	return ret;
}

void tee_dump_mem(void)
{
	tzmemdump(NULL);
	if (tlogger_store_msg(CONFIG_TEE_LOG_ACHIVE_PATH,
		sizeof(CONFIG_TEE_LOG_ACHIVE_PATH)) < 0)
		tloge("[cmd_monitor_tick]tlogger store lastmsg failed\n");
}

/* get meminfo (tee_mem + N * ta_mem < 4Kbyte) from tee */
static int get_tee_meminfo_cmd(void)
{
	int ret;
	struct tee_mem *mem = NULL;

	mem = mailbox_alloc(sizeof(*mem), MB_FLAG_ZERO);
	if (!mem)
		return -ENOMEM;

	ret = send_dump_mem(0, TA_MEMSTAT_ALL, mem);
	if (ret) {
		tloge("send dump failed\n");
		mailbox_free(mem);
		return ret;
	}

	mutex_lock(&g_meminfo_lock);
	ret = memcpy_s(&g_tee_meminfo, sizeof(g_tee_meminfo), mem, sizeof(*mem));
	if (ret)
		tloge("memcpy failed\n");
	mutex_unlock(&g_meminfo_lock);
	mailbox_free(mem);

	return ret;
}

static atomic_t g_cmd_send = ATOMIC_INIT(1);

void set_cmd_send_state(void)
{
	atomic_set(&g_cmd_send, 1);
}

int get_tee_meminfo(struct tee_mem *meminfo)
{
	errno_t s_ret;

	if (!meminfo)
		return -EINVAL;

	if (atomic_read(&g_cmd_send)) {
		if (get_tee_meminfo_cmd())
			return -EFAULT;
	} else {
		atomic_set(&g_cmd_send, 0);
	}

	mutex_lock(&g_meminfo_lock);
	s_ret = memcpy_s(meminfo, sizeof(*meminfo),
		&g_tee_meminfo, sizeof(g_tee_meminfo));
	mutex_unlock(&g_meminfo_lock);
	if (s_ret)
		return -1;

	return 0;
}
EXPORT_SYMBOL(get_tee_meminfo);

static void archivelog(const char *param)
{
	(void)param;
	tzdebug_archivelog();
}

static void tzdump(const char *param)
{
	(void)param;
	show_cmd_bitmap();
	wakeup_tc_siq(SIQ_DUMP_SHELL);
}

static void tzmemdump(const char *param)
{
	struct tee_mem *mem = NULL;

	(void)param;
	mem = mailbox_alloc(sizeof(*mem), MB_FLAG_ZERO);
	if (!mem) {
		tloge("mailbox alloc failed\n");
		return;
	}

	if (send_dump_mem(1, 1, mem))
		tloge("send dump mem failed\n");
	mailbox_free(mem);
}

static void tz_srv_sess_dump(const char *param)
{
	struct tc_ns_smc_cmd smc_cmd = { {0}, 0 };

	(void)param;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_DUMP_SRV_SESS;
	smc_cmd.cmd_type = CMD_TYPE_GLOBAL;
	if (tc_ns_smc(&smc_cmd))
		tloge("send dump service session failed\n");
}

static void tzmemstat(const char *param)
{
	(void)param;
	tzdebug_memstat();
}

static void tzlogwrite(const char *param)
{
	(void)param;
	(void)tz_log_write();
}

#define OFFSET_VALUE_BIT 8U
static void get_value(const char *param, uint32_t *value, uint32_t *index)
{
	uint32_t i;
	uint32_t val = 0;

	for (i = 0; i < OFFSET_VALUE_BIT; i++) {
		if (param[i] > '9' || param[i] < '0') {
			*value = val;
			*index = i;
			return;
		}
		val = (val * 10) + param[i] - '0';
	}

	*value = val;
	*index = i;
	return;
}

static void tzreserve_mem_map(const char *param)
{
	uint32_t offset;
	uint32_t i = 0;
	uint32_t index;
	uint32_t write_value;

	tloge("reserved mem map begin\n");
	if (!param) {
		tloge("param is NULL\n");
		return;
	}

	/* remain buffer has (128 - sizeof("reserve_mem") */
	if (param[i] == 'r') {
		i++;
		if (param[i++] != ':') {
			tloge("cmd not support, should begin with r:offset\n");
			return;
		}

		get_value((param + i), &offset, &index);
		secos_addr_test(true, offset, 0);
	} else if (param[i] == 'w') {
		i++;
		if (param[i++] != ':') {
			tloge("cmd not support, should begin with w:offset:value\n");
			return;
		}
		get_value((param + i), &offset, &index);
		if (param[i + index] != ':') {
			tloge("cmd not support, should begin with w:offset:value\n");
			return;
		}

		get_value((param + i + index + 1), &write_value, &index); /* add 1 for skip ":" */
		secos_addr_test(false, offset, write_value);
	} else {
		tloge("not support, should begin with r:offset or w:offset:value\n");
		return;
	}
}


#define MAX_CMD_NUM  3
#define MAX_CMD_LINE 20
#define MAX_PARAM_LINE 60
#define MAX_MEM_SIZE 0x4000U
#define MAX_VALUE_LEN 8U
#define MAX_ADDRSTR_LEN 18U
struct addr_ops {
	uint32_t pos[MAX_CMD_NUM];
	char address[MAX_CMD_LINE];
	char value[MAX_CMD_LINE];
};

static struct addr_ops g_sec_addr;

static int parse_param(const char *param, uint32_t len)
{
	uint32_t i;
	uint32_t j;

	/*
	 * param format as follows:
	 * (r)/(w):addr(Decimal):value
	 * 'w' for writing a value to a given address
	 * 'r' for reading a value form a giver address
	 */
	j = 0;
	for (i = 0; i < len; i++) {
		if (param[i] == ':') {
			if (j < MAX_CMD_NUM - 1)
				g_sec_addr.pos[j] = i;
			j++;
		}
	}
	if (j >= MAX_CMD_NUM ) {  /* Max number of ':' is 2 */
		tloge("cmd invalid\n");
		return -EINVAL;
	}

	if ((g_sec_addr.pos[1] - g_sec_addr.pos[0] - 1 >= MAX_ADDRSTR_LEN ) || (len - g_sec_addr.pos[1]  >= MAX_VALUE_LEN)) {
		tloge("invalid param\n");
		return -EINVAL;
	}
	if (param[0] != 'r' && param[0] != 'w' ) {
		tloge("cmd not support\n");
		return -EINVAL;
	}

	i = 0;
	for (j = g_sec_addr.pos[0] + 1; j <= g_sec_addr.pos[1] - 1; j++) /* skip ':'*/
		g_sec_addr.address[i++] = param[j];
	g_sec_addr.address[i] = '\0';

	i = 0;
	for (j = g_sec_addr.pos[1] + 1; j <= len; j++)
		g_sec_addr.value[i++] = param[j];
	g_sec_addr.value[i] = '\0';
	return 0;
}
void tz_secmem_test(const char *param)
{
	void __iomem *remap_addr = NULL;
	void *addr = NULL;
	uint32_t val;
	unsigned long long tmp;
	uint32_t len;

	/* if strlen is negative, len must be larger than MAX_PARAM_LINE, so it's invalied */
	len = (uint32_t)strlen(param);
	if (len >= MAX_PARAM_LINE) {
		tloge("param invalid\n");
		return;
	}

	if (parse_param(param, len) != 0) {
		tloge("parse params fail\n");
		return;
	}
	tloge("the second string : %s\n", g_sec_addr.address);

	if (kstrtoull(g_sec_addr.address + 2, 16, &tmp) < 0) { /* add 2 for skip '0x' */
		tloge("str address to unsigned long fail\n");
		return;
	}
	addr = (void *)(uintptr_t)tmp;

	if (kstrtoull(g_sec_addr.value, 10, &tmp) < 0) {
		tloge("str val to unsigned long fail\n");
		return;
	}
	val = (uint32_t)tmp;
	tloge("val = %u\n", val);

	remap_addr = ioremap((phys_addr_t)(uintptr_t)addr, MAX_MEM_SIZE);
	if (!remap_addr) {
		tloge("remap_addr failed\n");
		return;
	}
	tloge("remap_addr succ\n");
	if (param[0] == 'w') {
		tloge("read before write : %u \n", *((uint32_t *)remap_addr));
		tloge("write val : %u \n", val);
		*((uint32_t *)remap_addr) = val;
	} else {
		tloge("read addr value : %u \n", *((uint32_t *)remap_addr));
	}
	iounmap(remap_addr);
}

static struct opt_ops g_opt_arr[] = {
	{"archivelog", archivelog},
	{"dump", tzdump},
	{"memdump", tzmemdump},
	{"logwrite", tzlogwrite},
	{"dump_service", dump_services_status},
	{"memstat", tzmemstat},
	{"reserve_mem", tzreserve_mem_map},
	{"secmem_test", tz_secmem_test},
	{"srv_sess_dump", tz_srv_sess_dump},
};

static ssize_t tz_dbg_opt_read(struct file *filp, char __user *ubuf,
	size_t cnt, loff_t *ppos)
{
	char *obuf = NULL;
	char *p = NULL;
	ssize_t ret;
	uint32_t oboff = 0;
	uint32_t i;

	(void)(filp);

	obuf = kzalloc(DEBUG_OPT_LEN, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;
	p = obuf;

	for (i = 0; i < ARRAY_SIZE(g_opt_arr); i++) {
		int len = snprintf_s(p, DEBUG_OPT_LEN - oboff, DEBUG_OPT_LEN - oboff - 1,
			"%s ", g_opt_arr[i].name);
		if (len < 0) {
			kfree(obuf);
			tloge("snprintf opt name of idx %d failed\n", i);
			return -EINVAL;
		}
		p += len;
		oboff += len;
	}
	obuf[oboff - 1] = '\n';

	ret = simple_read_from_buffer(ubuf, cnt, ppos, obuf, oboff);
	kfree(obuf);

	return ret;
}

static ssize_t tz_dbg_opt_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[128] = {0};
	char *value = NULL;
	char *p = NULL;
	uint32_t i = 0;

	if (!ubuf || !filp || !ppos)
		return -EINVAL;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (!cnt)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;
	if (cnt > 0 && buf[cnt - 1] == '\n')
		buf[cnt - 1] = 0;
	value = buf;
	p = strsep(&value, ":"); /* when buf has no :, value may be NULL */
	if (!p)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(g_opt_arr); i++) {
		if (!strncmp(p, g_opt_arr[i].name,
			strlen(g_opt_arr[i].name)) &&
			strlen(p) == strlen(g_opt_arr[i].name)) {
			g_opt_arr[i].func(value);
			return cnt;
		}
	}
	return -EFAULT;
}


static const struct file_operations g_tz_dbg_opt_fops = {
	.owner = THIS_MODULE,
	.read = tz_dbg_opt_read,
	.write = tz_dbg_opt_write,
};

int tzdebug_init(void)
{
	(void)g_tz_dbg_dentry;
	(void)g_tz_dbg_opt_fops;
	return 0;
}

void tzdebug_exit(void)
{
}
