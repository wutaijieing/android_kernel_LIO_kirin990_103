/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: debug for usb.
 * Create: 2019-6-16
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include "chip_usb_debug.h"
#include <securec.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#ifdef CONFIG_HISI_TCPC
#include <linux/platform_drivers/usb/hisi_typec.h>
#endif
#include <linux/debugfs.h>
#include <linux/platform_drivers/usb/chip_usb_helper.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "dwc3-chip.h"
#include "chip_usb_bc12.h"

static ssize_t chip_dwc3_vboost_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	u32 vboost;

	if (sscanf_s(buf, "0x%x", &vboost) != 1) {
		usb_err("inject vboost!\n");
		return size;
	}
	/* vboost input scope 7 */
	if (vboost > 7) {
		usb_err("bad vboost input!\n");
		return size;
	}

	chip_dwc->usb3_phy_tx_vboost_lvl = vboost;
	usb_dbg("set usb3_phy_tx_vboost_lvl 0x%x\n",
			chip_dwc->usb3_phy_tx_vboost_lvl);

	return size;
}

static ssize_t chip_dwc3_vboost_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	return scnprintf(buf, size, "[vboost: 0x%x]\n", chip_dwc->usb3_phy_tx_vboost_lvl);
}

static ssize_t chip_dwc3_term_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	u32 term;
	int ret;

	if (sscanf_s(buf, "0x%x", &term) != 1) {
		usb_err("inject term!\n");
		return 0;
	}

	if (chip_dwc && chip_dwc->usb_phy && chip_dwc->usb_phy->set_term) {
		ret = chip_dwc->usb_phy->set_term(term);
	} else {
		usb_dbg("NULL!\n");
		return 0;
	}

	if (!ret)
		usb_dbg("set usb3_phy_term 0x%x\n", term);
	else
		usb_dbg("set usb3_phy_term failed %x\n", ret);

	return size;
}

static ssize_t chip_dwc3_term_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	u32 term = 0;
	int ret;
	int len;

	if (chip_dwc && chip_dwc->usb_phy && chip_dwc->usb_phy->get_term) {
		ret = chip_dwc->usb_phy->get_term(&term);
	} else {
		usb_dbg("NULL!\n");
		return scnprintf(buf, size, "[term: Error NULL]\n");
	}

	if (!ret) {
		usb_dbg("get usb3_phy_term 0x%x\n", term);
		len = scnprintf(buf, size, "[term: 0x%x]\n", term);
	} else {
		usb_dbg("get usb3_phy_term failed %x\n", ret);
		len = scnprintf(buf, size, "[term: Error get failed]\n");
	}

	return len;
}

static ssize_t chip_controller_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	u32 idx = 0;

	if (!chip_dwc->usbc_sets) {
		usb_err("do not support\n");
		return size;
	}

	if (sscanf_s(buf, "%u", &idx) != 1) {
		usb_err("input error!\n");
		return size;
	}

	if (chip_dwc->usbc_nums <= idx) {
		usb_info("input error\n");
		return size;
	}

	if (chip_dwc->usbc_working_idx == idx) {
		usb_info("%s is working\n", chip_dwc->usbc_sets[idx]->node->name);
		return size;
	}

	chip_dwc->usbc_idx = idx;
	chip_usb_switch_controller(chip_dwc);
	return size;
}

static ssize_t chip_controller_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	int len, tmplen;
	u32 i;

	if (!chip_dwc->usbc_sets)
		return  scnprintf(buf, size, "do not support\n");

	tmplen = scnprintf(buf, size, "working: %s, support:\n",
		chip_dwc->usbc_sets[chip_dwc->usbc_working_idx]->node->name);

	len = tmplen;
	if (len >= ((int)size))
		return len;

	for (i = 0; i < chip_dwc->usbc_nums; i++) {
		tmplen = scnprintf(buf + len, size - len, "%u: %s\n",
				i, chip_dwc->usbc_sets[i]->node->name);
		len += tmplen;
		if (len >= ((int)size))
			return len;
	}

	tmplen = scnprintf(buf + len, size - len, "please input number\n");

	len += tmplen;
	return len;
}

static ssize_t plugusb_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct chip_dwc3_device *chip_dwc3 = platform_get_drvdata(pdev);

	if (!chip_dwc3) {
		usb_err("chip_dwc3 NULL\n");
		return scnprintf(buf, PAGE_SIZE, "chip_dwc3 NULL\n");
	}
	return scnprintf(buf, PAGE_SIZE, "current state: %s\nusage: %s\n",
			chip_usb_state_string(chip_dwc3->state),
			"echo hoston/hostoff/deviceon/deviceoff > plugusb\n");
}

