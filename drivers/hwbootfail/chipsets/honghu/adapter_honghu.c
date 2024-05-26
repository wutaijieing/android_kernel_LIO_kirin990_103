/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: implement the platform interface for boot fail
 * Author: qidechun
 * Create: 2019-03-05
 */

/* ----includes ---- */
#include "adapter_honghu.h"
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/securec.h>
#include <hwbootfail/core/boot_interface.h>
#include <hwbootfail/chipsets/common/bootfail_common.h>
#include <hwbootfail/chipsets/common/adapter_common.h>
#include <hwbootfail/chipsets/common/bootfail_chipsets.h>

#ifdef CONFIG_ARCH_SHAOLINGUN

#include <linux/huanglong/mntn/platform/mntn_public_interface.h>
#include <linux/huanglong/mntn/mntn_bootup_keypoint.h>
#include <linux/huanglong/mntn/rdr_pub.h>

#define BF_ADDR_PREVFIX(x) (MNTN_##x)
#define bf_bbox_map        mntn_bbox_map
#define bf_bbox_unmap      mntn_bbox_unmap

#else

#include <linux/mntn/mntn_public_interface.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <mntn_public_interface.h>

#define BF_ADDR_PREVFIX(x) (HISI_##x)
#define bf_bbox_map        hisi_bbox_map
#define bf_bbox_unmap      hisi_bbox_unmap

#endif

/* ----local macroes ---- */
#define BFI_PART_SIZE 0x7E0000 /* 8MB -128 KB */
#define BFI_DEV_PATH "/dev/block/by-name/rrecord"
#define BL_LOG_NAME "fastboot_log"
#define KERNEL_LOG_NAME "last_kmsg"

/* ---- local prototypes ---- */
enum boot_detector_modid {
	MODID_BOOT_DETECTOR_START = BF_ADDR_PREVFIX(BB_MOD_BFM_START),
	MODID_BOOT_FAIL,
	MODID_BOOT_DETECTOR_END = BF_ADDR_PREVFIX(BB_MOD_BFM_END)
};

/* ---- local static variables ---- */
struct rdr_exception_info_s boot_detector_excp_info[] = {
	{
		{0, 0}, (u32)BF_ADDR_PREVFIX(BB_MOD_BFM_START), (u32)BF_ADDR_PREVFIX(BB_MOD_BFM_END),
		RDR_ERR, RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
		(u32)RDR_REENTRANT_DISALLOW, (u32)BFM_S_NATIVE_DATA_FAIL,
		0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-reboot", 0, 0, 0
	},
};

/* ---- local function definitions --- */
static unsigned int stage_to_kp(enum boot_stage stage)
{
	if (stage >= BL1_STAGE_START && stage <= BL1_STAGE_END)
		return STAGE_XLOADER_START;
	else if (stage >= BL2_STAGE_START && stage <= BL2_STAGE_END)
		return STAGE_FASTBOOT_START;
	else if (stage >= KERNEL_STAGE_START && stage <= KERNEL_STAGE_END)
		return STAGE_KERNEL_BOOTANIM_COMPLETE;
	else if (stage >= NATIVE_STAGE_START && stage <= NATIVE_STAGE_END)
		return STAGE_INIT_INIT_START;
	else if (stage >= FRAMEWORK_STAGE_START && stage < BOOT_SUCC_STAGE)
		return STAGE_ANDROID_ZYGOTE_START;

	return STAGE_BOOTUP_END;
}

static enum boot_stage kp_to_stage(unsigned int kp)
{
	if (kp >= STAGE_XLOADER_START && kp <= STAGE_XLOADER_END)
		return BL1_STAGE;
	else if (kp >= STAGE_FASTBOOT_START && kp <= STAGE_FASTBOOT_END)
		return BL2_STAGE;
	else if (kp >= STAGE_KERNEL_EARLY_INITCALL &&
		kp < STAGE_INIT_INIT_START)
		return KERNEL_STAGE;
	else if (kp >= STAGE_INIT_INIT_START &&
		kp < STAGE_ANDROID_ZYGOTE_START)
		return NATIVE_STAGE;
	else if (kp >= STAGE_ANDROID_ZYGOTE_START &&
		kp < STAGE_BOOTUP_END)
		return FRAMEWORK_STAGE;
	else if (kp >= STAGE_BOOTUP_END)
		return BOOT_SUCC_STAGE;

	return INVALID_STAGE;
}

static int honghu_set_boot_stage(int stage)
{
	unsigned int kp = stage_to_kp((enum boot_stage)stage);

	set_boot_keypoint(kp);

	return 0;
}

static int honghu_get_boot_stage(int *stage)
{
	unsigned int kp;

	if (unlikely(stage == NULL)) {
		print_invalid_params("stage is NULL!\n");
		return -1;
	}

	kp = get_boot_keypoint();
	*stage = kp_to_stage(kp);

	return 0;
}

