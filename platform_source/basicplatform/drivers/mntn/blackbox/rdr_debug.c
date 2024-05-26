/*
 * rdr_demo.c
 *
 * rdr unit test . (RDR: kernel run data recorder.)
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
#include <securec.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"

#define EXCEP_NOTIFY_CORE_MASK 0x0fffffff
#define EXCEP_RESET_CORE_MASK 0x0
#define EXCEP_REENTRANT_TYPE_NUM 2
#define EXCEP_MODID_SIZE 0x100
#define EXCEP_RESERVE_NUM 128
#define DFX_BB_MOD_AP_MIDDLE 0x81000000
#define DFX_BB_MOD_RESERVED_MIDDLE 0x89000000

u32 g_cleartext_test;
static struct semaphore rdr_module_sem_ap;
struct rdr_register_module_result current_info_ap;
static pfn_cb_dump_done pfn_cb_dumpdone_ap;
static u64 current_core_id_ap;
static u32 g_modid_ap;
void fn_dump_ap(u32 modid, u32 etype, u64 coreid, char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (pathname == NULL) {
		BB_PRINT_PN(" pathname is null\n");
		return;
	}
	if (current_core_id_ap != coreid) {
		BB_PRINT_PN(" coreid does not match\n");
		return;
	}
	BB_PRINT_PN(" ====================================\n");
	BB_PRINT_PN(" modid:          [0x%x]\n", modid);
	BB_PRINT_PN(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_PN(" exce tpye:      [0x%x]\n", etype);
	BB_PRINT_PN(" path name:      [%s]\n", pathname);
	BB_PRINT_PN(" dump start:     [0x%llx]\n", current_info_ap.log_addr);
	BB_PRINT_PN(" dump len:       [%u]\n", current_info_ap.log_len);
	BB_PRINT_PN(" nve:            [0x%llx]\n", current_info_ap.nve);
	BB_PRINT_PN(" callback:       [0x%pK]\n", pfn_cb);
	BB_PRINT_PN(" ====================================\n");
	pfn_cb_dumpdone_ap = pfn_cb;
	g_modid_ap = modid;
	up(&rdr_module_sem_ap);
}

void fn_reset_ap(u32 modid, u32 etype, u64 coreid)
{
	BB_PRINT_PN(" ====================================\n");
	BB_PRINT_PN(" modid:          [0x%x]\n", modid);
	BB_PRINT_PN(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_PN(" exce tpye:      [0x%x]\n", etype);
	BB_PRINT_PN(" ====================================\n");
}

void rdr_system_error_all(void)
{
	BB_PRINT_START();
	/* value of arg1 or arg2 of rdr_syserr_param_s is 0 as default */
	rdr_system_error(DFX_BB_MOD_MODEM_DRV_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_AP_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_AP_MIDDLE, 0, 0);
	rdr_system_error(DFX_BB_MOD_CP_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_TEE_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_HIFI_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_LPM_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_IOM_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_GENERAL_SEE_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_RESERVED_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_RESERVED_MIDDLE, 0, 0);
	rdr_system_error(DFX_BB_MOD_AP_MIDDLE, 0, 0);
	rdr_system_error(DFX_BB_MOD_CP_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_TEE_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_HIFI_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_LPM_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_IOM_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_GENERAL_SEE_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_RESERVED_START, 0, 0);
	rdr_system_error(DFX_BB_MOD_MODEM_DRV_START, 0, 0);
	BB_PRINT_END();
}