static ssize_t plugusb_dp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
#ifdef CONFIG_CONTEXTHUB_PD
#ifdef CONFIG_TCPC_CLASS
	struct pd_dpm_combphy_event event;
	int lunch = 1;

	usb_dbg("+\n");

	event.typec_orien = (enum typec_plug_orien_e)pd_dpm_get_cc_orientation();

	if (!strncmp(buf, "hoston", strlen("hoston"))) {
		event.dev_type = TCA_ID_FALL_EVENT;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = TCPC_USB31_CONNECTED;

		usb_err("hoston\n");
	} else if (!strncmp(buf, "hostoff", strlen("hostoff"))) {
		event.dev_type = TCA_ID_RISE_EVENT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		event.mode_type = TCPC_NC;

		usb_err("hostoff\n");
	} else if (!strncmp(buf, "deviceon", strlen("deviceon"))) {
		event.dev_type = TCA_CHARGER_CONNECT_EVENT;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = TCPC_USB31_CONNECTED;

		usb_err("deviceon\n");
	} else if (!strncmp(buf, "deviceoff", strlen("deviceoff"))) {
		event.dev_type = TCA_CHARGER_DISCONNECT_EVENT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		event.mode_type = TCPC_NC;

		usb_err("deviceoff\n");
	} else {
		lunch = 0;
	}

	if (lunch) {
		pd_dpm_handle_combphy_event(event);
		return size;
	}
#endif
#endif

	if (!strncmp(buf, "hifiusbon", strlen("hifiusbon"))) {
		chip_usb_otg_event(START_HIFI_USB);
		usb_err("hifiusbon\n");
	} else if (!strncmp(buf, "hifiusboff", strlen("hifiusboff"))) {
		chip_usb_otg_event(STOP_HIFI_USB);
		usb_err("hifiusboff\n");
	} else {
		usb_err("input state is illegal!\n");
	}

	usb_dbg("-\n");

	return size;
}

static ssize_t plugusb_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct chip_dwc3_device *chip_dwc3 = platform_get_drvdata(pdev);
	bool on = false;

	usb_dbg("+\n");
	if (!chip_dwc3) {
		usb_err("chip_dwc3 NULL\n");
		return size;
	}

	if (chip_dwc3->support_dp != 0)
		return  plugusb_dp_store(dev, attr, buf, size);

	if (!strncmp(buf, "hoston", strlen("hoston"))) {
		chip_usb_otg_event(ID_FALL_EVENT);
		on = true;
		usb_err("hoston\n");
	} else if (!strncmp(buf, "hostoff", strlen("hostoff"))) {
		chip_usb_otg_event(ID_RISE_EVENT);
		usb_err("hostoff\n");
	} else if (!strncmp(buf, "deviceon", strlen("deviceon"))) {
		chip_usb_otg_event(CHARGER_CONNECT_EVENT);
		on = true;
		usb_err("deviceon\n");
	} else if (!strncmp(buf, "deviceoff", strlen("deviceoff"))) {
		chip_usb_otg_event(CHARGER_DISCONNECT_EVENT);
		usb_err("deviceoff\n");
	} else if (!strncmp(buf, "hifiusbon", strlen("hifiusbon"))) {
		chip_usb_otg_event(START_HIFI_USB);
		usb_err("hifiusbon\n");
	} else if (!strncmp(buf, "hifiusboff", strlen("hifiusboff"))) {
		chip_usb_otg_event(STOP_HIFI_USB);
		on = true;
		usb_err("hifiusboff\n");
	} else {
		usb_err("input state is illegal!\n");
	}

	usb_dbg("on:%d\n", on);
#ifdef CONFIG_HISI_TCPC
	hisi_usb_typec_set_wake_lock(on);
#endif

	usb_dbg("-\n");
	return size;
}

DEVICE_ATTR(plugusb, 0644, plugusb_show, plugusb_store);

