/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Drivers for Crypto Core upgrading process.
 * Create: 2019/11/17
 */
#include "crypto_core_upgrade.h"
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_drivers/rpmb.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#include <vendor_rpmb.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_dcs.h"
#include "crypto_core_errno.h"
#include "crypto_core_fs.h"
#include "crypto_core_power.h"
#include "crypto_core_smc.h"

/* Cos id is "1", vote id is "0" (MSPC_UPGRADE). */
#define MSPC_UPGRADE_NATIVECOS_POWER_PARAM " 10"
/* Cos id is "5", vote id is "0" (MSPC_UPGRADE). */
#define MSPC_UPGRADE_FLASHCOS_POWER_PARAM  " 50"
#define MSPC_WAIT_ULOADER_TIME      200 /* 200ms */

#define MSPC_WAIT_POWER_DOWN        200 /* 200ms */
#define MSPC_WAIT_POWER_UPGRADE     200 /* 200ms */

#define MSPC_IMG_MIN_YEAR       2016
#define MSPC_IMG_MAX_YEAR       2050
#define MSPC_IMG_MAX_MONTH      12
#define MSPC_IMG_MAX_DAY        31
#define MSPC_IMG_MAX_HOUR       24
#define MSPC_IMG_MAX_MINUTE     60
#define MSPC_IMG_MAX_SECOND     60

/* Retry 3 more times if upgrade failed! */
#define MSPC_UPGRADE_RETRY_TIMES       3

struct mspc_img_name_map {
	uint32_t img_type;
	int8_t name[MSPC_IMG_SUB_FILE_NAME_LEN];
};

static struct mutex g_upgrade_mutex;
static struct wakeup_source *g_mspc_wake_lock = NULL;
static bool g_mspc_is_upgrading = false;
struct mspc_img_header g_img_header;
static struct mspc_ufs_flag_info g_ufs_flag;

static struct mspc_img_name_map g_img_name_info[] = {
	{ SLOADER_IMG_TYPE,      "SLOADER" },
	{ ULOADER_IMG_TYPE,      "ULOADER" },
	{ COS_IMG_TYPE,          "COS" },
	{ NATIVE_COS_IMG_TYPE,   "COS1" },
	{ COS_FLASH_IMG_TYPE,    "COS5" },
	{ DCS0_IMG_TYPE,         "DCS0_00" },
	{ DCS1_IMG_TYPE,         "DCS0_01" },
	{ DCS2_IMG_TYPE,         "DCS0_02" },
	{ DCS3_IMG_TYPE,         "DCS0_03" },
	{ DCS4_IMG_TYPE,         "DCS0_04" },
	{ DCS5_IMG_TYPE,         "DCS0_05" },
	{ DCS6_IMG_TYPE,         "DCS0_06" },
	{ DCS7_IMG_TYPE,         "DCS0_07" },
	{ DCS8_IMG_TYPE,         "DCS0_08" },
	{ DCS9_IMG_TYPE,         "DCS0_09" },
	{ DCS10_IMG_TYPE,        "DCS1_00" },
	{ DCS11_IMG_TYPE,        "DCS1_01" },
	{ DCS12_IMG_TYPE,        "DCS1_02" },
	{ DCS13_IMG_TYPE,        "DCS1_03" },
	{ DCS14_IMG_TYPE,        "DCS1_04" },
	{ DCS15_IMG_TYPE,        "DCS1_05" },
	{ DCS16_IMG_TYPE,        "DCS1_06" },
	{ DCS17_IMG_TYPE,        "DCS1_07" },
	{ DCS18_IMG_TYPE,        "DCS1_08" },
	{ DCS19_IMG_TYPE,        "DCS1_09" },
	{ OTP_IMG_TYPE,          "OTP" },
	{ OTP0_IMG_TYPE,         "OTP0" },
	{ OTP1_IMG_TYPE,         "OTP1" },
};

struct mspc_img_header *mspc_get_image_header(void)
{
	return &g_img_header;
}

bool mspc_check_is_upgrading(void)
{
	return g_mspc_is_upgrading;
}

void mspc_set_upgrading_flag(bool cfg)
{
	g_mspc_is_upgrading = cfg;
}

static DECLARE_COMPLETION(g_mspc_upgrade_complete);

/* signals a thread waiting on this completion */
void mspc_trigger_upgrade(void)
{
	complete(&g_mspc_upgrade_complete);
}

static int32_t mspc_retry_to_upgrade(const int8_t *para,
				     enum mspc_power_operation op_type)
{
	int32_t ret;

