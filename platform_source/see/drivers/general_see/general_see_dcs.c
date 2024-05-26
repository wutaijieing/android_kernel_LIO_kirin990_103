/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: This file is for dynamic code upgrade process, including
 *              send dynamic data to ATF by SMC call, and read encrypted
 *              dynamic data from RPMB.
 * Create: 2018-12-12
 */
#include "general_see_dcs.h"
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
#include <linux/of_gpio.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <securec.h>
#include "general_see.h"
#include "general_see_power.h"
#include "general_see_upgrade.h"

static atomic_t g_is_dcs_free = ATOMIC_INIT(0);
static char *g_dcs_buff_virt;
static phys_addr_t g_dcs_buff_phy;
static unsigned int g_dcs_image_cnt;

/*
 * @brief      : parse_dcs_id_from_name : get the dynamic image number
 *               from hisee_img.
 * @param[in]  : curr_file_info: the dynamic image name get from hisee_img.
 * @param[in]  : dcs_image_cnt_array: the result of dynamic image number.
 * @return     : HISEE_OK is success, others are failure.
 */
int parse_dcs_id_from_name(const struct img_file_info *curr_file_info,
			   unsigned int *dcs_image_cnt_array)
{
	unsigned int curr_dcs_id;
	unsigned int cos_id;

	if (!curr_file_info || !dcs_image_cnt_array) {
		pr_err("%s():input params error!\n", __func__);
		return HISEE_DCS_INPUT_PARAM_ERROR;
	}

	/* get the dcs id from image name, range is [0,9] */
	curr_dcs_id = hisee_atoi(curr_file_info->name + HISEE_IMG_DCS_IDX_OFF);
	if (curr_dcs_id >= HISEE_MAX_DCS_ID_NUMBER) {
		pr_err("%s():dynamic image index is invalid(%u)\n",
		       __func__, curr_dcs_id);
		return HISEE_DCS_IDX_ERROR;
	}

	/* group dcs image index by cos_id, then counter add one */
	cos_id = curr_file_info->name[HISEE_IMG_DCS_COS_ID_OFF] - '0';
	if (cos_id >= MAX_COS_IMG_ID) {
		pr_err("%s():dynamic image cos index is invalid(%u)\n",
		       __func__, cos_id);
		return HISEE_DCS_COS_ID_ERROR;
	}

	dcs_image_cnt_array[cos_id]++;
	if (dcs_image_cnt_array[cos_id] > HISEE_MAX_DCS_FILES) {
		pr_err("%s():dynamic image cnt is invalid(%u)\n",
		       __func__, dcs_image_cnt_array[cos_id]);
		return HISEE_DCS_ID_LARGE_ERROR;
	}

	return HISEE_OK;
}

/*
 * @brief      : hisee_dcs_read_param_check.
 * @param[in]  : cos_id: the cos index for current dynamic data upgrade process.
 * @param[in]  : type: the image file type.
 * @return     : HISEE_OK is success, others are failure.
 */
static int hisee_dcs_read_param_check(unsigned int cos_id,
				      enum hisee_img_file_type *type)
{
	unsigned int dcs_image_cnt;

	if (cos_id >= MAX_COS_IMG_ID) {
		pr_err("%s(): cos id is invalid(%u)\n", __func__, cos_id);
		return set_errno_then_exit(HISEE_DCS_COS_ID_ERROR);
	}

	/*
	 * For cos0, the dynamic image name is DCS0_00~DCS0_09, cos1 is
	 * DCS1_00~DCS1_09, the other cosX, it is like DCSX_00~DCSX_09
	 */
	*type = DCS0_IMG_TYPE + (cos_id * HISEE_MAX_DCS_FILES);

	dcs_image_cnt = get_hisee_data_ptr()->hisee_img_head.dcs_image_cnt[cos_id];
	if (dcs_image_cnt == 0 || dcs_image_cnt > HISEE_MAX_DCS_FILES) {
		pr_err("%s(): dynamic data count invalid(%u)\n",
		       __func__, dcs_image_cnt);
		return set_errno_then_exit(HISEE_DCS_IMAGECNT_ERROR);
	}