static void dump_callback(u32 modid,
	u32 etype, u64 coreid,	char *log_path, pfn_cb_dump_done pfn_cb)
{
	if (log_path == NULL) {
		print_err("%s: log_path is null\n", __func__);
		return;
	}

	print_info("bootfail Enter %s, log_path: %s\n", __func__, log_path);
}

static void reset_callback(u32 modid, u32 etype, u64 coreid)
{
	print_info("bootfail Enter %s, modid: %u etype: %u coreid:%llu.\n",
		__func__, modid, etype, coreid);
}

static int register_callbacks_to_rdr(void)
{
	struct rdr_module_ops_pub soc_ops;
	struct rdr_register_module_result retinfo;
	int ret;

	soc_ops.ops_dump = dump_callback;
	soc_ops.ops_reset = reset_callback;
	ret = rdr_register_module_ops(RDR_BFM, &soc_ops, &retinfo);
	if (ret != 0)
		print_err("rdr_register_module_ops failed! ret = [%d]\n", ret);

	return ret;
}

static void register_exceptions_to_rdr(void)
{
	unsigned int i;
	u32 ret;

	for (i = 0; i < array_size(boot_detector_excp_info); i++) {
		ret = rdr_register_exception(&boot_detector_excp_info[i]);
		if (ret == 0)
			print_err("rdr_register_exception failed! ret: %d\n", ret);
	}
}

static void honghu_reboot(void)
{
	(void)register_callbacks_to_rdr();
	register_exceptions_to_rdr();
	rdr_syserr_process_for_ap(MODID_BOOT_FAIL, 0, 0);
}

static void honghu_shutdown(void)
{
	print_info("%s: honghu not support\n", __func__);
}

void *bf_map(phys_addr_t addr, size_t size)
{
	return bf_bbox_map(addr, size);
}

void bf_unmap(const void *addr)
{
	bf_bbox_unmap(addr);
	return;
}

static void capture_bl_log(char *buf, unsigned int buf_size)
{
	unsigned int len;

	if (buf == NULL || buf_size == 0) {
		print_err("get bl log params err\n");
		return;
	}

	char *fastboot_log_addr = bf_map(BF_ADDR_PREVFIX(SUB_RESERVED_FASTBOOT_LOG_PYHMEM_BASE),
		BF_ADDR_PREVFIX(SUB_RESERVED_FASTBOOT_LOG_PYHMEM_SIZE));
	if (fastboot_log_addr == NULL) {
		print_err("bf_map fastboot log addr failed!\n");
		return;
	}

	len = min(buf_size, BF_ADDR_PREVFIX(SUB_RESERVED_FASTBOOT_LOG_PYHMEM_SIZE));
	if (memcpy_s(buf, buf_size, fastboot_log_addr, len) != EOK)
		print_err("capture_bl_log memcpy_s buf failed!\n");

	bf_unmap(fastboot_log_addr);
}

static void capture_kernel_log(char *buf, unsigned int buf_size)
{
	void *store_addr = NULL;
	void *last_applog_addr = NULL;
	void *last_kmsg_addr = NULL;
	unsigned int len;

	if (buf == NULL || buf_size == 0) {
		print_err("get kernel log params err\n");
		return;
	}

	store_addr = bf_map(BF_ADDR_PREVFIX(RESERVED_PSTORE_PHYMEM_BASE),
		BF_ADDR_PREVFIX(RESERVED_PSTORE_PHYMEM_SIZE));
	if (store_addr == NULL) {
		print_err("bf_map kernel log addr failed!\n");
		return;
	}
	last_applog_addr = store_addr + BF_ADDR_PREVFIX(RESERVED_PSTORE_PHYMEM_SIZE) -
		LAST_APPLOG_SIZE;
	last_kmsg_addr = last_applog_addr - LAST_KMSG_SIZE;
	len = min((unsigned int)LAST_KMSG_SIZE, buf_size);
	if (memcpy_s(buf, buf_size, last_kmsg_addr, len) != EOK)
		print_err("capture_kernel_log memcpy_s buf failed!\n");

	bf_unmap(store_addr);
}

char *get_bfi_dev_path(void)
{
	return BFI_DEV_PATH;
}

size_t get_bfi_part_size(void)
{
	return (size_t)BFI_PART_SIZE;
}

static void get_raw_part_info(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	adp->bfi_part.dev_path = BFI_DEV_PATH;
	adp->bfi_part.part_size = (unsigned int)get_bfi_part_size();
}

static void get_boot_stage_ops(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	adp->stage_ops.set_stage = honghu_set_boot_stage;
	adp->stage_ops.get_stage = honghu_get_boot_stage;
}

