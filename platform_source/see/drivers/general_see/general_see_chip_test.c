/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: implement hisee chip factory test function
 * Create: 2020-02-17
 */
#include "general_see_chip_test.h"
#include <asm/compiler.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fcntl.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/platform_drivers/rpmb.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#ifdef CONFIG_HISILICON_PLATFORM_HITEST
#include <platform_include/basicplatform/linux/hitest_slt.h>
#endif
#include <securec.h>
#ifdef CONFIG_GENERAL_SEE_MNTN
#include "general_see_mntn.h"
#endif
#include "platform_include/see/flash_general_see_otp.h"
#include "general_see.h"
#include "general_see_fs.h"
#include "general_see_power.h"
#include "general_see_upgrade.h"
#include "soc_acpu_baseaddr_interface.h"
#include "soc_sctrl_interface.h"

#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
#define set_cos_flash_buf_para() \
do {\
	cos_flash_buf_para[0] = HISEE_CHAR_SPACE;\
	cos_flash_buf_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;\
	cos_flash_buf_para[HISEE_COS_ID_POS] = '0' + COS_FLASH_IMG_ID;\
} while (0)
#endif

#define set_cos_default_buf_para() \
do {\
	cos_default_buf_para[0] = HISEE_CHAR_SPACE;\
	cos_default_buf_para[2] = '0' + COS_PROCESS_UPGRADE;\
	cos_default_buf_para[1] = '0' + COS_IMG_ID_0;\
} while (0)

/* check ret is ok or otherwise goto err_process */
#define check_error_then_goto(ret) \
do {\
	if ((ret) != HISEE_OK)\
		goto err_process;\
} while (0)

static enum hisee_at_type g_at_cmd_type = HISEE_AT_MAX;

/* flag to indicate running status of flash otp1 */
static enum e_run_status g_hisee_flash_otp1_status;

/*
 * @brief      : get hisee at cmd type from a global variable
 */
enum hisee_at_type hisee_get_at_type(void)
{
	return g_at_cmd_type;
}

/*
 * @brief      : set hisee at cmd type to a global variable
 */
void hisee_set_at_type(enum hisee_at_type type)
{
	g_at_cmd_type = type;
}

/*
 * @brief      : set the otp1 write work status
 */
void hisee_chiptest_set_otp1_status(enum e_run_status status)
{
	g_hisee_flash_otp1_status = status;
	pr_err("hisee set otp1 status %x\n", g_hisee_flash_otp1_status);
}

enum e_run_status hisee_chiptest_get_otp1_status(void)
{
	return g_hisee_flash_otp1_status;
}

/*
 * @brief      : check otp1 write work is running.flash_otp_task may not being
 *               created by set/get efuse _securitydebug_value.
 * @return     : true on running, otherwiase false.
 */
bool hisee_chiptest_otp1_is_running(void)
{
	pr_info("hisee otp1 work status %x\n", g_hisee_flash_otp1_status);
	if (g_hisee_flash_otp1_status == RUNNING)
		return true;

	if (g_hisee_flash_otp1_status == PREPARED &&
	    flash_otp_task_is_started() == true)
		return true;

	return false;
}

#ifndef CONFIG_GENERAL_SEE_DISABLE_KEY
static int otp_image_upgrade_func(const void *buf, int para)
{
	int ret;
	unsigned int cos_id = COS_IMG_ID_0;
	unsigned int process_id = 0;

	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	if (ret != HISEE_OK) {
		pr_err("%s() hisee_get_cosid failed ret=%d\n", __func__, ret);
		return set_errno_then_exit(ret);
	}
	if (cos_id != COS_IMG_ID_0 && cos_id != COS_IMG_ID_1) {
		pr_err("hisee:%s() cosid=%u not support otp image upgrade now, bypass!\n",
		       __func__, cos_id);
		return ret;
	}

	ret = hisee_poweron_booting_func(buf, 0);
	if (ret == HISEE_OK) {
		ret = write_hisee_otp_value(OTP_IMG_TYPE);
		(void)hisee_poweroff_func(buf, (int)HISEE_PWROFF_LOCK);
	}
	check_and_print_result();

	return set_errno_then_exit(ret);
}

static int hisee_write_rpmb_key(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_WRITE_RPMB_KEY);
	ret = send_smc_process(p_message_header,
			       buff_phy, HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_WRITE_RPMBKEY_TIMEOUT,
			       CMD_WRITE_RPMB_KEY);
	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int set_hisee_lcs_sm_otp(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	unsigned int result_offset;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_SET_LCS_SM);

	result_offset = HISEE_ATF_MESSAGE_HEADER_LEN;
	p_message_header->test_result_phy =
		(unsigned int)buff_phy + result_offset;
	p_message_header->test_result_size = SIZE_4K - result_offset;
	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_GENERAL_TIMEOUT, CMD_SET_LCS_SM);
	if (ret != HISEE_OK)
		pr_err("%s(): hisee reported fail code=%d\n",
		       __func__, *((int *)(void *)(buff_virt + result_offset)));

	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int upgrade_one_file_func(const char *filename, enum se_smc_cmd cmd)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	size_t image_size = 0;
	unsigned int result_offset;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	/* alloc coherent buff with vir&phy addr (64K for upgrade file) */
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device,
					       HISEE_SHARE_BUFF_SIZE,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, HISEE_SHARE_BUFF_SIZE,
		       0, HISEE_SHARE_BUFF_SIZE);

	/* read given file to buff */
	ret = filesys_read_img_from_file(
				filename,
				buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN,
				&image_size, HISEE_MAX_IMG_SIZE);
	if (ret != HISEE_OK) {
		pr_err("%s(): hisee_read_file failed, filename=%s, ret=%d\n",
		       __func__, filename, ret);
		dma_free_coherent(hisee_data_ptr->cma_device, HISEE_SHARE_BUFF_SIZE,
				  buff_virt, buff_phy);
		return set_errno_then_exit(ret);
	}
	image_size = image_size + HISEE_ATF_MESSAGE_HEADER_LEN;

	/* init and config the message */
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, cmd);
	/* reserve 256B for test result(err code from hisee) */
	result_offset = (u32)(image_size + SMC_TEST_RESULT_SIZE - 1) &
			(~(SMC_TEST_RESULT_SIZE - 1));
	if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
		p_message_header->test_result_phy =
			(unsigned int)buff_phy + result_offset;
		p_message_header->test_result_size =
			HISEE_SHARE_BUFF_SIZE - result_offset;
	} else {
		/*
		 * this case, test_result_phy will be 0 and
		 * atf will not write test result; and kernel side will record
		 * err code a meaningless val (but legal)
		 */
		result_offset = 0;
	}

	/* smc call (synchronously) */
	ret = send_smc_process(p_message_header, buff_phy, image_size,
			       HISEE_ATF_GENERAL_TIMEOUT, cmd);
	if (ret != HISEE_OK) {
		if (cmd != CMD_FACTORY_APDU_TEST) /* apdu test will not report err code */
			pr_err("%s(): hisee reported err code=%d\n",
			       __func__, *((int *)(void *)(buff_virt + result_offset)));
	}

	dma_free_coherent(hisee_data_ptr->cma_device, HISEE_SHARE_BUFF_SIZE,
			  buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int hisee_apdu_test_func(void)
{
	int ret;

	ret = upgrade_one_file_func("/mnt/hisee_fs/test.apdu.bin",
				    CMD_FACTORY_APDU_TEST);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int hisee_verify_isd_key(enum hisee_cos_imgid_type cos_id)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret = HISEE_OK;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (cos_id != COS_IMG_ID_0) {
		/* can do more actions in future if necessary */
		pr_err("hisee:%s() cosid=%d not support verify_isd_key now, bypass!\n",
		       __func__, cos_id);
		return ret;
	}

	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	if (memset_s(buff_virt, SIZE_4K, 0, SIZE_4K) != EOK) {
		pr_err("hisee:%s() memset_s error!", __func__);
		dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
				  buff_virt, buff_phy);
		return HISEE_ERROR;
	}

	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_HISEE_VERIFY_KEY);
	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_GENERAL_TIMEOUT, CMD_HISEE_VERIFY_KEY);
	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	if (ret == HISEE_OK)
		pr_err("hisee:%s() run success!", __func__);
	else
		pr_err("hisee:%s() run failed!", __func__);
	return ret;
}
#endif

