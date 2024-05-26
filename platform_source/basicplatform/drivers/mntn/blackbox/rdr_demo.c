/*
 * rdr_demo.c
 *
 * rdr demo for module. (RDR: kernel run data recorder.)
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <securec.h>

#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"

#define EXCEP_NOTIFY_CORE_MASK 0xffffffff
#define EXCEP_RESET_CORE_MASK 0x0
#define RDR_WD_ERROR_ADDR 0x83000001
#define RDR_RESET_ERROR_ADDR 0x83000011
#define RDR_ORDINARY_ERROR_ADDR 0x83000020
#define RDR_ORDINARY_ERROR_ADDR_END 0x83000100

static struct semaphore rdr_soc_sem;
struct rdr_register_module_result current_info;
static pfn_cb_dump_done pfn_cb_dumpdone;
static u64 current_core_id = RDR_TEEOS;
static u32 g_modid;

void rdr_syserr_test(u32 modid, u32 arg1, u32 arg2)
{
	BB_PRINT_START();
	rdr_system_error(modid, arg1, arg2);
	BB_PRINT_END();
}

void fn_dump(u32 modid, u32 etype, u64 coreid,
				char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (pathname == NULL) {
		BB_PRINT_ERR(" pathname is null\n");
		return;
	}
	BB_PRINT_PN(" ====================================\n");
	BB_PRINT_PN(" modid:          [0x%x]\n", modid);
	BB_PRINT_PN(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_PN(" exce tpye:      [0x%x]\n", etype);
	BB_PRINT_PN(" path name:      [%s]\n", pathname);
	BB_PRINT_PN(" dump start:     [0x%llx]\n", current_info.log_addr);
	BB_PRINT_PN(" dump len:       [%u]\n", current_info.log_len);
	BB_PRINT_PN(" nve:            [0x%llx]\n", current_info.nve);
	BB_PRINT_PN(" callback:       [0x%pK]\n", pfn_cb);
	BB_PRINT_PN(" ====================================\n");
	pfn_cb_dumpdone = pfn_cb;
	g_modid = modid;
	up(&rdr_soc_sem);
}

void fn_reset(u32 modid, u32 etype, u64 coreid)
{
	BB_PRINT_PN(" ====================================\n");
	BB_PRINT_PN(" modid:          [0x%x]\n", modid);
	BB_PRINT_PN(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_PN(" exce tpye:      [0x%x]\n", etype);
	BB_PRINT_PN(" ====================================\n");
	/* reset error */
	rdr_system_error(RDR_RESET_ERROR_ADDR, 0, 0);
}

int rdr_register_core(void)
{
	struct rdr_module_ops_pub s_module_ops;
	int ret;

	BB_PRINT_START();

	s_module_ops.ops_dump = fn_dump;
	s_module_ops.ops_reset = fn_reset;

	ret = rdr_register_module_ops(current_core_id, &s_module_ops,
				    &current_info);

	BB_PRINT_END();
	return ret;
}