	g_dcs_image_cnt = dcs_image_cnt;
	pr_info("%s(): the dynamic data count is %u\n",
		__func__, dcs_image_cnt);

	return HISEE_OK;
}

/*
 * @brief      : hisee_dcs_read : power up hisee and send dynamic data to atf.
 * @param[in]  : cos_id:the cos index for current dynamic data upgrade process.
 * @return     : HISEE_OK is success, others are failure.
 * @note       : It will call hisee_dcs_read to read all dynamic data from
 *               hisee_img, and then send those data to ATF by SMC call.
 */
int hisee_dcs_read(unsigned int cos_id)
{
	struct atf_message_header *p_message_header = NULL;
	int ret;
	unsigned int image_cnt = 0;
	unsigned int image_size;
	size_t buf_size;
	enum hisee_img_file_type type = 0;
	struct dcs_cnt_header *header = NULL;
	struct hisee_module_data *hisee_data_ptr = NULL;

	hisee_data_ptr = get_hisee_data_ptr();
	ret = hisee_dcs_read_param_check(cos_id, &type);
	dcs_check_result_and_return(ret);

	if (atomic_inc_return(&g_is_dcs_free) != HISEE_DCS_FREE_CNT) {
		atomic_dec(&g_is_dcs_free);
		return set_errno_then_exit(HISEE_DCS_MEM_FREE_ERROR);
	}

	if (g_dcs_image_cnt > HISEE_MAX_DCS_FILES) {
		pr_err("%s(): dcs image cnt err, cnt %d\n", __func__, g_dcs_image_cnt);
		return set_errno_then_exit(HISEE_DCS_IMAGECNT_ERROR);
	}

	buf_size = hisee_dcs_buff_size(g_dcs_image_cnt);
	/*
	 * For each dynamic data create CMA buffer and fill it up with dynamic
	 * data index and the corresponding the dynamic data.
	 */
	if (!g_dcs_buff_virt) {
		g_dcs_buff_virt = (char *)dma_alloc_coherent(
			hisee_data_ptr->cma_device,
			buf_size,
			&g_dcs_buff_phy, GFP_KERNEL);
		if (!g_dcs_buff_virt) {
			atomic_dec(&g_is_dcs_free);
			pr_err("%s(): dma_alloc_coherent failed\n", __func__);
			return set_errno_then_exit(HISEE_DCS_MEM_ALLOC_ERROR);
		}
	}

	/* the src and the dst is the same length. */
	(void)memset_s(g_dcs_buff_virt, buf_size, 0, buf_size);

	p_message_header = (struct atf_message_header *)g_dcs_buff_virt;
	set_message_header(p_message_header, CMD_UPGRADE_DCS);
	/* Set the number of dynamic data in the header structure. */
	header = (struct dcs_cnt_header *)(g_dcs_buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN);
	header->dcs_image_cnt = g_dcs_image_cnt;
	image_size = (unsigned int)(HISEE_ATF_MESSAGE_HEADER_LEN +
				    HISEE_DCS_COUNT_HEADER);

	while (image_cnt < g_dcs_image_cnt) {
		ret = filesys_hisee_read_image(type,
					       (g_dcs_buff_virt + image_size),
					       buf_size - image_size);
		if (ret < HISEE_OK ||
		    (unsigned int)ret > HISEE_DCS_MAX_IMG_SIZE) {
			pr_err("%s: read image failed, ret=%d, image_id=%u\n",
			       __func__, ret, image_cnt);
			goto error_process;
		}

		pr_info("%s(): image_idx=%d, file size is %u, type is %d\n",
			__func__, image_cnt, ret, type);
		header->dcs_image_size[image_cnt] = (unsigned int)ret;
		image_size += (unsigned int)(ret);
		if (image_size > buf_size) {
			pr_err("%s: image size overflow(%x)\n", __func__, image_size);
			goto error_process;
		}
		image_cnt++;
		type++;
	}

	ret = send_smc_process(p_message_header, g_dcs_buff_phy, image_size,
			       HISEE_ATF_DCS_TIMEOUT, CMD_UPGRADE_DCS);
	check_result_and_goto(ret, error_process);

	atomic_dec(&g_is_dcs_free);
	check_and_print_result();
	return set_errno_then_exit(ret);
error_process:
	dma_free_coherent(hisee_data_ptr->cma_device,
			  (size_t)hisee_dcs_buff_size(g_dcs_image_cnt),
			  g_dcs_buff_virt, g_dcs_buff_phy);
	g_dcs_buff_virt = NULL;
	g_dcs_buff_phy = 0;
	atomic_dec(&g_is_dcs_free);
	return set_errno_then_exit(ret);
}

