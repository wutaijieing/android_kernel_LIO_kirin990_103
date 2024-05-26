/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *  Description: Drivers for Crypto Core flash(efuse/otp) process.
 *  Create : 2020/03/01
 */

#include "crypto_core_flash.h"
#include <linux/completion.h>
#include <linux/err.h>
#include <platform_include/see/efuse_driver.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/types.h>
#include "crypto_core_errno.h"
#include "crypto_core_smc.h"
#include "crypto_core_upgrade.h"

static struct task_struct *g_mspc_flash_task;
static int32_t (*g_write_flash_func)(void);
static DECLARE_COMPLETION(g_mspc_flash_complete);

static int32_t mspc_flash_write_value(void)
{
	if (g_write_flash_func)
		return g_write_flash_func();
	else
		return MSPC_OK;
}

static int32_t mspc_flash_task(void *arg)
{
	int32_t ret;

	wait_for_completion(&g_mspc_flash_complete);
	ret = mspc_flash_write_value();

	pr_err("%s:ret %x\n", __func__, ret);
	g_mspc_flash_task = NULL;
	return ret;
}

/* resets the complete->done field to 0 ("not done") */
void mspc_reinit_flash_complete(void)
{
	reinit_completion(&g_mspc_flash_complete);
}

/* signals a thread waiting on this completion */
void mspc_release_flash_complete(void)
{
	complete(&g_mspc_flash_complete);
}

bool mspc_flash_is_task_started(void)
{
	if (!g_mspc_flash_task)
		return false;

	if (IS_ERR(g_mspc_flash_task))
		return false;

	return true;
}

void mspc_flash_register_func(int32_t (*fn_ptr) (void))
{
	if (fn_ptr)
		g_write_flash_func = fn_ptr;
}

void creat_flash_otp_thread(void)
{
	if (!g_mspc_flash_task) {
		g_mspc_flash_task = kthread_run(mspc_flash_task,
						NULL, "mspc_flash_task");
		if (IS_ERR(g_mspc_flash_task)) {
			g_mspc_flash_task = NULL;
			pr_err("%s:create flash task failed\n", __func__);
		}
	}
}

/* write otp1 image */
int32_t mspc_write_otp_image(void)
{
	int32_t ret;
	enum mspc_smc_cmd smc_cmd;

	ret = mspc_parse_image_header();
	if (ret != MSPC_OK) {
		pr_err("%s:parse mspc image header failed\n", __func__);
		return ret;
	}

	smc_cmd = MSPC_SMC_UPGRADE_OTP;
	ret = mspc_read_and_send_img(OTP1_IMG_TYPE, smc_cmd);
	if (ret != MSPC_OK) {
		pr_err("%s failed\n", __func__);
		return ret;
	}

	pr_err("%s successful\n", __func__);

	return ret;
}
