/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Base driver for Crypto Core, including initialization.
 * Create: 2019/11/11
 */

#include "crypto_core_internal.h"
#include <platform_include/see/efuse_driver.h>
#include <vendor_rpmb.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/platform_drivers/rpmb.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <securec.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_sctrl_interface.h>
#include "crypto_core_at.h"
#include "crypto_core_autotest.h"
#include "crypto_core_errno.h"
#include "crypto_core_factory.h"
#include "crypto_core_flash.h"
#include "crypto_core_fs.h"
#include "crypto_core.h"
#include "crypto_core_power.h"
#include "crypto_core_smc.h"
#include "crypto_core_smx.h"
#include "crypto_core_upgrade.h"

#define MSPC_WAIT_RPMP_TIMES                20
#define MSPC_WAIT_RPMB_DELAY                500  /* 500ms */

#define MSPC_WAIT_PARTITION_READY_CNT       5
#define MSPC_WAIT_PARTITION_DELAY           500 /* 500ms */

#define MSPC_ERRCODE_MAXLEN                 12

/* secflash fabricator define */
#define SECFLASH_NXP_FABRICATOR_ID    0x4790U
#define SECFLASH_ST_FABRICATOR_ID     0x5354U

/* Begin of msp core file conctrol define. */
#ifdef CONFIG_DFX_DEBUG_FS
static DEVICE_ATTR(mspc_ioctl, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP),
		   mspc_ioctl_show, mspc_ioctl_store);
static DEVICE_ATTR(mspc_power, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP),
		   mspc_powerctrl_show, mspc_powerctrl_store);
#else
static DEVICE_ATTR(mspc_ioctl, (S_IRUSR | S_IWUSR),
		   mspc_ioctl_show, mspc_ioctl_store);
static DEVICE_ATTR(mspc_power, (S_IRUSR | S_IRGRP),
		   mspc_powerctrl_show, NULL);
#endif /* CONFIG_DFX_DEBUG_FS */

static DEVICE_ATTR(mspc_check_ready, (S_IRUSR | S_IRGRP),
		   mspc_check_ready_show, NULL);
static DEVICE_ATTR(mspc_at_ctrl, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP),
		   mspc_at_ctrl_show, mspc_at_ctrl_store);

/* End of msp core file conctrol define. */

struct mspc_module_data g_mspc_data;
struct mspc_encos_header g_encos_header;
static int32_t g_mspc_status = MSPC_MODULE_UNREADY;

int32_t mspc_auto_test_func(const int8_t *buf, int32_t len)
{
#ifdef CONFIG_DFX_DEBUG_FS
	/* check mspc is powered on */
	if (mspc_get_vote_status() != MSPC_VOTE_STATUS_ON) {
		pr_err("%s mspc is not poweron\n", __func__);
		return MSPC_ERROR;
	}
	return mspc_channel_test_func(buf, len);;
#else
	return MSPC_OK;
#endif
}

static struct mspc_driver_func g_mspc_func_list[] = {
	{ "mspc_channel_test",            mspc_auto_test_func },
#ifdef CONFIG_DFX_DEBUG_FS
	{ "mspc_chiptest_rt_run",         mspc_chiptest_rt_run_func },
	{ "mspc_chiptest_rt_stop",        mspc_chiptest_rt_stop_func },
#endif
};

int32_t mspc_filectrl_preprocess(const int8_t *buf, size_t count)
{
	if (unlikely(!buf || count > MSPC_CMD_MAX_LEN)) {
		pr_err("%s:Invalid param:%lu\n", __func__, count);
		return	MSPC_INVALID_PARAM_ERROR;
	}

	if (unlikely(mspc_check_is_upgrading())) {
		pr_err("%s:mspc is upgrading,don't operate mspc!do it later.\n",
		       __func__);
		return MSPC_IS_UPGRADING_ERROR;
	}

	if (unlikely(!g_mspc_data.rpmb_is_ready)) {
		pr_err("%s:rpmb is not ready now!\n", __func__);
		return MSPC_RPMB_MODULE_INIT_ERROR;
	}
	return MSPC_OK;
}

