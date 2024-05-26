/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include "mdfx_priv.h"
#include "mdfx_utils.h"

static struct mutex dfx_lock;
bool mdfx_ready = false;
struct mdfx_pri_data *g_mdfx_data = NULL; //lint !e552

static int mdfx_get_capability(struct mdfx_pri_data *data, void __user *argp)
{
	int ret;

	if (IS_ERR_OR_NULL(data)) {
		mdfx_pr_err("file info NULL Pointer\n");
		return -EINVAL;
	}

	if (!argp) {
		mdfx_pr_err("argp NULL Pointer\n");
		return -EINVAL;
	}

	ret = copy_to_user(argp, &data->mdfx_caps, sizeof(data->mdfx_caps));
	if (ret != 0) {
		mdfx_pr_err("copy to user failed!ret=%d", ret);
		return -EFAULT;
	}

	mdfx_pr_info("mdfx_caps 0x%x\n", data->mdfx_caps);
	return 0;
}

static int mdfx_open(struct inode *inode, struct file *filp)
{
	struct mdfx_device_data *mdfx_data = NULL;
	struct mdfx_pri_data *priv_data = NULL;

	if (IS_ERR_OR_NULL(inode) || IS_ERR_OR_NULL(filp)) {
		mdfx_pr_err("NULL Pointer\n");
		return -1;
	}

	mdfx_pr_info(" +\n");

	mutex_lock(&dfx_lock);

	mdfx_data = container_of(inode->i_cdev, struct mdfx_device_data, cdev);
	if (IS_ERR_OR_NULL(mdfx_data)) {
		mutex_unlock(&dfx_lock);
		return -1;
	}

	priv_data = &mdfx_data->data;
	if (priv_data->ref_cnt) {
		mutex_unlock(&dfx_lock);
		return 0;
	}

	priv_data->ref_cnt++;
	filp->private_data = mdfx_data;
	mutex_unlock(&dfx_lock);

	mdfx_pr_info(" -\n");
	return 0;
}

static int mdfx_release(struct inode *inode, struct file *filp)
{
	struct mdfx_device_data *mdfx_data = NULL;
	struct mdfx_pri_data *priv_data = NULL;

	if (IS_ERR_OR_NULL(inode))
		return -1;

	if (!filp)
		return -EINVAL;

	mdfx_pr_info(" +\n");

	mutex_lock(&dfx_lock);
	mdfx_data = (struct mdfx_device_data *)filp->private_data;
	if (IS_ERR_OR_NULL(mdfx_data)) {
		mutex_unlock(&dfx_lock);
		return -1;
	}

	priv_data = &mdfx_data->data;

	if (!priv_data->ref_cnt) {
		mutex_unlock(&dfx_lock);
		return 0;
	}

	priv_data->ref_cnt--;
	mutex_unlock(&dfx_lock);

	mdfx_pr_info(" -\n");
	return 0;
}

static long mdfx_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct mdfx_device_data *mdfx_data = NULL;
	struct mdfx_pri_data *priv_data = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;
	long ret = -ENOSYS;

	if (!f || (arg == 0) || !(f->private_data))
		return -EINVAL;

	mdfx_pr_info(" + cmd 0x%x\n", cmd);

	mutex_lock(&dfx_lock);
	mdfx_data = (struct mdfx_device_data *)f->private_data;
	priv_data = &mdfx_data->data;

	switch (cmd) {
	case MDFX_ADD_VISITOR:
		ret = mdfx_add_visitor(priv_data, argp);
		break;
	case MDFX_REMOVE_VISITOR:
		ret = mdfx_remove_visitor(priv_data, argp);
		break;
	case MDFX_QUERY_FILE_SPEC:
		ret = mdfx_file_query_spec(priv_data, argp);
		break;
	case MDFX_DUMP_INFO:
		ret = mdfx_actor_do_ioctl(priv_data, argp, ACTOR_DUMPER);
		break;
	case MDFX_TRACE_INFO:
		ret = mdfx_actor_do_ioctl(priv_data, argp, ACTOR_TRACING);
		break;
	case MDFX_DELIVER_EVENT:
		ret = mdfx_deliver_event(priv_data, argp);
		break;
	case MDFX_DMD_REPORT:
		break;
	case MDFX_GET_CAPABILITY:
		ret = mdfx_get_capability(priv_data, argp);
		break;
	default:
		mdfx_pr_err("Unknown IOCTL request %u\n", cmd);
		break;
	}

	mutex_unlock(&dfx_lock);
	mdfx_pr_info(" -\n");

	return ret;
}