/*
 * @brief      : is_hisee_chiptest_slt.
 * @return     : ::bool, true on running, false on not running
 */
bool is_hisee_chiptest_slt(void)
{
#ifdef CONFIG_HISILICON_PLATFORM_HITEST
	/*
	 * if CONFIG_HISILICON_PLATFORM_HITEST = y, hitest_slt will be compiled.
	 * The interface is_running_kernel_slt() can be used.
	 */
	if (is_running_kernel_slt())
		return true;
	else
		return false;
#else
	return false;
#endif
}

#ifdef CONFIG_GENERAL_SEE_SUPPORT_CASDKEY
/*
 * @brief      : bypass_write_casd_key
 * @return     : ::int, 0 on success, other value on failure
 */
static int bypass_write_casd_key(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	/* alloc coherent buff with vir&phy addr */
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);

	/* init and config the message */
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_HISEE_WRITE_CASD_KEY);

	/* smc call (synchronously) */
	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_GENERAL_TIMEOUT,
			       CMD_HISEE_WRITE_CASD_KEY);
	if (ret != HISEE_OK)
		pr_err("%s() fail\n", __func__);

	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

/*
 * @brief      : hisee_write_casd_key
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_write_casd_key(void)
{
	int ret;

	if (!get_support_casd())
		return HISEE_OK;

	/* if is slt, bypass write casd in misc upgrade process */
	if (is_hisee_chiptest_slt())
		ret = bypass_write_casd_key();
	else if (get_bypass_casd())
	/* in default, do not write casd key */
		ret = bypass_write_casd_key();
	else
		ret = upgrade_one_file_func(HISEE_CASD_IMG_FULLNAME,
					    CMD_HISEE_WRITE_CASD_KEY);
	check_and_print_result();
	return set_errno_then_exit(ret);
}
#endif

static int g_hisee_flag_protect_lcs = HISEE_FALSE;
int hisee_debug(void)
{
#ifdef CONFIG_DFX_DEBUG_FS
	g_hisee_flag_protect_lcs = HISEE_TRUE;
#endif
	return g_hisee_flag_protect_lcs;
}

#ifdef CONFIG_GENERAL_SEE_SUPPORT_ULOADE
/*
 * @brief      : do the uloader boot process
 * @return     : ::int, 0 on success, other value on failure
 */
int hisee_uloader_image_upgrade(void)
{
	int ret;
	enum se_smc_cmd smc_cmd;

	/* Do the uloader image upgrade. */
	smc_cmd = CMD_UPGRADE_ULOADER;
	ret = hisee_upgrade_image_read(0, ULOADER_IMG_TYPE, smc_cmd);

	check_and_print_result();
	return ret;
}
#endif

#ifndef CONFIG_GENERAL_SEE_DISABLE_KEY
static int hisee_write_rpmb_key_process(const void *buf)
{
	int ret = HISEE_ERROR;
	int ret_pm;
	int write_rpmbkey_retry = 5; /* local retry count */

	while (write_rpmbkey_retry--) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_ULOADER
		ret_pm = hisee_uloader_image_upgrade();
		check_error_then_goto(ret_pm);
		hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);
#endif
		ret = hisee_write_rpmb_key();
		if (ret == HISEE_OK)
			break;

		ret_pm = hisee_poweroff_func(buf, HISEE_PWROFF_LOCK);
		check_error_then_goto(ret_pm);
		hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);
		ret_pm = hisee_poweron_upgrade_func(buf, 0);
		check_error_then_goto(ret_pm);
		hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);
	}

err_process:
	check_and_print_result();
	return ret;
}

static int hisee_apdu_test_process(enum hisee_cos_imgid_type cos_id)
{
	int ret;

	if (cos_id != COS_IMG_ID_0 && cos_id != COS_IMG_ID_1) {
		ret = wait_hisee_ready(HISEE_STATE_COS_READY,
				       DELAY_FOR_HISEE_POWERON_BOOTING);
		if (ret != HISEE_OK)
			pr_err("hisee:%s(): wait_hisee_ready failed,retcode=%d\n",
			       __func__, ret);

		return ret;
	}
	ret = hisee_apdu_test_func();
	check_error_then_goto(ret);

	/* send command to delete test applet */
	ret = send_apdu_cmd(HISEE_DEL_TEST_APPLET);
	check_error_then_goto(ret);

err_process:
	check_and_print_result();
	return ret;
}

static int hisee_manufacture_set_lcs_sm(unsigned int hisee_lcs_mode)
{
	int ret = HISEE_OK;

	if (hisee_lcs_mode == HISEE_DM_MODE_MAGIC &&
	    g_hisee_flag_protect_lcs == HISEE_FALSE) {
		ret = set_hisee_lcs_sm_otp();
		check_error_then_goto(ret);

		ret = set_hisee_lcs_sm_efuse();
		check_error_then_goto(ret);
	}

err_process:
	return ret;
}

/*
 * @brief      : poweron booting hisee in misc upgrade mode and do write casd
 *               key and misc image upgrade in order.
 *               go through: hisee misc ready -> write casd key to nvm ->
 *               misc upgrade to nvm -> hisee cos ready
 * @param[in]  : buf, content string buffer.
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_poweron_booting_misc_process(const void *buf)
{
	/* 3 characters: space,cos_id=0,process_id=COS_PROCESS_CHIP_TEST */
	char cos_default_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	unsigned int cos_id = 0;
	unsigned int process_id = 0;
	int ret;
#ifdef CONFIG_GENERAL_SEE_MISCIMG_PATCH
	enum hisee_img_file_type img_type = MISC3_IMG_TYPE;
#endif

	set_cos_default_buf_para();
	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	check_error_then_goto(ret);

	cos_default_buf_para[HISEE_COS_ID_POS] = '0' + cos_id;
	if (cos_id != COS_IMG_ID_0 && cos_id != COS_IMG_ID_1) {
		ret = hisee_poweron_booting_func((void *)cos_default_buf_para,
						 HISEE_POWER_ON_BOOTING);
		pr_err("%s() cosid=%u not support misc booting now, bypass!\n",
		       __func__, cos_id);
		check_error_then_goto(ret);
		/* wait hisee cos ready for later process */
		ret = wait_hisee_ready(HISEE_STATE_COS_READY,
				       HISEE_ATF_GENERAL_TIMEOUT);
		return ret; /* exit directly to bypass! */
	}

	/* poweron booting hisee and set the flag for the process */
	ret = hisee_poweron_booting_func((void *)cos_default_buf_para,
					 HISEE_POWER_ON_BOOTING_MISC);
	check_error_then_goto(ret);

	/* wait hisee ready for receiving images */
	ret = wait_hisee_ready(HISEE_STATE_MISC_READY, HISEE_ATF_GENERAL_TIMEOUT);
	check_error_then_goto(ret);

#ifdef CONFIG_GENERAL_SEE_SUPPORT_CASDKEY
	/*
	 * write casd key should be combined with misc image upgrade
	 * and be in right order
	 */
	ret = hisee_write_casd_key();
	check_error_then_goto(ret);
#endif

#ifdef CONFIG_GENERAL_SEE_MISCIMG_PATCH
	/* cos patch upgrade only supported in this function */
	ret = hisee_cos_patch_read(img_type +
				   (HISEE_MAX_MISC_IMAGE_NUMBER * cos_id));
	check_error_then_goto(ret);
