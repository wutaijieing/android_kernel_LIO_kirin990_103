/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the hw_rscan_module.c for root scanner kernel space init and
 *              deinit
 * Create: 2016-06-18
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <chipset_common/security/hw_kernel_stp_interface.h>
#include "./include/hw_rscan_utils.h"
#include "./include/hw_rscan_scanner.h"

#define RSCAN_LOG_TAG "hw_rscan_module"

static int __init rscan_module_init(void);
static void __exit rscan_module_exit(void);
static void rscan_init_do_work(struct work_struct *data);
static DECLARE_WORK(rscan_init, rscan_init_do_work);

static void rscan_init_do_work(struct work_struct *data)
{
	int result;

	(void)data;
	rs_log_debug(RSCAN_LOG_TAG, "rscan work init");

	do {
		/* init dynamic scanner */
		result = rscan_dynamic_init();
		if (result != 0) {
			rs_log_error(RSCAN_LOG_TAG, "dynamic scanner init failed: %d", result);
			break;
		}

		/* register to stp */
		(void)kernel_stp_scanner_register(stp_rscan_trigger);
	} while (0);

	rs_log_trace(RSCAN_LOG_TAG, "+++root scan init end, result:%d", result);
}

static int __init rscan_module_init(void)
{
	if (queue_work(system_wq, &rscan_init) == 0)
		rs_log_error(RSCAN_LOG_TAG, "add rscan_init failed!");

	return 0;
}

static void __exit rscan_module_exit(void)
{
	return;
}

late_initcall(rscan_module_init);   /* lint -save -e528 */
module_exit(rscan_module_exit);     /* lint -save -e528 */

MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_DESCRIPTION("Huawei root scanner");
