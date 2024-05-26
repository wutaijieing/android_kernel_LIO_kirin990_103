/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub ipc tcp amsm stub.
 * Create: 2021-10-14
 */
#include "common/common.h"
#include <linux/completion.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <securec.h>
#include <securectype.h>
#include "iomcu_link_ipc.h"
#include "iomcu_link_ipc_shm.h"

struct notifier_block g_iomcu_boot_nb;

int shmem_send(enum obj_tag module_id, const void *usr_buf, unsigned int usr_buf_size)
{
	struct write_info wr;

	if (usr_buf == NULL) {
		pr_err("[%s] usr_buf null err\n", __func__);
		return -1;
	}

	(void)memset_s(&wr, sizeof(wr), 0, sizeof(wr));
	wr.tag = module_id;
	wr.cmd = CMD_CMN_IPCSHM_REQ;
	wr.wr_buf = usr_buf;
	wr.wr_len = usr_buf_size;
	return write_customize_cmd(&wr, NULL, false);
}

int iomcu_ipc_init(void)
{
	return 0;
}

void iomcu_ipc_exit(void)
{
}

int iomcu_ipc_recv_register(void)
{
	return 0;
}

unsigned int shmem_get_capacity(void)
{
	return ipc_shm_get_capacity();
}