static int init_plugusb(struct device *dev)
{
	struct class *usb_class = NULL;
	struct device *usb_dev = NULL;
	char *usb_class_name = CONFIG_VENDOR_USB_CLASS_NAME;
	char *usb_dev_name = CONFIG_VENDOR_USB_DEV_NAME;
	int ret;

	usb_dbg("+\n");

	ret = device_create_file(dev, &dev_attr_plugusb);
	if (ret)
		return ret;

	usb_class = class_create(THIS_MODULE, usb_class_name);
	if (IS_ERR_OR_NULL(usb_class)) {
		usb_dbg("create %s error!\n", usb_class_name);
		return -ENODEV;
	}

	usb_dev = device_create(usb_class, NULL, 0, NULL, usb_dev_name);
	if (IS_ERR(usb_dev)) {
		usb_dbg("create %s error!\n", usb_dev_name);
		return -ENODEV;
	}

	ret = sysfs_create_link(&usb_dev->kobj, &dev->kobj, "interface");
	if (ret)
		usb_dbg("create link file error!\n");

	usb_dbg("-\n");
	return ret;
}

static void remove_plugusb(struct device *dev)
{
	device_remove_file(dev, &dev_attr_plugusb);
}

static ssize_t fakecharger_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc null\n");

	return scnprintf(buf, size, "[fake charger type: %s]\n",
					 chip_dwc->fake_charger_type == CHARGER_TYPE_NONE ? "not fake" :
					 charger_type_string(chip_dwc->fake_charger_type));
}

static ssize_t fakecharger_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	enum chip_charger_type charger_type = get_charger_type_from_str(buf, size);

	usb_dbg("+\n");
	if (!chip_dwc) {
		usb_err("chip_dwc null\n");
		return size;
	}

	mutex_lock(&chip_dwc->lock);
	chip_dwc->fake_charger_type = charger_type;
	mutex_unlock(&chip_dwc->lock);
	usb_dbg("-\n");

	return size;
}

static ssize_t hifi_ip_first_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc null\n");
	return scnprintf(buf, size, "%d\n", chip_dwc->hifi_ip_first);
}

static ssize_t hifi_ip_first_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	usb_dbg("+\n");
	if (!chip_dwc) {
		pr_err("%s: chip_dwc NULL\n", __func__);
		return size;
	}

	mutex_lock(&chip_dwc->lock);
	if (buf[0] == '1')
		chip_dwc->hifi_ip_first = 1;
	else if (buf[0] == '0')
		chip_dwc->hifi_ip_first = 0;
	mutex_unlock(&chip_dwc->lock);
	usb_dbg("-\n");

	return size;
}

static ssize_t hifi_usb_perf_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc null\n");

	return scnprintf(buf, size,
			"off2device time %dms\n"
			"off2host time %dms\n"
			"off2hifiusb time %dms\n"
			"U-disk enumeration time: %dms\n",
		jiffies_to_msecs(chip_dwc->start_device_complete_time_stamp
			- chip_dwc->start_device_time_stamp),
		jiffies_to_msecs(chip_dwc->start_host_complete_time_stamp
			- chip_dwc->start_host_time_stamp),
		jiffies_to_msecs(chip_dwc->start_hifiusb_complete_time_stamp /* Yes, it is start_host_time_stamp */
			- chip_dwc->start_host_time_stamp),
		jiffies_to_msecs(chip_dwc->device_add_time_stamp
			- chip_dwc->start_host_time_stamp));
}

static ssize_t hifi_usb_perf_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc) {
		pr_err("%s: chip_dwc NULL\n", __func__);
		return size;
	}
	return size;
}

/* buffer for write cmd to debugfs node */
static char _debug_write_buf[PAGE_SIZE];

/* charger debugfs interface */
static ssize_t hiusb_charger_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	enum chip_charger_type charger_type;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc null\n");

	mutex_lock(&chip_dwc->lock);
	charger_type = chip_dwc->charger_type;
	mutex_unlock(&chip_dwc->lock);

	return scnprintf(buf, size, "[(%d):Charger type = %s]\n"
			 "--------------------------------------------------\n"
			 "usage: echo {str} > chargertest\n"
			 "       sdp:        Standard Downstreame Port\n"
			 "       cdp:        Charging Downstreame Port\n"
			 "       dcp:        Dedicate Charging Port\n"
			 "       unknown:    non-standard\n"
			 "       none:       not connected\n"
			 "       provide:    host mode, provide power\n",
			charger_type, charger_type_string(charger_type));
}

static ssize_t hiusb_charger_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	enum chip_charger_type charger_type = get_charger_type_from_str(buf, size);

	if (!chip_dwc) {
		usb_err("chip_dwc null\n");
		return size;
	}

	mutex_lock(&chip_dwc->lock);
	chip_dwc->charger_type = charger_type;
	notify_charger_type(chip_dwc);
	mutex_unlock(&chip_dwc->lock);

	return size;
}

