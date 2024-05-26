/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_fb.h"
#include "hisi_fb_sysfs.h"

void hisifb_sysfs_init(struct hisi_fb_info *hfb_info)
{
	int i;

	disp_check_and_no_retval(!hfb_info, err, "fb_info is NULL\n");

	disp_pr_info(" ++++ ");
	hfb_info->sysfs_index = 0;
	for (i = 0; i < HISI_FB_SYSFS_ATTRS_NUM; i++)
		hfb_info->sysfs_attrs[i] = NULL;

	hfb_info->sysfs_attr_group.attrs = hfb_info->sysfs_attrs;
	disp_pr_info(" ---- ");
}

void hisifb_sysfs_attrs_append(struct hisi_fb_info *hfb_info, struct attribute *attr)
{
	disp_check_and_no_retval(!hfb_info, err, "fb_info is NULL\n");

	disp_check_and_no_retval(!attr, err, "attr is NULL\n");

	disp_pr_info(" ++++ ");
	if (hfb_info->sysfs_index >= HISI_FB_SYSFS_ATTRS_NUM) {
		disp_pr_err("fb%d, sysfs_atts_num %d is out of range %d!\n",
			hfb_info->id, hfb_info->sysfs_index, HISI_FB_SYSFS_ATTRS_NUM);
		return;
	}

	hfb_info->sysfs_attrs[hfb_info->sysfs_index] = attr;
	hfb_info->sysfs_index++;
	disp_pr_info(" ---- ");
}

int hisifb_sysfs_create(struct platform_device *pdev)
{
	int ret;
	struct hisi_fb_info *hfb_info = NULL;

	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL\n");
	disp_pr_info(" ++++ ");
	hfb_info = platform_get_drvdata(pdev);
	disp_check_and_return(!hfb_info, -EINVAL, err, "fb_info is NULL\n");
	disp_pr_info(" ++2++ ,%d, %s, %s\n", hfb_info->id, hfb_info->pdev->name, pdev->name);

	ret = sysfs_create_group(&hfb_info->fbi_info->dev->kobj, &(hfb_info->sysfs_attr_group));
	if (ret)
		disp_pr_err("fb%d sysfs group creation failed, error=%d!\n", hfb_info->id, ret);

	disp_pr_info(" ---- ");
	return ret;
}

void hisifb_sysfs_remove(struct platform_device *pdev)
{
	struct hisi_fb_info *hfb_info = NULL;
	disp_pr_info(" ++++ ");
	disp_check_and_no_retval(!pdev, err, "pdev is NULL\n");

	hfb_info = platform_get_drvdata(pdev);
	disp_check_and_no_retval(!hfb_info, err, "fb_info is NULL\n");

	sysfs_remove_group(&hfb_info->fbi_info->dev->kobj, &(hfb_info->sysfs_attr_group));

	hisifb_sysfs_init(hfb_info);

	disp_pr_info(" ---- ");
}