ssize_t mspc_ioctl_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	int32_t ret;
	uint32_t i;
	int8_t *ptr_name = NULL;
	int32_t (*func)(const int8_t *, int32_t) = NULL;

	ret = mspc_filectrl_preprocess(buf, count);
	if (ret != MSPC_OK)
		goto exit;

	for (i = 0; i < ARRAY_SIZE(g_mspc_func_list); i++) {
		ptr_name = g_mspc_func_list[i].func_name;
		if (ptr_name && !strncmp(buf, ptr_name, strlen(ptr_name)))
			break;
	}

	if (unlikely(i == ARRAY_SIZE(g_mspc_func_list) ||
		     !g_mspc_func_list[i].func)) {
		pr_err("%s: cannot find command, i:%d!\n", __func__, i);
		ret = MSPC_INVALID_PARAM_ERROR;
		goto exit;
	}

	func = g_mspc_func_list[i].func;
	mutex_lock(&g_mspc_data.mspc_mutex);
	ret = func(buf + strlen(g_mspc_func_list[i].func_name),
		   count - strlen(g_mspc_func_list[i].func_name));
	pr_err("%s:run function:%s\n", __func__, g_mspc_func_list[i].func_name);
	mutex_unlock(&g_mspc_data.mspc_mutex);
	if (unlikely(ret != MSPC_OK)) {
		pr_err("%s: ioctrl:%s failed, ret=%d\n", __func__,
		       g_mspc_func_list[i].func_name, ret);
		goto exit;
	}

	pr_err("%s:%s run successful!\n", __func__,
	       g_mspc_func_list[i].func_name);
exit:
	mspc_record_errno(ret);
	return (ret == MSPC_OK) ? (int32_t)count : ret;
}

