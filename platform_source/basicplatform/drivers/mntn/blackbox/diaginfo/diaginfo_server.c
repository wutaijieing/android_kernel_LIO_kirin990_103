/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 *
 * Create the vsock server, process diaginfo msg form uvmm and record to stroage.
 *
 * diaginfo_server.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/socket.h>
#include <linux/vm_sockets.h>
#include <linux/net.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <securec.h>
#include "../rdr_print.h"

#define PR_LOG_TAG "diaginfo_server"
#define DIAGINFO_MSG_MAX_LEN			256
#define DIAGINFO_DATE_LEN				16 /* Style: 19700101080000 */
#define DIAGINFO_DATA_LEN            	(DIAGINFO_MSG_MAX_LEN - DIAGINFO_DATE_LEN) /* diaginfo message max len is 240 */
#define DIAGINFO_SOCKET_RETRY_TIMES      6
#define DIAGINFO_VSOCK_SERVER_PORT     1025

typedef struct {
	uint err_id;
	char date[DIAGINFO_DATE_LEN];
	char data[DIAGINFO_DATA_LEN];
}diaginfo_record_s;

/*
 * Description:  parse vsock receive data
 * Input:        recv_msg: Time [%s], Error_ID [%u], Data [%s]
 * Return:       0:success ; other: fail
 */
static int parse_receive_data(const char *recv_msg, diaginfo_record_s *diaginfo_record)
{
	int ret;

	ret = sscanf_s(recv_msg, "Time [%[^]]], Error_ID [%u], Data [%[^]]]", diaginfo_record->date, DIAGINFO_DATE_LEN,
		&diaginfo_record->err_id, diaginfo_record->data, DIAGINFO_DATA_LEN);
	if (ret < 0) {
		BB_PRINT_ERR("%s:%d sscanf_s error\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int diaginfo_server_thread(void *arg)
{
	struct sockaddr_vm client;
	unsigned int client_len = sizeof(client);
	int ret;
	int server_fd;
	int connect_fd;
	int retry_times = 0;
	char vsock_recv_msg[DIAGINFO_MSG_MAX_LEN];
	diaginfo_record_s diaginfo_record;

	struct sockaddr_vm server = {
		.svm_family = AF_VSOCK,
		.svm_cid = VMADDR_CID_ANY,
		.svm_port = DIAGINFO_VSOCK_SERVER_PORT,

	};

	while(1) {
		server_fd = sys_socket(AF_VSOCK, SOCK_STREAM, 0);
		if (server_fd < 0) {
			retry_times ++;
			msleep(1000);
		} else {
			BB_PRINT_PN("%s:%d sys_socket success!server_fd is %d!\n", __func__, __LINE__ , server_fd);
			break;
		}

		if (retry_times > DIAGINFO_SOCKET_RETRY_TIMES) {
			BB_PRINT_ERR("%s:%d sys_socket failed!server_fd is %d!\n", __func__, __LINE__ , server_fd);
			sys_close(server_fd);
			return -1;
		}
	}

	ret = sys_bind(server_fd, (struct sockaddr*)(&server), sizeof(server));
	if (ret < 0) {
		BB_PRINT_ERR("%s:%d sys_bind failed, ret is %d!\n", __func__, __LINE__,ret);
		sys_close(server_fd);
		return -1;
	}

	ret = sys_listen(server_fd, 1);
	if (ret < 0) {
		BB_PRINT_ERR("%s:%d sys_listen failed,ret is %d!\n", __func__, __LINE__,ret);
		sys_close(server_fd);
		return -1;
	}

	connect_fd = sys_accept4(server_fd, (struct sockaddr*)(&client), &client_len, 0);
	if (connect_fd < 0) {
		BB_PRINT_ERR("%s:%d  sys_accept4 failed, connect_fd is %d!\n", __func__, __LINE__,connect_fd);
		sys_close(server_fd);
		return -1;
	}
	while (kthread_should_stop() == 0) {
		memset_s(vsock_recv_msg, DIAGINFO_MSG_MAX_LEN, 0, DIAGINFO_MSG_MAX_LEN);
		memset_s(&diaginfo_record, sizeof(diaginfo_record), 0, sizeof(diaginfo_record));
		ret = sys_recv(connect_fd, vsock_recv_msg, DIAGINFO_MSG_MAX_LEN, 0);
		if (ret > 0) {
			ret = parse_receive_data(vsock_recv_msg, &diaginfo_record);
			if (ret == 0)
				bbox_diaginfo_record(diaginfo_record.err_id, diaginfo_record.date, diaginfo_record.data);
		}
	}

	return 0;
}

int diaginfo_server_init(void)
{
	struct task_struct *thread = NULL;
	thread = kthread_run(diaginfo_server_thread, NULL, "diaginfo_server");
	if (!thread) {
		BB_PRINT_ERR("create diaginfo server thread faild.\r\n");
		return -1;
	}
	return 0;
}