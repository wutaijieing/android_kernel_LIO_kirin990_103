/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/dma-buf.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/version.h>
#include <isp_ddr_map.h>
#include <log/log_usertype.h>
#include <securec.h>
#include <asm/cacheflush.h>
#include <hisp_internel.h>

#define MEM_MAP_MAX_SIZE             (0x40000)
#define MEM_SHARED_SIZE              (0x1000)
#define ISP_IOCTL_MAGIC              (0x70)
#define ISP_IOCTL_MAX_NR             (0x10)
#define POLLING_TIME_NS              (10)
#define POLLING_TIMEOUT_NS           (400)
#define ISP_LOG_MAX_SIZE             (64)
#define ISP_LOG_TMP_SIZE             (0x1000)
#define ISP_RDR_SIZE                 (0x40000)

#define NATIONAL_BETA_VERSION        (3)
#define OVERSEC_BETA_VERSION         (5)

#define ISPCPU_LOG_FORCE_UART        (1 << 29)
#define ISPCPU_LOG_LEVEL_WATCHDOG    (1 << 28)
#define ISPCPU_LOG_RESERVE_27        (1 << 27)
#define ISPCPU_LOG_RESERVE_26        (1 << 26)
#define ISPCPU_LOG_RESERVE_25        (1 << 25)
#define ISPCPU_LOG_RESERVE_24        (1 << 24)
#define ISPCPU_LOG_RESERVE_23        (1 << 23)
#define ISPCPU_LOG_RESERVE_22        (1 << 22)
#define ISPCPU_LOG_RESERVE_21        (1 << 21)
#define ISPCPU_LOG_RESERVE_20        (1 << 20)
#define ISPCPU_LOG_RESERVE_19        (1 << 19)
#define ISPCPU_LOG_RESERVE_18        (1 << 18)
#define ISPCPU_LOG_RESERVE_17        (1 << 17)
#define ISPCPU_LOG_RESERVE_16        (1 << 16)
#define ISPCPU_LOG_RESERVE_15        (1 << 15)
#define ISPCPU_LOG_RESERVE_14        (1 << 14)
#define ISPCPU_LOG_RESERVE_13        (1 << 13)
#define ISPCPU_LOG_RESERVE_12        (1 << 12)
#define ISPCPU_LOG_RESERVE_11        (1 << 11)
#define ISPCPU_LOG_LEVEL_BETA        (1 << 10)
#define ISPCPU_LOG_RESERVE_09        (1 << 9)
#define ISPCPU_LOG_RESERVE_08        (1 << 8)
#define ISPCPU_LOG_LEVEL_DEBUG_ALGO  (1 << 7)
#define ISPCPU_LOG_LEVEL_ERR_ALGO    (1 << 6)
#define ISPCPU_LOG_LEVEL_TRACE       (1 << 5)
#define ISPCPU_LOG_LEVEL_DUMP        (1 << 4)
#define ISPCPU_LOG_LEVEL_DBG         (1 << 3)
#define ISPCPU_LOG_LEVEL_INFO        (1 << 2)
#define ISPCPU_LOG_LEVEL_WARN        (1 << 1)
#define ISPCPU_LOG_LEVEL_ERR         (1 << 0)
#define ISPCPU_LOG_LEVEL_MASK        (0x1FFFFFFF)
#define ISPCPU_DEFAULT_LOG_LEVEL     (0x7)
/*
 * exc_flag:
 * bit 0:exc cur
 * bit 1:ion flag
 * bit 2:exc rtos save
 * bit 3:exc handler over
 */
struct log_user_para {
	unsigned int log_write;
	unsigned int log_head_size;
	unsigned int exc_flag;
	unsigned int boot_flag;
};

#define LOG_WR_OFFSET       _IOWR(ISP_IOCTL_MAGIC, 0x00, struct log_user_para)
#define LOG_VERSION_TYPE    _IOWR(ISP_IOCTL_MAGIC, 0x03, int)