ssize_t mspc_ioctl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int32_t ret;
	int32_t err_code = mspc_get_recorded_errno();
	int8_t data[MSPC_CMD_NAME_LEN] = {0};
	int8_t *err_string = NULL;

	if (!buf) {
		pr_err("%s: NULL pointer!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	if (err_code != MSPC_OK)
		err_string = "mspc error";
	else
		err_string = "no error";

	ret = snprintf_s(data, MSPC_CMD_NAME_LEN, MSPC_ERRCODE_MAXLEN, "%d,",
			 err_code);
	if (ret == MSPC_SECLIB_ERROR) {
		pr_err("%s snprintf err\n", __func__);
		return MSPC_SECUREC_ERROR;
	}

	ret = strncat_s(data, MSPC_CMD_NAME_LEN, err_string, strlen(err_string));
	if (ret != EOK) {
		pr_err("%s: strncat err\n", __func__);
		return MSPC_SECUREC_ERROR;
	}
	ret = memcpy_s(buf, MSPC_BUF_SHOW_LEN, data, MSPC_CMD_NAME_LEN);
	if (ret != EOK) {
		pr_err("%s: memcpy err ret:%d\n", __func__, ret);
		return MSPC_SECUREC_ERROR;
	}

	pr_err("%s\n", buf);
	return strlen(buf);
}

int32_t mspc_get_init_status(void)
{
	return g_mspc_status;
}

void mspc_lock(void)
{
	mutex_lock(&g_mspc_data.mspc_mutex);
}

void mspc_unlock(void)
{
	mutex_unlock(&g_mspc_data.mspc_mutex);
}

void mspc_record_errno(int32_t error)
{
	atomic_set(&g_mspc_data.errno, error);
}

int32_t mspc_get_recorded_errno(void)
{
	return atomic_read(&g_mspc_data.errno);
}

struct device *mspc_get_device(void)
{
	return g_mspc_data.device;
}

struct mspc_encos_header *mspc_get_encos_header(void)
{
	return &g_encos_header;
}

uint32_t crypto_core_get_sm_efuse_pos(void)
{
	return g_mspc_data.sm_efuse_pos;
}

static int32_t mspc_wait_rpmb_ready(void)
{
	uint32_t retry = MSPC_WAIT_RPMP_TIMES;
	const uint32_t timeout = MSPC_WAIT_RPMB_DELAY;

	do {
		if (get_rpmb_init_status() == RPMB_DRIVER_IS_READY) {
			g_mspc_data.rpmb_is_ready = true;
			break;
		}
		msleep(timeout);
		retry--;
	} while (retry > 0);

	if (retry == 0)
		return MSPC_ERROR;

	return MSPC_OK;
}

int32_t mspc_get_lcs_mode(uint32_t *mode)
{
	int32_t ret = MSPC_GET_MSPC_LCS_ERROR;
	uint32_t value[CRYPTO_CORE_EFUSE_GROUP_NUM] = {0};
	uint32_t sm_efuse_pos;
	uint32_t group;
	uint32_t offset;

	if (!mode) {
		pr_err("%s:input invalid!\n", __func__);
		mspc_record_errno(ret);
		return ret;
	}

	sm_efuse_pos = crypto_core_get_sm_efuse_pos();
	group = sm_efuse_pos / CRYPTO_CORE_EFUSE_GROUP_BIT_SIZE;
	offset = sm_efuse_pos % CRYPTO_CORE_EFUSE_GROUP_BIT_SIZE;

	if (group >= CRYPTO_CORE_EFUSE_GROUP_NUM) {
		pr_err("%s:sm_flag_pos invalid!\n", __func__);
		mspc_record_errno(ret);
		return ret;
	}

	ret = get_efuse_hisee_value((uint8_t *)value,
				    CRYPTO_CORE_EFUSE_LENGTH,
				    CRYPTO_CORE_EFUSE_TIMEOUT);
	if (ret != 0) {
		pr_err("%s: read efuse failed,ret=%d\n", __func__, ret);
		ret = MSPC_GET_LCS_VALUE_ERROR;
		mspc_record_errno(ret);
		return ret;
	}

	*mode = ((value[group] >> offset) &
		 CRYPTO_CORE_EFUSE_MASK) == CRYPTO_CORE_EFUSE_SM_MODE ?
		 MSPC_SM_MODE_MAGIC : MSPC_DM_MODE_MAGIC;

	return MSPC_OK;
}

/* set lcs mode for lpmcu */
static void mspc_record_lcs_mode(void)
{
	uint32_t value;
	uint32_t mspc_lcs_mode = 0;
	uint8_t __iomem *addr = NULL;

	if (mspc_get_lcs_mode(&mspc_lcs_mode) != MSPC_OK) {
		pr_err("%s: get_mspc_lcs_mode failed!\n", __func__);
		return;
	}

	addr = ioremap(MSPC_LCS_BANK_REG, sizeof(uint32_t));
	if (!addr) {
		pr_err("%s: ioremap failed!\n", __func__);
		return;
	}
	value = readl(addr);
	if (mspc_lcs_mode == MSPC_DM_MODE_MAGIC)
		value |= BIT(MSPC_LCS_DM_BIT);
	else
		value &= (~BIT(MSPC_LCS_DM_BIT));

	writel(value, addr);
	iounmap(addr);
}

int32_t mspc_load_image2ddr(void)
{
	int8_t *buff_vir = NULL;
	phys_addr_t buff_phy = 0;
	struct mspc_atf_msg_header *p_msg_header = NULL;
	int32_t ret;
	uint32_t size;

	buff_vir = (int8_t *)dma_alloc_coherent(g_mspc_data.device, SIZE_4K,
		&buff_phy, GFP_KERNEL);
	if (!buff_vir) {
		pr_err("%s:malloc memory failed!\n", __func__);
		mspc_record_errno(MSPC_NO_RESOURCES_ERROR);
		return MSPC_NO_RESOURCES_ERROR;
	}
	(void)memset_s(buff_vir, SIZE_4K, 0, SIZE_4K);
	p_msg_header = (struct mspc_atf_msg_header *)buff_vir;
	mspc_set_atf_msg_header(p_msg_header, MSPC_SMC_READ_COS_IMAGE);
	size = MSPC_ATF_MESSAGE_HEADER_LEN;
	p_msg_header->cos_id = MSPC_NATIVE_COS_ID;
	ret = mspc_send_smc_process(p_msg_header, buff_phy, size,
				    MSPC_ATF_READ_IMG_TIMEOUT,
				    MSPC_SMC_READ_COS_IMAGE);
	if (ret != MSPC_OK)
		pr_err("%s:preload native image failed!\n", __func__);
	else
		pr_err("%s:load image success!\n", __func__);

	dma_free_coherent(g_mspc_data.device, SIZE_4K, buff_vir, buff_phy);
	return ret;
}

static int32_t mspc_wait_partition_ready(void)
{
	int8_t fullpath[MSPC_PTN_PATH_BUF_SIZE] = {0};
	uint32_t i, retry;
	uint32_t timeout = MSPC_WAIT_PARTITION_DELAY;
	const char *ptn_name[] = {
		MSPC_ENCOS_PARTITION_NAME, MSPC_FS_PARTITION_NAME
	};
	int32_t ret;

	for (i = 0; i < ARRAY_SIZE(ptn_name); i++) {
		retry = MSPC_WAIT_PARTITION_READY_CNT;
		ret = flash_find_ptn_s(ptn_name[i], fullpath, sizeof(fullpath));
		if (ret != 0) {
			pr_err("%s:find %u failed:%d\n", __func__, i, ret);
			return MSPC_FS_FIND_PTN_ERROR;
		}

		do {
			if(crypto_core_sys_access(fullpath, 0) == 0)
				break;
			msleep(timeout);
			retry--;
		} while (retry > 0);

		if (retry == 0)
			return MSPC_ACCESS_FILE_ERROR;
	}
	return MSPC_OK;
}

static int32_t mspc_read_flash_cos(int8_t *buff, uint32_t *size)
{
	int32_t ret;
	struct mspc_access_info access_info;
	uint32_t id = MSPC_FLASH_COS_ID - MSPC_ENCOS_START_ID;

	(void)memset_s(&access_info, sizeof(access_info),
		       0, sizeof(access_info));

	if (strncmp(g_encos_header.file[id].name,
		    MSPC_FLASH_COS_STRING,
		    strlen(MSPC_FLASH_COS_STRING)) != 0) {
		pr_err("%s:No flash cos!\n", __func__);
		return MSPC_OK;
	}

	access_info.access_type = MSPC_ENCOS_IMAGE_READ;
	access_info.partition_type = MSPC_PARTITON_ENCOS;
	access_info.size = g_encos_header.file[id].size;
	access_info.offset = g_encos_header.file[id].offset;
	ret = mspc_access_partition(buff, *size, &access_info);
	if (ret != MSPC_OK) {
		pr_err("%s:read flash cos failed!\n", __func__);
		return ret;
	}

	*size = g_encos_header.file[id].size;
	return ret;
}

int32_t mspc_load_flash_image(void)
{
	int8_t *buff_vir = NULL;
	phys_addr_t buff_phy = 0;
	struct mspc_atf_msg_header *p_msg_header = NULL;
	int32_t ret;
	uint32_t size = MSPC_SHARE_BUFF_SIZE;

	ret = mspc_get_encos_img_header(&g_encos_header);
	if (ret != MSPC_OK) {
		pr_err("%s:get encos header failed!\n", __func__);
		return ret;
	}

	buff_vir = (int8_t *)dma_alloc_coherent(g_mspc_data.device,
		size, &buff_phy, GFP_KERNEL);
	if (!buff_vir) {
		pr_err("%s:malloc memory failed!\n", __func__);
		mspc_record_errno(MSPC_NO_RESOURCES_ERROR);
		return MSPC_NO_RESOURCES_ERROR;
	}
	(void)memset_s(buff_vir, size, 0, size);
	p_msg_header = (struct mspc_atf_msg_header *)buff_vir;
	mspc_set_atf_msg_header(p_msg_header, MSPC_SMC_READ_COS_IMAGE);

	p_msg_header->cos_id = MSPC_FLASH_COS_ID;
	size -= MSPC_ATF_MESSAGE_HEADER_LEN;
	ret = mspc_read_flash_cos(buff_vir + MSPC_ATF_MESSAGE_HEADER_LEN, &size);
	if (ret != MSPC_OK) {
		pr_err("%s: readl flash cos failed!\n", __func__);
		goto exit;
	}

	size += MSPC_ATF_MESSAGE_HEADER_LEN;
	ret = mspc_send_smc_process(p_msg_header, buff_phy, size,
				    MSPC_ATF_READ_IMG_TIMEOUT,
				    MSPC_SMC_READ_COS_IMAGE);
	if (ret != MSPC_OK) {
		pr_err("%s:load flash image failed!\n", __func__);
		goto exit;
	}

	pr_err("%s:load flash image success!\n", __func__);
exit:
	dma_free_coherent(g_mspc_data.device, MSPC_SHARE_BUFF_SIZE,
			  buff_vir, buff_phy);
	return ret;
}

static bool mspc_run_without_secflash(void)
{
	int32_t len = 0;
	struct device_node *node = NULL;
	const char *run_status = NULL;

	node = of_find_compatible_node(NULL, NULL, "hisilicon,embedded-secflash-device");
	if (!node) {
		pr_err("%s:find compatible failed!\n", __func__);
		return false;
	}

	run_status = of_get_property(node, "run", &len);
	if (!run_status) {
		pr_err("%s:get property run_status err!\n", __func__);
		return false;
	}

	if (strncmp(run_status, "yes", sizeof("yes")) != 0) {
		pr_err("%s:run_status is not yes!\n", __func__);
		return false;
	}

	return true;
}

static bool mspc_check_bypass(void)
{
	int32_t ret;
	uint32_t status_result = SECFLASH_IS_ABSENCE_MAGIC;

	ret = mspc_secflash_is_available(&status_result);
	if (ret != MSPC_OK) {
		pr_err("%s: get secflash dts info fail\n", __func__);
		return false;
	}
	if (status_result == SECFLASH_IS_ABSENCE_MAGIC) {
		pr_info("%s: secflash dts is disable, ", __func__);
		if (mspc_run_without_secflash()) {
			pr_info("%s:but mspc still need to run!\n", __func__);
		} else {
			pr_info("%s:bypass mspc!\n", __func__);
			return true;
		}
	}
	return false;
}

static int32_t mspc_preprocess_thread(void *data)
{
	int32_t ret;
#if defined(MSPC_POWER_ON_IN_ADVANCE_FEATURE) || defined(CONFIG_CRYPTO_CORE_PINCODE)
	const int8_t *param = MSPC_PINCODE_POWER_PARAM;
#endif

	ret = mspc_wait_rpmb_ready();
	if (ret != MSPC_OK) {
		pr_err("%s:wait rpmb ready timeout!\n", __func__);
		goto exit;
	}

	/* record lcs mode to system bank register for lpmcu */
	mspc_record_lcs_mode();

	/* Flash cos is saved in ufs normal partition. */
	ret = mspc_wait_partition_ready();
	if (ret != MSPC_OK) {
		pr_err("%s:wait partition ready failed!\n", __func__);
		goto exit;
	}

	ret = mspc_load_image2ddr();
	if (ret != MSPC_OK) {
		pr_err("%s:load image failed!\n", __func__);
		goto exit;
	}

	/* After loading native image, mspc is ready to use. */
	g_mspc_status = MSPC_MODULE_READY;

	ret = mspc_load_flash_image();
	if (ret != MSPC_OK)
		pr_err("%s:load flash image err %x\n", __func__, ret);

	if (mspc_check_bypass()) {
		pr_err("%s:bypass mspc!\n", __func__);
		goto exit;
	}

	ret = mspc_check_boot_upgrade();
	if (ret != MSPC_OK)
		pr_err("%s:check boot upgrade err %x\n", __func__, ret);

#if defined(MSPC_POWER_ON_IN_ADVANCE_FEATURE) || defined(CONFIG_CRYPTO_CORE_PINCODE)
	ret = mspc_power_func(param, strlen(param), MSPC_POWER_ON_BOOTING,
			      MSPC_POWER_CMD_ON);
	if (ret != MSPC_OK) {
		pr_err("%s: power on mspc failed!\n", __func__);
		goto exit;
	}
	ssleep(MSPC_POWER_OFF_DELAY_TIME);
	ret = mspc_power_func(param, strlen(param), MSPC_POWER_OFF,
			      MSPC_POWER_CMD_OFF);
	if (ret != MSPC_OK)
		pr_err("%s: power off mspc failed!\n", __func__);
#endif
exit:
	pr_err("%s:run ret 0x%x\n", __func__, ret);
	mspc_record_errno(ret);
	return ret;
}

static int32_t mspc_remove(struct platform_device *pdev)
{
	if (g_mspc_data.device) {
		device_remove_file(g_mspc_data.device, &dev_attr_mspc_ioctl);
		device_remove_file(g_mspc_data.device, &dev_attr_mspc_power);
		device_remove_file(g_mspc_data.device,
				   &dev_attr_mspc_check_ready);
		device_remove_file(g_mspc_data.device,
				   &dev_attr_mspc_at_ctrl);
		of_reserved_mem_device_release(g_mspc_data.device);
	}
	return 0;
}

static uint32_t secflash_get_status_by_fabricator_id(uint32_t fabricator_id)
{
	uint32_t status_result;

	if (fabricator_id == SECFLASH_NXP_FABRICATOR_ID)
		status_result = SECFLASH_NXP_EXIST_MAGIC;
	else if (fabricator_id == SECFLASH_ST_FABRICATOR_ID)
		status_result = SECFLASH_ST_EXIST_MAGIC;
	else
		status_result = SECFLASH_ST_EXIST_MAGIC; /* return ST as default */
	return status_result;
}

int32_t mspc_secflash_is_available(uint32_t *status_result)
{
	int32_t ret;
	int32_t len = 0;
	struct device_node *node = NULL;
	const char *status = NULL;
	uint32_t fabricator_id = 0;

	if (!status_result) {
		pr_err("%s:ptr null!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}
	node = of_find_compatible_node(NULL, NULL, "hisilicon,embedded-secflash-device");
	if (!node) {
		pr_err("%s:find compatible failed!\n", __func__);
		return MSPC_SECFLASH_DTS_FOUND_ERROR;
	}

	status = of_get_property(node, "status", &len);
	if (!status) {
		pr_err("%s:get property status err!\n", __func__);
		return MSPC_SECFLASH_DTS_FOUND_ERROR;
	}

	if (strncmp(status, "ok", sizeof("ok")) != 0) {
		pr_err("%s:status is not ok!\n", __func__);
		*status_result = SECFLASH_IS_ABSENCE_MAGIC;
		return MSPC_OK;
	}

	ret = of_property_read_u32(node, "fabricator_id", &fabricator_id);
	if (ret != 0) {
		/* if there is no fabricator_id property, return ST as default */
		pr_err("%s:failed to get fabricator_id resource\n", __func__);
		*status_result = SECFLASH_ST_EXIST_MAGIC;
		return MSPC_OK;
	}

	*status_result = secflash_get_status_by_fabricator_id(fabricator_id);

	return MSPC_OK;
}

static void mspc_read_dts(const struct device *pdevice)
{
	uint32_t dts_u32_value = 0;

	if (!pdevice) {
		pr_err("%s:pdevice is null!\n", __func__);
		return;
	}

	if (of_property_read_u32(pdevice->of_node,
				 "sm_flag_pos",
				 &dts_u32_value) == 0) {
		g_mspc_data.sm_efuse_pos = dts_u32_value;
		pr_err("%s:sm_flag_pos %u\n",
		       __func__, g_mspc_data.sm_efuse_pos);
	} else {
		/* Config an invalid value: 0xFFFFFFFF. */
		g_mspc_data.sm_efuse_pos = 0xFFFFFFFF;
		pr_err("%s:get sm_flag_pos failed!\n", __func__);
	}

	if (of_find_property(pdevice->of_node, "is_fpga", NULL)) {
		g_mspc_data.is_fpga = true;
		pr_err("mspc is on FPGA!.\n");
	} else {
		g_mspc_data.is_fpga = false;
		pr_err("mspc is on non-FPGA!\n");
	}
}

static int32_t mspc_create_files(struct device *pdevice)
{
	int32_t ret = MSPC_OK;

	if (device_create_file(pdevice, &dev_attr_mspc_ioctl) != 0) {
		ret = MSPC_IOCTL_NODE_CREATE_ERROR;
		pr_err("%s:create mspc_ioctl file failed!\n", __func__);
		goto err_exit;
	}

	if (device_create_file(pdevice, &dev_attr_mspc_power) != 0) {
		ret = MSPC_POWER_NODE_CREATE_ERROR;
		pr_err("%s:create mspc_power file failed!\n", __func__);
		goto err_device_remove_file;
	}

	if (device_create_file(pdevice, &dev_attr_mspc_check_ready) != 0) {
		ret = MSPC_POWER_NODE_CREATE_ERROR;
		pr_err("%s:create mspc_check_ready file failed!\n", __func__);
		goto err_device_remove_file1;
	}

	if (device_create_file(pdevice, &dev_attr_mspc_at_ctrl) != 0) {
		ret = MSPC_AT_CTRL_NODE_CREATE_ERROR;
		pr_err("%s:create mspc_at_ctrl file failed!\n", __func__);
		goto err_device_remove_file2;
	}

	return ret;
err_device_remove_file2:
	device_remove_file(pdevice, &dev_attr_mspc_check_ready);
err_device_remove_file1:
	device_remove_file(pdevice, &dev_attr_mspc_power);
err_device_remove_file:
	device_remove_file(pdevice, &dev_attr_mspc_ioctl);
err_exit:
	return ret;
}

static void mspc_remove_files(struct device *pdevice)
{
	device_remove_file(pdevice, &dev_attr_mspc_at_ctrl);
	device_remove_file(pdevice, &dev_attr_mspc_check_ready);
	device_remove_file(pdevice, &dev_attr_mspc_power);
	device_remove_file(pdevice, &dev_attr_mspc_ioctl);
}

#ifdef CONFIG_CRYPTO_CORE_DISABLE_KEY
#define CRYPTO_CORE_PREPROCESS_TASK_ENABLE false
#define CRYPTO_CORE_UPGRADE_TASK_ENABLE    false
#else
#define CRYPTO_CORE_PREPROCESS_TASK_ENABLE true
#define CRYPTO_CORE_UPGRADE_TASK_ENABLE    true
#endif

struct crypto_core_thread {
	int32_t (*threadfn)(void *data);
	const char *name;
	bool enabled;
};

static int32_t mspc_create_thread(void)
{
	const struct crypto_core_thread thread[] = {
		{ mspc_preprocess_thread, "mspc_preprocess_task", CRYPTO_CORE_PREPROCESS_TASK_ENABLE },
		{ mspc_upgrade_thread,    "mspc_upgrade_task",    CRYPTO_CORE_UPGRADE_TASK_ENABLE },
	};
	struct task_struct *task = NULL;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(thread); i++) {
		if (thread[i].enabled) {
			task = kthread_run(thread[i].threadfn, NULL, "%s", thread[i].name);
			if (!task) {
				pr_err("%s:create %s failed!\n", __func__, thread[i].name);
				return MSPC_THREAD_CREATE_ERROR;
			}
		}
	}
	return MSPC_OK;
}

static int32_t __init mspc_probe(struct platform_device *pdev)
{
	struct device *pdevice = &(pdev->dev);
	int32_t ret;

	(void)memset_s((void *)&g_mspc_data,
		       sizeof(g_mspc_data), 0, sizeof(g_mspc_data));

	g_mspc_data.device = pdevice;

	/* Initialize mutex, semaphore etc.. */
	mutex_init(&(g_mspc_data.mspc_mutex));
	mspc_init_smc();
	ret = mspc_init_upgrade(pdevice);
	if (ret != MSPC_OK) {
		pr_err("%s:init_upgrade failed!\n", __func__);
		mspc_record_errno(ret);
		return ret;
	}
	mspc_init_power();
	mspc_factory_init();

	/* Aassign a reserved CMA memory region to mspc. */
	ret = of_reserved_mem_device_init(pdevice);
	if (ret != 0) {
		pr_err("%s:Register mspc shared cma failed!\n", __func__);
		mspc_record_errno(MSPC_CMA_DEVICE_INIT_ERROR);
		return ret;
	}
	pr_err("mspc shared cma registere successful!\n");

	/* Read dts attribute. */
	mspc_read_dts(pdevice);

	/* Create mspc files. */
	ret = mspc_create_files(pdevice);
	if (ret != MSPC_OK) {
		pr_err("%s:create files failed!\n", __func__);
		goto err_mem_release;
	}

	/* Create mspc task. */
	ret = mspc_create_thread();
	if (ret != MSPC_OK) {
		pr_err("%s:create task failed!\n", __func__);
		goto err_device_remove_file;
	}

	mspc_flash_register_func(mspc_flash_debug_switchs);
	pr_err("mspc module init successful!\n");
	mspc_record_errno(ret);
	return 0;

err_device_remove_file:
	mspc_remove_files(pdevice);
err_mem_release:
	of_reserved_mem_device_release(g_mspc_data.device);

	mspc_record_errno(ret);
	return ret;
}

static const struct of_device_id mspc_of_match[] = {
	{.compatible = "hisilicon,mspc-device" },
	{ }
};
MODULE_DEVICE_TABLE(of, mspc_of_match);

static struct platform_driver mspc_driver = {
	.probe = mspc_probe,
	.remove = mspc_remove,
	.suspend = mspc_suspend,
#ifdef MSPC_POWER_ON_IN_ADVANCE_FEATURE
	.resume = mspc_resume,
#endif
	.driver = {
		.name  = MSPC_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mspc_of_match),
	},
};

static int32_t __init mspc_module_init(void)
{
	int32_t ret = 0;

	ret = platform_driver_register(&mspc_driver);

	if (ret)
		pr_err("register mspc driver failed.\n");

	return ret;
}
static void __exit mspc_module_exit(void)
{
	platform_driver_unregister(&mspc_driver);
}
module_init(mspc_module_init);
module_exit(mspc_module_exit);

MODULE_AUTHOR("<wangyuzhu4@huawei.com>");
MODULE_DESCRIPTION("MSP CORE KERNEL DRIVER");
MODULE_LICENSE("GPL V2");