static ssize_t hiusb_eventmask_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc NULL\n");

	return scnprintf(buf, size, "%d\n", chip_dwc->eventmask);
}

static ssize_t hiusb_eventmask_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	int eventmask;

	if (!chip_dwc) {
		pr_err("%s: chip_dwc NULL\n", __func__);
		return size;
	}

	if (kstrtoint(buf, 0, &eventmask)) {
		pr_err("eventmask string to int failed\n");
		return size;
	}

	mutex_lock(&chip_dwc->lock);
	chip_dwc->eventmask = eventmask;
	mutex_unlock(&chip_dwc->lock);

	return size;
}

/* set eye diagram param debugfs interface */
/* cat eyepattern interface */
static ssize_t hiusb_eyepattern_param_show(void *dev_data, char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;

	if (!chip_dwc)
		return scnprintf(buf, size, "chip_dwc NULL\n");

	return scnprintf(buf, size, "device:0x%x\nhost:0x%x\n",
			chip_dwc->eye_diagram_param, chip_dwc->eye_diagram_host_param);
}

/* echo "xx" > eyepattern interface */
static ssize_t hiusb_eyepattern_param_store(void *dev_data, const char *buf, size_t size)
{
	struct chip_dwc3_device *chip_dwc = dev_data;
	unsigned int eye_diagram_param;

	if (!chip_dwc) {
		pr_err("%s: chip_dwc NULL\n", __func__);
		return size;
	}

	if (kstrtouint(buf, 0, &eye_diagram_param)) {
		pr_err("eye_diagram_param string to unsigned int failed\n");
		return size;
	}

	mutex_lock(&chip_dwc->lock);
	chip_dwc->eye_diagram_param = eye_diagram_param;
	chip_dwc->eye_diagram_host_param = eye_diagram_param;
	mutex_unlock(&chip_dwc->lock);

	return size;
}

/*
 * debugfs interface call back data
 */
#define DBG_NODE_NAME_LENG 16
struct hiusb_debug_attr {
	char name[DBG_NODE_NAME_LENG];
	void *dev_data;
	hiusb_debug_show_ops show;
	hiusb_debug_store_ops write;
	struct list_head list;
};

static struct list_head hiusb_dbg_list;

/*
 * A interface for help creat a debugfs file
 */
static int hiusb_debug_template_show(struct seq_file *s, void *d)
{
	struct hiusb_debug_attr *dattr = s->private;
	void *usbdev = NULL;
	char *buf = NULL;
	ssize_t alen = 0;

	if (!dattr) {
		pr_err("unknown option show!\n");
		return -EINVAL;
	}
	usbdev = dattr->dev_data;

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (dattr->show)
		alen = dattr->show(usbdev, buf, PAGE_SIZE);

	seq_write(s, buf, alen);

	kfree(buf);

	return 0;
}

static int hiusb_debug_template_open(struct inode *inode, struct file *file)
{
	return single_open(file, hiusb_debug_template_show, inode->i_private);
}

/*
 * A interface for help creat a debugfs file
 */
static ssize_t hiusb_debug_template_write(struct file *file,
			const char __user *buf, size_t size, loff_t *ppos)
{
	struct hiusb_debug_attr *dattr = ((struct seq_file *)file->private_data)->private;
	void *usbdev = NULL;

	if (!buf)
		return -EINVAL;

	if (!dattr) {
		pr_err("unknown option store!\n");
		return -EINVAL;
	}
	usbdev = dattr->dev_data;

	if (size >= PAGE_SIZE) {
		pr_err("set charger type cmd too long!\n");
		return -ENOMEM;
	}

	if (copy_from_user(_debug_write_buf, buf, size)) {
		pr_err("[USB.ERROR] Can't get user data!\n");
		return -ENOSPC;
	}
	_debug_write_buf[size] = '\0';

	pr_err("[USB.DBGFS] send cmd: (%s)\n", _debug_write_buf);
	if (dattr->write)
		dattr->write(usbdev, _debug_write_buf, size);

	return size;
}

static const struct file_operations hiusb_debug_template_fops = {
	.open = hiusb_debug_template_open,
	.read = seq_read,
	.write = hiusb_debug_template_write,
	.release = single_release,
};

