/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: log buff implement
 * Create:2019.11.05
 */

#include "iomcu_logbuff.h"
#include <securec.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/reboot.h>
#include <linux/timer.h>

#include "iomcu_ipc.h"
#include "platform_include/smart/linux/iomcu_boot.h"
#include "plat_func.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ddr_map.h"
#include "common/common.h"

#define LOG_BUFF_SIZE (1024 * 4)
#define DDR_LOG_BUFF_UPDATE_NR (DDR_LOG_BUFF_SIZE / LOG_BUFF_SIZE)
#define DDR_LOG_BUFF_UPDATE_WATER_MARK (DDR_LOG_BUFF_UPDATE_NR * 3 / 4)
#define DDR_LOG_BUFF_COPY_SIZE (DDR_LOG_BUFF_UPDATE_WATER_MARK * LOG_BUFF_SIZE)

#define LOG_SERIAL 1
#define LOG_BUFF 2

/* unite jiffies */
#define FLUSH_TIMEOUT (2*HZ)

static int g_is_opened;
static int g_is_new_data_available;
static int g_is_flush_complete;
static int g_flush_cnt;

static uint32_t g_sensorhub_log_r;
static uint32_t g_sensorhub_log_buf_head;
static uint32_t g_sensorhub_log_buf_rear;
static uint8_t *g_ddr_log_buff;
static uint8_t *g_local_log_buff;
static struct mutex g_logbuff_mutex;
static struct mutex g_logbuff_flush_mutex;
static struct proc_dir_entry *g_logbuff_dentry;
static uint32_t g_log_method = LOG_BUFF;
static uint32_t g_sensorhub_log_full_flag;
static bool g_inited;

#define CONFIG_FLUSH '1'
#define CONFIG_SERIAL '2'
#define CONFIG_BUFF '3'

#define NO_LOG_DEFAULT_LEVEL EMG_LEVEL

static DECLARE_WAIT_QUEUE_HEAD(g_sensorhub_log_waitq);
static DECLARE_WAIT_QUEUE_HEAD(g_sensorhub_log_flush_waitq);

static inline void print_stat(int i)
{
	ctxhub_dbg("[%d][r %x][head %x][rear %x][full_flag %d]\n",
		i, g_sensorhub_log_r, g_sensorhub_log_buf_head,
		g_sensorhub_log_buf_rear, g_sensorhub_log_full_flag);
}

static unsigned int sensorhub_log_buff_left(void)
{
	ctxhub_dbg("%s %d\n", __func__,
		(g_sensorhub_log_buf_rear >= g_sensorhub_log_r) ?
		(g_sensorhub_log_buf_rear - g_sensorhub_log_r) :
		(DDR_LOG_BUFF_SIZE - (g_sensorhub_log_r -
		g_sensorhub_log_buf_rear)));
	if (g_sensorhub_log_full_flag &&
		(g_sensorhub_log_buf_rear == g_sensorhub_log_r))
		return 0;
	return (g_sensorhub_log_buf_rear >= g_sensorhub_log_r) ?
		(g_sensorhub_log_buf_rear - g_sensorhub_log_r) :
		(DDR_LOG_BUFF_SIZE - (g_sensorhub_log_r -
			g_sensorhub_log_buf_rear));
}

static void update_local_buff_index(uint32_t new_rear)
{
	/* update g_sensorhub_log_r */
	if (g_flush_cnt && (sensorhub_log_buff_left() >=
		(DDR_LOG_BUFF_SIZE - DDR_LOG_BUFF_COPY_SIZE)))
		g_sensorhub_log_r = new_rear;
	/* update g_sensorhub_log_buf_head */
	if (g_flush_cnt)
		g_sensorhub_log_buf_head = new_rear;
	/* update g_sensorhub_log_buf_rear */
	g_sensorhub_log_buf_rear = new_rear;
	g_sensorhub_log_full_flag = 0;
	ctxhub_dbg("[%s] %d %d %d\n",
		__func__, g_sensorhub_log_r,
		g_sensorhub_log_buf_head, g_sensorhub_log_buf_rear);
}

