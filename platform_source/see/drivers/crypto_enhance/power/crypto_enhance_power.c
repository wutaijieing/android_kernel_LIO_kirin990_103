/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Create: 2019/01/04
 */

#include "crypto_enhance_power.h"
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

#ifdef CONFIG_PERI_DVFS
#include "platform_include/basicplatform/linux/peri_volt_poll.h"
#endif


/* record hieps power status. */
uint32_t g_hieps_power_status = HIEPS_POWER_STATUS_OFF;

#ifdef CONFIG_PERI_DVFS
#ifndef CONFIG_HW_PERI_DVS
/* The table to translate profile id to peri voltage id. */
static uint32_t g_hieps_voltage_table[MAX_PROFILE] = {
	PERI_VOLT_3,
	PERI_VOLT_2,
#ifndef CONFIG_CRYPTO_ENHANCE_WITHOUT_CPU
	PERI_VOLT_1,
#endif
	PERI_VOLT_0 };

#else
/* The table to translate profile id to peri voltage freq. */
static uint32_t g_hieps_freq_table[MAX_PROFILE] = {
	800000000,   /* 0.8V */
	700000000,   /* 0.7V */
#ifndef CONFIG_CRYPTO_ENHANCE_WITHOUT_CPU
	650000000,   /* 0.65V */
#endif
	600000000 }; /* 0.60V */
#endif /* CONFIG_HW_PERI_DVS */
#endif /* CONFIG_PERI_DVFS */

/*
 * @brief      : hieps_set_power_status : set hieps power status.
 */
static void hieps_set_power_status(void)
{
	g_hieps_power_status = HIEPS_POWER_STATUS_ON;
}

/*
 * @brief      : hieps_clear_power_status : clear hieps power status.
 */
static void hieps_clear_power_status(void)
{
	g_hieps_power_status = HIEPS_POWER_STATUS_OFF;
}

/*
 * @brief      : hieps_get_power_status : get hieps power status.
 *
 * @return     : hieps power status.
 */
static uint32_t hieps_get_power_status(void)
{
	return g_hieps_power_status;
}

#ifdef CONFIG_PERI_DVFS
#ifndef CONFIG_HW_PERI_DVS
/*
 * voltage voting adjusts peripheral voltage
 */
static int32_t hieps_dvfs_volt_vote(const uint32_t profile, uint32_t cmd)
{
	int vote_timeout = VOTE_TIMEOUT;
	int32_t ret;
	int32_t ret1;
	uint32_t voltage;
	struct peri_volt_poll *pvp = NULL;

	if (profile >= MAX_PROFILE) {
		pr_err("hieps dvfs:Invalid profile:%x\n", profile);
		return HIEPS_PARAM_ERROR;
	}

	voltage = g_hieps_voltage_table[profile];
	pvp = peri_volt_poll_get(DEV_HIEPS_VLOTAGE_POLL_ID, NULL);
	if (!pvp) {
		pr_err("hieps:get pvp failed!\n");
		return HIEPS_ERROR;
	}

	if (cmd == HIEPS_VOLTAGE_ON) {
		ret1 = peri_set_avs(pvp, PERI_AVS_ENABLE);
		ret1 += peri_wait_avs_update(pvp);
	}

	ret = peri_set_volt(pvp, voltage);
	if (ret) {
		pr_err("hieps:set voltage failed!\n");
		return HIEPS_ERROR;
	}

	udelay(VOTE_SET_DELAY); /* delay 300us for volt set */
	while (vote_timeout) {
		if (voltage <= peri_get_volt(pvp))
			break;
		vote_timeout--;
		udelay(10); /* each loop delay 10us, total 6000 * 10us = 60ms */
	}

	if (vote_timeout == 0) {
		pr_err("hieps:wait voltage changed to target failed!\n");
		return HIEPS_ERROR;
	}

	if (cmd == HIEPS_VOLTAGE_UPDATE)
		ret1 = peri_wait_avs_update(pvp);
	else if (cmd == HIEPS_VOLTAGE_OFF)
		ret1 = peri_set_avs(pvp, PERI_AVS_DISABLE);
	else
		; /* do nothing. */

	/* AVS is an optimization for power, so it also be ok when avs operation
	 * failed. Just print an error log here.
	 */
	if (ret1)
		pr_err("hieps: avs operation failed! command=%x\n", cmd);

	return HIEPS_OK;
}

#else
/*
 * Clock voting adjusts peripheral voltage
 */
