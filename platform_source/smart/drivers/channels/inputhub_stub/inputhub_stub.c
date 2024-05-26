/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Description: provide weak functions when closing all sensorhub kernel code.
 * Create: 2021-06-30
 */

#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/export.h>
#include <linux/notifier.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/smart/linux/iomcu_ipc.h>

int __weak rpc_status_change_for_camera(unsigned int status)
{
	pr_err("rpc_status_change_for_camera is invalid;\n");
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(rpc_status_change_for_camera);

int __weak ap_color_report(int value[], int length)
{
	pr_err("ap_color_report is invalid;\n");
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(ap_color_report);

struct dsm_client *__weak shb_dclient = NULL;

int __weak register_iom3_recovery_notifier(struct notifier_block *nb)
{
	pr_err("register_iom3_recovery_notifier is invalid;\n");
	return 0;
}

int __weak write_customize_cmd(const struct write_info *wr, struct read_info *rd, bool is_lock)
{
	pr_err("write_customize_cmd is invalid;\n");
	return -EINVAL;
}

void __weak save_light_to_sensorhub(uint32_t mipi_level, uint32_t bl_level)
{
	pr_err("save_light_to_sensorhub is invalid;\n");
}

void __weak send_dc_status_to_sensorhub(uint32_t dc_status)
{
	pr_err("send_dc_status_to_sensorhub is invalid;\n");
}

int __weak send_thp_open_cmd(void)
{
	pr_err("send_thp_open_cmd is invalid;\n");
	return -1;
}

int __weak send_thp_close_cmd(void)
{
	pr_err("send_thp_close_cmd is invalid;\n");
	return -1;
}

int __weak send_thp_ap_event(int inpara_len, const char *inpara, uint8_t cmd)
{
	pr_err("send_thp_ap_event is invalid;\n");
	return -1;
}

int __weak send_tp_ap_event(int inpara_len, const char *inpara, uint8_t cmd)
{
	pr_err("send_tp_ap_event is invalid;\n");
	return -1;
}

int __weak send_thp_driver_status_cmd(unsigned char power_on,
	unsigned int para, unsigned char cmd)
{
	pr_err("send_thp_driver_status_cmd is invalid;\n");
	return -1;
}

int __weak send_thp_algo_sync_event(int inpara_len, const char *inpara)
{
	pr_err("send_thp_algo_sync_event is invalid;\n");
	return -1;
}

int __weak send_thp_auxiliary_data(unsigned int inpara_len, const char *inpara)
{
	pr_err("send_thp_auxiliary_data is invalid;\n");
	return -EINVAL;
}

int __weak shmem_send(enum obj_tag module_id, const void *usr_buf, unsigned int usr_buf_size)
{
	pr_err("shmem_send is invalid;\n");
	return -EINVAL;
}