void register_exception_for_test(u32 modid_start, u32 modid_end)
{
	static unsigned int loop;
	struct rdr_exception_info_s einfo;
	u32 ret;

	einfo.e_modid = modid_start;
	einfo.e_modid_end = modid_end;
	einfo.e_process_priority = RDR_OTHER;
	einfo.e_reboot_priority = RDR_REBOOT_NO;
	einfo.e_notify_core_mask = EXCEP_NOTIFY_CORE_MASK;
	einfo.e_reset_core_mask = EXCEP_RESET_CORE_MASK;
	einfo.e_reentrant = loop % EXCEP_REENTRANT_TYPE_NUM;
	einfo.e_exce_type = RDR_EXCE_WD;
	einfo.e_from_core = loop % RDR_CORE_MAX;
	if (memcpy_s(einfo.e_from_module, MODULE_NAME_LEN,
	    "RDR_UT_TEST", sizeof("RDR_UT_TEST")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return;
	}

	if (memcpy_s(einfo.e_desc, STR_EXCEPTIONDESC_MAXLEN,
	    "RDR_UT_TEST for rdr_register_exception_ap_type",
	    sizeof("RDR_UT_TEST for rdr_register_exception_ap_type")) != EOK) {
		BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
		return;
	}

	einfo.e_reserve_u32 = loop * EXCEP_RESERVE_NUM;
	einfo.e_reserve_p = &einfo;
	einfo.e_callback = 0;

	ret = rdr_register_exception(&einfo);
	if (ret == 0)
		BB_PRINT_ERR("rdr_register_exception fail\n");
	loop++;
}

void register_exception_ap(u32 modid)
{
	register_exception_for_test(modid, modid + EXCEP_MODID_SIZE);
}

void register_exception_ap_all(void)
{
	BB_PRINT_START();
	register_exception_ap(DFX_BB_MOD_MODEM_DRV_START);
	register_exception_ap(DFX_BB_MOD_AP_START);
	register_exception_ap(DFX_BB_MOD_AP_MIDDLE);
	register_exception_ap(DFX_BB_MOD_CP_START);
	register_exception_ap(DFX_BB_MOD_TEE_START);
	register_exception_ap(DFX_BB_MOD_HIFI_START);
	register_exception_ap(DFX_BB_MOD_LPM_START);
	register_exception_ap(DFX_BB_MOD_IOM_START);
	register_exception_ap(DFX_BB_MOD_GENERAL_SEE_START);
	BB_PRINT_END();
}

void rdr_register_core_ap(u64 coreid)
{
	struct rdr_module_ops_pub s_module_ops;
	struct rdr_register_module_result retinfo;
	int ret;

	BB_PRINT_START();

	s_module_ops.ops_dump = fn_dump_ap;
	s_module_ops.ops_reset = fn_reset_ap;

	ret = rdr_register_module_ops(coreid, &s_module_ops, &retinfo);
	if (ret < 0)
		BB_PRINT_ERR("rdr_register_module_ops fail, ret = [%d]\n", ret);

	current_info_ap.log_addr = retinfo.log_addr;
	current_info_ap.log_len = retinfo.log_len;
	current_info_ap.nve = retinfo.nve;

	current_core_id_ap = coreid;
	BB_PRINT_END();
}

void rdr_register_core_for_test(u64 coreid)
{
	rdr_register_core_ap(coreid);
}

static int rdr_module_thread_body_ap(void *arg)
{
	BB_PRINT_START();
	while (1) {
		if (down_interruptible(&rdr_module_sem_ap))
			return -1;
		BB_PRINT_PN
		    (" ==============dump alllog start==============\n");
		BB_PRINT_PN
		    (" ... ... ... ... ... ... ... ... ... ... ... .\n");
		msleep(1000);
		BB_PRINT_PN
		    (" ==============dump alllog e n d==============\n");
		if (pfn_cb_dumpdone_ap != NULL)
			pfn_cb_dumpdone_ap(g_modid_ap, current_core_id_ap);
	}
}

void rdr_cleartext_test_enable(void)
{
	g_cleartext_test = 1;
}

u32 get_cleartext_test(void)
{
	return g_cleartext_test;
}

#ifdef CONFIG_DFX_BB_DEBUG
int rdr_debug_init(void)
{
	BB_PRINT_START();
	sema_init(&rdr_module_sem_ap, 0);
	if (!kthread_run(rdr_module_thread_body_ap, NULL, "rdr_module_thread")) {
		BB_PRINT_ERR("create thread rdr_main_thread faild\n");
		BB_PRINT_END();
		return -1;
	}

	if (rdr_demo_init()) {
		BB_PRINT_ERR("init rdr_demo faild\n");
		BB_PRINT_END();
		return -1;
	}
	BB_PRINT_END();
	return 0;
}
#endif
