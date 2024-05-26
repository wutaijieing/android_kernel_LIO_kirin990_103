/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Contexthub activity recognition driver.
 * Create: 2017-03-31
 */

#include <linux/err.h>
#include <linux/fs.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_wakeup.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <net/genetlink.h>
#include <securec.h>
#include "inputhub_api/inputhub_api.h"
#include "common/common.h"
#include "shmem/shmem.h"
#include "smart_ar.h"
#include "ext_sensorhub_api.h"

#define HISMART_BLOCK_SIZE 1024
#define MC_AR_SVR  0x36
enum ar_status_common_id {
	MC_AP_AR_STATUS_OPEN = 0x01,
	MC_AP_AR_STATUS_CONFIG = 0x02,
	MC_AP_AR_STATUS_CLOSE = 0x03,
	MC_AP_AR_STATUS_REPORT = 0x04,
	MC_AP_AR_STATUS_FLUSH = 0x05,
	MC_AP_AR_STATUS_CMD_NUM,
};

int send_cmd_ext_intputhub(unsigned char cmd_type, unsigned int subtype, char *buf, size_t count)
{
	char auto_buffer[MAX_PKT_LENGTH_AP] = {0};
	int ret = 0;
	struct command send_com;

	send_com.service_id = MC_AR_SVR;
	send_com.send_buffer = buf;
	send_com.send_buffer_len = count;
	if (cmd_type == CMD_CMN_OPEN_REQ) {
		send_com.command_id = MC_AP_AR_STATUS_OPEN;
	} else if (cmd_type == CMD_CMN_CONFIG_REQ) {
		send_com.command_id = MC_AP_AR_STATUS_CONFIG;
		ret = memcpy_s(auto_buffer, MAX_PKT_LENGTH_AP, &subtype, sizeof(unsigned int));
		if (ret != EOK) {
			ctxhub_err("%s :line[%d] memset err\n", __func__, __LINE__);
			return ret;
		}
		if (count > 0) {
			ret = memcpy_s(auto_buffer + sizeof(unsigned int),
				       MAX_PKT_LENGTH_AP - sizeof(unsigned int), buf, count);
			if (ret != EOK) {
				ctxhub_err("%s :line[%d] memcpy err\n", __func__, __LINE__);
				return ret;
			}
		}
		send_com.send_buffer = auto_buffer;
		send_com.send_buffer_len = count + sizeof(unsigned int);
	} else {
		send_com.command_id = MC_AP_AR_STATUS_CLOSE;
	}
	ret = send_command(EXT_SENSORHUB_CHANNEL, &send_com, false, NULL);
	ctxhub_info("%s :line[%d] send_cmd end, command_id is [%d]\n ", __func__, __LINE__, send_com.command_id);
	return ret;
}

int get_ar_data_from_mcu_ext(unsigned char service_id, unsigned char command_id, unsigned char *data,
				    int data_len)
{
	struct pkt_header *head = NULL;

	if (data == NULL)
		return 0;
	head = (struct pkt_header *)data;
	head->length = data_len - sizeof(struct pkt_header);
	if (command_id == MC_AP_AR_STATUS_REPORT)
		return get_ar_data_from_mcu(head);
	else if (command_id == MC_AP_AR_STATUS_FLUSH)
		return ar_state_shmem(head);
	return 0;
}

void register_data_callback_from_1135(void)
{
	struct subscribe_cmds sub_cmd;
	struct sid_cid sid[2];

	sid[0].service_id = MC_AR_SVR;
	sid[0].command_id = MC_AP_AR_STATUS_REPORT;
	sid[1].service_id = MC_AR_SVR;
	sid[1].command_id = MC_AP_AR_STATUS_FLUSH;
	sub_cmd.cmds = &sid[0];
	sub_cmd.cmd_cnt = 2;
	register_data_callback(AR_CHANNEL, &sub_cmd, get_ar_data_from_mcu_ext);
}

void unregister_data_callback_from_1135(void)
{
	unregister_data_callback(AR_CHANNEL);
}