/*
 * exc_flag:
 * bit 0:exc cur
 * bit 1:ion flag
 * bit 2:exc rtos save
 * bit 3:exc handler over
 */
struct hisplog_device_s {
	struct device *ispdev;
	void __iomem *share_mem;
	struct hisp_shared_para *share_para;
	wait_queue_head_t wait_ctl;
	struct timer_list sync_timer;
	atomic_t open_cnt;
	int use_cacheable_rdr;
	int initialized;
	atomic_t timer_cnt;
	unsigned int local_loglevel;
	unsigned int version_type;
} hisplog_dev;

int hisplogcat_sync(void)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	dev->share_mem = hisp_get_ispcpu_shrmem_va();
	if (dev->share_mem == NULL) {
		pr_err("[%s] Failed: share_mem.%pK\n",
			__func__, dev->share_mem);
		return -ENOMEM;
	}
	dev->share_para = (struct hisp_shared_para *)dev->share_mem;

	if (dev->share_para->log_flush_flag)
		wake_up(&dev->wait_ctl);

	return 0;
}

static void hisplog_sync_timer_fn(struct timer_list *timer)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	if (hisplogcat_sync() < 0)
		pr_err("[%s] Failed: hisplogcat_sync.%pK\n",
			__func__, dev->share_para);

	mod_timer(timer,
		  jiffies + msecs_to_jiffies(POLLING_TIME_NS));
}

/*lint -save -e529 -e438*/
void hisplogcat_stop(void)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	pr_info("[%s] +\n", __func__);
	if (dev->initialized == 0) {
		pr_err("[%s] ISP RDR not ready\n", __func__);
		return;
	}

	if (atomic_read(&dev->open_cnt) == 0) {
		pr_err("[%s] Failed : device not ready open_cnt.%d\n",
			__func__, atomic_read(&dev->open_cnt));
		return;
	}

	if (atomic_read(&dev->timer_cnt) == 0) {
		pr_err("[%s] Failed : timer_cnt.%d...Nothing todo\n",
			__func__, atomic_read(&dev->timer_cnt));
		return;
	}

	del_timer_sync(&dev->sync_timer);
	atomic_set(&dev->timer_cnt, 0);
	if (hisplogcat_sync() < 0)
		pr_err("[%s] Failed: hisplogcat_sync\n", __func__);
	pr_info("[%s] -\n", __func__);
}

void hisplog_clear_info(void)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return;
	}

	dev->share_mem = hisp_get_ispcpu_shrmem_va();
	if (!dev->share_mem) {
		pr_err("[%s] Failed: share_mem.%pK\n",
			__func__, dev->share_mem);
		return;
	}

	dev->share_para = (struct hisp_shared_para *)dev->share_mem;
	dev->share_para->log_cache_write  = 0;
	dev->share_para->log_flush_flag   = 1;
	wake_up(&dev->wait_ctl);
}

int hisplogcat_start(void)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	pr_info("[%s] +\n", __func__);
	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	if (atomic_read(&dev->open_cnt) == 0) {
		pr_err("[%s] Failed : device not ready open_cnt.%d\n",
			__func__, atomic_read(&dev->open_cnt));
		return -ENODEV;
	}

	if (atomic_read(&dev->timer_cnt) != 0) {
		pr_err("[%s] Failed : timer_cnt.%d...stop isplogcat\n",
			__func__, atomic_read(&dev->timer_cnt));
		hisplogcat_stop();
	}

	mod_timer(&dev->sync_timer,
		  jiffies + msecs_to_jiffies(POLLING_TIME_NS));
	atomic_set(&dev->timer_cnt, 1);
	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hisplog_open(struct inode *inode, struct file *filp)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	pr_info("[%s] +\n", __func__);

	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	if (atomic_read(&dev->open_cnt) != 0) {
		pr_err("%s: Failed: has been opened\n", __func__);
		return -EBUSY;
	}

	atomic_inc(&dev->open_cnt);
	if (hisp_ispcpu_is_powerup())
		hisplogcat_start();
	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hisplog_ioctl_check(unsigned int cmd, unsigned long args)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	if (_IOC_TYPE(cmd) != ISP_IOCTL_MAGIC) {
		pr_err("[%s] type is wrong.\n", __func__);
		return -EINVAL;
	}

	if (_IOC_NR(cmd) >= ISP_IOCTL_MAX_NR) {
		pr_err("[%s] number is wrong.\n", __func__);
		return -EINVAL;
	}

	if (dev->share_para == NULL) {
		if (hisplogcat_sync() < 0)
			pr_err("[%s] Failed: hisplogcat_sync.%pK\n",
				__func__, dev->share_para);
		pr_err("[%s] Failed : share_para.%pK\n",
			__func__, dev->share_para);
		return -EAGAIN;
	}

	if (args == 0) {
		pr_err("[%s] cmd[%d] args NULL", __func__, cmd);

		return -EFAULT;
	}

	return 0;
}