#endif

	/* misc image upgrade only supported in this function */
	ret = misc_image_upgrade_func(cos_id);
	check_error_then_goto(ret);

	/* wait hisee cos ready for later process */
	ret = wait_hisee_ready(HISEE_STATE_COS_READY, HISEE_ATF_GENERAL_TIMEOUT);
	check_error_then_goto(ret);

	/* write current misc version into record area */
	ret = hisee_update_misc_version(cos_id);
	check_error_then_goto(ret);

err_process:
	check_and_print_result();
	return ret;
}
#endif

/*
 * @brief      : execute hisee nvm format operation
 * @return     : 0 on success, other value on failure.
 */
#ifdef CONFIG_GENERAL_SEE_MISCIMG_SECUPGRADE
int run_hisee_nvmformat(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}

	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_FORMAT_RPMB);

	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_NVM_FORMAT_TIMEOUT, CMD_FORMAT_RPMB);
	if (ret != HISEE_OK)
		pr_err("%s(): hisee reported fail code=%d\n", __func__, ret);

	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}
#endif

#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
/*
 * @brief      : execute cos flash image upgrade, poweron hisee -> upgrade cos
 *               flash image -> poweroff hisee.
 * @param[in]  : buf, command string buffer
 * @param[in]  : factory_test_flg, indicates factory manufacture or adb shell.
 *               Not used now.
 * @return     : 0 on success, other value on failure.
 */
static int cos_flash_image_upgrade_func(const void *buf,
					unsigned int factory_test_flg)
{
	char cos_flash_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	int ret, ret1;

	ret = hisee_poweroff_func(buf, 0);
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() poweroff failed. retcode=%d\n",
		       __func__, ret);
		ret = HISEE_MULTICOS_POWEROFF_ERROR;
		goto err_process;
	}
	set_cos_flash_buf_para();

	ret = hisee_poweron_upgrade_func(cos_flash_buf_para, 0);
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() power failed, ret = %d\n", __func__, ret);
		ret = HISEE_MULTICOS_POWERON_UPGRADE_ERROR;
		goto err_process;
	}

	hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);
	ret1 = cos_upgrade_image_read(COS_FLASH_IMG_ID, COS_FLASH_IMG_TYPE);
	if (ret1 != HISEE_OK) {
		pr_err("hisee:%s upgrade failed, ret = %d\n", __func__, ret1);
		ret = HISEE_MULTICOS_IMG_UPGRADE_ERROR;
	}
	ret1 = hisee_poweroff_func(cos_flash_buf_para, 0);
	if (ret1 != HISEE_OK) {
		pr_err("hisee:%s() poweroff failed. retcode=%d\n",
		       __func__, ret1);
		if (ret == HISEE_OK)
			ret = HISEE_MULTICOS_POWEROFF_ERROR;
	}

err_process:
	check_and_print_result();
	return ret;
}

/*
 * @brief      : load and boot cos flash image, let hisee core running cos flash
 *               image.
 * @return     : 0 on success, other value on failure.
 */
int cos_flash_image_boot_func(void)
{
	int ret;
	char cos_flash_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};

	set_cos_flash_buf_para();
	ret = hisee_poweron_booting_func((void *)cos_flash_buf_para,
					 HISEE_POWER_ON_BOOTING);
	check_error_then_goto(ret);
	/* wait hisee ready for receiving images */
	ret = wait_hisee_ready(HISEE_STATE_COS_READY, HISEE_ATF_GENERAL_TIMEOUT);
	check_error_then_goto(ret);

err_process:
	check_and_print_result();
	return ret;
}

#ifndef CONFIG_GENERAL_SEE_DISABLE_KEY
/*
 * @brief      : write rpmbkey for hisee, only the hisee is in SM lcs
 * @return     : 0 on success, other value on failure.
 * @notes      : cos flash image must be booted at first in hisee.
 */
static int sm_write_rpmb_key_process(void)
{
	int ret = HISEE_OK;
	int write_rpmbkey_retry = 5; /* local retry count */
	char cos_flash_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};

	if (get_rpmb_support_key_num() > SINGLE_RPMBKEY_NUM &&
	    get_rpmb_key_status() == KEY_NOT_READY) {
		set_cos_flash_buf_para();
		while (write_rpmbkey_retry--) {
			ret = hisee_write_rpmb_key();
			if (ret == HISEE_OK)
				break;
			ret = hisee_poweroff_func((void *)cos_flash_buf_para, 0);
			check_error_then_goto(ret);
			ret = hisee_poweron_booting_func((void *)cos_flash_buf_para, 0);
			check_error_then_goto(ret);
			hisee_mdelay(DELAY_FOR_HISEE_POWERON_BOOTING);
		}
	}

err_process:
	check_and_print_result();
	return ret;
}

/*
 * @brief      : load cos flash image, then complete the factory test for hisee
 * @param[in]  : buf, command string buffer
 * @param[in]  : para, factory test parameters
 * @return     : 0 on success, other value on failure.
 * @notes      : cos flash image must be booted at first in hisee.
 */
int load_cos_flash_do_factory_test(const void *buf, int para)
{
	int ret1, ret;
	unsigned int cos_flash_exist_flg = HISEE_FALSE;
	const void *p_curr_cos_buf = NULL;
	/* 3 characters: space,cos_id=3,process_id=COS_PROCESS_CHIP_TEST */
	char cos_flash_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	/* 3 characters: space,cos_id=0,process_id=COS_PROCESS_CHIP_TEST */
	char cos_default_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};

	ret = check_cos_flash_file_exist(&cos_flash_exist_flg);
	if (ret != HISEE_OK || cos_flash_exist_flg == HISEE_FALSE) {
		pr_err("hisee:%s() cos flash file not exist.\n", __func__);
		if (get_rpmb_key_status() == KEY_READY)
			ret = HISEE_OK;
		else
			ret = HISEE_MULTICOS_COS_FLASH_FILE_ERROR;
		goto err_process;
	}
	if (!buf || (*(const char *)buf == HISEE_CHAR_NEWLINE) ||
	    (*(const char *)buf == '\0')) {
		set_cos_default_buf_para();
		p_curr_cos_buf = (const void *)cos_default_buf_para;
	} else {
		p_curr_cos_buf = buf;
	}
	set_cos_flash_buf_para();
	ret = cos_flash_image_upgrade_func(p_curr_cos_buf, para);
	check_error_then_goto(ret);
	ret = cos_flash_image_boot_func();
	check_error_then_goto(ret);
	hisee_mdelay(DELAY_FOR_HISEE_POWERON_BOOTING);

	ret = sm_write_rpmb_key_process();
	check_result_and_goto(ret, poweroff_process);

	ret = run_hisee_nvmformat();
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() run_hisee_nvmformat failed, ret=%d\n",
		       __func__, ret);
		goto poweroff_process;
	}

	ret = hisee_poweroff_func(cos_flash_buf_para, 0);
	check_error_then_goto(ret);
	/* wait hisee power down, if timeout or fail, return errno */
	ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN, DELAY_FOR_HISEE_POWEROFF);
	check_error_then_goto(ret);

	if (para == HISEE_FACTORY_TEST_VERSION) {
		/* poweron upgrading hisee */
		ret = hisee_poweron_upgrade_func(p_curr_cos_buf, 0);
		check_error_then_goto(ret);
		hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);
	} else if (para == HISEE_SERVICE_WORK_VERSION) {
		/* poweron booting hisee */
		ret = hisee_poweron_booting_func(p_curr_cos_buf,
						 HISEE_POWER_ON_BOOTING);
		check_error_then_goto(ret);
	} else {
		pr_err("%s() do nothing.\n", __func__);
	}

	goto err_process;

poweroff_process:
	ret1 = hisee_poweroff_func(cos_flash_buf_para, 0);
	if (ret1 != HISEE_OK)
		pr_err("hisee:%s() hisee_poweroff_func failed1, ret=%d\n",
		       __func__, ret1);

