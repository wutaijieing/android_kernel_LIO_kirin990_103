/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: bindfile and empower cert position info
 * Create: 2022/04/26
 */

#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <linux/compat.h>
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <securec.h>
#include <platform_include/see/uapp.h>

#define EMPOWER_CERT_PATH           "/empower_cert"
#define UAPP_BINDFILE_PATH          "/uapp/bindfile"
#define UAPP_BINDFILE_BACKUP_PATH   "/uapp/bindfile_backup"

static u32 uapp_get_file_pos(struct uapp_file_pos *pos, char *node_path)
{
	int ret;
	errno_t err;
	const char *ptn = NULL;
	struct device_node *np = NULL;

	pr_info("enter %s\n", __func__);

	np = of_find_node_by_path(node_path);
	if (!np) {
		pr_err("error, cannot find node %s\n", node_path);
		return UAPP_FIND_DTS_NODE_ERR;
	}

	ret = of_property_read_string(np, "ptn", &ptn);
	if (ret != 0) {
		pr_err("error 0x%x, cannot read ptn\n", ret);
		return UAPP_READ_DTS_NODE_ERR;
	}

	err = strcpy_s(pos->ptn, sizeof(pos->ptn), ptn);
	if (err != EOK) {
		pr_err("error 0x%x, copy ptn\n", err);
		return UAPP_MEMCPY_S_ERR;
	}

	ret = of_property_read_u64(np, "offset", &pos->offset);
	if (ret != 0) {
		pr_err("error 0x%x, cannot read offset\n", ret);
		return UAPP_READ_DTS_NODE_ERR;
	}

	ret = of_property_read_u64(np, "size", &pos->size);
	if (ret != 0) {
		pr_err("error 0x%x, cannot read size\n", ret);
		return UAPP_READ_DTS_NODE_ERR;
	}

	return UAPP_OK;
}

u32 uapp_get_bindfile_pos(struct uapp_bindfile_pos *bindfile)
{
	u32 ret;

	ret = uapp_get_file_pos(&bindfile->bindfile, UAPP_BINDFILE_PATH);
	if (ret != 0) {
		pr_err("error 0x%x, get node path %s\n", ret, UAPP_BINDFILE_PATH);
		return ret;
	}

	ret = uapp_get_file_pos(&bindfile->bindfile_backup, UAPP_BINDFILE_BACKUP_PATH);
	if (ret != 0) {
		pr_err("error 0x%x, get node path %s\n", ret, UAPP_BINDFILE_BACKUP_PATH);
		return ret;
	}

	return 0;
}

u32 uapp_get_empower_cert_pos(struct uapp_file_pos *pos)
{
	u32 ret;

	if (!pos) {
		pr_err("error, pos is NULL\n");
		return UAPP_NULL_PTR_ERR;
	}

	ret = uapp_get_file_pos(pos, EMPOWER_CERT_PATH);
	if (ret != UAPP_OK) {
		pr_err("error 0x%x, get uapp empower cert pos\n", ret);
		return ret;
	}

	return UAPP_OK;
}