static int sensorhub_logbuff_open(struct inode *inode, struct file *file)
{
	ctxhub_info("[%s]\n", __func__);
	mutex_lock(&g_logbuff_mutex);
	if (g_is_opened) {
		ctxhub_err("%s sensorhub logbuff already opened !\n", __func__);
		mutex_unlock(&g_logbuff_mutex);
		return RET_FAIL;
	}
	g_sensorhub_log_r = g_sensorhub_log_buf_head;
	g_is_opened = 1;
	mutex_unlock(&g_logbuff_mutex);
	return 0;
}

static int sensorhub_logbuff_release(struct inode *inode, struct file *file)
{
	ctxhub_info("sensorhub logbuff release\n");
	mutex_lock(&g_logbuff_mutex);
	g_is_opened = 0;
	mutex_unlock(&g_logbuff_mutex);
	return 0;
}


static ssize_t logbuff_read(size_t cnt, char __user *buf)
{
	size_t remain;
	size_t ret;

	if (!buf)
		return 0;

	print_stat(2);
	ctxhub_dbg("[%s] copy cnt %u, %x\n", __func__, cnt, g_sensorhub_log_r);
	if (g_sensorhub_log_r > DDR_LOG_BUFF_SIZE) {
		remain = 0;
		ctxhub_err("%s g_sensorhub_log_r is too large\n", __func__);
	} else {
		remain = DDR_LOG_BUFF_SIZE - g_sensorhub_log_r;
	}
	ret = cnt;
	/* copy to user */
	if (cnt > remain) {
		if (copy_to_user(
			buf + remain, g_local_log_buff, (cnt - remain)
			)) {
			ctxhub_err("%s failed to copy to user\n", __func__);
			ret = 0;
			goto out;
		}
		cnt = remain;
	}

	if ((g_sensorhub_log_r + cnt) > DDR_LOG_BUFF_SIZE) {
		ctxhub_err("%s read size overflow:%u,%u\n", __func__, g_sensorhub_log_r, cnt);
		ret = 0;
		goto out;
	}

	if (g_sensorhub_log_r < DDR_LOG_BUFF_SIZE && cnt > 0) {
		if (copy_to_user(buf, g_local_log_buff + g_sensorhub_log_r, cnt)) {
			ctxhub_err("%s failed to copy to user\n", __func__);
			ret = 0;
			goto out;
		}
	}
	/* update reader pointer */
	g_sensorhub_log_r = (g_sensorhub_log_r + ret) % DDR_LOG_BUFF_SIZE;
	if (!g_sensorhub_log_full_flag &&
		g_sensorhub_log_r == g_sensorhub_log_buf_rear)
		g_sensorhub_log_full_flag = 1;

out:
	return ret;
}

static ssize_t sensorhub_logbuff_read(
	struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int ret;
	size_t cnt;
	ssize_t read_size = 0;

	cnt = (count > DDR_LOG_BUFF_COPY_SIZE) ?
		DDR_LOG_BUFF_COPY_SIZE : count;
	ctxhub_dbg("[%s]\n", __func__);
	if (!buf || !count || !g_local_log_buff)
		goto err;

	/* check cap */
	if (sensorhub_log_buff_left() >= cnt)
		goto copy;

	ret = wait_event_interruptible( //lint !e578
		g_sensorhub_log_waitq, g_is_new_data_available != 0);
	if (ret) {
		goto err;
	}
copy:
	mutex_lock(&g_logbuff_mutex);
	g_is_new_data_available = 0;
	read_size = logbuff_read(cnt, buf);
	mutex_unlock(&g_logbuff_mutex);
err:
	print_stat(3);
	return read_size;
}

static const struct file_operations g_sensorhub_logbuff_operations = {
	.open = sensorhub_logbuff_open,
	.read = sensorhub_logbuff_read,
	.release = sensorhub_logbuff_release,
};

