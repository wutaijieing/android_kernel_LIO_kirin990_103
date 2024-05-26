/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: hieps Receive atf ipi interrupt,  cma shared memory command
 *              process Handle hieps power tasks, send result to atf
 * Create: 2018-12-01
 */

#include "crypto_enhance.h"
#include <asm/compiler.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <securec.h>
#ifdef CONFIG_CRYPTO_ENHANCE_POWERON_WITH_SMMU
#include "tcu_power.h"
#else
#include "crypto_enhance_power.h"
#endif
#ifdef CONFIG_CRYPTO_ENHANCE_SMX_ATCMD
#include "crypto_engine_at.h"
#endif

/* Hieps devices data. */
struct hieps_module_data g_hieps_data;

/*
 * @brief      : hieps_smc_send
 * @param[in]  : smcfid, args to atf
 */
noinline int hieps_smc_send(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct arm_smccc_res res = {0};
	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
		      0, 0, 0, 0, &res);
	return (int)res.a0;
#else
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			"smc    #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
#endif
}

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * @brief      : hieps_atf_kernel_test  atf-->kernel test
 * @param[in]  : NA
 * @return     : 0：success，!0：fail
 */
static int32_t hieps_atf_kernel_test(void)
{
	int32_t ret;

	ret = hieps_smc_send((u64)HIEPS_SMC_FID, (u64)HIEPS_ATF_KERNEL_TEST_CMD,
		(u64)HIEPS_ATF_KERNEL_TEST_SUCCESS, (u64)0);
	if (ret != HIEPS_OK)
		pr_err("%s-%d:Send smc failed! ret = %x\n",
			__func__, __LINE__, ret);
	else
		pr_err("%s-%d:Send smc successful!\n",
			__func__, __LINE__);

	return ret;
}

/*
 * @brief      : hieps_loop_test  ca-->kernel-->ca test
 * @param[in]  : NA
 * @return     : 0：success，!0：fail
 */
static int32_t hieps_loop_test(uint32_t data)
{
	int32_t ret;
	uint32_t return_data = ~data;

	/* Send result to ATF by smc. */
	ret = hieps_smc_send((u64)HIEPS_SMC_FID,
		(u64)HIEPS_LOOP_TEST_CMD, (u64)return_data, (u64)0);
	if (ret != HIEPS_OK) {
		pr_err("%s-%d:send loop test result to atf failed! ret=%x\n",
			__func__, __LINE__, ret);
	} else {
		pr_err("%s-%d:send loop test result to atf successful!\n",
			__func__, __LINE__);
	}

	return ret;
}
#endif

/*
 * @brief      : hieps_cmd_process  cmd process
 * @param[in]  : cma_info struct
 * @return     : 0：success，!0：fail
 */
static int32_t hieps_cmd_process(struct hieps_cma_info *cma_data)
{
	int32_t ret;
	uint32_t cmd = (uint32_t)cma_data->x1;
	uint32_t data = (uint32_t)cma_data->x2;

	switch (cmd) {
	case HIEPS_POWER_ON_CMD:
		/* Fall-through */
	case HIEPS_POWER_OFF_CMD:
		ret = hieps_power_cmd(data, cmd);
		break;
	case HIEPS_DVFS_CMD:
		ret = hieps_change_voltage(data);
		break;
#ifdef CONFIG_DFX_DEBUG_FS
	case HIEPS_ATF_KERNEL_TEST_CMD:
		ret = hieps_atf_kernel_test();
		break;
	case HIEPS_LOOP_TEST_CMD:
		ret = hieps_loop_test(data);
		break;
#endif
	default:
		pr_err("%s-%d:Invalid cmd:%x\n", __func__, __LINE__, cmd);
		ret = HIEPS_CMD_ERROR;
		break;
	}

	return ret;
}

/*
 * @brief      : permanent thread wait for ipi interrupt
 * @param[in]  : arg: param for kernel thread
 * @return     : 0：success，!0：fail
 */