	ret = mspc_exception_to_reset_rpmb();
	if (ret != 0) {
		pr_err("%s():reset rpmb failed! ret=%d\n", __func__, ret);
		ret = mspc_get_recorded_errno();
		return ret;
	}

	ret = mspc_power_func(para, strlen(para),
			      MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret != MSPC_OK) {
		pr_err("%s: power off msp core failed!\n", __func__);
		return ret;
	}

	/* Delay 200ms to wait msp core down. */
	msleep(MSPC_WAIT_POWER_DOWN);

	ret = mspc_power_func(para, strlen(para),
			      op_type, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s: power on msp core failed!\n", __func__);
		return ret;
	}

	/* Delay 200ms to wait msp core ready to upgrade. */
	msleep(MSPC_WAIT_POWER_UPGRADE);

	return ret;
}

static int32_t mspc_get_image_index(uint32_t type,
				    struct mspc_img_header *header,
				    uint32_t *index)
{
	uint32_t i;
	int8_t *img_name = NULL;
	const uint32_t name_size = MSPC_IMG_SUB_FILE_NAME_LEN;

	for (i = 0; i < ARRAY_SIZE(g_img_name_info); i++) {
		if (type == g_img_name_info[i].img_type) {
			img_name = g_img_name_info[i].name;
			break;
		}
	}

	if (i == ARRAY_SIZE(g_img_name_info)) {
		pr_err("%s:Invalid image type:%u\n", __func__, type);
		return MSPC_INVALID_PARAM_ERROR;
	}

	for (i = 0; i < header->file_cnt; i++) {
		if (!strncmp(img_name, header->file[i].name, name_size))
			break;
	}

	if (i == header->file_cnt) {
		pr_err("%s:Cannot find image:%u!\n", __func__, type);
		return MSPC_IMG_SUB_FILE_ABSENT_ERROR;
	}

	*index = i;
	return MSPC_OK;
}

int32_t mspc_read_image(uint32_t type, int8_t *buf, uint32_t *size)
{
	int32_t ret;
	uint32_t index = 0;
	struct mspc_access_info access_info;

	if (!buf || !size) {
		pr_err("%s:NULL pointer!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	(void)memset_s(&access_info, sizeof(access_info),
		       0, sizeof(access_info));
	ret = mspc_get_image_index(type, &g_img_header, &index);
	if (ret != MSPC_OK) {
		pr_err("%s:get image index failed!\n", __func__);
		return ret;
	}

	access_info.access_type = MSPC_IMAGE_READ;
	access_info.size = g_img_header.file[index].size;
	access_info.offset = g_img_header.file[index].offset;
	ret = mspc_access_partition(buf, *size, &access_info);
	if (ret != MSPC_OK) {
		pr_err("%s:read flash failed!\n", __func__);
		return ret;
	}

	*size = g_img_header.file[index].size;
	return ret;
}

int32_t mspc_read_and_send_img(uint32_t type, uint32_t cmd)
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

	buff_virt = (int8_t *)dma_alloc_coherent(device, MSPC_SHARE_BUFF_SIZE,
		&buff_phy, GFP_KERNEL);
	if (!buff_virt) {
		pr_err("%s: Alloc memory failed!\n", __func__);
		return MSPC_NO_RESOURCES_ERROR;
	}

	(void)memset_s(buff_virt, MSPC_SHARE_BUFF_SIZE,
		       0, MSPC_SHARE_BUFF_SIZE);
	p_msg_header = (struct mspc_atf_msg_header *)buff_virt;
	mspc_set_atf_msg_header(p_msg_header, cmd);
	p_msg_header->cos_id = (type == COS_FLASH_IMG_TYPE) ? MSPC_FLASH_COS_ID : MSPC_NATIVE_COS_ID;

	/*
	 * img_size is the buffer size for input and
	 * is the real data size for output.
	 */
	img_size = MSPC_SHARE_BUFF_SIZE - MSPC_ATF_MESSAGE_HEADER_LEN;
	ret = mspc_read_image(type, buff_virt + MSPC_ATF_MESSAGE_HEADER_LEN,
			      &img_size);
	if (ret != MSPC_OK) {
		pr_err("%s: read image failed!\n", __func__);
		goto err_free_memory;
	}

	img_size += MSPC_ATF_MESSAGE_HEADER_LEN;
	ret = mspc_send_smc_process(p_msg_header, buff_phy,
				    img_size, MSPC_ATF_UPGRADE_TIMEOUT, cmd);
	if (ret != MSPC_OK) {
		pr_err("%s:send image to ATF failed!\n", __func__);
		goto err_free_memory;
	}

err_free_memory:
	dma_free_coherent(device, MSPC_SHARE_BUFF_SIZE, buff_virt, buff_phy);
	return ret;
}