static void hisplog_config_user_para(struct log_user_para *tmp)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	if ((!hisp_get_last_boot_state() && hisp_ispcpu_is_powerup()) == 1)
		tmp->boot_flag = 1;//first boot
	else
		tmp->boot_flag = 0;

	hisp_set_last_boot_state(hisp_ispcpu_is_powerup());

	if (dev->use_cacheable_rdr)
		dev->share_para->log_flush_flag = 0;

	tmp->log_write = dev->share_para->log_cache_write;
	tmp->log_head_size = dev->share_para->log_head_size;
	tmp->exc_flag = dev->share_para->exc_flag;
	pr_debug("[%s] write = %u, size = %d.\n", __func__,
			tmp->log_write, tmp->log_head_size);
}

static long hisplog_ioctl(struct file *filp, unsigned int cmd,
		unsigned long args)
{
	struct hisplog_device_s *dev = &hisplog_dev;
	struct log_user_para tmp;
	int ret;
	void __iomem *rdr_va;
	long jiffies_time;

	pr_debug("[%s] cmd.0x%x +\n", __func__, cmd);

	ret = hisplog_ioctl_check(cmd, args);
	if (ret < 0) {
		pr_err("[%s] failed: hisplog_ioctl_check.%d", __func__, ret);
		return ret;
	}

	switch (cmd) {
	case LOG_WR_OFFSET:
		jiffies_time = (long)msecs_to_jiffies(POLLING_TIMEOUT_NS);
		ret = wait_event_timeout(dev->wait_ctl,
				dev->share_para->log_flush_flag, jiffies_time);
		if (ret == 0) {
			pr_debug("[%s] wait timeout, ret.%d\n", __func__, ret);
			return -ETIMEDOUT;
		}

		hisplog_config_user_para(&tmp);

		rdr_va = hisp_rdr_va_get();
		if (rdr_va == NULL)
			pr_err("[%s] Failed : rdr_va is NULL\n", __func__);
		else
			__inval_dcache_area(rdr_va, ISP_RDR_SIZE);

		ret = (int)copy_to_user((void __user *)args, &tmp, sizeof(tmp));
		if (ret != 0) {
			pr_err("[%s] copy_to_user failed.\n", __func__);
			return -EFAULT;
		}
		break;

	case LOG_VERSION_TYPE:
		if (_IOC_SIZE(cmd) > sizeof(int)) {
			pr_err("cmd arg size too large");
			return -EINVAL;
		}

		ret = (int)copy_from_user(&dev->version_type,
			(void __user *)args, sizeof(int));
		if (ret != 0) {
			pr_err("[%s] copy_from_user failed.\n", __func__);
			return -EFAULT;
		}
		pr_info("[%s] version type.%d\n", __func__, dev->version_type);
		break;

	default:
		pr_err("[%s] don't support cmd.\n", __func__);
		break;
	};

	pr_debug("[%s] -\n", __func__);

	return 0;
}

