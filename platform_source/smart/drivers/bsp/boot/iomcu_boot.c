/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Contexthub boot.
 * Create: 2019-06-10
 */

#include "common/common.h"
#include "securec.h"
#include <linux/delay.h>
#include <linux/err.h>
#include <platform_include/smart/linux/iomcu_boot.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <linux/platform_drivers/bsp_syscounter.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <soc_pmctrl_interface.h>

#define IOMCU_SMPLAT_SCFG_START (IOMCU_CONFIG_START + DDR_CONFIG_SIZE * 3 / 8)
#define IOMCU_SMPLAT_SCFG_SIZE (DDR_CONFIG_SIZE * 5 / 8)
#define IOMCU_SMPLAT_DDR_MAGIC 0xabcd1243

struct plat_cfg_str *g_smplat_scfg = NULL;
struct dsm_client *__weak shb_dclient = NULL;

/***************__weak start****************/
void __weak inputhub_init_before_boot(void) {}
void __weak inputhub_init_after_boot(void) {}
void __weak inputhub_route_exit(void) {}
void __weak save_light_to_sensorhub(uint32_t mipi_level, uint32_t bl_level) {}
int __weak rpc_status_change_for_camera(unsigned int status)
{
	return 0;
}
int __weak send_thp_driver_status_cmd(unsigned char status, unsigned int para,  unsigned char cmd)
{
	return 0;
}
int __weak send_thp_ap_event(int inpara_len, const char *inpara, uint8_t cmd)
{
	return 0;
}
int __weak send_thp_algo_sync_event(int inpara_len, const char *inpara)
{
	return 0;
}
int __weak send_thp_auxiliary_data(unsigned int inpara_len, const char *inpara)
{
	return 0;
}
int __weak iomcu_dubai_log_fetch(uint32_t event_type, void *data, uint32_t length)
{
	return 0;
}
int __weak ap_color_report(int value[], int length)
{
	return 0;
}
/***************__weak end******************/

static int g_boot_iom3 = STARTUP_IOM3_CMD;

static int boot_iomcu(void)
{
	int ret;

	ret = RPROC_ASYNC_SEND(ipc_ap_to_iom_mbx, (mbox_msg_t *)&g_boot_iom3, 1);
	if (ret)
		pr_err("RPROC_ASYNC_SEND error in %s\n", __func__);
	return ret;
}

inline struct plat_cfg_str *get_plat_cfg_str(void)
{
	return g_smplat_scfg;
}

void write_timestamp_base_to_sharemem(void)
{
	u64 syscnt = 0;
	u64 kernel_ns;
	struct timespec64 ts;

	if (g_smplat_scfg == NULL) {
		pr_err("%s g_smplat_scfg is null\n", __func__);
		return;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_boottime_ts64(&ts);
#else
	get_monotonic_boottime64(&ts);
#endif
#ifdef CONFIG_BSP_SYSCOUNTER
	syscnt = bsp_get_syscount();
#endif
	kernel_ns = (u64)(ts.tv_sec * NSEC_PER_SEC) + (u64)ts.tv_nsec;

	g_smplat_scfg->timestamp_base.syscnt = syscnt;
	g_smplat_scfg->timestamp_base.kernel_ns = kernel_ns;
	pr_info("%s syscnt[0x%lx]kns[0x%lx]\n", __func__, g_smplat_scfg->timestamp_base.syscnt,
		g_smplat_scfg->timestamp_base.kernel_ns);
}

/*lint -e446*/
static void iomcu_free_shareddr(void)
{
	if (g_smplat_scfg != NULL) {
		iounmap(g_smplat_scfg);
		g_smplat_scfg = NULL;
	}
}

static int write_defualt_config_info_to_sharemem(void)
{
	int ret;

	if (g_smplat_scfg == NULL)
		g_smplat_scfg = (struct plat_cfg_str *)ioremap_wc(IOMCU_SMPLAT_SCFG_START, IOMCU_SMPLAT_SCFG_SIZE);

	if (g_smplat_scfg == NULL) {
		pr_err("%s g_smplat_scfg is null\n", __func__);
		return -EPIPE;
	}

	ret = memset_s(g_smplat_scfg, IOMCU_SMPLAT_SCFG_SIZE, 0, sizeof(struct plat_cfg_str));
	if (ret != EOK) {
		pr_err("%s, memset_s err", __func__);
		iounmap(g_smplat_scfg);
		g_smplat_scfg = NULL;
		return ret;
	}

	g_smplat_scfg->magic = IOMCU_SMPLAT_DDR_MAGIC;
	return 0;
}

/*lint +e446*/

static int inputhub_mcu_init(void)
{
	int ret;
	pr_info("%s start>\n", __func__);
	if (get_contexthub_dts_status())
		return -EACCES;
	ret = write_defualt_config_info_to_sharemem();
	if (ret != 0) {
		pr_err("write_defualt_config_info_to_sharemem err\n");
		return ret;
	}
	write_timestamp_base_to_sharemem();
	ret = iomcu_ipc_init();
	if (ret != 0) {
		pr_err("iomcu_ipc_init err\n");
		goto ERR_FREE_DDR;
	}
	inputhub_init_before_boot();
	ret = iomcu_dump_init();
	if (ret != 0) {
		pr_err("iomcu_dump_init err\n");
		goto ERR_FREE_IPC;
	}
	ret = iomcu_ipc_recv_register();
	if (ret != 0) {
		pr_err("iomcu_ipc_recv_register err\n");
		goto ERR_FREE_DUMP;
	}
	ret = boot_iomcu();
	if (ret != 0) {
		pr_err("boot_iomcu err\n");
		goto ERR_FREE_RECV;
	}
	inputhub_init_after_boot();
	pr_info("%s <end\n", __func__);
	return 0;
ERR_FREE_RECV:
	RPROC_MONITOR_UNREGISTER(ipc_iom_to_ap_mbx, &g_iomcu_boot_nb);
ERR_FREE_DUMP:
	iomcu_dump_exit();
ERR_FREE_IPC:
	iomcu_ipc_exit();
ERR_FREE_DDR:
	iomcu_free_shareddr();
	return ret;
}

static void __exit inputhub_mcu_exit(void)
{
	RPROC_PUT(ipc_ap_to_iom_mbx);
	RPROC_PUT(ipc_iom_to_ap_mbx);
}

late_initcall(inputhub_mcu_init);
module_exit(inputhub_mcu_exit);

MODULE_DESCRIPTION("input hub bridge");
MODULE_LICENSE("GPL");