err_process:
	check_and_print_result();
	return ret;
}
#endif /* CONFIG_GENERAL_SEE_DISABLE_KEY */
#endif /* CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS */

#ifdef CONFIG_GENERAL_SEE_NVMFORMAT_TEST
int hisee_nvmformat_func(const void *buf, int para)
{
	int ret;

	(void)buf;
	(void)para;
#ifdef CONFIG_GENERAL_SEE_MISCIMG_SECUPGRADE
	/* Format RPMB firstly, then format nvm. */
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	ret = load_cos_flash_do_factory_test(NULL, HISEE_SERVICE_WORK_VERSION);
	check_error_then_goto(ret);
#else
	ret = run_hisee_nvmformat();
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() run_hisee_nvmformat failed, ret=%d\n",
		       __func__, ret);
		goto err_process;
	}

	ret = hisee_poweron_upgrade_func(NULL, 0);
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() power failed, cos_id=0,ret=%d\n",
		       __func__, ret);
		goto err_process;
	}

	hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);
	ret = handle_cos_image_upgrade(NULL, HISEE_FACTORY_TEST_VERSION);
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() cos image 0upgrade failed,ret=%d\n",
		       __func__, ret);
		goto err_process;
	}
#endif /* CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS */
#endif /* CONFIG_GENERAL_SEE_MISCIMG_SECUPGRADE */

	/* Format nvm in misc upgrade process. */
	ret = hisee_poweroff_func(NULL, HISEE_PWROFF_LOCK);
	check_error_then_goto(ret);

	/* wait hisee power down, if timeout or fail, return errno */
	ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN, DELAY_FOR_HISEE_POWEROFF);
	check_error_then_goto(ret);

	/* poweron booting hisee and set the flag for the process */
	ret = hisee_poweron_booting_func(NULL, HISEE_POWER_ON_BOOTING_MISC);
	check_error_then_goto(ret);

	/* wait hisee ready for receiving images */
	ret = wait_hisee_ready(HISEE_STATE_MISC_READY, HISEE_ATF_GENERAL_TIMEOUT);
	check_error_then_goto(ret);

	ret = hisee_poweroff_func(NULL, HISEE_PWROFF_LOCK);
	check_error_then_goto(ret);

	/* poweron booting hisee and set the flag for the process */
	ret = hisee_poweron_booting_func(NULL, HISEE_POWER_ON_BOOTING);
	check_error_then_goto(ret);

err_process:
	check_and_print_result();
	return ret;
}
#endif /* CONFIG_GENERAL_SEE_NVMFORMAT_TEST */

#ifndef CONFIG_GENERAL_SEE_DISABLE_KEY
static int hisee_manufacture_image_upgrade_process(const void *buf,
						   unsigned int hisee_lcs_mode)
{
	int ret;
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	unsigned int cos_id = COS_IMG_ID_0;
	unsigned int process_id = 0;
#endif

	ret = hisee_force_power_off();
	check_error_then_goto(ret);

	/* wait hisee power down, if timeout or fail, return errno */
	ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN, DELAY_FOR_HISEE_POWEROFF);
	check_error_then_goto(ret);

	ret = hisee_poweron_upgrade_func(buf, 0);
	check_error_then_goto(ret);
	hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);

	if (hisee_lcs_mode == HISEE_DM_MODE_MAGIC) {
		/* DM mode can write rpmbkey multiple with no harm */
		ret = hisee_write_rpmb_key_process(buf);
		check_error_then_goto(ret);
	}
#if defined CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() hisee_get_cosid failed ret=%d\n",
		       __func__, ret);
		goto err_process;
	}
	/* only cos0/1 support load cos_flash image */
	if (cos_id == COS_IMG_ID_0 || cos_id == COS_IMG_ID_1) {
		ret = load_cos_flash_do_factory_test(buf,
						     HISEE_FACTORY_TEST_VERSION);
		check_error_then_goto(ret);
	}
#elif defined CONFIG_GENERAL_SEE_MISCIMG_SECUPGRADE
	ret = run_hisee_nvmformat();
	if (ret != HISEE_OK)
		pr_err("hisee:%s() run_hisee_nvmformat failed, ret=%d\n",
		       __func__, ret);
#endif
	ret = cos_image_upgrade_func(buf, HISEE_FACTORY_TEST_VERSION);
	check_error_then_goto(ret);
	hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

	ret = hisee_poweron_booting_misc_process(buf);
	check_error_then_goto(ret);

	if (hisee_lcs_mode == HISEE_DM_MODE_MAGIC) {
		ret = otp_image_upgrade_func(buf, 0);
		check_error_then_goto(ret);
	}

err_process:
	check_and_print_result();
	return ret;
}
#endif

/*
 * @brief      : send simple smc cmd to atf without extra data
 * @param[in]  : cmd_type, command type
 */
int mspc_send_smc(unsigned int cmd_type)
{
	int ret;
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	pr_err("%s enter\n", __func__);
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, cmd_type);
	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_COS_TIMEOUT, cmd_type);
	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K,
			  buff_virt, buff_phy);
	pr_err("%s exit\n", __func__);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

#ifndef CONFIG_GENERAL_SEE_DISABLE_KEY
static int hisee_total_manufacture_func(const void *buf, int para)
{
	int ret, ret1;
	unsigned int hisee_lcs_mode = 0;
	void *p_buff_para = NULL;
	unsigned int cos_id;
	char factory_slt_test_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	unsigned int cos_image_num;

#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	p_buff_para = (void *)factory_slt_test_para;
	cos_image_num = HISEE_SUPPORT_COS_FILE_NUMBER;
#else
	p_buff_para = NULL;
	cos_image_num = HISEE_MIN_COS_IMAGE_NUMBER;
#endif

	factory_slt_test_para[0] = HISEE_CHAR_SPACE; /* space character */
	factory_slt_test_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;
	reinit_hisee_complete();
	ret = get_hisee_lcs_mode(&hisee_lcs_mode);
	check_error_then_goto(ret);

	for (cos_id = 0; cos_id < cos_image_num; cos_id++) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
		/*
		 * If there is no image for current cos id in hisee_img,
		 * bypass upgrading.
		 */
		if (get_hisee_data_ptr()->hisee_img_head.is_cos_exist[cos_id] !=
		    HISEE_COS_EXIST) {
			pr_err("%s: there is no cos%u image in hisee_img!\n",
			       __func__, cos_id);
			continue;
		}
#endif

		factory_slt_test_para[HISEE_COS_ID_POS] = '0' + cos_id;
		ret = hisee_manufacture_image_upgrade_process(p_buff_para,
							      hisee_lcs_mode);
		check_error_then_goto(ret);

		ret = hisee_verify_isd_key(cos_id);
		check_error_then_goto(ret);

		ret = hisee_apdu_test_process(cos_id);
		check_error_then_goto(ret);
		pr_err("hisee:%s() cos_id=%u, success!\n", __func__, cos_id);
	}
	ret = hisee_manufacture_set_lcs_sm(hisee_lcs_mode);
	check_error_then_goto(ret);
	pr_err("hisee:%s() set hisee to SM state succes\n", __func__);
	ret = HISEE_OK;
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	/* remove /mnt/hisee_fs/cos_flash.img after pinstall success. */
	filesys_rm_cos_flash_file();
#endif
err_process:
	ret1 = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
	if (ret == HISEE_OK)
		ret = ret1;
	hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

	if (ret == HISEE_OK) {
		hisee_chiptest_set_otp1_status(PREPARED);
		release_hisee_complete(); /* sync signal for flash_otp_task */
	}

	return set_errno_then_exit(ret);
}
#endif /* CONFIG_GENERAL_SEE_DISABLE_KEY */

#ifdef CONFIG_GENERAL_SEE_AT_SMX
/*
 * @brief      : run flash cos to disable smx
 */