static int hisplog_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct hisplog_device_s *dev = &hisplog_dev;
	u64 isprdr_addr;
	unsigned long size;
	int ret;

	pr_info("[%s] +\n", __func__);
	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	isprdr_addr = hisp_rdr_addr_get();
	if (isprdr_addr == 0) {
		pr_err("[%s] Failed : isprdr_addr.0\n", __func__);
		return -ENOMEM;
	}

	if (vma == NULL) {
		pr_err("%s: vma is NULL\n", __func__);
		return -EINVAL;
	}

	if (vma->vm_start == 0) {
		pr_err("[%s] Failed : vm_start.0x%lx\n",
			__func__, vma->vm_start);
		return -EINVAL;
	}

	size = vma->vm_end - vma->vm_start;
	if (size > MEM_MAP_MAX_SIZE) {
		pr_err("%s: size.0x%lx.\n", __func__, size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	ret = remap_pfn_range(vma, vma->vm_start,
			(isprdr_addr >> PAGE_SHIFT),
			size, vma->vm_page_prot);
	if (ret) {
		pr_err("[%s] remap_pfn_range failed, ret.%d\n", __func__, ret);
		return ret;
	}

	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hisplog_release(struct inode *inode, struct file *filp)
{
	struct hisplog_device_s *dev = &hisplog_dev;

	pr_info("[%s] +\n", __func__);
	if (dev->initialized == 0) {
		pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
		return -ENXIO;
	}

	if (atomic_read(&dev->open_cnt) <= 0) {
		pr_err("%s: Failed: has been closed\n", __func__);
		return -EBUSY;
	}

	hisplogcat_stop();
	dev->share_para = NULL;
	atomic_dec(&dev->open_cnt);
	pr_info("[%s] -\n", __func__);

	return 0;
}

struct level_switch_s loglevel[] = {
	{ISPCPU_LOG_USE_APCTRL, "yes", "no", "LOG Controlled by AP"},
	{ISPCPU_LOG_TIMESTAMP_FPGAMOD, "yes", "no", "LOG Timestamp syscounter"},
	{ISPCPU_LOG_FORCE_UART, "enable", "disable", "uart"},
	{ISPCPU_LOG_LEVEL_WATCHDOG, "enable", "disable", "watchdog"},
	{ISPCPU_LOG_RESERVE_27, "enable", "disable", "reserved 27"},
	{ISPCPU_LOG_RESERVE_26, "enable", "disable", "reserved 26"},
	{ISPCPU_LOG_RESERVE_25, "enable", "disable", "reserved 25"},
	{ISPCPU_LOG_RESERVE_24, "enable", "disable", "reserved 24"},
	{ISPCPU_LOG_RESERVE_23, "enable", "disable", "reserved 23"},
	{ISPCPU_LOG_RESERVE_22, "enable", "disable", "reserved 22"},
	{ISPCPU_LOG_RESERVE_21, "enable", "disable", "reserved 21"},
	{ISPCPU_LOG_RESERVE_20, "enable", "disable", "reserved 20"},
	{ISPCPU_LOG_RESERVE_19, "enable", "disable", "reserved 19"},
	{ISPCPU_LOG_RESERVE_18, "enable", "disable", "reserved 18"},
	{ISPCPU_LOG_RESERVE_17, "enable", "disable", "reserved 17"},
	{ISPCPU_LOG_RESERVE_16, "enable", "disable", "reserved 16"},
	{ISPCPU_LOG_RESERVE_15, "enable", "disable", "reserved 15"},
	{ISPCPU_LOG_RESERVE_14, "enable", "disable", "reserved 14"},
	{ISPCPU_LOG_RESERVE_13, "enable", "disable", "reserved 13"},
	{ISPCPU_LOG_RESERVE_12, "enable", "disable", "reserved 12"},
	{ISPCPU_LOG_RESERVE_11, "enable", "disable", "reserved 11"},
	{ISPCPU_LOG_LEVEL_BETA, "enable", "disable", "beta"},
	{ISPCPU_LOG_RESERVE_09, "enable", "disable", "reserved 9"},
	{ISPCPU_LOG_RESERVE_08, "enable", "disable", "reserved 8"},
	{ISPCPU_LOG_LEVEL_DEBUG_ALGO, "enable", "disable", "algodebug"},
	{ISPCPU_LOG_LEVEL_ERR_ALGO, "enable", "disable", "algoerr"},
	{ISPCPU_LOG_LEVEL_TRACE, "enable", "disable", "trace"},
	{ISPCPU_LOG_LEVEL_DUMP, "enable", "disable", "dump"},
	{ISPCPU_LOG_LEVEL_DBG, "enable", "disable", "dbg"},
	{ISPCPU_LOG_LEVEL_INFO, "enable", "disable", "info"},
	{ISPCPU_LOG_LEVEL_WARN, "enable", "disable", "warn"},
	{ISPCPU_LOG_LEVEL_ERR, "enable", "disable", "err"},
};

void hisploglevel_update(void)
{
	struct hisplog_device_s *dev = &hisplog_dev;
	struct hisp_shared_para *param = NULL;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		hisp_unlock_sharedbuf();
		return;
	}

	param->logx_switch = (param->logx_switch & ~ISPCPU_LOG_LEVEL_MASK) |
		(dev->local_loglevel & ISPCPU_LOG_LEVEL_MASK);
	pr_info("[%s] 0x%x = 0x%x\n", __func__, param->logx_switch,
			dev->local_loglevel);
	hisp_unlock_sharedbuf();
}

static ssize_t hisplogctrl_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	struct hisplog_device_s *dev = &hisplog_dev;
	struct hisp_shared_para *param = NULL;
	unsigned int logx_switch = 0;
	char *tmp = NULL;
	ssize_t size = 0;
	unsigned int i;
	int ret = 0;

	if (buf == NULL) {
		pr_err("[%s] Failed : buf.%pK\n", __func__, buf);
		return 0;
	}

	tmp = kzalloc(ISP_LOG_TMP_SIZE, GFP_KERNEL);
	if (tmp == NULL)
		return 0;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param != NULL)
		logx_switch = param->logx_switch;

	for (i = 0; i < ARRAY_SIZE(loglevel); i++)
		size += snprintf_s(tmp+size, ISP_LOG_MAX_SIZE,
			ISP_LOG_MAX_SIZE - 1, "[%s.%s] : %s\n",
			(param ?
			((logx_switch & loglevel[i].level) ?
			loglevel[i].enable_cmd : loglevel[i].disable_cmd) :
			"ispoffline"),
			((dev->local_loglevel & loglevel[i].level) ?
			loglevel[i].enable_cmd : loglevel[i].disable_cmd),
			loglevel[i].info);/*lint !e421 */

	hisp_unlock_sharedbuf();
	ret = memcpy_s((void *)buf, ISP_LOG_TMP_SIZE,
		       (void *)tmp, ISP_LOG_TMP_SIZE);
	if (ret != 0)
		pr_err("[%s] Failed : memcpy_s buf ISP_LOG_TMP_SIZE.%d\n",
				__func__, ret);

	kfree((void *)tmp);
	return size;
}

