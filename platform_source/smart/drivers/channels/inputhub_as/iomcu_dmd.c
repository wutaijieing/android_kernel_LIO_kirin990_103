/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description:dft channel.c
 * Create:2019.11.06
 */
#include <securec.h>

#include <linux/types.h>
#include <linux/errno.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "iomcu_log.h"

#define BUFFER_SIZE 1024

#ifdef CONFIG_HUAWEI_DSM
static int sensorhub_img_dump(int type, void *buff, int size)
{
	return 0;
}

static struct dsm_client *g_shb_dclient;

struct dsm_client_ops g_sensorhub_ops = {
	.poll_state = NULL,
	.dump_func = sensorhub_img_dump,
};

struct dsm_dev g_dsm_sensorhub = {
	.name = "dsm_sensorhub",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &g_sensorhub_ops,
	.buff_size = BUFFER_SIZE,
};

struct dsm_client *get_shb_dclient(void)
{
	return g_shb_dclient;
}

struct dsm_dev *get_shb_dsm_dev(void)
{
	return &g_dsm_sensorhub;
}

void dmd_init(void)
{
	g_shb_dclient = dsm_register_client(&g_dsm_sensorhub);
	if (!g_shb_dclient)
		ctxhub_err("[%s], dsm register failed!\n", __func__);
}

#endif