static int32_t hieps_dvfs_clk_vote(const uint32_t profile, uint32_t cmd)
{
	int32_t ret;
	uint32_t freq;

	if (profile >= MAX_PROFILE) {
		pr_err("hieps dvfs:Invalid profile:%x\n", profile);
		return HIEPS_PARAM_ERROR;
	}

	switch (cmd) {
	case HIEPS_VOLTAGE_ON:
		ret = clk_prepare_enable(g_hieps_data.hieps_clk);
		if (ret != 0) {
			pr_err("hieps dvfs: hieps_clk enable err:%d\n",ret);
			return HIEPS_ERROR;
		}
		freq = g_hieps_freq_table[profile];
		ret = clk_set_rate(g_hieps_data.hieps_clk, freq);
		if (ret != 0) {
			pr_err("hieps: hieps_clk set_rate:%x err:%d\n",
			       freq, ret);
			clk_disable_unprepare(g_hieps_data.hieps_clk);

			return HIEPS_ERROR;
		}
		break;
	case HIEPS_VOLTAGE_UPDATE:
		freq = g_hieps_freq_table[profile];
		ret = clk_set_rate(g_hieps_data.hieps_clk, freq);
		if (ret != 0) {
			pr_err("hieps dvfs: hieps_clk set_rate:%x err:%d\n",
			       freq, ret);
			return HIEPS_ERROR;
		}
		break;
	case HIEPS_VOLTAGE_OFF:
		clk_disable_unprepare(g_hieps_data.hieps_clk);
		break;
	default:
		pr_err("hieps dvfs: wrong cmd:%x\n", cmd);

		return HIEPS_PARAM_ERROR;
	}

	return HIEPS_OK;
}
#endif /* CONFIG_HW_PERI_DVS */
#endif /* CONFIG_PERI_DVFS */

/*
 * @brief      : hieps_dvfs : set hieps voltage.
 *
 * @param[in]  : profile : the voltage to vote.
 *
 * @param[in]  : cmd: vote command.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
static int32_t hieps_dvfs(const uint32_t profile, uint32_t cmd)
{
#ifdef CONFIG_PERI_DVFS

#ifndef CONFIG_HW_PERI_DVS
	return hieps_dvfs_volt_vote(profile, cmd);

#else
	return hieps_dvfs_clk_vote(profile, cmd);

#endif /* CONFIG_HW_PERI_DVS */

#else
	(void)profile;
	(void)cmd;

	return HIEPS_OK;
#endif /* CONFIG_PERI_DVFS */
}

/*
 * @brief      : hieps_power_ctrl : power hieps.
 *
 * @param[in]  : profile : the profile to power.
 * @param[in]  : cmd : the command: power on or off hieps.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_power_ctrl(uint32_t profile, uint32_t cmd)
{
	int32_t ret, ret1;

	if (g_hieps_data.hieps_regulator == NULL) {
		pr_err("%s-%d: hieps regulator hasnot been intialized!\n",
				__func__, __LINE__);
		return HIEPS_ALLOC_ERR;
	}

	if (cmd == HIEPS_POWER_ON_CMD) {
		if (hieps_get_power_status() == HIEPS_POWER_STATUS_ON) {
			pr_err("%s:hieps is already on!\n", __func__);
			return HIEPS_OK;
		}
		/* Vote on peripheral voltage. */
		ret = hieps_dvfs(profile, HIEPS_VOLTAGE_ON);
		CHECK_RESULT_GOTO(ret, exit);

		/* Open ppll2 clock. */
		ret = clk_prepare_enable(g_hieps_data.hieps_clk_source);
		CHECK_RESULT_GOTO(ret, err_down_dvfs);

		/* Power on hieps. */
		ret = regulator_enable(g_hieps_data.hieps_regulator);
		CHECK_RESULT_GOTO(ret, err_close_clock);
	} else if (cmd == HIEPS_POWER_OFF_CMD) {
		if (hieps_get_power_status() == HIEPS_POWER_STATUS_OFF) {
			pr_err("%s:hieps is already off!\n", __func__);
			return HIEPS_OK;
		}
		/* Power off hieps. */
		ret = regulator_disable(g_hieps_data.hieps_regulator);
		CHECK_RESULT_GOTO(ret, exit);

		/* Close ppll2 clock. */
		clk_disable_unprepare(g_hieps_data.hieps_clk_source);
		/* Vote off peripheral voltage. */
		ret = hieps_dvfs(PROFILE_060V, HIEPS_VOLTAGE_OFF);
		CHECK_RESULT_GOTO(ret, err_open_clock);
	} else {
		pr_err("%s-%d:Invalid power cmd:%x\n", __func__, __LINE__, cmd);
		return HIEPS_PARAM_ERROR;
	}

	goto exit;