int register_exception(void)
{
	struct rdr_exception_info_s einfo;
	unsigned int ret;

	(void)memset_s(&einfo, sizeof(einfo), 0, sizeof(einfo));
	einfo.e_modid = RDR_WD_ERROR_ADDR;
	einfo.e_modid_end = RDR_WD_ERROR_ADDR;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_WAIT;
	einfo.e_notify_core_mask = (u64)(RDR_TEEOS | RDR_AP);
	einfo.e_reset_core_mask = RDR_TEEOS;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = RDR_EXCE_WD;
	einfo.e_from_core = RDR_TEEOS;
	if (memcpy_s(einfo.e_from_module, MODULE_NAME_LEN - 1, "RDR_DEMO_TEST", sizeof("RDR_DEMO_TEST")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	if (memcpy_s(einfo.e_desc, STR_EXCEPTIONDESC_MAXLEN - 1, "RDR_DEMO_TEST for teeos wd issue.",
	    sizeof("RDR_DEMO_TEST for teeos wd issue.")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	ret = rdr_register_exception(&einfo);
	if (!ret)
		BB_PRINT_ERR("[%s:%d]: regist wd issue exception err\n]", __func__, __LINE__);
	(void)memset_s(&einfo, sizeof(einfo), 0, sizeof(einfo));
	einfo.e_modid = RDR_RESET_ERROR_ADDR;
	einfo.e_modid_end = RDR_RESET_ERROR_ADDR;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = (u64)(RDR_TEEOS | RDR_AP);
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = RDR_EXCE_WD;
	einfo.e_from_core = RDR_TEEOS;
	if (memcpy_s(einfo.e_from_module, MODULE_NAME_LEN - 1, "RDR_DEMO_TEST", sizeof("RDR_DEMO_TEST")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	if (memcpy_s(einfo.e_desc, STR_EXCEPTIONDESC_MAXLEN - 1, "RDR_DEMO_TEST for teeos reset failed issue.",
	    sizeof("RDR_DEMO_TEST for teeos reset failed issue.")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	ret = rdr_register_exception(&einfo);
	if (!ret)
		BB_PRINT_ERR("[%s:%d]: regist reset failed issue exception err\n]", __func__, __LINE__);
	(void)memset_s(&einfo, sizeof(einfo), 0, sizeof(einfo));
	einfo.e_modid = RDR_ORDINARY_ERROR_ADDR;
	einfo.e_modid_end = RDR_ORDINARY_ERROR_ADDR_END;
	einfo.e_process_priority = RDR_OTHER;
	einfo.e_reboot_priority = RDR_REBOOT_WAIT;
	einfo.e_notify_core_mask = EXCEP_NOTIFY_CORE_MASK;
	einfo.e_reset_core_mask = EXCEP_RESET_CORE_MASK;
	einfo.e_reentrant = (u32)RDR_REENTRANT_ALLOW;
	einfo.e_exce_type = RDR_EXCE_INITIATIVE;
	einfo.e_from_core = RDR_TEEOS;
	if (memcpy_s(einfo.e_from_module, MODULE_NAME_LEN - 1, "RDR_DEMO_TEST", sizeof("RDR_DEMO_TEST")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	if (memcpy_s(einfo.e_desc, STR_EXCEPTIONDESC_MAXLEN - 1, "RDR_DEMO_TEST for teeos ordinary issue.",
	    sizeof("RDR_DEMO_TEST for teeos ordinary issue.")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return 0;
	}
	ret = rdr_register_exception(&einfo);
	if (!ret)
		BB_PRINT_ERR("[%s:%d]: regist ordinary issue exception err\n]", __func__, __LINE__);
	return (int)ret;
}

static int rdr_soc_thread_body(void *arg)
{
	BB_PRINT_START();
	while (1) {
		if (down_interruptible(&rdr_soc_sem))
			return -1;
		BB_PRINT_PN(" ==============dump alllog start==============\n");
		BB_PRINT_PN(" ... ... ... ... ... ... ... ... ... ... ... .\n");
		msleep(1000);
		BB_PRINT_PN(" ==============dump alllog e n d==============\n");
		if (pfn_cb_dumpdone != NULL)
			pfn_cb_dumpdone(g_modid, current_core_id);
	}
}

int demo_init_early(void)
{
	int ret;
	/* register module */
	ret = rdr_register_core();
	if (ret != 0)
		return ret;
	/* regitster exception */
	ret = register_exception();
	return ret;
}

int rdr_demo_init(void)
{
	int ret;

	BB_PRINT_START();

	ret = demo_init_early();
	if (ret <= 0) {
		BB_PRINT_ERR("init demo faild\n");
		BB_PRINT_END();
		return -1;
	}

	sema_init(&rdr_soc_sem, 0);
	if (!kthread_run(rdr_soc_thread_body, NULL, "rdr_soc_thread")) {
		BB_PRINT_ERR("create thread rdr_main_thread faild\n");
		BB_PRINT_END();
		return -1;
	}
	BB_PRINT_END();
	return 0;
}
