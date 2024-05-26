/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2010-2020. All rights reserved.
 * Description: partition table
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "partition: " fmt

#include <linux/err.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#define A_POSTFIX "_a"
#define B_POSTFIX "_b"

#define DEVICE_NAME_SIZE 64
#define MAX_PTB_NUM 128
#define MAX_PTN_NAME_LENGTH 36
#define MAX_RETRY_COUNTS 200

struct ptb_info {
	char ptn_name[DEVICE_NAME_SIZE];
	char dev_name[DEVICE_NAME_SIZE];
	int partno;
};

struct partition_data {
	struct ptb_info ptb[MAX_PTB_NUM];
	int part_tbl_nums;
	unsigned int boot_type;
	struct partition_operators *ops;
};

struct partition_data partition;
void partition_operation_register(struct partition_operators *ops)
{
	partition.ops = ops;
}

int partition_get_storage_type(void)
{
	int type = ERROR_BOOT_TYPE;

	if (partition.ops && partition.ops->get_boot_partition_type)
		type = partition.ops->get_boot_partition_type();

	return type;
}

int partition_set_storage_type(unsigned int ptn_type)
{
	int ret = -1;

	if (partition.ops && partition.ops->set_boot_partition_type)
		ret = partition.ops->set_boot_partition_type(ptn_type);

	return ret;
}

static unsigned int get_boot_type(void)
{
	int type;

	if (partition.boot_type != 0xFF)
		return partition.boot_type;

	type = partition_get_storage_type();
	if ((type != XLOADER_A) && (type != XLOADER_B)) {
		pr_err("%s: get boot partition failed!\n", __func__);
		return XLOADER_A;
	}

	partition.boot_type = type;
	return type;
}

static int transform_ptn_name(const char *part_nm, char *part_nm_tmp, unsigned int nm_tmp_len)
{
	unsigned int nm_len;
	unsigned int type;

	nm_len = strlen(part_nm);
	if (nm_len + sizeof(A_POSTFIX) > nm_tmp_len) {
		pr_err("%s:%d Invalid input part_nm\n", __func__, __LINE__);
		return PTN_ERROR;
	}

	strncpy(part_nm_tmp, part_nm, nm_len);
	type = get_boot_type();
	if (type == XLOADER_B)
		strncpy(part_nm_tmp + nm_len, B_POSTFIX, sizeof(B_POSTFIX));
	else
		strncpy(part_nm_tmp + nm_len, A_POSTFIX, sizeof(A_POSTFIX));

	return 0;
}

static int get_ptb_from_blkdev(const char *devicename)
{
	struct block_device *bdev = NULL;
	struct disk_part_tbl *ptbl = NULL;
	struct hd_struct *part = NULL;
	int i, ret = -1;

	bdev = blkdev_get_by_path(devicename, FMODE_READ | FMODE_NDELAY, 0);
	if (IS_ERR(bdev)) {
		pr_err("%s:%d blkdev get %s failed\n", __func__, __LINE__, devicename);
		return ret;
	}

	if (!bdev->bd_disk)
		goto out;

	ptbl = rcu_dereference(bdev->bd_disk->part_tbl);
	if (!ptbl)
		goto out;

	for (i = 0; i < ptbl->len; i++) {
		part = rcu_dereference(ptbl->part[i]);
		if (!part || !part->info)
			continue;

		if (partition.part_tbl_nums < MAX_PTB_NUM) {
			partition.ptb[partition.part_tbl_nums].partno = part->partno;
			strcpy(partition.ptb[partition.part_tbl_nums].ptn_name,
			       part->info->volname);
			strcpy(partition.ptb[partition.part_tbl_nums].dev_name, devicename);
			partition.part_tbl_nums++;
		}
	}
	ret = 0;

out:
	blkdev_put(bdev, FMODE_READ | FMODE_NDELAY);
	return ret;
}

