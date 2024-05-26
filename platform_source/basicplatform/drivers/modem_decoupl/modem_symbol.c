/*
 * libc_sec_export.c for export symbols of security functions
 * require at least one entry.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/memblock.h>

#include <linux/of_fdt.h>
#include <linux/reboot.h>

#ifdef CONFIG_MMC_DW_MUX_SDSIM
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc_mux_sdsim.h>
#endif

#include <platform_include/see/efuse_driver.h>
#include <platform_include/basicplatform/linux/usb/bsp_acm.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/display/linux/dpu_drmdriver.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/platform_drivers/platform_qos.h>
#include <linux/sched/debug.h>
#else
#include <linux/sched.h>
#include <load_image.h>
#include <drv_mailbox_cfg.h>
#endif

#include <rdr_audio_soc.h>

#ifdef CONFIG_HWGPS
#include <huawei_platform/connectivity/huawei_gps.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
EXPORT_SYMBOL(ksys_rmdir);
EXPORT_SYMBOL(ksys_rename);
EXPORT_SYMBOL(ksys_unlink);
EXPORT_SYMBOL(ksys_ioctl);
EXPORT_SYMBOL(ksys_write);
EXPORT_SYMBOL(ksys_getdents64);
EXPORT_SYMBOL(ksys_open);
EXPORT_SYMBOL(ksys_fsync);
EXPORT_SYMBOL(ksys_read);
EXPORT_SYMBOL(ksys_access);
EXPORT_SYMBOL(ksys_mkdir);
EXPORT_SYMBOL(ksys_lseek);
EXPORT_SYMBOL(vfs_fstatat);

#ifndef CONFIG_DEVFREQ_GOV_PLATFORM_QOS
void platform_qos_add_request_memory_latency(struct platform_qos_request *req, int value)
{
    (void)req;
    (void)value;
}
EXPORT_SYMBOL(platform_qos_add_request_memory_latency);
#endif

#ifndef CONFIG_PLATFORM_QOS
void platform_qos_remove_request(struct platform_qos_request *req)
{
    (void)req;
}
EXPORT_SYMBOL(platform_qos_remove_request);

void platform_qos_update_request(struct platform_qos_request *req, int new_value)
{
    (void)req;
    (void)new_value;
}
EXPORT_SYMBOL(platform_qos_update_request);
#endif

#else
EXPORT_SYMBOL(find_task_by_vpid);
EXPORT_SYMBOL(sys_rmdir);
EXPORT_SYMBOL(sys_rename);
#ifdef CONFIG_LOAD_IMAGE
EXPORT_SYMBOL(bsp_reset_core_notify);
#endif
EXPORT_SYMBOL(sys_llseek);
EXPORT_SYMBOL(sys_unlink);
EXPORT_SYMBOL(sys_ioctl);
EXPORT_SYMBOL(sys_write);
EXPORT_SYMBOL(sys_getdents64);
EXPORT_SYMBOL(show_mem);
EXPORT_SYMBOL(sys_open);
EXPORT_SYMBOL(sys_fsync);
EXPORT_SYMBOL(sys_read);
EXPORT_SYMBOL(sys_access);
EXPORT_SYMBOL(sys_mkdir);
EXPORT_SYMBOL(sys_lseek);
EXPORT_SYMBOL(DRV_MAILBOX_READMAILDATA);
EXPORT_SYMBOL(DRV_MAILBOX_SENDMAIL);
EXPORT_SYMBOL(DRV_MAILBOX_REGISTERRECVFUNC);
#endif

EXPORT_SYMBOL(show_stack);
EXPORT_SYMBOL(set_efuse_kce_value);
EXPORT_SYMBOL(BSP_USB_RegUdiEnableCB);
EXPORT_SYMBOL(bsp_acm_read);
EXPORT_SYMBOL(get_efuse_kce_value);
EXPORT_SYMBOL(get_efuse_chipid_value);
EXPORT_SYMBOL(bsp_acm_ioctl);
EXPORT_SYMBOL(sched_setaffinity);
EXPORT_SYMBOL(sched_getaffinity);
EXPORT_SYMBOL(of_get_flat_dt_prop);
#ifdef CONFIG_DFX_BB
EXPORT_SYMBOL(bbox_check_edition);
#endif
EXPORT_SYMBOL(bsp_acm_open);
EXPORT_SYMBOL(machine_restart);
EXPORT_SYMBOL(bsp_acm_close);
EXPORT_SYMBOL(get_efuse_dieid_value);
EXPORT_SYMBOL(bsp_acm_write);
EXPORT_SYMBOL(BSP_USB_RegUdiDisableCB);
EXPORT_SYMBOL(memblock);
EXPORT_SYMBOL(num_to_str);
EXPORT_SYMBOL(parameq);

#ifdef CONFIG_MMC_DW_MUX_SDSIM
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
extern int dw_mci_check_himntn(int feature);
EXPORT_SYMBOL(dw_mci_check_himntn);
#endif
EXPORT_SYMBOL(sd_sim_detect_run);
EXPORT_SYMBOL(detect_status_to_string);
EXPORT_SYMBOL(sd_sim_detect_status_current);
#endif

#ifdef CONFIG_HIFI_BB
EXPORT_SYMBOL(hifireset_regcbfunc);
#endif

#ifdef CONFIG_DRM_DRIVER
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
EXPORT_SYMBOL(atfd_hisi_service_access_register_smc);
#endif
#endif

#ifdef CONFIG_HWGPS
EXPORT_SYMBOL(set_gps_ref_clk_enable);
#endif