static int logbuff_full_callback(const struct pkt_header *head)
{
	int cnt = DDR_LOG_BUFF_COPY_SIZE;
	uint32_t remain;
	int ret;
	uint32_t update_index;
	uint32_t new_rear;
	log_buff_req_t *pkt = (log_buff_req_t *) head;

	if (pkt->index > DDR_LOG_BUFF_SIZE / LOG_BUFF_SIZE) {
		ctxhub_err("%s index is too large, log maybe lost\n", __func__);
		update_index = DDR_LOG_BUFF_SIZE;
	} else {
		update_index = (pkt->index * LOG_BUFF_SIZE);
	}
	new_rear = (update_index + DDR_LOG_BUFF_COPY_SIZE) % DDR_LOG_BUFF_SIZE;
	ctxhub_info("[%s]\n", __func__);
	if (update_index != g_sensorhub_log_buf_rear)
		ctxhub_err("%s unsync index, log maybe lost\n", __func__);
	/* get rotate log buff index */
	mutex_lock(&g_logbuff_mutex);
	print_stat(4); /* stat 4 */
	update_local_buff_index(new_rear);
	print_stat(5); /* stat 5 */
	remain = DDR_LOG_BUFF_SIZE - update_index;
	/* update reader pointer */
	if (remain < DDR_LOG_BUFF_COPY_SIZE) {
		ret = memcpy_s(g_local_log_buff, DDR_LOG_BUFF_SIZE, g_ddr_log_buff, cnt - remain);
		if (ret != EOK) {
			mutex_unlock(&g_logbuff_mutex);
			ctxhub_err("[%s] memcpy_s fail[%d]\n", __func__, ret);
			return ret;
		}

		cnt = remain;
	}

	ret = memcpy_s(g_local_log_buff + update_index, (DDR_LOG_BUFF_SIZE - update_index),
			g_ddr_log_buff + update_index, cnt);
	if (ret != EOK) {
		mutex_unlock(&g_logbuff_mutex);
		ctxhub_err("[%s] memcpy_s fail[%d]\n", __func__, ret);
		return ret;
	}

	if (!g_flush_cnt)
		g_flush_cnt = 1;

	g_is_new_data_available = 1;
	mutex_unlock(&g_logbuff_mutex);

	/* wake up reader */
	ctxhub_dbg("[%s] wakeup\n", __func__);
	wake_up_interruptible(&g_sensorhub_log_waitq);
	return 0;
}

static int logbuff_flush_callback(const struct pkt_header *head)
{
	/* sensorhub has flush tcm log buff
	 * we need update logbuff global vars and flush it to file system
	 */
	log_buff_req_t *pkt = (log_buff_req_t *) head;
	uint32_t flush_head = (pkt->index * LOG_BUFF_SIZE);
	uint32_t flush_size;
	uint32_t remain;
	int ret;
	int timeout_cnt = 100;

	flush_size = (flush_head > g_sensorhub_log_buf_rear) ?
		(flush_head - g_sensorhub_log_buf_rear) :
		(DDR_LOG_BUFF_SIZE - (g_sensorhub_log_buf_rear - flush_head));
	ctxhub_dbg("[%s] index: %d\n", __func__, pkt->index);
	/* wait reader till we can update the head */
	while (sensorhub_log_buff_left() >
		(DDR_LOG_BUFF_COPY_SIZE - flush_size) && timeout_cnt--)
		msleep(1);
	if (timeout_cnt < 0)
		ctxhub_warn("%s timeout, some log will lost", __func__);

	/* get rotate log buff index */
	mutex_lock(&g_logbuff_mutex);
	remain = DDR_LOG_BUFF_SIZE - g_sensorhub_log_buf_rear;
	if (remain < flush_size) {
		ret = memcpy_s(g_local_log_buff, DDR_LOG_BUFF_SIZE, g_ddr_log_buff,
			flush_size - remain);
		if (ret != EOK) {
			mutex_unlock(&g_logbuff_mutex);
			ctxhub_err("[%s] memcpy_s fail[%d]\n", __func__, ret);
			return ret;
		}

		flush_size = remain;
	}

	ret = memcpy_s(g_local_log_buff + g_sensorhub_log_buf_rear,
		DDR_LOG_BUFF_SIZE - g_sensorhub_log_buf_rear,
		g_ddr_log_buff + g_sensorhub_log_buf_rear,
		flush_size);
	if (ret != EOK) {
		mutex_unlock(&g_logbuff_mutex);
		ctxhub_err("[%s] memcpy_s fail[%d]\n", __func__, ret);
		return ret;
	}

	print_stat(6); /* stat 6 */
	update_local_buff_index(flush_head);
	print_stat(7); /* stat 7 */
	if (!g_flush_cnt)
		g_flush_cnt = 1;

	g_is_new_data_available = 1;
	mutex_unlock(&g_logbuff_mutex);

	/* wake up reader */
	wake_up_interruptible(&g_sensorhub_log_waitq);
	g_is_flush_complete = 1;
	wake_up_interruptible(&g_sensorhub_log_flush_waitq);
	return 0;
}