/* register file show & store */
static void hiusb_debug_quick_register(const char *name, void *dev_data,
		hiusb_debug_show_ops show, hiusb_debug_store_ops store)
{
	struct hiusb_debug_attr *new_attr = NULL;
	int ret;

	if (!name)
		return;

	if (!hiusb_dbg_list.next) {
		pr_info("[USB.DBGFS] quick register init!\n");
		INIT_LIST_HEAD(&hiusb_dbg_list);
	}

	/* free in hiusb_debugfs_destroy */
	new_attr = kzalloc(sizeof(*new_attr), GFP_KERNEL);
	if (!new_attr)
		return;

	ret = strncpy_s(new_attr->name, sizeof(new_attr->name), name, DBG_NODE_NAME_LENG - 1);
	if (ret != EOK)
		pr_info("[USB.DBGFS] copy DBG name failed!\n");
	new_attr->name[DBG_NODE_NAME_LENG - 1] = '\0';
	new_attr->dev_data = dev_data;
	new_attr->show = show;
	new_attr->write = store;

	list_add(&new_attr->list, &hiusb_dbg_list);
}

/* chip usb debugfs sys init */
static struct dentry *hiusb_debug_init(void)
{
	struct list_head *pos = NULL;
	struct hiusb_debug_attr *pattr = NULL;
	struct dentry *root = NULL;

	root = debugfs_create_dir("hiusb", usb_debug_root);
	if (IS_ERR_OR_NULL(root))
		return NULL;

	list_for_each(pos, &hiusb_dbg_list) {
		pattr = list_entry(pos, struct hiusb_debug_attr, list);
		pr_info("[USB.DBGFS] register:%s\n", pattr->name);
		/* creat file permission 0600 */
		debugfs_create_file(pattr->name, 0600, root, pattr, &hiusb_debug_template_fops);
	}

	return root;
}

static void hiusb_debugfs_destroy(struct dentry *root)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct hiusb_debug_attr *pattr = NULL;

	debugfs_remove_recursive(root);

	list_for_each_safe(pos, next, &hiusb_dbg_list) {
		pattr = list_entry(pos, struct hiusb_debug_attr, list);
		pr_info("[USB.DBGFS] destroy node:%s\n", pattr->name);
		list_del(&pattr->list);
		kfree(pattr);
	}

	INIT_LIST_HEAD(&hiusb_dbg_list);
}

int create_attr_file(struct chip_dwc3_device *chip_dwc3)
{
	struct device *dev = NULL;
	void *dev_data = NULL;
	int ret;

	usb_dbg("+\n");

	if (!chip_dwc3)
		return -EINVAL;

	dev = &chip_dwc3->pdev->dev;
	ret = init_plugusb(dev);
	if (ret)
		return ret;

	dev_data = platform_get_drvdata(to_platform_device(dev));
	hiusb_debug_quick_register("charger", dev_data,
		hiusb_charger_show, hiusb_charger_store);

	hiusb_debug_quick_register("eventmask", dev_data,
		hiusb_eventmask_show, hiusb_eventmask_store);

	hiusb_debug_quick_register("eyepattern", dev_data,
		hiusb_eyepattern_param_show,
		hiusb_eyepattern_param_store);

	hiusb_debug_quick_register("fakecharger", dev_data,
		fakecharger_show, fakecharger_store);

	hiusb_debug_quick_register("hifi_ip_first", dev_data,
		hifi_ip_first_show, hifi_ip_first_store);

	hiusb_debug_quick_register("perf", dev_data,
		hifi_usb_perf_show, hifi_usb_perf_store);

	hiusb_debug_quick_register("vboost", dev_data,
		chip_dwc3_vboost_show, chip_dwc3_vboost_store);

	hiusb_debug_quick_register("term", dev_data,
		chip_dwc3_term_show, chip_dwc3_term_store);

	hiusb_debug_quick_register("controller",
		platform_get_drvdata(to_platform_device(dev)),
		chip_controller_show,
		chip_controller_store);

	chip_dwc3->debug_root = hiusb_debug_init();
	if (!chip_dwc3->debug_root)
		return -EFAULT;

	usb_dbg("-\n");
	return 0;
}

void remove_attr_file(struct chip_dwc3_device *chip_dwc3)
{
	struct device *dev = NULL;

	if (!chip_dwc3)
		return;

	dev = &chip_dwc3->pdev->dev;
	remove_plugusb(dev);
	hiusb_debugfs_destroy(chip_dwc3->debug_root);
}