static void hisplogctrl_usage(void)
{
	unsigned int i = 0;

	pr_info("<Usage: >\n");
	for (i = 0; i < ARRAY_SIZE(loglevel); i++) {
		if ((loglevel[i].level == ISPCPU_LOG_USE_APCTRL) ||
		    (loglevel[i].level == ISPCPU_LOG_TIMESTAMP_FPGAMOD))
			continue;

		pr_info("echo <%s>:<%s/%s> > hisplogctrl\n", loglevel[i].info,
			loglevel[i].enable_cmd, loglevel[i].disable_cmd);
	}
}

static ssize_t hisplogctrl_store(struct device *pdev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct hisplog_device_s *dev = &hisplog_dev;
	unsigned int i = 0;
	int len = 0, flag = 0;
	char *p = NULL;

	if ((buf == NULL) || (count == 0)) {
		pr_err("[%s] Failed : buf.%pK, count.0x%lx\n",
				__func__, buf, count);
		return 0;
	}

	p = memchr(buf, ':', count);
	if (p == NULL)
		return (ssize_t)count;

	len = (int)(p - buf);
	for (i = 0; i < ARRAY_SIZE(loglevel); i++) {
		if ((loglevel[i].level == ISPCPU_LOG_USE_APCTRL) ||
		    (loglevel[i].level == ISPCPU_LOG_TIMESTAMP_FPGAMOD))
			continue;

		if (!strncmp(buf, loglevel[i].info, len)) {
			flag = 1;
			p += 1;
			if (!strncmp(p, loglevel[i].enable_cmd,
					(int)strlen(loglevel[i].enable_cmd)))
				dev->local_loglevel |= loglevel[i].level;
			else if (!strncmp(p, loglevel[i].disable_cmd,
					(int)strlen(loglevel[i].disable_cmd)))
				dev->local_loglevel &= ~(loglevel[i].level);
			else
				flag = 0;
			break;
		}
	}

	if (!flag)
		hisplogctrl_usage();

	if (hisp_log_beta_mode()) {
		if (dev->version_type == NATIONAL_BETA_VERSION ||
			dev->version_type == OVERSEC_BETA_VERSION) {
			dev->local_loglevel &= ~(ISPCPU_LOG_LEVEL_INFO);
			pr_info("[%s] beta version disable logi\n", __func__);
		}
	} else {
		dev->local_loglevel &= ~(ISPCPU_LOG_LEVEL_BETA);
	}

	hisploglevel_update();

	return (ssize_t)count;
}

