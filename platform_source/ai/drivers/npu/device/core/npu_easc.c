/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-03-22
 */
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>
#include <asm/io.h>

#include "npu_easc.h"
#include "npu_common.h"
#include "npu_log.h"
#include "npu_platform.h"
#include "npu_atf_subsys.h"

static struct npu_easc_exception_info g_npu_easc_excp_info;

static void npu_easc_exception_work(struct work_struct *work __attribute__((__unused__)))
{
	npu_drv_err("npu easc throw out error, %d times in total!\n",
		g_npu_easc_excp_info.excp_total_count.counter);
	return;
}

static void npu_easc_excp_irq_clr()
{
	int ret;

	ret = atfd_service_npu_smc(NPU_EXCEPTION_PROC, NPU_EXCP_EASC, 0, 0);
	if (ret != 0)
		npu_drv_err("easc excp irq clear failed, ret:%d", ret);

	return;
}

irqreturn_t npu_easc_excp_irq_handler(int irq, void *data)
{
	(void)data;
	(void)irq;

	npu_easc_excp_irq_clr();

	if ((g_npu_easc_excp_info.easc_excp_wq != NULL) &&
			(atomic_inc_return(&g_npu_easc_excp_info.excp_total_count) < NPU_EASC_EXCP_COUNT_MAX))
		queue_work(g_npu_easc_excp_info.easc_excp_wq, &g_npu_easc_excp_info.easc_excp_work);

	return IRQ_HANDLED;
}

int npu_easc_init()
{
	u32 easc_irq = 0;
	int ret      = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return -EINVAL;
	}
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_EASC] == 0)
		return 0;

	atomic_set(&g_npu_easc_excp_info.excp_total_count, 0);

	easc_irq = (u32)plat_info->dts_info.irq_easc;
	ret = request_irq(easc_irq, npu_easc_excp_irq_handler, IRQF_TRIGGER_NONE,
		"npu_easc_excp_irq_handler", NULL);
	if (ret != 0) {
		npu_drv_err("request easc irq failed\n");
		return ret;
	}

	g_npu_easc_excp_info.easc_excp_wq = create_workqueue("npu-drv-easc");
	if (g_npu_easc_excp_info.easc_excp_wq == NULL) {
		npu_drv_err("create easc workqueue error\n");
		free_irq(easc_irq, NULL);
		return -ENOMEM;
	}
	INIT_WORK(&g_npu_easc_excp_info.easc_excp_work, npu_easc_exception_work);
	return ret;
}

int npu_easc_deinit()
{
	u32 easc_irq = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return -EINVAL;
	}
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_EASC] == 0)
		return 0;

	destroy_workqueue(g_npu_easc_excp_info.easc_excp_wq);
	easc_irq = (u32)plat_info->dts_info.irq_easc;
	free_irq(easc_irq, NULL);

	return 0;
}
