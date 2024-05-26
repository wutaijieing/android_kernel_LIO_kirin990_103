/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: This file is for dynamic code upgrade process, including
 *              send dynamic data to ATF by SMC call, and read encrypted
 *              dynamic data from RPMB.
 * Create: 2018-12-12
 */

#include "crypto_core_dcs.h"
#include <asm/compiler.h>
#include <linux/atomic.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
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
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_errno.h"
#include "crypto_core_fs.h"
#include "crypto_core_power.h"
#include "crypto_core_smc.h"
#include "crypto_core_upgrade.h"

atomic_t g_is_dcs_free = ATOMIC_INIT(0);
static char *g_dcs_buff_virt;
static phys_addr_t g_dcs_buff_phy;
static uint32_t g_dcs_image_cnt;

/*
 * @brief      : mspc_dcs_read_param_check.
 * @param[in]  : cos_id: the cos index for current dynamic data upgrade process.
 * @param[in]  : type: the image file type.
 * @return     : MSPC_OK is success, others are failure.
 */
static int mspc_dcs_read_param_check(uint32_t cos_id, uint32_t *type)
{
	int ret;
	uint32_t dcs_image_cnt;
	struct mspc_img_header *img_header = NULL;

	if (cos_id != MSPC_NATIVE_COS_ID) {
		pr_err("%s(): cos id is invalid %d\n", __func__, cos_id);
		return MSPC_DCS_COS_ID_ERROR;
	}

	img_header = mspc_get_image_header();
	if (!img_header)
		return MSPC_ERROR;
	(void)memset_s(img_header, sizeof(struct mspc_img_header),
		       0, sizeof(struct mspc_img_header));

	ret = mspc_get_and_parse_img_header(img_header);
	if (ret != MSPC_OK) {
		pr_err("%s:get mspc image header failed!\n", __func__);
		return MSPC_DCS_PARSER_IMG_ERROR;
	}

	/*
	 * For cos0, the dynamic image name is DCS0_00~DCS0_09, cos1 is
	 * DCS1_00~DCS1_09, the other cosX, it is like DCSX_00~DCSX_09
	 */
	*type = DCS0_IMG_TYPE + (cos_id * MSPC_MAX_DCS_FILES);

	dcs_image_cnt = img_header->dcs_image_cnt[cos_id];
	if (dcs_image_cnt == 0 || dcs_image_cnt > MSPC_MAX_DCS_FILES) {
		pr_err("%s(): dynamic data count invalid %d \n",
		       __func__, dcs_image_cnt);
		return MSPC_DCS_IMAGECNT_ERROR;
	}

	g_dcs_image_cnt = dcs_image_cnt;
	pr_err("%s(): the dynamic data count is %d\n",
	       __func__, dcs_image_cnt);

	return MSPC_OK;
}

/*
 * @brief      : mspc_dcs_read : power up mspc and send dynamic data to atf.
 * @param[in]  : cos_id:the cos index for current dynamic data upgrade process.
 * @return     : MSPC_OK is success, others are failure.
 * @note       : It will call mspc_dcs_read to read all dynamic data from
 *                mspc_img, and then send those data to ATF by SMC call.
 */
int mspc_dcs_read(uint32_t cos_id)
{
	struct mspc_atf_msg_header *p_message_header = NULL;
	int ret = MSPC_ERROR;
	uint32_t image_cnt = 0;
	uint32_t image_size;
	uint32_t buff_size;
	uint32_t size;
	uint32_t type = 0;
	struct dcs_cnt_header *header = NULL;
	struct device *mspc_device = NULL;

	mspc_device = mspc_get_device();
	if (!mspc_device) {
		pr_err("%s:get devices failed!\n", __func__);
		return ret;
	}

	ret = mspc_dcs_read_param_check(cos_id, &type);
	dcs_check_result_and_return(ret);

	if (atomic_inc_return(&g_is_dcs_free) != MSPC_DCS_FREE_CNT) {
		atomic_dec(&g_is_dcs_free);
		ret = MSPC_DCS_MEM_FREE_ERROR;
		goto exit;
	}

	if (g_dcs_image_cnt > MSPC_MAX_DCS_FILES) {
		pr_err("%s: image cnt error(%x)\n", __func__, g_dcs_image_cnt);
		ret = MSPC_DCS_IMAGECNT_ERROR;
		goto exit;
	}
	buff_size = MSPC_DCS_BUFF_SIZE(g_dcs_image_cnt);
	/*
	 * For each dynamic data create CMA buffer and fill it up with dynamic
	 * data index and the corresponding the dynamic data.
	 */
	if (!g_dcs_buff_virt) {
		g_dcs_buff_virt = (void *)dma_alloc_coherent(
			mspc_device,
			(size_t)buff_size,
			&g_dcs_buff_phy, GFP_KERNEL);
		if (!g_dcs_buff_virt) {
			atomic_dec(&g_is_dcs_free);
			pr_err("%s(): dma_alloc_coherent failed\n", __func__);
			ret = MSPC_DCS_MEM_ALLOC_ERROR;
			goto exit;
		}
	}

	/* the src and the dst is the same length. */
	(void)memset_s(g_dcs_buff_virt, (size_t)buff_size,
		       0, (size_t)buff_size);

	p_message_header = (struct mspc_atf_msg_header *)g_dcs_buff_virt;
	mspc_set_atf_msg_header(p_message_header, MSPC_SMC_UPGRADE_DCS);
	/* Set the number of dynamic data in the header structure. */
	header = (struct dcs_cnt_header *)(g_dcs_buff_virt +
					   MSPC_ATF_MESSAGE_HEADER_LEN);
	header->dcs_image_cnt = g_dcs_image_cnt;
	image_size = (uint32_t)(MSPC_ATF_MESSAGE_HEADER_LEN +
				MSPC_DCS_COUNT_HEADER);

	while (image_cnt < g_dcs_image_cnt) {
		size = buff_size - image_size;
		ret = mspc_read_image(type,
				      g_dcs_buff_virt + image_size, &size);
		if (ret != MSPC_OK) {
			pr_err("%s: read image failed, ret=%d, image_id=%d\n",
			       __func__, ret, image_cnt);
			goto error_process;
		}

		pr_err("%s(): image_idx=%d, file size is %d, type is %d\n",
		       __func__, image_cnt, size, type);
		if (size > MSPC_DCS_FILE_MAX_SIZE) {
			pr_err("%s: size overlfow(%x)\n", __func__, size);
			goto error_process;
		}
		header->dcs_image_size[image_cnt] = size;
		image_size += (uint32_t)size;
		if (image_size > buff_size) {
			pr_err("%s: total size overlfow(%x)\n", __func__,
			       image_size);
			goto error_process;
		}
		image_cnt++;
		type++;
	}

	ret = mspc_send_smc_process(p_message_header, g_dcs_buff_phy,
				    image_size,
				    MSPC_ATF_DCS_TIMEOUT, MSPC_SMC_UPGRADE_DCS);
	if (ret == MSPC_OK) {
		pr_err("%s: run successful!\n", __func__);
		atomic_dec(&g_is_dcs_free);
		goto exit;
	}

error_process:
	dma_free_coherent(mspc_device,
			  (size_t)buff_size,
			  g_dcs_buff_virt, g_dcs_buff_phy);
	g_dcs_buff_virt = NULL;
	g_dcs_buff_phy = 0;
	atomic_dec(&g_is_dcs_free);
exit:
	mspc_record_errno(ret);
	return ret;
}