static int mspc_disable_smx(void)
{
	int ret, ret1;
	unsigned int exist_flg = 0;
	char buf_para[MAX_CMD_BUFF_PARAM_LEN] = " ";

	buf_para[HISEE_COS_ID_POS] = '0' + COS_IMG_ID_5;
	buf_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;

	ret = check_cos_flash_file_exist(&exist_flg);
	if (ret != HISEE_OK)
		return ret;
	/* if no exist flash cos to disable smx, return err */
	if (exist_flg != COS_FLASH_EXIST) {
		pr_err("%s flash cos not exist\n", __func__);
		return HISEE_MULTICOS_COS_FLASH_FILE_ERROR;
	}

	/* run flashCOS to close smx */
	ret = cos_flash_image_boot_func();
	check_error_then_goto(ret);

	ret = mspc_send_smc(CMD_SET_SMX);
err_process:
	ret1 = hisee_poweroff_func(buf_para, HISEE_PWROFF_LOCK);
	if (ret == HISEE_OK)
		ret = ret1;

	hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);
	return ret;
}

#define MSPC_SETSMX_PARA_MAXLEN  3
#define MSPC_SETSMX_NUM_LEN      1
/*
 * @brief      : disable smx feature function
 */
int mspc_set_smx_func(const void *buf, int para)
{
	const char *input = (const char *)buf;
	int ret;

	hisee_set_at_type(HISEE_AT_SETSMX);

	if (!input) {
		pr_err("%s buf parameters is null\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}

	/* input len at leaset 3, format as ",x," */
	if (strnlen(input, MSPC_SETSMX_PARA_MAXLEN) < MSPC_SETSMX_PARA_MAXLEN)
		return set_errno_then_exit(HISEE_INVALID_PARAMS);

	/* input must start with ',' */
	if (*(input++) != ',') {
		pr_err("%s para not start with ,\n", __func__);
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	}
	/* input must end with ',' */
	if (input[MSPC_SETSMX_NUM_LEN] != ',') {
		pr_err("%s para not end with ,\n", __func__);
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	}
	/* num 0 for enable smx */
	if (input[0] == '0') {
		if (hisee_get_smx_func(NULL, 0) != SMX_ENABLE) {
			pr_err("%s smx disabled\n", __func__);
			return set_errno_then_exit(SMX_DISABLE);
		}
		return set_errno_then_exit(HISEE_OK);
	}
	if (input[0] != '1') {
		pr_err("%s para not support\n", __func__);
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	}

	/* num 1 for disable smx */
	if (hisee_get_smx_func(NULL, 0) == SMX_DISABLE)
		return set_errno_then_exit(HISEE_OK);
	ret = mspc_disable_smx();

	return set_errno_then_exit(ret);
}
#endif

#ifdef CONFIG_GENERAL_SEE_DISABLE_KEY
#define HISEE_FS_BUF_SIZE 128

static int is_cos_flash_exist(unsigned int *exist_flg)
{
	int ret;
	struct hisee_img_header local_img_header;

	if (!exist_flg) {
		pr_err("hisee:%s(): invalid params.\n", __func__);
		return HISEE_INVALID_PARAMS;
	}

	*exist_flg = HISEE_FALSE;
	(void)memset_s((void *)&local_img_header, sizeof(local_img_header),
		       0, sizeof(local_img_header));
	ret = hisee_parse_img_header((char *)&local_img_header);
	if (ret != HISEE_OK) {
		pr_err("%s():hisee parse header failed, ret=%d\n",
		       __func__, ret);
		return set_errno_then_exit(ret);
	}
	*exist_flg = HISEE_TRUE;
	return HISEE_OK;
}

/* erase cos flash in hisee_fs partition */
int hisee_fs_rm_cos_flash_file(void)
{
	int fd = -1;
	ssize_t cnt;
	mm_segment_t old_fs;
	char fullpath[HISEE_FS_BUF_SIZE] = {0};
	char data_buf[HISEE_FS_BUF_SIZE] = {0};
	unsigned int i;
	int ret;

	ret = hisee_get_partition_path(fullpath, HISEE_FS_BUF_SIZE);
	if (ret != HISEE_OK)
		return set_errno_then_exit(HISEE_INVALID_PARAMS);

	old_fs = get_fs();
	/* the kernel API generate pclint warning, ignore it */
	set_fs(KERNEL_DS); /*lint !e501*/

	fullpath[HISEE_FS_BUF_SIZE - 1] = 0;
	fd = (int)hisee_sys_open(fullpath, O_WRONLY, HISEE_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s():open %s failed %d\n", __func__, fullpath, fd);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		return set_errno_then_exit(ret);
	}

	cnt = 0;
	for (i = 0; i < HISEE_MAX_IMG_SIZE / HISEE_FS_BUF_SIZE; i++)
		cnt += hisee_sys_write((unsigned int)fd,
				 (char __user *)data_buf, sizeof(data_buf));

	ret = hisee_sys_fsync((unsigned int)fd);
	if (ret < 0) {
		pr_err("%s():fail to sync %s.\n", __func__, fullpath);
		ret = HISEE_ENCOS_SYNC_FILE_ERROR;
	}
	if (cnt != (ssize_t)HISEE_MAX_IMG_SIZE) {
		pr_err("%s(): access %s failed, return %d\n",
		       __func__, fullpath, cnt);
		ret = HISEE_ACCESS_FILE_ERROR;
	}

	hisee_sys_close((unsigned int)fd);
	set_fs(old_fs);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int hisee_disable_otp_key(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	int image_size;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}

	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_DISABLE_KEY);

	image_size = HISEE_ATF_MESSAGE_HEADER_LEN;
	ret = send_smc_process(p_message_header, buff_phy,
			       (unsigned int)image_size,
			       HISEE_ATF_GENERAL_TIMEOUT, CMD_DISABLE_KEY);
	if (ret != HISEE_OK)
		pr_err("%s(): hisee reported fail code=%d\n", __func__, ret);

	dma_free_coherent(hisee_data_ptr->cma_device,
			  (unsigned long)(SIZE_4K), buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

static int load_cos_flash_disable_key(const void *buf, int para)
{
	int ret1, ret;
	const void *p_curr_cos_buf = NULL;
	/* 3 characters: space,cos_id=5,process_id=COS_PROCESS_CHIP_TEST */
	char cos_flash_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};

	if (!buf || (*(const char *)buf == HISEE_CHAR_NEWLINE) ||
	    (*(const char *)buf == '\0')) {
		set_cos_flash_buf_para();
		p_curr_cos_buf = (const void *)cos_flash_buf_para;
	} else {
		p_curr_cos_buf = buf;
	}
	set_cos_flash_buf_para();
	ret = cos_flash_image_upgrade_func(p_curr_cos_buf, para);
	check_error_then_goto(ret);
	ret = cos_flash_image_boot_func();
	check_result_and_goto(ret, poweroff_process);
	hisee_mdelay(DELAY_FOR_HISEE_POWERON_BOOTING);

	ret = hisee_disable_otp_key();
	if (ret != HISEE_OK) {
		pr_err("hisee:%s() disable otp key failed, ret=%d\n",
		       __func__, ret);
		goto poweroff_process;
	}

poweroff_process:
	ret1 = hisee_poweroff_func(cos_flash_buf_para, 0);
	if (ret1 != HISEE_OK)
		pr_err("hisee:%s() hisee_poweroff_func failed1, ret=%d\n",
		       __func__, ret1);

err_process:
	check_and_print_result();
	return ret;
}

static int hisee_disable_otp_key_func(void)
{
	int ret, ret1;
	void *p_buff_para = NULL;
	unsigned int cos_id = COS_IMG_ID_5;
	char factory_test_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	unsigned int cos_flash_exist_flg = HISEE_FALSE;
	unsigned int hisee_lcs_mode = 0;

	p_buff_para = (void *)factory_test_para;
	factory_test_para[0] = HISEE_CHAR_SPACE; /* space character */
	factory_test_para[HISEE_COS_ID_POS] = '0' + cos_id;
	factory_test_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;

	ret = get_hisee_lcs_mode(&hisee_lcs_mode);
	check_error_then_goto(ret);

	ret = load_cos_flash_disable_key(p_buff_para,
					 HISEE_FACTORY_TEST_VERSION);
	if (ret != HISEE_OK) {
		/* if LCS is SM, then return OK */
		if (hisee_lcs_mode == HISEE_SM_MODE_MAGIC) {
			pr_err("hisee:%s() lcs is SM\n", __func__);
			/* remove cos_flash if exist */
			ret = is_cos_flash_exist(&cos_flash_exist_flg);
			if (ret == HISEE_OK && cos_flash_exist_flg == HISEE_TRUE)
				(void)hisee_fs_rm_cos_flash_file();
			return HISEE_OK;
		}
		goto err_process;
	}

	if (hisee_lcs_mode == HISEE_DM_MODE_MAGIC) {
		ret = set_hisee_lcs_sm_efuse();
		check_error_then_goto(ret);
		pr_err("hisee:%s() set hisee to SM state success\n", __func__);
	}
	/* remove image in hisee_fs after disable key success. */
	ret = hisee_fs_rm_cos_flash_file();
	check_error_then_goto(ret);
	pr_err("hisee:%s() delete cos flash success\n", __func__);

err_process:
	ret1 = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
	if (ret == HISEE_OK)
		ret = ret1;
	hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

	return set_errno_then_exit(ret);
}
#endif

static int factory_test_body(void *arg)
{
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
		pr_err("%s hisee factory test state error: %x\n",
		       __func__, hisee_data_ptr->factory_test_state);
		ret = HISEE_FACTORY_STATE_ERROR;
		goto exit;
	}
#ifdef CONFIG_GENERAL_SEE_DISABLE_KEY
	ret = hisee_disable_otp_key_func();
#else
	ret = hisee_total_manufacture_func(NULL, 0);
#endif
	if (ret != HISEE_OK)
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
	else
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_SUCCESS;

exit:
	check_and_print_result();
	return set_errno_then_exit(ret);
}

int hisee_parallel_manufacture_func(const void *buf, int para)
{
	int ret = HISEE_OK;
	struct task_struct *factory_test_task = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING &&
	    !hisee_chiptest_otp1_is_running()) {
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_RUNNING;
		factory_test_task = kthread_run(factory_test_body,
						NULL, "factory_test_body");
		if (!factory_test_task) {
			ret = HISEE_THREAD_CREATE_ERROR;
			hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
			pr_err("hisee err create factory_test_task failed\n");
		}
	}
	return set_errno_then_exit(ret);
}

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
static void __iomem *g_hisee_temp_addr;
/*
 * @brief      : hisee_temp_limit_cfg.
 * @param[in]  : flag, on or off
 */
