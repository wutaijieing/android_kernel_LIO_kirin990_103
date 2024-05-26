/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: RPMB Debug Driver
 * Create: 2012-05-01
 * History: 2019-03-18  structure optimization
 */

#include <linux/debugfs.h>
#include <linux/platform_drivers/rpmb.h>
#include "vendor_rpmb.h"

/*
 * debugfs defination start here, debug smc is closed in secure world in user
 * version, so debug can not used in user version
 */
static int get_counter_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_COUNTER, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(get_counter_fops, get_counter_fops_get, NULL, "%llu\n");

static int read_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_READ, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(read_fops, read_fops_get, NULL, "%llu\n");

static int write_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_WRITE, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(write_fops, write_fops_get, NULL, "%llu\n");

static int format_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_FORMAT, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(format_fops, format_fops_get, NULL, "%llu\n");

static int write_capability_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_WRITE_CAPABILITY, (u64)0x0, (u64)0x0,
			   (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(write_capability_fops, write_capability_fops_get, NULL,
			"%llu\n");

static int read_capability_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_READ_CAPABILITY, (u64)0x0, (u64)0x0,
			   (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(read_capability_fops, read_capability_fops_get, NULL,
			"%llu\n");

static int rpmb_partiton_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_PARTITION, (u64)0x0, (u64)0x0,
			   (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_partiton_fops, rpmb_partiton_fops_get, NULL,
			"%llu\n");

static int rpmb_multi_key_set_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_MULTI_KEY, (u64)0x0, (u64)0x0,
			   (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_multi_key_set_fops, rpmb_multi_key_set_fops_get,
			NULL, "%llu\n");
static int rpmb_config_view_fops_get(void *data, u64 *val)
{
	atfd_rpmb_smc((u64)RPMB_SVC_CONFIG_VIEW, (u64)0x0, (u64)0x0,
			   (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_config_view_set_fops, rpmb_config_view_fops_get,
			NULL, "%llu\n");

static int rpmb_get_key_status_fops_get(void *data, u64 *val)
{
	int key_status;
	key_status = get_rpmb_key_status();
	pr_err("rpmb_key status:%d\n", key_status);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_get_key_status_fops, rpmb_get_key_status_fops_get,
			NULL, "%llu\n");

static int rpmb_get_support_key_num_fops_get(void *data, u64 *val)
{
	u32 key_num;
	key_num = get_rpmb_support_key_num();
	pr_err("rpmb_key num is %d\n", key_num);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_get_support_key_num_fops,
			rpmb_get_support_key_num_fops_get, NULL, "%llu\n");

int rpmb_debugfs_init(void)
{
	/* debugfs for debug only */
	struct dentry *debugfs_rpmb = NULL;
	struct rpmb_request *request;

	request = rpmb_get_request();
	if (!request)
		return -EINVAL;

	debugfs_rpmb = debugfs_create_dir("rpmb", NULL);
	if (IS_ERR(debugfs_rpmb)) {
		pr_err("%s debugfs_create_dir fail\n", __func__);
		return -1;
	}
	if (!debugfs_rpmb) {
		pr_err("%s debugfs_rpmb is NULL\n", __func__);
		return -1;
	}
	request->rpmb_debug.key_num = get_rpmb_support_key_num();
	debugfs_create_file("get_counter", S_IRUSR | S_IWUSR, debugfs_rpmb,
			    NULL, &get_counter_fops);
	debugfs_create_file("read", S_IRUSR | S_IWUSR, debugfs_rpmb, NULL,
			    &read_fops);
	debugfs_create_file("write", S_IRUSR | S_IWUSR, debugfs_rpmb, NULL,
			    &write_fops);
	debugfs_create_file("format", S_IRUSR | S_IWUSR, debugfs_rpmb,
			    NULL, &format_fops);
	debugfs_create_file("write_capability", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL, &write_capability_fops);
	debugfs_create_file("read_capability", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL, &read_capability_fops);
	debugfs_create_file("partition_see", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL, &rpmb_partiton_fops);
	debugfs_create_file("multi_key_set", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL, &rpmb_multi_key_set_fops);
	debugfs_create_file("rpmb_config_view", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL,
			    &rpmb_config_view_set_fops);
	debugfs_create_file("rpmb_key_status", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL, &rpmb_get_key_status_fops);
	debugfs_create_file("rpmb_key_num", S_IRUSR | S_IWUSR,
			    debugfs_rpmb, NULL,
			    &rpmb_get_support_key_num_fops);

	debugfs_create_u16("partition_start", S_IRUSR | S_IWUSR,
			   debugfs_rpmb,
			   &request->rpmb_debug.partition_start);
	debugfs_create_u64("partition_size", S_IRUSR | S_IWUSR,
			   debugfs_rpmb,
			   &request->rpmb_debug.partition_size);
	debugfs_create_u64("result", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.result);
	debugfs_create_u64("test_time", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.test_time);
	debugfs_create_u32("key_id", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.key_num);
	debugfs_create_u32("func_id", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.func_id);
	debugfs_create_u16("storage_debug", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.storage_debug);
	debugfs_create_u16("start", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.start);
	debugfs_create_u16("block_count", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.block_count);
	debugfs_create_u16("write_value", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.write_value);
	debugfs_create_u16("read_check", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &request->rpmb_debug.read_check);
	debugfs_create_u8("capability_times", S_IRUSR | S_IWUSR,
			  debugfs_rpmb,
			  &request->rpmb_debug.capability_times);
	debugfs_create_u8("multi_region_num", S_IRUSR | S_IWUSR,
			  debugfs_rpmb,
			  &request->rpmb_debug.multi_region_num);
	debugfs_create_u8("partition_id", S_IRUSR | S_IWUSR, debugfs_rpmb,
			  &request->rpmb_debug.partition_id);
	return 0;
}
EXPORT_SYMBOL(rpmb_debugfs_init);