err_close_clock:
	clk_disable_unprepare(g_hieps_data.hieps_clk_source);

err_down_dvfs:
	ret1 = hieps_dvfs(PROFILE_060V, HIEPS_VOLTAGE_OFF);
	CHECK_RESULT_PRINT(ret1);
	return ret;

err_open_clock:
	ret1 = clk_prepare_enable(g_hieps_data.hieps_clk_source);
	CHECK_RESULT_PRINT(ret1);
	ret1 = regulator_enable(g_hieps_data.hieps_regulator);
	CHECK_RESULT_PRINT(ret1);

exit:
	return ret;
}

/*
 * @brief      : hieps_power_cmd: power hieps and reture result to ATF.
 *
 * @param[in]  : profile : the profile to power.
 * @param[in]  : cmd : the command: power on or off hieps.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_power_cmd(const uint32_t profile, uint32_t cmd)
{
	int32_t ret, ret1;
	uint32_t power_result;

	ret = hieps_power_ctrl(profile, cmd);
	if (ret != HIEPS_OK) {
		pr_err("%s:power(%x) failed!\n", __func__, cmd);
		power_result = HIEPS_POWER_FAILED;
	} else {
		power_result = HIEPS_POWER_SUCCESS;
	}

	/* Send result to ATF by smc. */
	ret = hieps_smc_send((u64)HIEPS_SMC_FID, (u64)cmd,
			     (u64)power_result, (u64)0); /* 0: unused param. */
	if (ret != HIEPS_OK) {
		pr_err("%s:send power result to atf failed! ret=%x\n",
				__func__, ret);
		if (power_result == HIEPS_POWER_SUCCESS)
			goto err_free_power;
		else
			goto exit;
	}

	if (power_result == HIEPS_POWER_SUCCESS) {
		if (cmd == HIEPS_POWER_ON_CMD) {
			pr_err("hieps:poweron successful!\n");
			hieps_set_power_status();
		} else {
			pr_err("hieps:poweroff successful!\n");
			hieps_clear_power_status();
		}
		goto exit;
	}

err_free_power:
	/* If hieps power successful, but send result failed,
	 * we need power hieps with the opposite command.
	 */
	if (cmd == HIEPS_POWER_ON_CMD)
		cmd = HIEPS_POWER_OFF_CMD;
	else
		cmd = HIEPS_POWER_ON_CMD;

	ret1 = hieps_power_ctrl(profile, cmd);
	if (ret1 != HIEPS_OK)
		pr_err("hieps restore power failed, ret=%x\n", ret1);

exit:
	return ret;
}


/*
 * @brief      : hieps_change_voltage : change hieps voltage.
 *
 * @param[in]  : profile : the profile to change.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_change_voltage(const uint32_t profile)
{
	int32_t ret;
	uint32_t power_result;

	/* Vote on peripheral voltage. */
	ret = hieps_dvfs(profile, HIEPS_VOLTAGE_UPDATE);
	if (ret != HIEPS_OK) {
		pr_err("%s-%d:hieps dvfs failed! ret=%d\n",
				__func__, __LINE__, ret);
		power_result = HIEPS_POWER_FAILED;
	} else {
		pr_err("hieps:dvfs successful!\n");
		power_result = HIEPS_POWER_SUCCESS;
	}

	/* Send result to ATF by smc. */
	ret = hieps_smc_send((u64)HIEPS_SMC_FID,
			(u64)HIEPS_DVFS_CMD, (u64)power_result, (u64)0);
	if (ret != HIEPS_OK)
		pr_err("%s-%d:send power result to atf failed! ret=%x\n",
		       __func__, __LINE__, ret);

	return ret;
}

/*
 * @brief      : hieps_suspend : hieps suspend process.
 *
 * @param[in]  : pdev : hieps devices.
 * @param[in]  : state : hieps state.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_suspend(struct platform_device *pdev, struct pm_message state)
{
	int32_t ret;

	pr_err("hieps suspend: +\n");
	if (hieps_get_power_status() == HIEPS_POWER_STATUS_ON) {
		pr_err("hieps: poweroff hieps\n");
		msleep(HIEPS_SUSPEND_DELAY_TIME);
		ret = hieps_power_ctrl(PROFILE_080V, HIEPS_POWER_OFF_CMD);
		if (ret != HIEPS_OK) {
			pr_err("hieps power off failed, ret=%x\n", ret);
			/* If failed, power off again. */
			ret = hieps_power_ctrl(PROFILE_080V, HIEPS_POWER_OFF_CMD);
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
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_resume(struct platform_device *pdev)
{
	pr_err("hieps resume!\n");

	return HIEPS_OK;
}