static void hisee_temp_limit_cfg(enum hisee_tmp_cfg_state flag)
{
	unsigned int value;

	if (!g_hisee_temp_addr) {
		g_hisee_temp_addr = ioremap(HISEE_HIGH_TEMP_PROTECT_ADDR,
					    sizeof(unsigned int));
		if (!g_hisee_temp_addr) {
			pr_err("hisee init temp addr failed!\n");
			return;
		}
	}
	value = readl(g_hisee_temp_addr);
	if (flag == HISEE_TEMP_CFG_OFF)
		value |= BIT(HISEE_HIGH_TEMP_PROTECT_DISABLE_BIT);
	else
		value &= (~BIT(HISEE_HIGH_TEMP_PROTECT_DISABLE_BIT));

	writel(value, g_hisee_temp_addr);
}
#endif

/* hisee slt test function begin */
#ifdef CONFIG_GENERAL_SEE_CHIPTEST_SLT
int hisee_total_slt_func(const void *buf, int para)
{
	int ret = HISEE_OK;
	int ret1;
	void *p_buff_para = NULL;
	unsigned int cos_id;
	char factory_slt_test_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	unsigned int cos_image_num;

#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	p_buff_para = (void *)factory_slt_test_para;
	cos_image_num = HISEE_SUPPORT_COS_FILE_NUMBER;
#else
	p_buff_para = NULL;
	cos_image_num = HISEE_MIN_COS_IMAGE_NUMBER;
#endif
	factory_slt_test_para[0] = HISEE_CHAR_SPACE;
	factory_slt_test_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	/* avoid hitemp environment test fail */
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_OFF);
#endif

	for (cos_id = 0; cos_id < cos_image_num; cos_id++) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
		/*
		 * If there is no image for current cos id in hisee_img,
		 * bypass upgrading.
		 */
		if (get_hisee_data_ptr()->hisee_img_head.is_cos_exist[cos_id] !=
		    HISEE_COS_EXIST) {
			pr_err("%s: there is no cos%u image in hisee_img!\n",
			       __func__, cos_id);
			continue;
		}
#endif

		factory_slt_test_para[HISEE_COS_ID_POS] = '0' + cos_id;
		ret = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
		check_error_then_goto(ret);
		hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

		ret = hisee_poweron_upgrade_func(p_buff_para, 0);
		check_error_then_goto(ret);
		hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE);

		ret = hisee_write_rpmb_key_process(p_buff_para);
		check_error_then_goto(ret);

		ret = cos_image_upgrade_func(p_buff_para,
					     HISEE_FACTORY_TEST_VERSION);
		check_error_then_goto(ret);
		hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

		ret = hisee_poweron_booting_misc_process(p_buff_para);
		check_error_then_goto(ret);

		ret = hisee_apdu_test_process(cos_id);
		check_error_then_goto(ret);

		pr_err("hisee:%s() cos_id=%u,success!\n", __func__, cos_id);
		ret = HISEE_OK;
	}
err_process:
	ret1 = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
	if (ret == HISEE_OK)
		ret = ret1;

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	/* avoid hitemp environment test fail */
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_ON);
#endif

	return set_errno_then_exit(ret);
}

int hisee_read_slt_func(const void *buf, int para)
{
	int err_code;
	int ret = HISEE_OK;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	err_code = get_hisee_errno();
	if (err_code == HISEE_OK) {
		if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_SUCCESS) {
			pr_err("%s() SLT test is not success, test_state=%x\n",
			       __func__, hisee_data_ptr->factory_test_state);
			ret = HISEE_ERROR;
		}
	} else {
		ret = err_code;
		pr_err("%s() ret=%d\n", __func__, ret);
	}
	return ret;
}

static int slt_test_body(void *arg)
{
	int ret;
	int retry_cnt = 3; /* local retry cnt */
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
		pr_err("%s hisee slt test state error: %x\n",
		       __func__, hisee_data_ptr->factory_test_state);
		ret = HISEE_FACTORY_STATE_ERROR;
		goto exit;
	}
	do {
		ret = hisee_total_slt_func(NULL, 0);
		if (ret == HISEE_OK) {
			hisee_data_ptr->factory_test_state =
				HISEE_FACTORY_TEST_SUCCESS;
			check_and_print_result();
			return set_errno_then_exit(ret);
		}
		retry_cnt--;
	} while (retry_cnt > 0);
	hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;

exit:
	check_and_print_result();
	return set_errno_then_exit(ret);
}

int hisee_parallel_total_slt_func(const void *buf, int para)
{
	int ret = HISEE_OK;
	struct task_struct *slt_test_task = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_RUNNING;
		slt_test_task = kthread_run(slt_test_body,
					    NULL, "slt_test_body");
		if (!slt_test_task) {
			ret = HISEE_THREAD_CREATE_ERROR;
			hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
			pr_err("hisee err create slt_test_task failed\n");
		} else {
			pr_err("%s() success!\n", __func__);
		}
	}
	return set_errno_then_exit(ret);
}

#endif /* CONFIG_GENERAL_SEE_CHIPTEST_SLT */