static void __manual_flush(const struct pkt_header *pkt, int size)
{
	struct write_info winfo;
	int ret;

	ctxhub_dbg("flush sensorhub log buff\n");
	/* do log flush */
	mutex_lock(&g_logbuff_flush_mutex);
	winfo.tag = pkt->tag;
	winfo.cmd = CMD_LOG_BUFF_FLUSH;
	winfo.wr_len = pkt->length;
	winfo.wr_buf = NULL;
	ret = write_customize_cmd(&winfo, NULL, true);
	if (ret != 0)
		ctxhub_err("%s, send cmd failed!\n", __func__);
	if (!wait_event_interruptible_timeout( //lint !e578
		g_sensorhub_log_flush_waitq,
		g_is_flush_complete != 0, FLUSH_TIMEOUT))
		ctxhub_err("%s no response", __func__);
	ctxhub_dbg("flush sensorhub log buff done\n");
	g_is_flush_complete = 0;
	mutex_unlock(&g_logbuff_flush_mutex);
}

static ssize_t logbuff_config_set(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct write_info winfo;
	struct pkt_header pkt = {
		.tag = TAG_LOG_BUFF,
		.resp = NO_RESP,
		.length = 0
	};

	if (!buf || count == 0) {
		ctxhub_err("%s, inputhub pointer is null or buf len is 0\n", __func__);
		return 0;
	}

	winfo.tag = TAG_LOG_BUFF,
	winfo.wr_len = 0;
	winfo.wr_buf = NULL;

	if (buf[0] == CONFIG_FLUSH) {
		__manual_flush(&pkt, sizeof(pkt));
	} else if (buf[0] == CONFIG_SERIAL) {
		ctxhub_info("sensorhub log use serial port\n");
		winfo.cmd = CMD_LOG_SER_REQ;
		(void)write_customize_cmd(&winfo, NULL, true);
		g_log_method = LOG_SERIAL;
	} else if (buf[0] == CONFIG_BUFF) {
		ctxhub_info("sensorhub log use log buff\n");
		winfo.cmd = CMD_LOG_USEBUF_REQ;
		(void)write_customize_cmd(&winfo, NULL, true);
		g_log_method = LOG_BUFF;
	} else {
		ctxhub_err("%s wrong input, \'1\' for flush \'2\' for serial \'3\' for buff\n",
			__func__);
		return -EINVAL;
	}
	return count;
}

static ssize_t logbuff_config_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf_s(
		buf, PAGE_SIZE, "%s\n", (g_log_method == LOG_SERIAL) ? "serial" : "buff");
}

static ssize_t logbuff_flush_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pkt_header pkt = {
		.tag = TAG_LOG_BUFF,
		.resp = NO_RESP,
		.length = 0
	};

	/* do flush here */
	__manual_flush(&pkt, sizeof(pkt));
	return sprintf_s(buf, PAGE_SIZE, "0\n");
}

static DEVICE_ATTR(logbuff_config, S_IWUSR | S_IRUGO,
	logbuff_config_show, logbuff_config_set);

static DEVICE_ATTR(logbuff_flush, S_IRUGO, logbuff_flush_show, NULL);

static struct platform_device g_sensorhub_logbuff = {
	.name = "sensorhub_logbuff",
	.id = -1,
};

void reset_logbuff(void)
{
	struct config_on_ddr *config = get_config_on_ddr();

	mutex_lock(&g_logbuff_mutex);
	g_sensorhub_log_r = 0;
	g_sensorhub_log_buf_head = 0;
	g_sensorhub_log_buf_rear = 0;
	g_flush_cnt = 0;
	/* write the head to ddr config block */
	if (config != NULL)
		config->logbuff_cb_backup.mutex = 0;
	if (g_local_log_buff != NULL && g_ddr_log_buff != NULL) {
		(void)memset_s(g_local_log_buff, DDR_LOG_BUFF_SIZE, 0, DDR_LOG_BUFF_SIZE);
		(void)memset_s(g_ddr_log_buff, DDR_LOG_BUFF_SIZE, 0, DDR_LOG_BUFF_SIZE);
	}
	mutex_unlock(&g_logbuff_mutex);
}

