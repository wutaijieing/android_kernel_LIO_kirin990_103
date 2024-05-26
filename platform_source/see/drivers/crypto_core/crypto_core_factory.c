/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Drivers for Crypto Core chip test.
 * Create: 2019/11/17
 */

#include "crypto_core_factory.h"
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <general_see_mntn.h>
#include <vendor_rpmb.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/printk.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_errno.h"
#include "crypto_core_flash.h"
#include "crypto_core_fs.h"
#include "crypto_core_power.h"
#include "crypto_core_smc.h"
#include "crypto_core_upgrade.h"

#define MSPC_WAIT_COS_READY_TIME         10000 /* 10s */
#define MSPC_WAIT_COS_DOWN_TIME          10000 /* 10s */
#define MSPC_STATE_FLASH_READY           MSPC_STATE_NATIVE_READY

#define MSPC_POLL_WAIT_TIME               100    /* 100ms */
#define MSPC_ENTER_FACTORY_MODE_WAIT_TIME 20000  /* 20s */

static struct mutex g_mspc_otp_mutex;

/* flag to indicate running status of flash otp1 */
enum otp1_status {
	NO_NEED = 0,
	PREPARED,
	RUNING,
	FINISH,
};

/* flag to indicate running status of flash otp1 */
static uint32_t g_mspc_flash_otp1_status;

static atomic_t g_mspc_secdebug_disable;

/* check otp1 write work is running */
/* flash_otp_task may not being created by set/get efuse _securitydebug_value */
bool mspc_chiptest_otp1_is_running(void)
{
	pr_info("mspc otp1 work status %x\n", g_mspc_flash_otp1_status);
	if (g_mspc_flash_otp1_status == RUNING) {
		return true;
	}

	if (g_mspc_flash_otp1_status == PREPARED &&
	    mspc_flash_is_task_started()) {
		return true;
	}

	return false;
}

void mspc_chiptest_set_otp1_status(enum otp1_status status)
{
	g_mspc_flash_otp1_status = status;
	pr_err("mspc set otp1 state:%x\n", g_mspc_flash_otp1_status);
}

void mspc_set_secdebug_status(bool is_debug_disable)
{
	atomic_set(&g_mspc_secdebug_disable, (int)is_debug_disable);
}

bool mspc_get_secdebug_status(void)
{
	return (bool)atomic_read(&g_mspc_secdebug_disable);
}