static const struct file_operations mdfx_fops = {
	.owner = THIS_MODULE,
	.open = mdfx_open,
	.release = mdfx_release,
	.unlocked_ioctl = mdfx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mdfx_ioctl,
#endif
};

static int mdfx_dev_init(struct platform_device *pdev)
{
	int ret;
	struct mdfx_device_data *mdfx_data = NULL;

	if (IS_ERR_OR_NULL(pdev))
		return -1;

	mdfx_data = dev_get_drvdata(&pdev->dev);
	if (!mdfx_data) {
		mdfx_pr_err("dev_get_drvdata fail\n");
		return -1;
	}

	// register device no.
	ret = alloc_chrdev_region(&mdfx_data->devno, 0, 1, "hisi_mdfx");
	if (ret) {
		mdfx_pr_err("alloc_chrdev_region failed ret = %d\n", ret);
		goto chrdev_alloc_err;
	}

	// init cdev, and add it to system
	cdev_init(&mdfx_data->cdev, &mdfx_fops);
	mdfx_data->cdev.owner = THIS_MODULE;

	ret = cdev_add(&mdfx_data->cdev, mdfx_data->devno, 1);
	if (ret) {
		mdfx_pr_err("cdev_add failed ret = %d\n", ret);
		ret = -ENOMEM;
		goto add_cdev_err;
	}

	// create device node
	mdfx_data->dev_class = class_create(THIS_MODULE, "mdfx_device");
	if (IS_ERR_OR_NULL(mdfx_data->dev_class)) {
		mdfx_pr_err("Unable to create dfx class; errno = %ld\n",
						PTR_ERR(mdfx_data->dev_class));
		mdfx_data->dev_class = NULL;
		ret = -ENOMEM;
		goto create_dev_class_err;
	}

	mdfx_data->dfx_cdevice = device_create(mdfx_data->dev_class, NULL,
								mdfx_data->devno, NULL, "%s", "hisi_mdfx");
	if (IS_ERR_OR_NULL(mdfx_data->dfx_cdevice)) {
		mdfx_pr_err("Failed to create hall dfx_cdevice\n");
		ret = -ENOMEM;
		goto create_cdevice_err;
	}

	dev_set_drvdata(mdfx_data->dfx_cdevice, mdfx_data);
	return 0;

create_cdevice_err:
	class_destroy(mdfx_data->dev_class);
create_dev_class_err:
	cdev_del(&mdfx_data->cdev);
add_cdev_err:
	unregister_chrdev_region(mdfx_data->devno, 1);
chrdev_alloc_err:
	return ret;
}

static void mdfx_data_init(struct mdfx_pri_data *mdfx_data)
{
	if (IS_ERR_OR_NULL(mdfx_data)) {
		mdfx_pr_err("mdfx_data NULL Pointer\n");
		return;
	}

	mdfx_sysfs_init(mdfx_data);
	mdfx_file_data_init(&mdfx_data->file_spec);
	mdfx_event_init(mdfx_data);
	mdfx_debugger_init(mdfx_data);

	/*
	 * init all actors, they will be listerner for event manager
	 * so, need init event first
	 */
	mdfx_actors_init(mdfx_data);
	mdfx_visitors_init(&mdfx_data->visitors);

	mdfx_data->var_log_file_count = mdfx_data->file_spec.file_max_num;
	mdfx_data->var_event_log_file_count = MDFX_LOG_FILE_MAX_COUNT;
}

static void mdfx_data_deinit(struct mdfx_pri_data *mdfx_data)
{
	if (IS_ERR_OR_NULL(mdfx_data)) {
		mdfx_pr_err("mdfx_data NULL Pointer\n");
		return;
	}

	mdfx_visitors_deinit(&mdfx_data->visitors);
	mdfx_event_deinit(&mdfx_data->event_manager);
	mdfx_saver_thread_deinit(&mdfx_data->log_saving_thread);
	mdfx_saver_thread_deinit(&mdfx_data->dump_saving_thread);
	mdfx_saver_thread_deinit(&mdfx_data->trace_saving_thread);
}

static void mdfx_dev_deinit(struct platform_device *pdev,
	struct mdfx_device_data *dfx_data)
{
	if (IS_ERR_OR_NULL(pdev)) {
		mdfx_pr_err("pdev NULL Pointer\n");
		return;
	}