void emg_flush_logbuff(void)
{
	log_buff_req_t pkt;

	pkt.index = g_sensorhub_log_buf_rear / LOG_BUFF_SIZE;
	ctxhub_info("[%s] update head %x\n", __func__, pkt.index);
	if (!g_inited) {
		ctxhub_err("logbuff not g_inited!\n");
		return;
	}
	/* notify userspace */
	logbuff_full_callback((const struct pkt_header *)&pkt);
	msleep(100); // sleep 100 ticks
}

static struct delayed_work g_sensorhub_logbuff_off_check_work;

int set_log_level(int tag, int argv[], int argc)
{
	struct write_info pkg_ap;
	uint32_t log_level;
	int ret;
	struct config_on_ddr *config = get_config_on_ddr();

	if (argc != 1 || !argv)
		return RET_FAIL;

	log_level = argv[0];

	if (log_level > DEBUG_LEVEL)
		return RET_FAIL;

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));

	pkg_ap.tag = TAG_SYS;
	pkg_ap.cmd = CMD_SYS_LOG_LEVEL_REQ;
	pkg_ap.wr_buf = &log_level;
	pkg_ap.wr_len = sizeof(log_level);

	ret = write_customize_cmd(&pkg_ap, NULL, true);
	if (ret != 0) {
		ctxhub_err("%s faile to write cmd\n", __func__);
		return RET_FAIL;
	}

	if (config != NULL) {
		config->log_level = log_level;
		ctxhub_info("%s set log level %d success\n", __func__, log_level);
	}
	return 0;
}
static void  sensorhub_logbuff_off_check(struct work_struct *work)
{
	int level = NO_LOG_DEFAULT_LEVEL;

	ctxhub_info("%s\n", __func__);
	if (!g_is_opened)
		// set log level to emg
		set_log_level(TAG_LOG_BUFF, &level, 1);
}

int logbuff_dentry_init(void)
{
	g_logbuff_dentry = proc_create("sensorhub_logbuff", S_IRUSR | S_IRGRP,
		NULL, &g_sensorhub_logbuff_operations);
	if (g_logbuff_dentry == NULL) {
		ctxhub_err("%s failed to create g_logbuff_dentry\n", __func__);
		return RET_FAIL;
	}
	return 0;
}

int logbuff_device_register(void)
{
	int ret;

	ret = platform_device_register(&g_sensorhub_logbuff);
	if (ret) {
		ctxhub_err("%s: platform_device_register failed, ret:%d.\n",
			__func__, ret);
		return RET_FAIL;
	}
	return 0;
}

int creat_logbuff_config_file(void)
{
	int ret;

	ret = device_create_file(&g_sensorhub_logbuff.dev,
		&dev_attr_logbuff_config);
	if (ret) {
		ctxhub_err("%s: create %s file failed, ret:%d.\n",
			__func__, "dev_attr_logbuff_config", ret);
		return RET_FAIL;
	}
	return 0;
}

int creat_logbuff_flush_file(void)
{
	int ret;

	ret = device_create_file(&g_sensorhub_logbuff.dev,
		&dev_attr_logbuff_flush);
	if (ret) {
		ctxhub_err("%s: create %s file failed, ret:%d.\n",
			__func__, "dev_attr_logbuff_flush", ret);
		return RET_FAIL;
	}
	return 0;
}

int logbuff_register_alert_notifier(void)
{
	int ret;

	ret = register_mcu_event_notifier(TAG_LOG_BUFF,
		CMD_LOG_BUFF_ALERT, logbuff_full_callback);
	if (ret) {
		ctxhub_err("%s failed register notifier CMD_LOG_BUFF_ALERT:ret %d\n",
			__func__, ret);
		return RET_FAIL;
	}
	return 0;
}

int logbuff_register_event_notifier(void)
{
	int ret;

	ret = register_mcu_event_notifier(TAG_LOG_BUFF,
		CMD_LOG_BUFF_FLUSHP, logbuff_flush_callback);
	if (ret) {
		ctxhub_err("%s failed register notifier CMD_LOG_BUFF_FLUSH:ret %d\n",
			__func__, ret);
		return RET_FAIL;
	}
	return 0;
}