static int get_all_disk_part_tbl(void *arg)
{
	const char *name = NULL;
	struct device_node *np = NULL;
	int index = 0;
	int count = 0;
	struct kstat stat;

	np = of_find_compatible_node(NULL, NULL, "vendor,partition");
	if (!np) {
		pr_err("%s:%d find dtsi failed\n", __func__, __LINE__);
		return 0;
	}

	while (!of_property_read_string_index(np, "partition-device-names", index, &name)) {
		while ((vfs_stat(name, &stat) != 0) && (count < MAX_RETRY_COUNTS)) {
			count++;
			msleep(50);
		}
		if (get_ptb_from_blkdev(name))
			pr_err("%s:%d get ptb from %s failed\n", __func__, __LINE__, name);

		index++;
		count = 0;
	}

	pr_err("%s:%d get part tbl nums is %d\n", __func__, __LINE__, partition.part_tbl_nums);
	return 0;
}

static int find_part_tbl(const char *part_nm)
{
	int i;

	for (i = 0; i < partition.part_tbl_nums; i++) {
		if (!strcmp(part_nm, partition.ptb[i].ptn_name))
			return i;
	}

	return -1;
}

static int update_bdev_patch(char *bdev_path, unsigned int len, int index)
{
	char part[MAX_PTN_NAME_LENGTH] = { 0 };

	sprintf(part, "%s%d", partition.ptb[index].dev_name, partition.ptb[index].partno);

	if (strlen(part) > len)
		return -1;

	strcpy(bdev_path, part);
	return 0;
}

/*
 * Description: Get block device path by partition name.
 * Input: part_nm, bdev_path buffer length
 * Output: bdev_path, block device path, such as:/dev/sdd3
 */
int flash_find_ptn_s(const char *part_nm, char *bdev_path, unsigned int len)
{
	char part_nm_tmp[MAX_PTN_NAME_LENGTH] = { 0 };
	int num;

	if (!bdev_path || !part_nm || !strlen(part_nm)) {
		pr_err("%s:%d Invalid input\n", __func__, __LINE__);
		return PTN_ERROR;
	}

	num = find_part_tbl(part_nm);
	if (num >= 0)
		return update_bdev_patch(bdev_path, len, num);

	if (transform_ptn_name(part_nm, part_nm_tmp, sizeof(part_nm_tmp))) {
		pr_err("%s:%d transform ptn name fail!\n", __func__, __LINE__);
		return PTN_ERROR;
	}

	num = find_part_tbl(part_nm_tmp);
	if (num >= 0)
		return update_bdev_patch(bdev_path, len, num);

	pr_err("%s: partition not found, part_nm = %s\n", __func__, part_nm);
	return PTN_ERROR;
}
EXPORT_SYMBOL(flash_find_ptn_s);

#ifdef CONFIG_DFX_DEBUG_FS
int find_ptn_test(void)
{
	char tmp[MAX_PTN_NAME_LENGTH + 1] = { 0 };
	int ret, i;
	char ptns[][MAX_PTN_NAME_LENGTH] = {
		"dfx", "fastboot",
	};

	pr_err("%s:%d ++\n", __func__, __LINE__);
	for (i = 0; i < partition.part_tbl_nums; i++)
		pr_err("%s index = %d, ptn_name = %s, dev_name = %s, partno = %d\n", __func__, i,
		       partition.ptb[i].ptn_name, partition.ptb[i].dev_name,
		       partition.ptb[i].partno);

	for (i = 0; i < sizeof(ptns) / sizeof(ptns[0]); i++) {
		ret = flash_find_ptn_s(ptns[i], tmp, sizeof(tmp));
		if (ret)
			pr_err("%s find %s ptn fail\n", __func__, ptns[i]);
		else
			pr_err("%s find %s ptn is %s\n", __func__, ptns[i], tmp);
	}

	pr_err("%s:%d --\n", __func__, __LINE__);
	return 0;
}
#endif

static int __init partition_init(void)
{
	struct task_struct *ptn_update = NULL;
	int ret = 0;

	partition.boot_type = 0xFF;
	partition.part_tbl_nums = 0;

	/* pre get boot type */
	(void)get_boot_type();

	ptn_update = kthread_run(get_all_disk_part_tbl, NULL, "partition_update");
	if (!ptn_update) {
		ret = -1;
		pr_err("%s:%d kthread run failed.\n", __func__, __LINE__);
	}
	return ret;
}
late_initcall_sync(partition_init);