int32_t mspc_send_smc(uint32_t cmd_type)
{
	int32_t ret;
	int8_t *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct mspc_atf_msg_header *p_msg_header = NULL;
	uint32_t img_size;
	struct device *device = mspc_get_device();

	if (!device) {
		pr_err("%s:Get mspc device failed!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	buff_virt = (int8_t *)dma_alloc_coherent(device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s dma_alloc_coherent failed\n", __func__);
		return MSPC_NO_RESOURCES_ERROR;
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_msg_header = (struct mspc_atf_msg_header *)buff_virt;
	mspc_set_atf_msg_header(p_msg_header, cmd_type);
	p_msg_header->cos_id = MSPC_FLASH_COS_ID;
	p_msg_header->result_addr = (uint32_t)buff_phy +
				    MSPC_ATF_MESSAGE_HEADER_LEN;
	p_msg_header->result_size = sizeof(uint32_t);
	img_size = MSPC_ATF_MESSAGE_HEADER_LEN + sizeof(uint32_t);
	ret = mspc_send_smc_process(p_msg_header, buff_phy, img_size,
	       MSPC_ATF_SMC_TIMEOUT, cmd_type);
	pr_err("%s result %x\n", __func__,
	       *(uint32_t *)(buff_virt + MSPC_ATF_MESSAGE_HEADER_LEN));
	dma_free_coherent(device, SIZE_4K, buff_virt, buff_phy);
	pr_err("%s exit\n", __func__);
	return ret;
}

static int32_t mspc_manufacture_check_lcs(void)
{
	int32_t ret;
	uint32_t mspc_lcs_mode = 0;

	ret = mspc_get_lcs_mode(&mspc_lcs_mode);
	if (ret != MSPC_OK) {
		pr_err("%s:get mspc lcs failed!\n", __func__);
		return ret;
	}

	if (mspc_lcs_mode != MSPC_SM_MODE_MAGIC) {
		pr_err("%s: mspc lcs isnot sm!\n", __func__);
		return MSPC_LCS_MODE_ERROR;
	}

	return MSPC_OK;
}

/* function to process for flash cos when securedebug disabled */
static int32_t mspc_flash_debug_flash_cos_fn(void)
{
	int32_t ret;

	ret = mspc_write_otp_image();
	if (ret != MSPC_OK) {
		pr_err("%s:write otp1 image failed\n", __func__);
		return ret;
	}
	/* delete flashcos image */
	ret = mspc_remove_flash_cos();
	if (ret != MSPC_OK) {
		pr_err("%s:remove flash cos failed\n", __func__);
		return ret;
	}

	return MSPC_OK;
}

/* function to process for flash cos when pinstall */
static int32_t mspc_pinstall_flash_cos_fn(void)
{
	int32_t ret;
	bool is_debug_disable = false;

	/* prepare secflash initial key for set urs */
	ret = mspc_send_smc(MSPC_SMC_SECFLASH_INITKEY);
	if (ret != MSPC_OK) {
		pr_err("%s:prepare secflash initkey failed\n", __func__);
		return ret;
	}

	is_debug_disable = mspc_get_secdebug_status();
	/* If secdebug is DebugDisable, write otp1 */
	if (is_debug_disable) {
		ret = mspc_write_otp_image();
		if (ret != MSPC_OK) {
			pr_err("%s:write otp1 image failed %x\n", __func__, ret);
			return ret;
		}
		pr_err("%s: flash debug disable\n", __func__);
	}
	return MSPC_OK;
}

/* power on flash cos, run fn, then power off */
int32_t mspc_run_flash_cos(int32_t (*fn)(void))
{
	int32_t ret, ret1;
	uint32_t time = (uint32_t)MSPC_WAIT_COS_READY_TIME;
	const int8_t *param = MSPC_FLASH_COS_POWER_PARAM;

	/* Power on flashCOS, then process fn */
	ret = mspc_power_func(param, strlen(param),
			      MSPC_POWER_ON_BOOTING, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s:power on flash cos failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = mspc_wait_state(MSPC_STATE_FLASH_READY, time);
	if (ret != MSPC_OK)
		pr_err("%s:wait flash cos ready timeout!\n", __func__);
	else
		ret = (fn) ? fn() : MSPC_OK;

	if (ret != MSPC_OK)
		hisee_mntn_debug_dump();

	/* power off flashCOS */
	ret1 = mspc_power_func(param, strlen(param),
			       MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret1 != MSPC_OK) {
		pr_err("%s:power off flash cos failed!\n", __func__);
		goto exit;
	}

	/* Wait for powering down msp core. */
	time = (uint32_t)MSPC_WAIT_COS_DOWN_TIME;
	ret1 = mspc_wait_state(MSPC_STATE_POWER_DOWN, time);
	if (ret1 != MSPC_OK)
		pr_err("%s:wait flash cos down tiemout!\n", __func__);
exit:
	return (ret == MSPC_OK) ? ret1 : ret;
}

/* set secflash to urs mode */
static int32_t mspc_set_secflash_urs(void)
{
	int32_t ret, ret1;
	uint32_t time = (uint32_t)MSPC_WAIT_COS_READY_TIME;
	const int8_t *param = MSPC_NATIVE_COS_POWER_PARAM;

	/* Power on nativeCOS */
	ret = mspc_power_func(param, strlen(param),
			      MSPC_POWER_ON_BOOTING_SECFLASH, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s:power on native cos failed! ret=%x\n", __func__, ret);
		return ret;
	}

	/* Wait for secflash ready */
	ret = mspc_wait_state(MSPC_STATE_SECFLASH, time);
	if (ret != MSPC_OK)
		pr_err("%s:wait secflash ready timeout\n", __func__);
	else
		ret = mspc_send_smc(MSPC_SMC_SECFLASH_SETURS);
	pr_err("%s:ret %x\n", __func__, ret);

	if (ret != MSPC_OK)
		hisee_mntn_debug_dump();

	/* power off nativeCOS */
	ret1 = mspc_power_func(param, strlen(param),
			       MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret1 != MSPC_OK) {
		pr_err("%s:power off native cos failed!\n", __func__);
		goto exit;
	}

	/* Wait for powering down msp core. */
	time = (uint32_t)MSPC_WAIT_COS_DOWN_TIME;
	ret1 = mspc_wait_state(MSPC_STATE_POWER_DOWN, time);
	if (ret1 != MSPC_OK)
		pr_err("%s:wait native cos down tiemout!\n", __func__);
exit:
	return (ret == MSPC_OK) ? ret1 : ret;
}

int32_t mspc_enter_factory_mode(void)
{
	int32_t wait_time = MSPC_ENTER_FACTORY_MODE_WAIT_TIME;
	int32_t vote_atf, vote_tee;
	int32_t ret;
	uint32_t msg[MSPC_SMC_PARAM_SIZE];

	ret = mspc_force_power_off();
	if (ret != MSPC_OK) {
		pr_err("%s power off mspc failed\n", __func__);
		return ret;
	}

	msg[MSPC_SMC_PARAM_0] = 0;
	msg[MSPC_SMC_PARAM_1] = (uint32_t)MSPC_NATIVE_COS_ID;
	while (wait_time > 0) {
		ret = mspc_send_smc_no_ack((uint64_t)MSPC_FN_MAIN_SERVICE_CMD,
					   (uint64_t)MSPC_SMC_ENTER_FACTORY_MODE,
					   (const char *)msg, sizeof(msg));
		if (ret == MSPC_OK)
			return ret;
		msleep(MSPC_POLL_WAIT_TIME);
		wait_time -= MSPC_POLL_WAIT_TIME;
	}
	pr_err("%s err %d\n", __func__, ret);

	vote_atf = mspc_send_smc_no_ack((uint64_t)HISEE_MNTN_ID,
					(uint64_t)HISEE_SMC_GET_ATF_VOTE, NULL, 0);
	vote_tee = mspc_send_smc_no_ack((uint64_t)HISEE_MNTN_ID,
					(uint64_t)HISEE_SMC_GET_TEE_VOTE, NULL, 0);
	pr_err("vote vote atf[0x%x] vote TEE[0x%x]\n", vote_atf, vote_tee);

	return MSPC_ENTER_FACTROY_MODE_ERROR;
}

int32_t mspc_exit_factory_mode(void)
{
	int32_t ret;
	uint32_t msg[MSPC_SMC_PARAM_SIZE] = {0};

	msg[MSPC_SMC_PARAM_0] = 0;
	msg[MSPC_SMC_PARAM_1] = (uint32_t)MSPC_NATIVE_COS_ID;
	ret = mspc_send_smc_no_ack((uint64_t)MSPC_FN_MAIN_SERVICE_CMD,
				   (uint64_t)MSPC_SMC_EXIT_FACTORY_MODE,
				   (const char *)msg, sizeof(msg));
	if (ret != MSPC_OK) {
		pr_err("%s err %d\n", __func__, ret);
		return MSPC_EXIT_FACTROY_MODE_ERROR;
	}

	return MSPC_OK;
}

/* do some check before pinstall test */
static int32_t mspc_pinstall_pre_check(void)
{
	int32_t ret;
	bool is_new_cos = false;

	ret = mspc_manufacture_check_lcs();
	if (ret != MSPC_OK)
		return ret;

	/* check rpmb key, should be ready */
	if (get_rpmb_key_status() != KEY_READY) {
		pr_err("%s rpmb key not ready\n", __func__);
		return MSPC_RPMB_KEY_UNREADY_ERROR;
	}

	ret = mspc_enter_factory_mode();
	if (ret != MSPC_OK) {
		pr_err("%s enter factory mode fail\n", __func__);
		return ret;
	}

	/* switch storage media to rpmb */
	ret = mspc_send_smc(MSPC_SMC_SWITCH_RPMB);
	if (ret != MSPC_OK) {
		pr_err("%s switch to rpmb fail\n", __func__);
		return ret;
	}

	/* check hisee_fs is erased */
	ret = mspc_check_hisee_fs_empty();
	if (ret != MSPC_OK) {
		pr_err("%s hisee_fs check fail\n", __func__);
		return ret;
	}

	/* check whether new image is upgraded */
	ret = mspc_check_new_image(&is_new_cos);
	if (ret != MSPC_OK || is_new_cos) {
		pr_err("%s:check boot upgrade fail\n", __func__);
		return MSPC_BOOT_UPGRADE_CHK_ERROR;
	}

	return MSPC_OK;
}

/* check whether secdebug disabled by efuse */
static int32_t mspc_check_secdebug_disable(void)
{
	int32_t ret;
	bool is_debug_disable = false;

	ret = efuse_check_secdebug_disable(&is_debug_disable);
	if (ret != 0) {
		pr_err("%s:check secdebug failed\n", __func__);
		return MSPC_CHECK_SECDEBUG_ERROR;
	}

	mspc_set_secdebug_status(is_debug_disable);
	return MSPC_OK;
}

/* things to do when secdebug is disabled */
static int32_t mspc_pinstall_secdebug_disabled(void)
{
	int32_t ret;
	bool is_debug_disable;

	is_debug_disable = mspc_get_secdebug_status();
	/* nothing to do if secdebug not disabled */
	if (!is_debug_disable)
		return MSPC_OK;

	/* set secflash to urs mode */
	ret = mspc_set_secflash_urs();
	if (ret != MSPC_OK) {
		pr_err("%s:set secflash urs err %d\n", __func__, ret);
		return ret;
	}

	/* delete flashcos image */
	ret = mspc_remove_flash_cos();
	if (ret != MSPC_OK) {
		pr_err("%s:remove flash cos err %d\n", __func__, ret);
		return ret;
	}

	return MSPC_OK;
}

/* function run when AT^MSPC=PINSTALL */
int32_t mspc_total_manufacture_func(void)
{
	int32_t ret;
	uint32_t state = 0;

	mspc_reinit_flash_complete();
	ret = mspc_pinstall_pre_check();
	if (ret != MSPC_OK) {
		pr_err("%s:pre_check failed\n", __func__);
		goto exit;
	}

	ret = mspc_check_flash_cos_file(&state);
	if (ret != MSPC_OK || state == FLASH_COS_EMPTY) {
		pr_err("%s:check flash cos failed\n", __func__);
		ret = MSPC_FLASH_COS_ERROR;
		goto exit;
	}

	/* Test already, exit directly. */
	if (state == FLASH_COS_ERASE) {
		pr_err("%s:flash cos is erased\n", __func__);
		goto exit;
	}

	/* check secdebug before run flashcos */
	ret = mspc_check_secdebug_disable();
	if (ret != MSPC_OK) {
		pr_err("%s:check secdebug failed\n", __func__);
		goto exit;
	}

	ret = mspc_run_flash_cos(mspc_pinstall_flash_cos_fn);
	if (ret != MSPC_OK) {
		pr_err("%s:run flash cos failed\n", __func__);
		goto exit;
	}

	ret = mspc_pinstall_secdebug_disabled();
exit:
	if (ret == MSPC_OK) {
		mspc_chiptest_set_otp1_status(PREPARED);
		/* sync signal for flash_otp_task */
		mspc_release_flash_complete();
	}

	mspc_record_errno(ret);
	return ret;
}

#ifdef CONFIG_DFX_DEBUG_FS
#define MSPC_WAIT_NATIVE_READY_TIME       30000 /* 30s */
#define MSPC_WAIT_NATIVE_DOWN_TIME        50 /* 50ms */
/* Indicate the factory test status */
static enum mspc_factory_test_state g_factory_test_state = MSPC_FACTORY_TEST_NORUNNING;

int32_t mspc_total_slt_func(void)
{
	int32_t ret, ret1;
	const int8_t *param = MSPC_NATIVE_COS_POWER_PARAM;
	uint32_t time = (uint32_t)MSPC_WAIT_NATIVE_DOWN_TIME;

	/* Power down msp core to make sure msp core is off status. */
	ret = mspc_power_func(param, strlen(param),
			      MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret != MSPC_OK) {
		pr_err("%s:power off native cos failed!\n", __func__);
		goto exit;
	}

	/* Wait 50ms at most for powering down msp core. */
	ret = mspc_wait_state(MSPC_STATE_POWER_DOWN, time);
	if (ret != MSPC_OK) {
		pr_err("%s:wait native cos down tiemout!\n", __func__);
		goto exit;
	}

	ret = mspc_upgrade_image(MSPC_UPGRADE_CHIP_TEST);
	if (ret != MSPC_OK) {
		pr_err("%s:upgrade native cos failed!\n", __func__);
		goto exit;
	}

	/* Power on msp core to check whether upgrading is successful. */
	ret = mspc_power_func(param, strlen(param),
			      MSPC_POWER_ON_BOOTING, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s:power on native cos failed!\n", __func__);
		goto exit;
	}

	/* Wait 30s at most for native ready. */
	time = (uint32_t)MSPC_WAIT_NATIVE_READY_TIME;
	ret = mspc_wait_state(MSPC_STATE_FLASH_READY, time);
	if (ret != MSPC_OK)
		pr_err("%s:wait native cos ready tiemout!\n", __func__);

	/*
	 * Regardless of whether waiting native ready is successful or failed,
	 * we have to power off msp core.
	 */
	ret1 = mspc_power_func(param, strlen(param),
			       MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret1 != MSPC_OK) {
		pr_err("%s: power off msp core failed!\n", __func__);
		ret = ret1;
	}

exit:
	mspc_record_errno(ret);
	return ret;
}

#define MSPC_CHIPTEST_RT_DELAY	  2000 /* 2s */
#define MSPC_CHIPTEST_RT_COUNT	  450

static bool g_mspc_stop_rt_task = false;

/* Begin of rt code. */
/* run nativeCOS in rt process */
static int32_t mspc_chiptest_rt_process(void *arg)
{
	int32_t ret, ret1;
	int32_t loop = MSPC_CHIPTEST_RT_COUNT;
	const int8_t *param = MSPC_NATIVE_COS_POWER_PARAM;
	uint32_t time = (uint32_t)MSPC_WAIT_NATIVE_READY_TIME;

	while (!kthread_should_stop()) {
		ret = mspc_power_func(param, strlen(param),
				      MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
		if (ret != MSPC_OK) {
			pr_err("%s:power off native cos failed!\n", __func__);
			goto err_process;
		}

		ret = mspc_power_func(param, strlen(param),
				      MSPC_POWER_ON_BOOTING, MSPC_POWER_CMD_ON);
		if (ret != MSPC_OK) {
			pr_err("%s:power on native cos failed!\n", __func__);
			goto err_process;
		}

		/* Wait 30s at most for native ready. */
		ret = mspc_wait_state(MSPC_STATE_FLASH_READY, time);
		if (ret != MSPC_OK) {
			pr_err("%s:wait native cos ready tiemout!\n", __func__);
			break; /* quit loop, then power off msp core. */
		}

		msleep(MSPC_CHIPTEST_RT_DELAY);

		loop--;
		if (g_mspc_stop_rt_task || loop <= 0)
			break;
	};

	ret1 = mspc_power_func(param, strlen(param),
			       MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	ret = (ret == MSPC_OK) ? ret1 : ret;
	if (ret == MSPC_OK) {
		g_factory_test_state = MSPC_FACTORY_TEST_SUCCESS;
		pr_err("%s: loop %d successful!\n", __func__,
		       MSPC_CHIPTEST_RT_COUNT - loop);
		return MSPC_OK;
	}

err_process:
#ifdef CONFIG_HUAWEI_DSM
	if (hisee_mntn_record_dmd_info(DSM_HISEE_POWER_ON_OFF_ERROR_NO,
				      "mspc chiptest rt process fail") != 0)
		pr_err("%s:dmd info failed!\n", __func__);
#endif
	g_factory_test_state = MSPC_FACTORY_TEST_FAIL;
	pr_err("%s:loop %d failed, ret=0x%x\n", __func__,
	       MSPC_CHIPTEST_RT_COUNT - loop, ret);
	return ret;
}

int32_t mspc_chiptest_rt_run_func(const int8_t *buf, int32_t len)
{
	int32_t ret;
	uint32_t status_result = SECFLASH_IS_ABSENCE_MAGIC;
	struct task_struct *rt_test_task = NULL;

	ret = mspc_secflash_is_available(&status_result);
	if (ret == MSPC_OK && status_result == SECFLASH_IS_ABSENCE_MAGIC) {
		pr_info("%s: secflash is absence, return OK directly!\n", __func__);
		return MSPC_OK;
	}

	ret = MSPC_OK;
	if (g_factory_test_state != MSPC_FACTORY_TEST_RUNNING) {
		g_factory_test_state = MSPC_FACTORY_TEST_RUNNING;
		g_mspc_stop_rt_task = false;
		rt_test_task = kthread_run(mspc_chiptest_rt_process,
					   NULL, "mspc_rt_task");
		if (!rt_test_task) {
			ret = MSPC_THREAD_CREATE_ERROR;
			g_factory_test_state = MSPC_FACTORY_TEST_FAIL;
			pr_err("%s(): creat rt task fail!\n", __func__);
		}
	}
#ifdef CONFIG_HUAWEI_DSM
	/* dmd info as mntn, if error, only print error info, not return */
	if (ret != MSPC_OK) {
		if (hisee_mntn_record_dmd_info(DSM_HISEE_POWER_ON_OFF_ERROR_NO,
					      "mspc chiptest rt run fail"))
			pr_err("%s(): dmd info failed!\n", __func__);
	}
#endif
	return ret;
}

int32_t mspc_chiptest_rt_stop_func(const int8_t *buf, int32_t len)
{
	g_mspc_stop_rt_task = true;

	return MSPC_OK;
}
/* End of rt code. */
#endif /* CONFIG_DFX_DEBUG_FS */

/* function run when send AT^SECUREDEBUG=, to rm debug switch in mspc */
int32_t mspc_flash_debug_switchs(void)
{
	int32_t ret;
	uint32_t state = 0;

	mutex_lock(&g_mspc_otp_mutex);
	mspc_chiptest_set_otp1_status(RUNING);

	ret = mspc_check_flash_cos_file(&state);
	if (ret != MSPC_OK || state == FLASH_COS_EMPTY) {
		pr_err("%s:check flash cos failed!\n", __func__);
		ret = MSPC_FLASH_COS_ERROR;
		goto exit;
	}

	/* Test already, exit directly. */
	if (state == FLASH_COS_ERASE) {
		pr_err("%s:flash cos is erased!\n", __func__);
		goto exit;
	}

	/* set secflash urs before erase flashcos */
	ret = mspc_set_secflash_urs();
	if (ret != MSPC_OK) {
		pr_err("%s:set secflash urs err, ret %d\n", __func__, ret);
		goto exit;
	}

	ret = mspc_run_flash_cos(mspc_flash_debug_flash_cos_fn);
	pr_err("%s: run flash cos ret %d\n", __func__, ret);

exit:
	mspc_chiptest_set_otp1_status(FINISH);
	mutex_unlock(&g_mspc_otp_mutex);

	mspc_record_errno(ret);
	return ret;
}

int32_t mspc_check_boot_upgrade(void)
{
	int32_t ret;
	bool is_new_cos = false;

	ret = mspc_manufacture_check_lcs();
	if (ret != MSPC_OK)
		return ret;

	/* check whether new image is upgraded */
	ret = mspc_check_new_image(&is_new_cos);
	if (ret != MSPC_OK) {
		pr_err("%s:check boot upgrade result failed!\n", __func__);
		return ret;
	}
	if (is_new_cos) {
		pr_err("%s:fail to upgrade to new img!\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
		if (hisee_mntn_record_dmd_info(DSM_HISEE_POWER_ON_OFF_ERROR_NO,
					      "boot upgrade fail"))
			pr_err("%s:dmd info failed!\n", __func__);
#endif
		return MSPC_BOOT_UPGRADE_CHK_ERROR;
	}

	return MSPC_OK;
}

void mspc_factory_init(void)
{
	mutex_init(&g_mspc_otp_mutex);
}