int remap_ddr_log_buff(void)
{
	g_ddr_log_buff = (uint8_t *)ioremap_wc(DDR_LOG_BUFF_ADDR_AP, DDR_LOG_BUFF_SIZE); //lint !e446
	if (g_ddr_log_buff == NULL) {
		ctxhub_err("%s failed remap log buff\n", __func__);
		return RET_FAIL;
	}
	return 0;
}

int mloc_local_log_buff(void)
{
	g_local_log_buff = (uint8_t *)vmalloc(DDR_LOG_BUFF_SIZE);
	if (g_local_log_buff == NULL) {
		ctxhub_err("%s failed to malloc\n", __func__);
		return RET_FAIL;
	}
	return 0;
}

static void logbuff_init_success(void)
{
	(void)memset_s(g_local_log_buff, DDR_LOG_BUFF_SIZE, 0, DDR_LOG_BUFF_SIZE);
	g_inited = true;
	INIT_DELAYED_WORK(&g_sensorhub_logbuff_off_check_work,
		sensorhub_logbuff_off_check);
	queue_delayed_work(system_freezable_wq,
		&g_sensorhub_logbuff_off_check_work, 5 * 60 * HZ); // delay 300 Hz
	ctxhub_info("[%s] done\n", __func__);
}

static int sensorhub_logbuff_init(void)
{
	if (get_contexthub_dts_status())
		return RET_FAIL;
	ctxhub_info("[%s]\n", __func__);
	if (!get_sensor_mcu_mode()) {
		ctxhub_err("%s :mcu boot fail, logbuff init err\n", __func__);
		return RET_FAIL;
	}

	mutex_init(&g_logbuff_mutex);
	mutex_init(&g_logbuff_flush_mutex);

	if (logbuff_dentry_init())
		goto PROC_ERR;
	if (logbuff_device_register())
		goto REGISTER_ERR;
	if (creat_logbuff_config_file())
		goto SYSFS_ERR_1;
	if (creat_logbuff_flush_file())
		goto SYSFS_ERR_2;
	if (logbuff_register_alert_notifier())
		goto NOTIFY_ERR;
	if (logbuff_register_event_notifier())
		goto NOTIFY_ERR_1;
	if (remap_ddr_log_buff())
		goto REMAP_ERR;
	if (mloc_local_log_buff())
		goto MALLOC_ERR;
	logbuff_init_success();
	return 0;
MALLOC_ERR:
	iounmap(g_ddr_log_buff);
	g_ddr_log_buff = NULL;
REMAP_ERR:
	unregister_mcu_event_notifier(TAG_LOG_BUFF,
		CMD_LOG_BUFF_FLUSHP, logbuff_flush_callback);
NOTIFY_ERR_1:
	unregister_mcu_event_notifier(TAG_LOG_BUFF,
		CMD_LOG_BUFF_ALERT, logbuff_full_callback);
NOTIFY_ERR:
	device_remove_file(&g_sensorhub_logbuff.dev, &dev_attr_logbuff_flush);
SYSFS_ERR_2:
	device_remove_file(&g_sensorhub_logbuff.dev, &dev_attr_logbuff_config);
SYSFS_ERR_1:
	platform_device_unregister(&g_sensorhub_logbuff);
REGISTER_ERR:
	proc_remove(g_logbuff_dentry);
PROC_ERR:
	mutex_destroy(&g_logbuff_mutex);
	mutex_destroy(&g_logbuff_flush_mutex);
	return RET_FAIL;
}

static void sensorhub_logbuff_exit(void)
{
	iounmap(g_ddr_log_buff);
	g_ddr_log_buff = NULL;
	vfree(g_local_log_buff);
	g_local_log_buff = NULL;
	unregister_mcu_event_notifier(
		TAG_LOG_BUFF, CMD_LOG_BUFF_FLUSHP, logbuff_flush_callback);
	unregister_mcu_event_notifier(
		TAG_LOG_BUFF, CMD_LOG_BUFF_ALERT, logbuff_full_callback);
	device_remove_file(&g_sensorhub_logbuff.dev, &dev_attr_logbuff_flush);
	device_remove_file(&g_sensorhub_logbuff.dev, &dev_attr_logbuff_config);
	platform_device_unregister(&g_sensorhub_logbuff);
	proc_remove(g_logbuff_dentry);
}

late_initcall_sync(sensorhub_logbuff_init);
module_exit(sensorhub_logbuff_exit);

MODULE_LICENSE("GPL");