static int32_t mspc_process_image(uint32_t type)
{
	int32_t ret;
	enum mspc_smc_cmd smc_cmd;
	uint32_t img_type;

	/* Load uloader first. */
	smc_cmd = MSPC_SMC_UPGRADE_ULOADER;
	ret = mspc_read_and_send_img(ULOADER_IMG_TYPE, smc_cmd);
	if (ret != MSPC_OK) {
		pr_err("%s: load uloader failed!\n", __func__);
		return ret;
	}

	/*
	 * Delay 200ms for uloader boot complete. When uloader boot
	 * successful, it will send cos upgrade request to ATF, then
	 * the download request will be ready.
	 */
	msleep(MSPC_WAIT_ULOADER_TIME);

	/* Then load cos to ATF and upgrade. */
	if (type == MSPC_UPGRADE_FLASHCOS)
		img_type = COS_FLASH_IMG_TYPE;
	else
		img_type = NATIVE_COS_IMG_TYPE;
	smc_cmd = MSPC_SMC_UPGRADE_COS;
	ret = mspc_read_and_send_img(img_type, smc_cmd);
	if (ret != MSPC_OK)
		pr_err("%s: upgrade cos failed!\n", __func__);
	else
		pr_err("%s: upgrade cos successful!\n", __func__);

	return ret;
}

static const int8_t *mspc_get_upgrade_power_param(uint32_t type)
{
	const int8_t *para = NULL;

	if (type == MSPC_UPGRADE_FLASHCOS)
		para = MSPC_UPGRADE_FLASHCOS_POWER_PARAM;
	else
		para = MSPC_UPGRADE_NATIVECOS_POWER_PARAM;

	return para;
}

#define MSPC_WAIT_DOWN_TIME	   500000 /* 500s */
static int32_t mspc_upgrade_prepare_power(uint32_t type,
					  enum mspc_power_operation *op_type)
{
	int32_t ret;
	uint32_t mspc_lcs = 0;
	const int8_t *para = NULL;

	ret = mspc_get_lcs_mode(&mspc_lcs);
	if (ret != MSPC_OK) {
		pr_err("%s: get lcs mode failed!\n", __func__);
		return ret;
	}
	if (mspc_lcs == MSPC_SM_MODE_MAGIC)
		*op_type = MSPC_POWER_ON_UPGRADE_SM;
	else
		*op_type = MSPC_POWER_ON_UPGRADE;

	ret = mspc_wait_state(MSPC_STATE_POWER_DOWN, MSPC_WAIT_DOWN_TIME);
	if (ret != MSPC_OK) {
		pr_err("%s:wait for mspc power down timeout!\n", __func__);
		return ret;
	}

	para = mspc_get_upgrade_power_param(type);
	ret = mspc_power_func(para, strlen(para), *op_type, MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s: power on msp core failed!\n", __func__);
		return ret;
	}

	/* Delay to wait msp core ready to upgrade. */
	msleep(MSPC_WAIT_POWER_UPGRADE);
	pr_err("%s:prepare for upgrading successful!\n", __func__);

	return ret;
}

static bool mspc_is_timestamp_valid(const union mspc_timestamp_info *time)
{
	if (time->timestamp.year < MSPC_IMG_MIN_YEAR ||
	    time->timestamp.year >= MSPC_IMG_MAX_YEAR) {
		return false;
	}

	if (time->timestamp.month > MSPC_IMG_MAX_MONTH)
		return false;

	if (time->timestamp.day > MSPC_IMG_MAX_DAY)
		return false;

	if (time->timestamp.hour >= MSPC_IMG_MAX_HOUR)
		return false;

	if (time->timestamp.minute >= MSPC_IMG_MAX_MINUTE)
		return false;

	if (time->timestamp.second >= MSPC_IMG_MAX_SECOND)
		return false;

	return true;
}

