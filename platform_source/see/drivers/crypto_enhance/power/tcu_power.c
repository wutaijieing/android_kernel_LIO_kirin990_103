/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: baltimore power smmu tcu
 * Create: 2020/05/18
 */

#include <asm/compiler.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include "crypto_enhance.h"
#include "crypto_enhance_power.h"

/* record hieps power status. */
uint32_t g_hieps_power_status = HIEPS_POWER_STATUS_OFF;

static void hieps_set_power_status(void)
{
	g_hieps_power_status = HIEPS_POWER_STATUS_ON;
}

static void hieps_clear_power_status(void)
{
	g_hieps_power_status = HIEPS_POWER_STATUS_OFF;
}

static uint32_t hieps_get_power_status(void)
{
	return g_hieps_power_status;
}

int32_t hieps_change_voltage(uint32_t data)
{
	(void)data;

	return HIEPS_OK;
}

static int32_t hieps_power_ctrl(uint32_t cmd)
{
	int32_t ret = HIEPS_ERROR;
	uint32_t status;

	if (!g_hieps_data.smmu_tcu_supply) {
		pr_err("%s-%d: hieps smmu regulator hasnot been initialized!\n",
		       __func__, __LINE__);
		return HIEPS_ALLOC_ERR;
	}
	status = hieps_get_power_status();
	if (cmd == HIEPS_POWER_ON_CMD && status == HIEPS_POWER_STATUS_OFF)
		ret = regulator_enable(g_hieps_data.smmu_tcu_supply);
	else if (cmd == HIEPS_POWER_OFF_CMD && status == HIEPS_POWER_STATUS_ON)
		ret = regulator_disable(g_hieps_data.smmu_tcu_supply);
	else if (cmd == HIEPS_POWER_OFF_CMD && status == HIEPS_POWER_STATUS_OFF)
		ret = HIEPS_OK;
	else if (cmd == HIEPS_POWER_ON_CMD && status == HIEPS_POWER_STATUS_ON)
		ret = HIEPS_OK;
	else
		pr_err("%s-%d: hieps power status error,cmd = 0x%x, status = 0x%x\n",
		       __func__, __LINE__, cmd, status);

	if (ret != HIEPS_OK)
		pr_err("%s-%d: hieps smmu regulator fail!,cmd = %u, ret = %d\n",
		       __func__, __LINE__, cmd, ret);

	return ret;
}

/*
 * @brief      : hieps_power_cmd: power hieps tcu and reture result to ATF.
 *
 * @param[in]  : cmd : the command: power on or off hieps tcu.
 *
 * @return     : HIEPS_OK:successfully, others: failed.
 */
int32_t hieps_power_cmd(const uint32_t profile, uint32_t cmd)
{
	int32_t ret;
	uint32_t power_result;

	(void)profile;
	ret = hieps_power_ctrl(cmd);
	if (ret != HIEPS_OK) {
		pr_err("%s:power(%x) failed!\n", __func__, cmd);
		power_result = HIEPS_POWER_FAILED;
	} else {
		power_result = HIEPS_POWER_SUCCESS;
	}

	/* send result to ATF by smc. */
	ret = hieps_smc_send((u64)HIEPS_SMC_FID, (u64)cmd,
			     (u64)power_result, (u64)0); /* 0: unused param. */
	if (ret != HIEPS_OK) {
		pr_err("%s:send power result to atf failed! ret=%d\n",
		       __func__, ret);
		if (power_result == HIEPS_POWER_SUCCESS)
			goto err_free_power;
		else
			goto exit;
	}

	if (power_result == HIEPS_POWER_SUCCESS) {
		if (cmd == HIEPS_POWER_ON_CMD) {
			pr_err("hieps:tcu poweron successful!\n");
			hieps_set_power_status();
		} else {
			pr_err("hieps:tcu poweroff successful!\n");
			hieps_clear_power_status();
		}
	}

	return ret;

err_free_power:
	/* If hieps power successful, but send result failed,
	 * we need power hieps with the opposite command.
	 */
	if (cmd == HIEPS_POWER_ON_CMD) {
		if (hieps_power_ctrl(HIEPS_POWER_OFF_CMD) != HIEPS_OK)
			pr_err("%s:hieps restore power off failed\n", __func__);
	} else {
		if (hieps_power_ctrl(HIEPS_POWER_ON_CMD) != HIEPS_OK)
			pr_err("%s:hieps restore power on failed\n", __func__);
	}

exit:
	return ret;
}

/*
 * @brief      : hieps_suspend : hieps suspend process.
 *
 * @param[in]  : pdev : hieps devices.
 * @param[in]  : state : hieps state.
 *
 * @return     : HIEPS_OK:successfully, others: failed.
 */
int32_t hieps_suspend(struct platform_device *pdev, struct pm_message state)
{
	int32_t ret;

	pr_err("hieps suspend: +\n");
	if (hieps_get_power_status() == HIEPS_POWER_STATUS_ON) {
		pr_err("hieps: poweroff tcu\n");
		msleep(HIEPS_SUSPEND_DELAY_TIME);
		ret = hieps_power_ctrl(HIEPS_POWER_OFF_CMD);
		if (ret != HIEPS_OK) {
			pr_err("hieps power off failed, ret=%x\n", ret);
			/* If failed, power off again. */
			msleep(HIEPS_SUSPEND_DELAY_TIME);
			ret = hieps_power_ctrl(HIEPS_POWER_OFF_CMD);
			if (ret != HIEPS_OK) {
				pr_err("second hieps power off failed, ret=%x\n", ret);
				return HIEPS_ERROR;
			}
		}
		hieps_clear_power_status();
	}
	pr_err("hieps suspend: -\n");

	return HIEPS_OK;
}

/*
 * @brief      : hieps_resume : hieps resume process.
 *
 * @param[in]  : pdev : hieps devices.
 *
 * @return     : HIEPS_OK:successfully, others: failed.
 */
int32_t hieps_resume(struct platform_device *pdev)
{
	pr_err("hieps resume!\n");

	return HIEPS_OK;
}