static void get_sysctl_ops(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	adp->sys_ctl.reboot = honghu_reboot;
	adp->sys_ctl.shutdown = honghu_shutdown;
}

static void get_phys_mem_info(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	adp->pyhs_mem_info.base = BF_ADDR_PREVFIX(RESERVED_MNTN_DDR_BFMR_BASE);
	adp->pyhs_mem_info.size = BF_SIZE_1K;
}

static void get_log_ops_info(struct adapter *adp)
{
	errno_t ret;

	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	ret = strncpy_s(adp->bl_log_ops.log_name,
		sizeof(adp->bl_log_ops.log_name), BL_LOG_NAME,
		min(strlen(BL_LOG_NAME), sizeof(adp->bl_log_ops.log_name) - 1));
	if (ret != EOK)
		print_err("get log BL_LOG_NAME strncpy_s failed\n");

	adp->bl_log_ops.log_size = (unsigned int)
		BF_ADDR_PREVFIX(SUB_RESERVED_FASTBOOT_LOG_PYHMEM_SIZE);
	adp->bl_log_ops.capture_bl_log = capture_bl_log;

	ret = strncpy_s(adp->kernel_log_ops.log_name,
		sizeof(adp->kernel_log_ops.log_name),
		KERNEL_LOG_NAME, min(strlen(KERNEL_LOG_NAME),
		sizeof(adp->kernel_log_ops.log_name) - 1));
	if (ret != EOK)
		print_err("get log KERNEL_LOG_NAME strncpy_s failed\n");

	adp->kernel_log_ops.log_size = (unsigned int)LAST_KMSG_SIZE;
	adp->kernel_log_ops.capture_kernel_log = capture_kernel_log;
}

static void degrade(int excp_type)
{
	print_info("%s: honghu not support\n", __func__);
}

static void bypass(int excp_type)
{
	print_info("%s: honghu not support\n", __func__);
}

static void load_backup(const char *part_name)
{
	print_info("%s: honghu not support\n", __func__);
}

static void notify_storage_fault(unsigned long long bopd_mode)
{
	print_info("%s: honghu not support\n", __func__);
}

static int honghu_read_from_phys_mem(unsigned long dst,
	unsigned long dst_max, void *phys_mem_addr, unsigned long data_len)
{
	char *temp_dst = NULL;
	char *temp_addr = NULL;
	unsigned long i;
	size_t bytes_to_read;

	bytes_to_read = min(dst_max, data_len);
	/*lint -e446*/
	temp_addr = (char *)bf_map((phys_addr_t)(uintptr_t)phys_mem_addr,
		bytes_to_read);
	/*lint +e446*/
	if (temp_addr == NULL)
		return -1;

	temp_dst = (char *)(uintptr_t)dst;
	for (i = 0; i < bytes_to_read; i++) {
		*temp_dst = readb(temp_addr);
		temp_dst++;
		temp_addr++;
	}
	bf_unmap(temp_addr);

	return 0;
}

static int honghu_write_to_phys_mem(unsigned long dst,
	unsigned long dst_max, void *src, unsigned long src_len)
{
	char *temp_src = NULL;
	char *temp_addr = NULL;
	unsigned long i;
	size_t bytes_to_write;

	bytes_to_write = min(dst_max, src_len);
	/*lint -e446*/
	temp_addr = (char *)bf_map((phys_addr_t)dst, bytes_to_write);
	/*lint +e446*/
	if (temp_addr == NULL)
		return -1;

	temp_src = (char *)src;
	for (i = 0; i < bytes_to_write; i++) {
		writeb(*temp_src, temp_addr);
		temp_src++;
		temp_addr++;
	}
	bf_unmap(temp_addr);

	return 0;
}

static void platform_adapter_init(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	get_raw_part_info(adp);
	get_phys_mem_info(adp);
	get_log_ops_info(adp);
	get_sysctl_ops(adp);
	get_boot_stage_ops(adp);
	adp->prevent.degrade = degrade;
	adp->prevent.bypass = bypass;
	adp->prevent.load_backup = load_backup;
	adp->notify_storage_fault = notify_storage_fault;
}

static void rewrite_adapter_init(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return;
	}

	/* common interface is not available, need rewrite for honghu */
	adp->pyhs_mem_info.ops.read = honghu_read_from_phys_mem;
	adp->pyhs_mem_info.ops.write = honghu_write_to_phys_mem;
}

int honghu_adapter_init(struct adapter *adp)
{
	if (adp == NULL) {
		print_invalid_params("adp is NULL!\n");
		return -1;
	}

	if (common_adapter_init(adp) != 0) {
		print_err("init adapter common failed!\n");
		return -1;
	}
	platform_adapter_init(adp);
	rewrite_adapter_init(adp);

	return 0;
}