static bool mspc_is_new_image(const struct mspc_ufs_flag_info *curr,
			      const struct mspc_ufs_flag_info *pre)
{
	const uint32_t cos_id = MSPC_NATIVE_COS_ID;

	/* Check whethe previous info is invalid. */
	if (pre->magic != MSPC_SW_VERSION_MAGIC ||
	    pre->timestamp.value == 0)
		return true;

	/* Check previous info timestamp. */
	if (!mspc_is_timestamp_valid(&(pre->timestamp)))
		return true;

	/* Check previous version magic. */
	if (pre->magic != MSPC_SW_VERSION_MAGIC)
		return true;

	/* Check current previous version magic. */
	if (curr->magic != MSPC_SW_VERSION_MAGIC)
		return false;

	/* Check image version firstly. The version is 0 so far. */
	if (curr->img_ver[cos_id].version > pre->img_ver[cos_id].version)
		return true;

	if (curr->img_ver[cos_id].version < pre->img_ver[cos_id].version)
		return false;

	/* If version is equal, compare the timestamp. */
	if (curr->timestamp.value > pre->timestamp.value)
		return true;

	return false;
}

int32_t mspc_parse_image_header(void)
{
	int32_t ret;

	(void)memset_s(&g_img_header, sizeof(g_img_header),
		0, sizeof(g_img_header));

	ret = mspc_get_and_parse_img_header(&g_img_header);
	if (ret != MSPC_OK)
		pr_err("%s:get mspc image header failed!\n", __func__);
	return ret;
}

static int32_t mspc_update_ufs_flag(void)
{
	/* Only support natvie cos. */
	const uint32_t cos_id = MSPC_NATIVE_COS_ID;
	int32_t ret;

	(void)memset_s(&g_ufs_flag, sizeof(g_ufs_flag),
		       0, sizeof(g_ufs_flag));

	ret = mspc_parse_image_header();
	if (ret != MSPC_OK) {
		pr_err("%s:parse mspc image header failed!\n", __func__);
		return ret;
	}

	mspc_parse_timestamp(g_img_header.timestamp,
			     MSPC_IMG_TIME_STAMP_LEN,
			     &(g_ufs_flag.timestamp));
	g_ufs_flag.img_ver[cos_id].version = g_img_header.img_ver[cos_id].version;
	if (!mspc_is_timestamp_valid(&(g_ufs_flag.timestamp))) {
		pr_err("%s:Invalid timestamp in header!\n", __func__);
		return MSPC_IMG_HEADER_TIMESTAMP_ERROR;
	}

	g_ufs_flag.magic = MSPC_SW_VERSION_MAGIC;
	g_ufs_flag.header_version = MSPC_UFS_FLAG_HEADER_VER1;

	return MSPC_OK;
}

int32_t mspc_check_new_image(bool *is_new)
{
	int32_t ret;
	struct mspc_ufs_flag_info previous = {0};
	struct mspc_access_info info;

	if (!is_new) {
		pr_err("%s:NULL pointer!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	ret = mspc_update_ufs_flag();
	if (ret != MSPC_OK) {
		pr_err("%s:update ufs flag failed!\n", __func__);
		return ret;
	}

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));
	info.access_type = MSPC_FLAG_INFO_READ;
	ret = mspc_access_partition((int8_t *)&previous,
				    sizeof(previous), &info);
	if (ret != MSPC_OK) {
		pr_err("%s:get previous image info failed!\n", __func__);
		return ret;
	}

	if (mspc_is_new_image(&g_ufs_flag, &previous)) {
		*is_new = true;
		return MSPC_OK;
	}

	*is_new = false;
	return MSPC_OK;
}

static int32_t mspc_upgrade_prepare_info(int32_t type)
{
	int32_t ret;
	bool is_new_cos = false;
	uint32_t upgrade_run_flag = MSPC_IMAGE_UPGRADE_RUNNING;
	struct mspc_access_info info;

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));

	/*
	 * If type is chip test or boot exception,
	 * donot need to check new image.
	 */
	if (type == MSPC_UPGRADE_NORMAL) {
		ret = mspc_check_new_image(&is_new_cos);
		if (ret != MSPC_OK) {
			pr_err("%s:check new image failed!\n", __func__);
			return ret;
		}
		if (!is_new_cos) {
			pr_err("%s:There is no new image!\n", __func__);
			return MSPC_OLD_IMAGE_ERROR;
		}
	} else {
		ret = mspc_parse_image_header();
		if (ret != MSPC_OK) {
			pr_err("%s:parse mspc image header failed!\n", __func__);
			return ret;
		}
	}

	info.access_type = MSPC_UPGRADE_FLAG_WRITE;
	ret = mspc_access_partition((int8_t *)&upgrade_run_flag,
				    sizeof(upgrade_run_flag),
				    &info);
	if (ret != MSPC_OK)
		pr_err("%s: write run flag failed! ret:%x\n", __func__, ret);

	return ret;
}