/*
 * @brief      : mspc_dcs_data_load : load the dynamic data to atf.
 * @param[in]  : void
 * @return     : void
 */
void mspc_dcs_data_load(void)
{
	const uint32_t cos_id = MSPC_NATIVE_COS_ID;
	int ret;

	ret = mspc_dcs_read(cos_id);
	if (ret != MSPC_OK)
		pr_err("%s: read dcs failed!\n", __func__);
}

/*
 * @brief      : mspc_free_dcs_mem : free the cma memory allocated for dynmaic
 *               data upgrade when it upgrade completed.
 * @param[in]  : void
 * @return     : void
 */
void mspc_free_dcs_mem(void)
{
	struct device *mspc_device = NULL;

	mspc_device = mspc_get_device();
	if (!mspc_device) {
		pr_err("%s:get devices failed!\n", __func__);
		return;
	}

	if (atomic_inc_return(&g_is_dcs_free) == MSPC_DCS_FREE_CNT) {
		if (g_dcs_buff_virt != NULL) {
			pr_err("%s free DCS CMA.\n", __func__);
			dma_free_coherent(mspc_device,
					  (size_t)MSPC_DCS_BUFF_SIZE(g_dcs_image_cnt), /*lint !e647*/
					  g_dcs_buff_virt, g_dcs_buff_phy);
			g_dcs_buff_virt = NULL;
			g_dcs_buff_phy = 0;
		}
	}
	atomic_dec(&g_is_dcs_free);
}

/*
 * @brief      : mspc_cos_dcs_upgrade :
 *               power up mspc and send dynamic data to atf.
 * @param[in]  : buf: the buf contain infomation of cos index and
 *               processor id for mspc power up.
 * @return     : void
 * @note       : It will call mspc_dcs_read to read all dynamic data from
 *               mspc_img, and then send those data to ATF by SMC call.
 *               After that, the mspc will be powered on, it will send
 *               a request for dynamic data upgrade to ATF, then ATF will
 *               send the dynamic data to mspc by IPC&Mailbox.
 *               When all dynmaic data is reay, mspc will set the
 *               state of mspc to MSPC_STATE_DCS_UPGRADE_DONE state
 *               to indicate the dynamic data is upgrade completed.
 */
int32_t mspc_upgrade_dcs(const int8_t *para, uint32_t size)
{
	int32_t ret;
	/* Currently, we only support cos0 for dynmaic data upgrade. */
	const uint32_t cos_id = MSPC_NATIVE_COS_ID;

	ret = mspc_power_func(para, size,
			      MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret != MSPC_OK) {
		pr_err("%s: power off mspc failed!\n", __func__);
		return ret;
	}

	ret = mspc_dcs_read(cos_id);
	if (ret != MSPC_OK) {
		pr_err("%s: read dcs failed!\n", __func__);
		return ret;
	}

	ret = mspc_power_func(para, size,
			      MSPC_POWER_ON_DCS, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s: power on mspc failed!\n", __func__);
		return ret;
	}

	ret = mspc_wait_state(MSPC_STATE_DCS_UPGRADE_DONE,
			      MSPC_WAIT_DCS_DONE_TIMEOUT);
	if (ret != MSPC_OK) {
		pr_err("%s: power on mspc failed!\n", __func__);
		return ret;
	}

	pr_err("%s:run successful!\n", __func__);
	return ret;
}