#ifdef CONFIG_GENERAL_SEE_FACTORY_SECURITY_CHECK
#define OFFSET_FAULT_ADDR       0
#define OFFSET_FAULT_VALUE      4
#define OFFSET_TEMPSENSOR_VALUE 8
/*
 * @brief      : hisee_factory_check
 * @return     : 0 on success, other value on failure.
 */
static int hisee_factory_check(void)
{
	char *buff_virt = NULL;
	phys_addr_t buff_phy = 0;
	struct atf_message_header *p_message_header = NULL;
	int ret;
	unsigned int result_offset;
	char *result_addr = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	buff_virt = (char *)dma_alloc_coherent(hisee_data_ptr->cma_device, SIZE_4K,
					       &buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		return set_errno_then_exit(HISEE_NO_RESOURCES);
	}
	(void)memset_s(buff_virt, SIZE_4K, 0, SIZE_4K);
	p_message_header = (struct atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_HISEE_FACTORY_CHECK);

	result_offset = HISEE_ATF_MESSAGE_HEADER_LEN;
	p_message_header->test_result_phy =
		(unsigned int)buff_phy + result_offset;
	p_message_header->test_result_size = SIZE_4K - result_offset;
	ret = send_smc_process(p_message_header, buff_phy,
			       HISEE_ATF_MESSAGE_HEADER_LEN,
			       HISEE_ATF_GENERAL_TIMEOUT,
			       CMD_HISEE_FACTORY_CHECK);
	result_addr = buff_virt + result_offset;
	pr_err("%s(): hisee reported code=%x %x %x\n", __func__,
	       *((unsigned int *)(result_addr + OFFSET_FAULT_ADDR)),
	       *((unsigned int *)(result_addr + OFFSET_FAULT_VALUE)),
	       *((unsigned int *)(result_addr + OFFSET_TEMPSENSOR_VALUE)));

	dma_free_coherent(hisee_data_ptr->cma_device, SIZE_4K, buff_virt, buff_phy);
	check_and_print_result();
	return set_errno_then_exit(ret);
}

/*
 * @brief      : hisee_factory_check_body
 * @param[in]  : buf: include the information of cos id, processor id.
 * @param[in]  : para: to indicate it is in which mode, like factory or usr.
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_factory_check_body(void *arg)
{
	int ret = HISEE_OK;
	int ret1;
	void *p_buff_para = NULL;
	unsigned int cos_id;
	char factory_slt_test_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	unsigned int cos_image_num;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
	p_buff_para = (void *)factory_slt_test_para;
	cos_image_num = HISEE_SUPPORT_COS_FILE_NUMBER;
#else
	cos_image_num = HISEE_MIN_COS_IMAGE_NUMBER;
#endif
	factory_slt_test_para[0] = HISEE_CHAR_SPACE;
	factory_slt_test_para[HISEE_PROCESS_TYPE_POS] = '0' + COS_PROCESS_UPGRADE;

	/*
	 * To avoid enter into the otp flash process
	 * in case the status is PREPARED.
	 */
	hisee_chiptest_set_otp1_status(NO_NEED);
#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_OFF);
#endif

	for (cos_id = 0; cos_id < cos_image_num; cos_id++) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
		/*
		 * If there is no image for current cos id in hisee_img,
		 * bypass upgrading.
		 */
		if (hisee_data_ptr->hisee_img_head.is_cos_exist[cos_id] !=
		    HISEE_COS_EXIST) {
			pr_err("%s: there is no cos%u image in hisee_img!\n",
			       __func__, cos_id);
			continue;
		}
#endif
		factory_slt_test_para[HISEE_COS_ID_POS] = '0' + cos_id;
		ret = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
		check_error_then_goto(ret);
		/* wait hisee power down, if timeout or fail, return errno */
		ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN,
				       DELAY_FOR_HISEE_POWEROFF);
		check_error_then_goto(ret);

		ret = hisee_poweron_booting_func(p_buff_para, 0);
		check_error_then_goto(ret);

		ret = wait_hisee_ready(HISEE_STATE_COS_READY,
				       HISEE_ATF_GENERAL_TIMEOUT);
		check_error_then_goto(ret);

		ret = hisee_factory_check();
		check_error_then_goto(ret);
		pr_err("hisee:%s() cos_id=%u, success!\n", __func__, cos_id);
	}
err_process:
	if (ret != HISEE_OK)
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
	else
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_SUCCESS;

	ret1 = hisee_poweroff_func(p_buff_para, HISEE_PWROFF_LOCK);
	if (ret == HISEE_OK)
		ret = ret1;

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_ON);
#endif
	return set_errno_then_exit(ret);
}

/*
 * @brief      : hisee_factory_check_func
 * @param[in]  : buf: include the information of cos id, processor id.
 * @param[in]  : para: to indicate it is in which mode, like factory or usr.
 * @return     : ::int, 0 on success, other value on failure
 */
int hisee_factory_check_func(const void *buf, int para)
{
	int ret = HISEE_OK;
	struct task_struct *factory_check_test_task = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_RUNNING;
		factory_check_test_task = kthread_run(hisee_factory_check_body,
						      NULL,
						      "factory_check_test_body");
		if (!factory_check_test_task) {
			ret = HISEE_THREAD_CREATE_ERROR;
			hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
			pr_err("hisee err create factory_check_test_task failed\n");
		} else {
			pr_err("%s() success!\n", __func__);
		}
	}
	return set_errno_then_exit(ret);
}
#else
/*
 * @brief      : hisee_factory_check_func
 * @param[in]  : buf: include the information of cos id, processor id.
 * @param[in]  : para: to indicate it is in which mode, like factory or usr.
 * @return     : ::int, 0 on success, other value on failure
 */
int hisee_factory_check_func(const void *buf, int para)
{
	int ret = HISEE_OK;

	get_hisee_data_ptr()->factory_test_state = HISEE_FACTORY_TEST_SUCCESS;
	return set_errno_then_exit(ret);
}
#endif /* end of CONFIG_GENERAL_SEE_FACTORY_SECURITY_CHECK */

#ifdef CONFIG_GENERAL_SEE_CHIPTEST_RT
#define HISEE_CHIPTEST_RT_POWER_ONOFF_DELAY 2000 /* unit: s */
#define HISEE_CHIPTEST_RT_POWER_ONOFF_COUNT 450

static struct task_struct *g_hisee_chiptest_rt_task;
static int g_is_stop_hisee_chiptest_rt_task;

/*
 * @brief      : hisee_chiptest_rt_process
 * @param[in]  : arg, argument string buffer
 * @return     : 0 on success, others on failure.
 * @notes      : rt test only support cos0 image when multicos function enable,
 *               multi cos image test may bu support in future
 */
static int hisee_chiptest_rt_process(void *arg)
{
	int ret;
	int loop = HISEE_CHIPTEST_RT_POWER_ONOFF_COUNT;
	/* 3 characters: space,cos_id=0,process_id=COS_PROCESS_CHIP_TEST */
	char cos_default_buf_para[MAX_CMD_BUFF_PARAM_LEN] = {0};
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	set_cos_default_buf_para();

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	/* avoid hitemp environment test fail */
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_OFF);
#endif

	ret = hisee_poweroff_func((void *)cos_default_buf_para, 0);
	check_error_then_goto(ret);

	/* wait hisee power down, if timeout or fail, return errno */
	ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN, DELAY_FOR_HISEE_POWEROFF);
	check_error_then_goto(ret);

	while (!kthread_should_stop()) {
		ret = hisee_poweron_upgrade_func((void *)cos_default_buf_para, 0);
		check_error_then_goto(ret);

		msleep(HISEE_CHIPTEST_RT_POWER_ONOFF_DELAY);

		ret = hisee_poweroff_func((void *)cos_default_buf_para, 0);
		check_error_then_goto(ret);

		msleep(HISEE_CHIPTEST_RT_POWER_ONOFF_DELAY);

		loop--;
		if (g_is_stop_hisee_chiptest_rt_task || !loop)
			break;
	};

