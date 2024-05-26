/*
 * dubai_dpm.c
 *
 * dubai dpm module
 *
 * Copyright (C) 2020-2020 Huawei Technologies Co., Ltd.
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
#include "dubai_dpm.h"
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

int ioctl_dpm_enable_module(const void __user *argp)
{
	unsigned int dpm_switch_cmd = 0;
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	ret = copy_from_user(&dpm_switch_cmd, argp, sizeof(dpm_switch_cmd));
	if (ret != 0)
		pr_err("%s failed, ret is %d\n", __func__, ret);
	else
		dpm_parse_switch_cmd(dpm_switch_cmd);

	return ret;
}

int ioctl_dpm_get_peri_num(void __user *argp)
{
	unsigned int peri_ip_num;
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	peri_ip_num = dpm_peri_num();
	ret = copy_to_user(argp, &peri_ip_num, sizeof(peri_ip_num));
	if (ret != 0)
		pr_err("%s failed, ret is %d\n", __func__, ret);

	return ret;
}

/*
 * ip_energy.length = ip_num * sizeof(struct ip_energy)
 * 1 copy_from_user get length
 * 2 copy_to_user ip_energy and length
 */
int ioctl_dpm_get_peri_data(void __user *argp)
{
	int ret;
	unsigned int peri_ip_size = 0;
	unsigned int peri_ip_num, peri_data_size;
	struct dubai_transmit_t *peri_data = NULL;
	const unsigned int ip_size = sizeof(struct ip_energy);

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	ret = copy_from_user(&peri_ip_size, argp, sizeof(peri_ip_size));
	if (ret != 0) {
		pr_err("%s copy from user failed\n", __func__);
		goto err_handler;
	}
	/* calculate ip_num */
	peri_ip_num = peri_ip_size / ip_size;
	if (peri_ip_size % ip_size != 0 ||
	    peri_ip_num < 1 || peri_ip_num > DPM_MODULE_NUM) {
		pr_err("invalid user parameter: %u\n", peri_ip_size);
		goto err_handler;
	}

	peri_data_size = peri_ip_size + sizeof(struct dubai_transmit_t);
	peri_data = kzalloc(peri_data_size, GFP_KERNEL);
	if (peri_data == NULL) {
		pr_err("%s peri data kzalloc fail\n", __func__);
		goto err_handler;
	}

	ret = get_dpm_peri_data(peri_data, peri_ip_num);
	if (ret != 0) {
		pr_err("%s get peri data fail!\n", __func__);
		goto err_handler;
	}
	ret = copy_to_user(argp, peri_data, peri_data_size);
	if (ret != 0) {
		pr_err("%s copy to user failed\n", __func__);
		goto err_handler;
	}

	kfree(peri_data);
	return 0;
err_handler:
	if (peri_data != NULL)
		kfree(peri_data);
	return -EFAULT;
}

int ioctl_dpm_get_gpu_data(void __user *argp)
{
	unsigned long long energy;
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	energy = get_dpm_chdmod_power(DPM_GPU_ID);
	ret = copy_to_user(argp, &energy, sizeof(energy));
	if (ret != 0)
		pr_err("%s failed, ret is %d\n", __func__, ret);

	return ret;
}

int ioctl_dpm_get_npu_data(void __user *argp)
{
	unsigned long long energy;
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	energy = get_dpm_chdmod_power(DPM_NPU_ID);
	ret = copy_to_user(argp, &energy, sizeof(energy));
	if (ret != 0)
		pr_err("%s failed, ret is %d\n", __func__, ret);

	return ret;
}

#ifdef CONFIG_MODEM_DPM
int ioctl_dpm_get_mdm_num(void __user *argp)
{
	unsigned int mdm_num;
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	if (get_dpm_modem_num == NULL) {
		pr_err("%s modem callback is NULL!\n", __func__);
		return -EFAULT;
	}
	mdm_num = get_dpm_modem_num();
	ret = copy_to_user(argp, &mdm_num, sizeof(mdm_num));
	if (ret != 0)
		pr_err("%s failed, ret is %d\n", __func__, ret);

	return ret;
}

/*
 * mdm_energy.length = mdm_num * sizeof(struct mdm_energy)
 * 1 copy_from_user get length
 * 2 copy_to_user mdm_energy and length
 */
int ioctl_dpm_get_mdm_data(void __user *argp)
{
	int ret;
	unsigned int mdm_ip_size = 0;
	unsigned int mdm_ip_num, mdm_data_size;
	struct mdm_transmit_t *mdm_data = NULL;
	const unsigned int ip_size = sizeof(struct mdm_energy);

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	if (get_dpm_modem_data == NULL) {
		pr_err("%s modem callback is NULL\n", __func__);
		goto err_handler;
	}
	ret = copy_from_user(&mdm_ip_size, argp, sizeof(mdm_ip_size));
	if (ret != 0) {
		pr_err("%s copy from user failed\n", __func__);
		goto err_handler;
	}
	mdm_ip_num = mdm_ip_size / ip_size;
	if (mdm_ip_size % ip_size != 0 ||
	    mdm_ip_num < 1 || mdm_ip_num > MDM_MODULE_MAX) {
		pr_err("invalid user parameter: %u\n", mdm_ip_size);
		goto err_handler;
	}

	mdm_data_size = mdm_ip_size + sizeof(struct mdm_transmit_t);
	mdm_data = kzalloc(mdm_data_size, GFP_KERNEL);
	if (mdm_data == NULL) {
		pr_err("%s mdm data kzalloc fail\n", __func__);
		goto err_handler;
	}

	ret = get_dpm_modem_data(mdm_data, mdm_ip_num);
	if (ret != 0) {
		pr_err("%s get mdm data fail\n", __func__);
		goto err_handler;
	}
	ret = copy_to_user(argp, mdm_data, mdm_data_size);
	if (ret != 0) {
		pr_err("%s copy to user failed\n", __func__);
		goto err_handler;
	}

	kfree(mdm_data);
	return 0;
err_handler:
	if (mdm_data != NULL)
		kfree(mdm_data);
	return -EFAULT;
}
#endif