static int hieps_permanent_thread(void *arg)
{
	int ret;
	struct hieps_cma_info *kernel_atf_info = NULL;

	g_hieps_data.hieps_buff_virt = (void *)dma_alloc_coherent(g_hieps_data.cma_device,
		(u64)SIZE_CMA, &(g_hieps_data.hieps_buff_phy), GFP_KERNEL);
	if (g_hieps_data.hieps_buff_virt == NULL) {
		pr_err("%s(): dma_alloc_coherent failed!\n", __func__);
		ret = HIEPS_CMA_MALLOC_ERROR;
		return ret;
	}
	pr_info("%s():dma_alloc_coherent seccess!\n", __func__);

	kernel_atf_info = (struct hieps_cma_info *)g_hieps_data.hieps_buff_virt;
	(void)memset_s(g_hieps_data.hieps_buff_virt, (u64)SIZE_CMA, 0x00, (u64)SIZE_CMA);

	/* send cma phy addr to atf by smc */
	ret = hieps_smc_send((u64)HIEPS_SMC_FID, (u64)HIEPS_SYNC_CMA_ALLOC_CMD,
		(u64)g_hieps_data.hieps_buff_phy, (u64)SIZE_CMA);
	if (ret != HIEPS_OK) {
		pr_err("%s():kernel send addr to atf error!\n", __func__);
		dma_free_coherent(g_hieps_data.cma_device, (u64)SIZE_CMA,
				g_hieps_data.hieps_buff_virt, g_hieps_data.hieps_buff_phy);
		return ret;
	}
	pr_info("%s():kernel send addr to atf success!\n", __func__);

	while (!kthread_should_stop()) {
		ret = down_timeout(&(g_hieps_data.atf_sem), MAX_SCHEDULE_TIMEOUT);
		if (-ETIME == ret) {
			ret = HIEPS_TIMEOUT_ERROR;
		} else {
			ret = hieps_cmd_process(kernel_atf_info);
			if (ret != HIEPS_OK)
				pr_err("%s(): cmd process failed, cmd = %x, ret = %x.\n",
					__func__, (uint32_t)kernel_atf_info->x1, ret);
		}
	}

	if (g_hieps_data.hieps_buff_virt != NULL) {
		dma_free_coherent(g_hieps_data.cma_device, (u64)SIZE_CMA,
				g_hieps_data.hieps_buff_virt, g_hieps_data.hieps_buff_phy);
		g_hieps_data.hieps_buff_virt = NULL;
	}
	return ret;
}

/*
 * @brief      : hieps_active  IPI interrupt
 * @note       : receive IPI interrupt from atf then release semaphore.
 */
void crypto_enhance_active(void)
{
	up(&(g_hieps_data.atf_sem));
}
EXPORT_SYMBOL(crypto_enhance_active);

#ifdef CONFIG_CRYPTO_ENHANCE_SMX_ATCMD
static DEVICE_ATTR(mspe_at_ctrl, (S_IRUSR | S_IWUSR),
		   mspe_at_ctrl_show, mspe_at_ctrl_store);

static int hieps_remove(struct platform_device *pdev)
{
	int ret = HIEPS_OK;

	if (g_hieps_data.cma_device)
		device_remove_file(g_hieps_data.cma_device,
				   &dev_attr_mspe_at_ctrl);

	return ret;
}
#endif

static int hieps_probe_second_stage(struct platform_device *pdev)
{
	struct task_struct *hieps_test_task = NULL;
	int ret;

	/* Initialize hieps semaphore. */
	sema_init(&(g_hieps_data.atf_sem), 0);

	/* Create hieps task. */
	hieps_test_task = kthread_run(hieps_permanent_thread,
		NULL, "hieps_permanent_thread");
	if (hieps_test_task == NULL) {
		ret = HIEPS_KCREATE_ERROR;
		pr_err("%s-%d:Create hieps_test_task failed!\n",
			__func__, __LINE__);
		return ret;
	}

#ifdef CONFIG_CRYPTO_ENHANCE_SMX_ATCMD
	if (device_create_file(g_hieps_data.dev, &dev_attr_mspe_at_ctrl)) {
		ret = HIEPS_IOCTL_NODE_CREATE_ERROR;
		pr_err("hieps err unable to create mspe_at_ctrl attributes\n");
		return ret;
	}
#endif

	return HIEPS_OK;
}

/*
 * @brief      : hieps_probe : intialized hieps module.
 *
 * @param[in]  : pdev : hieps devices.
 *
 * @return     : HIEPS_OK: successful, others: failed.
 */
