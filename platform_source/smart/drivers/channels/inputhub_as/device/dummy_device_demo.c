/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: dummy device demo implement.
 * Create: 2019-11-05
 */
#include <securec.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include "device_common.h"
#include "common/common.h"

static int g_detect_times;
static struct device_manager_node *g_device_dummy;

static int set_dummy_cfg_data(void)
{
	return RET_FAIL;
}

static int dummy_device_detect(struct device_node *dn)
{
	// retry times + boot try once
	if (g_detect_times < SENSOR_DETECT_RETRY + 1) {
		g_detect_times++;
		return RET_FAIL;
	}
	return RET_SUCC;
}

static int dummy_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_dummy = device_manager_node_alloc();
	if (!g_device_dummy) {
		ctxhub_err("[%s], g_device_dummy alloc failed!\n", __func__);
		return -EINVAL;
	}

	/*
	 * write dummy device for construct detect fail
	 * use gyro because gyro in dts, otherwise will not detect
	 */
	g_device_dummy->device_name_str = "gyro";
	g_device_dummy->tag = TAG_GYRO;
	g_device_dummy->detect = dummy_device_detect;
	g_device_dummy->cfg = set_dummy_cfg_data;
	ret = register_contexthub_device(g_device_dummy);
	if (ret != 0) {
		ctxhub_err("%s, register %d dummy failed\n",
			__func__, g_device_dummy->tag);
		device_manager_node_free(g_device_dummy);
		g_device_dummy = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_dummy->tag);
	return 0;
}

static void __exit dummy_device_exit(void)
{
	unregister_contexthub_device(g_device_dummy);
	device_manager_node_free(g_device_dummy);
}

device_initcall_sync(dummy_device_init);
module_exit(dummy_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