/*lint -e846 -e514 -e778 -e866 -e84*/
static DEVICE_ATTR(isplogctrl, 0440, hisplogctrl_show,
		hisplogctrl_store);
/*lint +e846 +e514 +e778 +e866 +e84*/
static const struct file_operations hisplog_ops = {
	.open           = hisplog_open,
	.release        = hisplog_release,
	.unlocked_ioctl = hisplog_ioctl,
	.compat_ioctl   = hisplog_ioctl,
	.mmap           = hisplog_mmap,
	.owner          = THIS_MODULE,
};

static struct miscdevice hisplog_miscdev = {
	.minor  = 255,
	.name   = "isp_log",
	.fops   = &hisplog_ops,
};

static int __init hisplog_init(void)
{
#ifdef DEBUG_HISP
	struct hisplog_device_s *dev = &hisplog_dev;
	int ret = 0;

	pr_info("[%s] +\n", __func__);

	dev->initialized = 0;
	init_waitqueue_head(&dev->wait_ctl);

	ret = misc_register(&hisplog_miscdev);
	if (ret != 0) {
		pr_err("[%s] Failed : misc_register.%d\n", __func__, ret);
		return ret;
	}

	ret = device_create_file(hisplog_miscdev.this_device,
				 &dev_attr_isplogctrl);
	if (ret != 0)
		pr_err("[%s] Faield : device_create_file.%d\n", __func__, ret);

	dev->local_loglevel = ISPCPU_DEFAULT_LOG_LEVEL;
	atomic_set(&dev->open_cnt, 0);
	atomic_set(&dev->timer_cnt, 0);
	timer_setup(&dev->sync_timer, hisplog_sync_timer_fn, 0);
	dev->version_type =  0;
	dev->use_cacheable_rdr = 1;
	dev->ispdev = hisplog_miscdev.this_device;
	dev->initialized = 1;
	pr_info("[%s] -\n", __func__);
#endif
	return 0;
}

static void __exit hisplog_exit(void)
{
#ifdef DEBUG_HISP
	struct hisplog_device_s *dev = &hisplog_dev;

	pr_info("[%s] +\n", __func__);
	misc_deregister((struct miscdevice *)&hisplog_miscdev);
	dev->initialized = 0;
	pr_info("[%s] -\n", __func__);
#endif
}

module_init(hisplog_init);
module_exit(hisplog_exit);
MODULE_LICENSE("GPL v2");