	if (dfx_data) {
		device_destroy(dfx_data->dev_class, dfx_data->devno);
		class_destroy(dfx_data->dev_class);
		cdev_del(&dfx_data->cdev);
		unregister_chrdev_region(dfx_data->devno, 1);

		mdfx_data_deinit(&dfx_data->data);
	}
}

static int mdfx_probe(struct platform_device *pdev)
{
	struct mdfx_device_data *mdfx_data = NULL;
	int ret;

	mdfx_pr_info(" +\n");

	if (IS_ERR_OR_NULL(pdev)) {
		mdfx_pr_err("NULL Pointer\n");
		return -1;
	}

	mdfx_data = kzalloc(sizeof(*mdfx_data), GFP_KERNEL);
	if (!mdfx_data) {
		mdfx_pr_err("alloc memory for mdfx_data failed\n");
		return -ENOMEM;
	}

	mdfx_data->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, mdfx_data);
	g_mdfx_data = &mdfx_data->data;

	// read property from dtsi
	ret = mdfx_read_dtsi(mdfx_data);
	if (ret != 0) {
		mdfx_pr_err("read file property failed,  ret = %d\n", ret);
		goto probe_err;
	}

	/* init actors and event manager */
	mdfx_data_init(&mdfx_data->data);

	ret = mdfx_dev_init(pdev);
	if (ret != 0) {
		mdfx_pr_err("dev init failed, ret = %d\n", ret);
		goto probe_err;
	}

	ret = mdfx_saver_kthread_init(&mdfx_data->data.log_saving_thread, "log_saving_thread");
	if (ret != 0) {
		mdfx_pr_err("log saving kernel thread init failed, ret = %d\n", ret);
		goto probe_err;
	}
	ret = mdfx_saver_kthread_init(&mdfx_data->data.dump_saving_thread, "dump_saving_thread");
	if (ret != 0) {
		mdfx_pr_err("dump saving kernel thread init failed, ret = %d\n", ret);
		goto probe_err;
	}
	ret = mdfx_saver_kthread_init(&mdfx_data->data.trace_saving_thread, "trace_saving_thread");
	if (ret != 0) {
		mdfx_pr_err("trace saving kernel thread init failed, ret = %d\n", ret);
		goto probe_err;
	}

	mdfx_ready = true;

	mutex_init(&dfx_lock);
	mdfx_sysfs_create_attrs(pdev);

	mdfx_pr_info(" -\n");
	return 0;

probe_err:
	mdfx_data_deinit(&mdfx_data->data);
	mdfx_dev_deinit(pdev, mdfx_data);

	kfree(mdfx_data);
	mdfx_data = NULL;
	g_mdfx_data = NULL;

	return ret;
}

static int mdfx_remove(struct platform_device *pdev)
{
	struct mdfx_device_data *dfx_data = NULL;

	mdfx_pr_info(" +\n");

	if (IS_ERR_OR_NULL(pdev)) {
		mdfx_pr_err("NULL Pointer\n");
		return -EINVAL;
	}

	mdfx_sysfs_remove_attrs(pdev);

	dfx_data = dev_get_drvdata(&pdev->dev);
	mdfx_dev_deinit(pdev, dfx_data);

	if (dfx_data) {
		kfree(dfx_data);
		dfx_data = NULL;
	}

	g_mdfx_data = NULL;
	mdfx_ready = false;
	mutex_destroy(&dfx_lock);

	mdfx_pr_info(" -\n");
	return 0;
}

static const struct of_device_id mdfx_device_supply_ids[] = {
	{
		.compatible = DTS_COMP_DFX_NAME,
		.data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, mdfx_device_supply_ids);

static struct platform_driver mdfx_driver = {
	.probe = mdfx_probe,
	.remove = mdfx_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "dpu_media_dfx",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mdfx_device_supply_ids),
	},
};

static int __init mdfx_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&mdfx_driver);
	if (ret) {
		mdfx_pr_err("platform media dfx driver register failed\n");
		return ret;
	}

	return ret;
}

static void __exit mdfx_driver_exit(void)
{
	(void)platform_driver_unregister(&mdfx_driver);
}

module_init(mdfx_driver_init);
module_exit(mdfx_driver_exit);

MODULE_LICENSE("GPL2");
MODULE_AUTHOR("Tech. Co., Ltd.");
MODULE_DESCRIPTION("media dfx driver");