static int32_t mspc_upgrade_process(uint32_t type,
				    enum mspc_power_operation op_type)
{
	int32_t ret;
	uint32_t retry = 0;
	const int8_t *para = NULL;

	para = mspc_get_upgrade_power_param(type);

	do {
		ret = mspc_process_image(type);
		if (ret == MSPC_OK)
			return ret;

		pr_err("%s:upgrade image failed! retry=%d\n", __func__, retry);

		if (mspc_retry_to_upgrade(para, op_type) != MSPC_OK) {
			pr_err("%s:reset failed!\n", __func__);
			break;
		}

		retry++;
	} while (retry < MSPC_UPGRADE_RETRY_TIMES);

	return ret;
}

static int32_t mspc_update_upgrading_info(void)
{
	int32_t ret;
	struct mspc_access_info info;

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));

	info.access_type = MSPC_FLAG_INFO_WRITE;
	ret = mspc_access_partition((int8_t *)&g_ufs_flag,
				    sizeof(g_ufs_flag), &info);
	if (ret != MSPC_OK) {
		pr_err("%s: write version to ufs failed!\n", __func__);
		return ret;
	}

	return ret;
}

int32_t mspc_upgrade_image(uint32_t type)
{
	int32_t ret, ret1;
	const int8_t *para = NULL;
	enum mspc_power_operation op_type = MSPC_POWER_ON_UPGRADE_SM;

	para = mspc_get_upgrade_power_param(type);
	mutex_lock(&g_upgrade_mutex);
	__pm_stay_awake(g_mspc_wake_lock);

	ret = mspc_upgrade_prepare_power(type, &op_type);
	if (ret != MSPC_OK) {
		pr_err("%s: prepare power for upgrade failed!\n", __func__);
		goto exit;
	}

	ret = mspc_upgrade_prepare_info(type);
	if (ret != MSPC_OK) {
		/* If there is no new image, exit with success. */
		if (ret == MSPC_OLD_IMAGE_ERROR)
			ret = MSPC_OK;
		goto err_power_off;
	}

	ret = mspc_upgrade_process(type, op_type);
	if (ret != MSPC_OK) {
		pr_err("%s: upgrade image failed!\n", __func__);
		goto err_power_off;
	}
	pr_err("%s():image has been upgraded!\n", __func__);

	/* If upgrade success, store the current image info to ufs. */
	ret = mspc_update_upgrading_info();
	if (ret != MSPC_OK) {
		pr_err("%s: update mspc upgrading info failed!\n", __func__);
		goto err_power_off;
	}

#ifdef CONFIG_CRYPTO_CORE_SUPPORT_DCS
	if (type != MSPC_UPGRADE_FLASHCOS)
		ret = mspc_upgrade_dcs(para, strlen(para));
#endif

err_power_off:
	ret1 = mspc_power_func(para, strlen(para),
			       MSPC_POWER_OFF, MSPC_POWER_CMD_OFF);
	if (ret1 != MSPC_OK) {
		pr_err("%s: power off msp core failed!\n", __func__);
		ret = ret1;
	}

exit:
	__pm_relax(g_mspc_wake_lock);
	mutex_unlock(&g_upgrade_mutex);
	mspc_record_errno(ret);
	if (ret == MSPC_OK)
		pr_err("%s(): mspc upgrade image successful!\n", __func__);
	else
		pr_err("%s(): mspc upgrade image failed!\n", __func__);
	return ret;
}

/* clear ufs flag, so that mspc will be upgraded when next reboot */
int32_t mspc_upgrade_thread(void *data)
{
	int32_t ret;

	wait_for_completion(&g_mspc_upgrade_complete);
	(void)memset_s(&g_ufs_flag, sizeof(g_ufs_flag), 0, sizeof(g_ufs_flag));
	ret = mspc_update_upgrading_info();
	pr_err("%s:clear ufs flag ret %d\n", __func__, ret);
	return ret;
}

int32_t mspc_init_upgrade(struct device *dev)
{
	mutex_init(&g_upgrade_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_mspc_wake_lock = wakeup_source_register(dev, "hisi-mspc");
#else
	(void)dev;
	g_mspc_wake_lock = wakeup_source_register("hisi-mspc");
#endif
	if (!g_mspc_wake_lock) {
		pr_err("%s: register wakeup source failed\n", __func__);
		return MSPC_REGISTER_WAKEUP_ERROR;
	}
	return MSPC_OK;
}