#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	/* avoid hitemp environment test fail */
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_ON);
#endif

	hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_SUCCESS;
	pr_info("%s(): loop %d!\n", __func__, loop);
	pr_info("%s(): run success\n", __func__);
	return HISEE_OK;

err_process:
#ifdef CONFIG_GENERAL_SEE_HIGH_TEMP_PROTECT_SWITCH
	/* avoid hitemp environment test fail */
	hisee_temp_limit_cfg(HISEE_TEMP_CFG_ON);
#endif

	/* dmd info as mntn, if error, only print error info, not return */
	if (hisee_mntn_record_dmd_info(DSM_HISEE_POWER_ON_OFF_ERROR_NO,
				       "hisee chiptest rt process fail"))
		pr_err("%s(): dmd info fail!\n", __func__);

	hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
	pr_err("%s(): loop %d!\n", __func__, loop);
	pr_err("%s(): run fail, ret 0x%x\n", __func__, ret);

	return ret;
}

int hisee_chiptest_rt_run_func(const void *buf, int para)
{
	int ret = HISEE_OK;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	if (hisee_data_ptr->factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
		hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_RUNNING;
		g_is_stop_hisee_chiptest_rt_task = HISEE_FALSE;
		g_hisee_chiptest_rt_task = kthread_run(hisee_chiptest_rt_process,
						       NULL, "hisee_rt_task");
		if (!g_hisee_chiptest_rt_task) {
			ret = HISEE_THREAD_CREATE_ERROR;
			hisee_data_ptr->factory_test_state = HISEE_FACTORY_TEST_FAIL;
			pr_err("%s(): kthread_run fail!\n", __func__);
		}
	}
	/* dmd info as mntn, if error, only print error info, not return */
	if (ret != HISEE_OK) {
		if (hisee_mntn_record_dmd_info(
			DSM_HISEE_POWER_ON_OFF_ERROR_NO,
			"hisee chiptest rt run fail"))
			pr_err("%s(): dmd info fail!\n", __func__);
	}
	return ret;
}

int hisee_chiptest_rt_stop_func(const void *buf, int para)
{
	g_is_stop_hisee_chiptest_rt_task = HISEE_TRUE;
	return HISEE_OK;
}
#endif /* end of CONFIG_GENERAL_SEE_CHIPTEST_RT */

#ifdef CONFIG_GENERAL_SEE_SUPPORT_CASDKEY
/*
 * @brief      : response for AT^HISEE=CASD
 * @param[out] : buf, response string buffer
 * @param[in]  : size, the length for buf
 * @param[out] : result, response result type
 * @return     : > 0 on success, others on failure.
 */
static int hisee_at_casd_response(char *buf, size_t size, int result)
{
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	if (!buf || size == 0)
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	hisee_data_ptr = get_hisee_data_ptr();
	if (result == HISEE_OK)
		ret = sprintf_s(buf, size, "^HISEE:CASD,%u",
				hisee_data_ptr->casd_data.curr_pack);
	else if (result == HISI_CASD_HASH_ERROR)
		ret = sprintf_s(buf, size, "^HISEE:CASD,%u,TRANSMIT,-1",
				hisee_data_ptr->casd_data.curr_pack);
	else
		ret = sprintf_s(buf, size, "^HISEE:CASD,%u,TRANSMIT,0",
				hisee_data_ptr->casd_data.curr_pack);

	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s sprintf_s err\n", __func__);
		return set_errno_then_exit(HISEE_SECUREC_ERR);
	}
	return ret;
}

/*
 * @brief      : response for AT^HISEE=VERIFYCASD
 * @param[out] : buf, response string buffer
 * @param[in]  : size, the length for buf
 * @param[out] : result, response result type
 * @return     : > 0 on success, others on failure.
 */
static int hisee_at_verifycasd_response(char *buf, size_t size, int result)
{
	int ret;
	struct hisee_module_data *hisee_data_ptr = NULL;

	if (!buf || size == 0)
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	hisee_data_ptr = get_hisee_data_ptr();
	if (result == HISEE_OK)
		ret = sprintf_s(buf, size, "^HISEE:VERIFYCASD,%u",
				hisee_data_ptr->casd_data.curr_pack);
	else
		ret = sprintf_s(buf, size, "^HISEE:VERIFYCASD,%u,VERIFY,-2",
				hisee_data_ptr->casd_data.curr_pack);

	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s sprintf_s err\n", __func__);
		return set_errno_then_exit(HISEE_SECUREC_ERR);
	}
	return ret;
}
#endif

#ifdef CONFIG_GENERAL_SEE_AT_SMX
/*
 * @brief      : response for AT^HISEE=SETHISEESMX
 * @param[out] : buf, response string buffer
 * @param[in]  : size, the length for buf
 * @param[out] : result, response result type
 * @return     : > 0 on success, others on failure.
 */
static int hisee_at_setsmx_response(char *buf, size_t size, int result)
{
	int ret = 0;

	if (!buf || size == 0)
		return set_errno_then_exit(HISEE_INVALID_PARAMS);

	if (result != HISEE_OK)
		ret = sprintf_s(buf, size, "^HISEE:SETHISEESMX,%d", result);

	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s sprintf_s err\n", __func__);
		return set_errno_then_exit(HISEE_SECUREC_ERR);
	}
	return ret;
}
#endif

/* list of response for AT^HISEE= */
static struct hisee_at_response g_hisee_at_tbl[] = {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_CASDKEY
	{ HISEE_AT_CASD,       hisee_at_casd_response },
	{ HISEE_AT_VERIFYCASD, hisee_at_verifycasd_response },
#endif
#ifdef CONFIG_GENERAL_SEE_AT_SMX
	{ HISEE_AT_SETSMX,     hisee_at_setsmx_response },
#endif
};

/*
 * @brief      : hisee_at_result_show, use this function to print AT result
 * @param[out] : buf, result string buffer
 * @return     : ::ssize_t, > 0 on success, others on failure.
 */
ssize_t hisee_at_result_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	unsigned int i;
	int ret;
	int result;
	int at_type;

	at_type = hisee_get_at_type();
	hisee_set_at_type(HISEE_AT_MAX); /* set the type to default */

	if (!buf) {
		pr_err("%s buf parameters is null\n", __func__);
		return set_errno_then_exit(HISEE_INVALID_PARAMS);
	}
	result = get_hisee_errno();

	for (i = 0; i < ARRAY_SIZE(g_hisee_at_tbl); i++) {
		if (at_type != g_hisee_at_tbl[i].at_type ||
		    !g_hisee_at_tbl[i].handler)
			continue;
		ret = g_hisee_at_tbl[i].handler(buf, HISEE_BUF_SHOW_LEN, result);
		break;
	}

	if (i == ARRAY_SIZE(g_hisee_at_tbl)) {
		ret = sprintf_s(buf, HISEE_BUF_SHOW_LEN, "UNSUPPORT");
		if (ret == HISEE_SECLIB_ERROR) {
			pr_err("%s sprintf_s err\n", __func__);
			return set_errno_then_exit(HISEE_SECUREC_ERR);
		}
	}

	return (ssize_t)ret;
}

#if defined(CONFIG_SMX_PROCESS) || defined(CONFIG_GENERAL_SEE_AT_SMX)
/*
 * @brief      : get smx status of the phone. Input is not used
 * @param[in]  : buf, not used
 * @param[in]  : para, not used
 * @return     : ::int, SMX_ENABLE on enable, SMX_DISABLE on disable.
 */
int hisee_get_smx_func(const void *buf, int para)
{
	int smx;

	smx = send_smc_no_ack(CMD_SMX_GET_EFUSE, NULL, 0);
	/* SMX_PROCESS_1: smx is not disabled */
	if (smx != (int)SMX_PROCESS_1) {
		pr_err("%s(): %x\n", __func__, smx);
		return SMX_DISABLE;
	}
	return SMX_ENABLE;
}
#endif