/*
 * @brief      : hisee_dcs_data_load : load the dynamic data to atf.
 * @param[in]  : void
 * @return     : void
 */
void hisee_dcs_data_load(void)
{
	unsigned int cos_id;
	int ret;

	for (cos_id = 0; cos_id < HISEE_SUPPORT_COS_FILE_NUMBER; cos_id++) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
		/*
		 * If there is no image for current cos id
		 * in hisee_img, bypass upgrading.
		 */
		if (get_hisee_data_ptr()->hisee_img_head.is_cos_exist[cos_id] !=
		    HISEE_COS_EXIST) {
			pr_info("%s: there is no cos%d image in hisee_img!\n",
				__func__, cos_id);
			continue;
		}
#endif
		ret = hisee_dcs_read(cos_id);
		check_result_and_goto(ret, exit);
	}
exit:
	return;
}

/*
 * @brief      : hisee_free_dcs_mem : free the cma memory allocated for dynamic
 *               data upgrade when it upgrade completed.
 * @param[in]  : void
 * @return     : void
 */
void hisee_free_dcs_mem(void)
{
	if (atomic_inc_return(&g_is_dcs_free) == HISEE_DCS_FREE_CNT) {
		if (g_dcs_buff_virt) {
			pr_info("%s free DCS CMA.\n", __func__);
			dma_free_coherent(
				get_hisee_data_ptr()->cma_device,
				(size_t)hisee_dcs_buff_size(g_dcs_image_cnt),
				g_dcs_buff_virt, g_dcs_buff_phy);
			g_dcs_buff_virt = NULL;
			g_dcs_buff_phy = 0;
		}
	}
	atomic_dec(&g_is_dcs_free);
}

/*
 * @brief      : hisee_cos_dcs_upgrade :
 *               power up hisee and send dynamic data to atf.
 * @param[in]  : buf: the buf contain information of cos index and
 *               processor id for hisee power up.
 * @return     : void
 * @note       : It will call hisee_dcs_read to read all dynamic data from
 *               hisee_img, and then send those data to ATF by SMC call.
 *               After that, the hisee will be powered on, it will send
 *               a request for dynamic data upgrade to ATF, then ATF will
 *               send the dynamic data to hisee by IPC&Mailbox.
 *               When all dynamic data is reay, hisee will set the
 *               state of hisee to HISEE_STATE_DCS_UPGRADE_DONE state
 *               to indicate the dynamic data is upgrade completed.
 */
void hisee_cos_dcs_upgrade(const void *buf)
{
	int ret;
	unsigned int cos_id = MAX_COS_IMG_ID;
	unsigned int process_id = 0;

	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	dcs_check_result_and_return_void(ret);

	/* Currently, we only support cos0 for dynamic data upgrade. */
	if (cos_id != COS_IMG_ID_0) {
		pr_err("%s(): cos_id=%u bypass dcs Upgrade.\n",
		       __func__, cos_id);
		return;
	}

	ret = hisee_poweroff_func(buf, HISEE_PWROFF_LOCK);
	dcs_check_result_and_return_void(ret);

	ret = hisee_dcs_read(cos_id);
	dcs_check_result_and_return_void(ret);

	ret = hisee_poweron_booting_func(buf, 0);
	dcs_check_result_and_return_void(ret);

	ret = wait_hisee_ready(HISEE_STATE_DCS_UPGRADE_DONE,
			       HISEE_ATF_GENERAL_TIMEOUT);
	dcs_check_result_and_return_void(ret);

	check_and_print_result_with_cosid();
}