static int __init hieps_probe(struct platform_device *pdev)
{
	struct device *pdevice = &(pdev->dev);
	int ret = HIEPS_ERROR;

	/* Clear g_hieps_data. */
	(void)memset_s(&g_hieps_data, sizeof(g_hieps_data), 0, sizeof(g_hieps_data));

	g_hieps_data.dev = &(pdev->dev);

	/* Get hieps regulator which is used in power management of hieps. */
	g_hieps_data.hieps_regulator = devm_regulator_get(g_hieps_data.dev, HIEPS_REGULATOR);
	if (IS_ERR(g_hieps_data.hieps_regulator)) {
		pr_err("%s-%d:Couldn't get regulator hieps!\n",
			__func__, __LINE__);
		ret = HIEPS_HIFACE_GET_ERROR;
		goto error;
	} else {
		pr_info("%s Get regulator hiface successful!", __func__);
	}

#ifdef CONFIG_CRYPTO_ENHANCE_POWERON_WITH_SMMU
	/* Get smmu tcu regulator */
	g_hieps_data.smmu_tcu_supply = devm_regulator_get(g_hieps_data.dev, "ip-smmu-tcu");
	if (IS_ERR(g_hieps_data.smmu_tcu_supply)) {
		pr_err("%s-%d:hieps Couldn't get smmu tcu regulator!\n",
				__func__, __LINE__);
		ret = HIEPS_HIFACE_GET_ERROR;
		goto error;
	} else {
		pr_info("%s Get regulator smmu tcu successful!", __func__);
	}
#endif

	/* Get hieps clock sources. */
	g_hieps_data.hieps_clk_source = devm_clk_get(g_hieps_data.dev, "hieps_ppll");
	if (IS_ERR(g_hieps_data.hieps_clk_source)) {
		pr_err("%s-%d:Couldn't get hieps clock source!\n",
			__func__, __LINE__);
		ret = HIEPS_HIFACE_GET_ERROR;
		goto error;
	}

	g_hieps_data.hieps_clk = devm_clk_get(g_hieps_data.dev, "hieps_clk");
	if (IS_ERR(g_hieps_data.hieps_clk)) {
		pr_err("%s-%d:Get hieps clock failed!\n",
			__func__, __LINE__);
		ret = HIEPS_HIFACE_GET_ERROR;
		goto error;
	} else {
		pr_info("%s-%d:Get hieps clock successful!\n",
			__func__, __LINE__);
	}

	g_hieps_data.cma_device = pdevice;
	/* Initialize */
	ret = of_reserved_mem_device_init(pdevice);
	if (ret != HIEPS_OK) {
		pr_err("%s-%d:hieps shared cma pool with ATF registered failed!\n",
			__func__, __LINE__);
		goto error;
	} else {
		pr_info("hieps: shared cma pool with ATF registered success!\n");
	}

	ret = hieps_probe_second_stage(pdev);
	if (ret != HIEPS_OK)
		goto error;

	ret = HIEPS_OK;
	pr_err("HiEPS: init OK!\n");
	goto exit;

error:
	pr_err("HiEPS: init failed!\n");
exit:
	return ret;
}

static const struct of_device_id hieps_of_match[] = {
	{.compatible = "hisilicon,hieps-device"},
	{ }
};
MODULE_DEVICE_TABLE(of, hieps_of_match);

static struct platform_driver hieps_driver = {
	.probe = hieps_probe,
#ifdef CONFIG_CRYPTO_ENHANCE_SMX_ATCMD
	.remove = hieps_remove,
#endif
	.suspend = hieps_suspend,
	.driver = {
		.name  = HIEPS_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hieps_of_match),
	},
};

static int __init hieps_module_init(void)
{
	int ret;

	ret = platform_driver_register(&hieps_driver);
	if (ret)
		pr_err("register hieps driver failed.\n");

	return ret;
}

static void __exit hieps_module_exit(void)
{
	platform_driver_unregister(&hieps_driver);
}

module_init(hieps_module_init);
module_exit(hieps_module_exit);

MODULE_DESCRIPTION("HIEPS KERNEL DRIVER");
MODULE_LICENSE("GPL V2");
