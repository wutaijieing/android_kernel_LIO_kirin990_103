/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2016. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/suspend.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/err.h>

#include "osl_sem.h"
#include <osl_generic.h>
#include <osl_thread.h>
#include <osl_spinlock.h>

#include <bsp_ipc.h>
#include <bsp_slice.h>
#include <bsp_ring_buffer.h>
#include <bsp_modem_log.h>
#include <bsp_pm_om.h>

#include <bsp_print.h>
#include <bsp_pcdev.h>
#include <mdrv_pcdev.h>
#include "pm_om_platform.h"
#include "securec.h"

#undef THIS_MODU
#define THIS_MODU mod_pm_om

#define IPC_ACPU_INT_SRC_CCPU_MODEM_LOG (IPC_ACPU_INT_SRC_CCPU_PM_OM)

#define modem_log_pr_err(fmt, ...) bsp_err(fmt, ##__VA_ARGS__)
#define modem_log_pr_err_once(fmt, ...) bsp_err(fmt, ##__VA_ARGS__)
#define modem_log_pr_debug(fmt, ...)  // pr_err("[modem log]: " fmt, ##__VA_ARGS__)
enum MODEM_LOG_FLAG {
    MODEM_DATA_OK = 1,
    MODEM_NR_DATA_OK = 2,
    MODEM_ICC_CHN_READY = 4,
    MODEM_NRICC_CHN_READY = 8,
};
u32 icc_channel_ready;
u32 icc_channel_pending;

/* ģ�鷵�ش����� */
enum MODEM_LOG_ERR_TYPE {
    MODEM_LOG_NO_MEM = -ENOMEM,
    MODEM_LOG_NO_USR_INFO = -35,
    MODEM_LOG_NO_IPC_SRC = -36,
};

struct logger_log {
    struct log_usr_info *usr_info;
    wait_queue_head_t wq;   /* The wait queue for reader */
    struct miscdevice misc; /* The "misc" device representing the log */
    struct mutex mutex;     /* The mutex that protects the @buffer */
    struct list_head logs;  /* The list of log channels */
    u32 fopen_cnt;
};

struct modem_log {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    struct wakeup_source *wake_lock;
#else
    struct wakeup_source wake_lock;
#endif
    struct notifier_block pm_notify;
    u32 init_flag;
};

static LIST_HEAD(modem_log_list);
struct modem_log g_modem_log;

/*lint --e{64,826,785,528}*/ /* 64&826:list_for_each_entry, 785 Too few initializers for struct define; 528:
                                module_init */

static void modem_log_wake_lock(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_stay_awake(g_modem_log.wake_lock);
#else
    __pm_stay_awake(&g_modem_log.wake_lock);
#endif
}

static void modem_log_wakeup_event(unsigned int msec)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_wakeup_event(g_modem_log.wake_lock, msec);
#else
    __pm_wakeup_event(&g_modem_log.wake_lock, msec);
#endif
}

static void modem_log_wake_unlock(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_relax(g_modem_log.wake_lock);
#else
    __pm_relax(&g_modem_log.wake_lock);
#endif
}

static struct logger_log *get_modem_log_from_name(const char *name)
{
    struct logger_log *log = NULL;

    list_for_each_entry(log, &modem_log_list, logs)
    {
        if (!strncmp(log->misc.name, name, 50)) {
            return log;
        }
    }

    return NULL;
}

struct logger_log *get_modem_log_from_minor(int minor)
{
    struct logger_log *log = NULL;

    list_for_each_entry(log, &modem_log_list, logs)
    {
        if (log->misc.minor == minor) {
            return log;
        }
    }

    return NULL;
}

int kernel_user_memcpy(void *dest, u32 dest_max, void *src, u32 count)
{ /*lint --e{715} suppress destMax not referenced*/
    unsigned long ret;
    unsigned long src_addr = (uintptr_t)src;
    src = (void *)(uintptr_t)src_addr;
    if (dest == NULL) {
        modem_log_pr_err("dest is NULL ptr\n");
        return -1;
    }
    ret = copy_to_user(dest, src, (unsigned long)count);
    if (ret != 0) {
        modem_log_pr_err("copy_to_user err\n");
        return -1;
    }
    return 0;
}

void modem_log_ring_buffer_get(struct log_usr_info *usr_info, struct ring_buffer *rb)
{
    int ret;
    if ((usr_info != NULL) && (usr_info->mem != NULL)) {
        rb->buf = usr_info->ring_buf;
        rb->read = usr_info->mem->read;
        rb->write = usr_info->mem->write;
        rb->size = usr_info->mem->size;
    } else {
        ret = memset_s((void *)rb, sizeof(*rb), 0, sizeof(*rb));
        if (ret) {
            modem_log_pr_err("rb memset ret = %d\n", ret);
        }
    }
}

static unsigned int modem_log_poll(struct file *file, poll_table *wait)
{
    struct logger_log *log = NULL;
    unsigned int ret = 0;
    u32 read = 0;
    u32 write = 0;

    modem_log_pr_debug("%s entry\n", __func__);
    if (!(file->f_mode & FMODE_READ)) {
        return ret;
    }

    log = file->private_data;
    if (log->usr_info && log->usr_info->mem) {
        read = log->usr_info->mem->read;
        write = log->usr_info->mem->write;
    }

    mutex_lock(&log->mutex);
    if (read != write) {
        ret = POLLIN | POLLRDNORM;
    }
    mutex_unlock(&log->mutex);

    if (!ret) {
        poll_wait(file, &log->wq, wait);
    }

    modem_log_pr_debug("%s exit\n", __func__);

    return ret;
}

/*
 * modem_log_read - our log's read() method
 * 1) O_NONBLOCK works
 * 2) If there are no log to read, blocks until log is written to
 *  3) Will set errno to EINVAL if read
 */

static ssize_t modem_log_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{ /*lint --e{715} suppress pos not referenced*/
    struct logger_log *log = (struct logger_log *)file->private_data;
    struct ring_buffer rb;
    ssize_t ret = 1; /*lint !e446 (Warning -- side effect in initializer)*/
    u32 len = 0;
    DEFINE_WAIT(wait); /*lint !e446 (Warning -- side effect in initializer)*/

    modem_log_pr_debug("%s entry\n", __func__);

    while (1) {
        mutex_lock(&log->mutex);

        prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

        if (!log->usr_info->mem_is_ok) {
            goto skip_read;
        }

        modem_log_ring_buffer_get(log->usr_info, &rb);

        if (log->usr_info->read_func) {
            ret = log->usr_info->read_func(log->usr_info, buf, (u32)count);
            if (ret) {
                mutex_unlock(&log->mutex);
                pr_err_once("[modem log]user(%s) read method invoked\n",
                            log->usr_info->dev_name); /* lint !e727: (Info -- Symbol '__print_once' not explicitly
                                                         initialized) */
                break;
            }
        }

        ret = is_ring_buffer_empty(&rb);
    skip_read:
        mutex_unlock(&log->mutex);
        if (!ret) /* has data to be read, break out */
        {
            break;
        }
        if (file->f_flags & O_NONBLOCK) {
            modem_log_pr_err_once(
                "try again, when read file flag is O_NONBLOCK\n"); /*lint !e727: (Info -- Symbol '__print_once' not explicitly initialized)*/
            ret = -EAGAIN;
            break;
        }

        if (signal_pending(current)) {
            ret = -EINTR;
            break;
        }

        modem_log_wake_unlock(); /*lint !e455*/
        schedule();
        modem_log_pr_debug("give up cpu in modem_log_read\n");
    }

    finish_wait(&log->wq, &wait);
    if (ret) {
        return ret;
    }

    mutex_lock(&log->mutex);

    len = bsp_ring_buffer_readable_size(&rb);
    len = min(len, (u32)count);
    ret = bsp_ring_buffer_out(&rb, buf, len, (MEMCPY_FUNC)kernel_user_memcpy);
    if (ret < 0) {
        goto out;
    }

    log->usr_info->mem->read = rb.read;

    modem_log_pr_debug("%s exit\n", __func__);

out:
    mutex_unlock(&log->mutex);

    return ret;
}

/*
 * modem_log_open - our log's open() method, not support open twice by same application
 */
static int modem_log_open(struct inode *inode, struct file *file)
{
    struct logger_log *log;

    log = get_modem_log_from_minor(MINOR(inode->i_rdev));
    if (unlikely(NULL == log)) { /*lint !e730: (Info -- Boolean argument to function)*/
        modem_log_pr_err("device get fail\n");
        return -ENODEV;
    }

    file->private_data = log;

    mutex_lock(&log->mutex);
    if (log->usr_info->open_func && log->usr_info->open_func(log->usr_info)) { /* generally writer tell reader memory
                                                                                  info  */
        modem_log_pr_err("user(%s) open method error, exit\n", log->usr_info->dev_name);
        mutex_unlock(&log->mutex);
        return -EINVAL;
    }

    if (log->usr_info->mem) {
        log->usr_info->mem->app_is_active = 1; /* reader is ready  */
    }

    log->fopen_cnt++;

    mutex_unlock(&log->mutex);

    /* indicate that application exit exceptionally, need to clear ring buffer */
    if (log->fopen_cnt > 1) {
        modem_log_pr_err("file_open_cnt=%d\n", log->fopen_cnt);
        mutex_lock(&log->mutex);
        if (log->usr_info->mem) {
            log->usr_info->mem->read = log->usr_info->mem->write;
        }
        mutex_unlock(&log->mutex);
    }

    modem_log_pr_debug("%s entry\n", __func__);

    return 0;
}

/*
 * modem_log_release - our log's close() method
 */
static int modem_log_release(struct inode *inode, struct file *file)
{ /*lint --e{715} suppress inode not referenced*/
    struct logger_log *log;

    log = file->private_data;
    if (unlikely(NULL == log)) { /*lint !e730: (Info -- Boolean argument to function)*/
        modem_log_pr_err("device release fail\n");
        return -ENODEV;
    }

    mutex_lock(&log->mutex);
    if (log->usr_info->close_func && log->usr_info->close_func(log->usr_info)) {
        modem_log_pr_err("user(%s) release method error, exit\n", log->usr_info->dev_name);
    }
    if (log->usr_info->mem) {
        log->usr_info->mem->app_is_active = 0; /* reader cannot work  */
    }
    mutex_unlock(&log->mutex);

    modem_log_wake_unlock();

    modem_log_pr_debug("%s entry\n", __func__);
    return 0;
}

static const struct file_operations modem_log_fops = {
    .read = modem_log_read,
    .poll = modem_log_poll,
    .open = modem_log_open,
    .release = modem_log_release,
};

/*
 * modem_log_wakeup_all - wake up all waitquque
 */
void modem_log_wakeup_all(void)
{
    struct logger_log *log = NULL;
    list_for_each_entry(log, &modem_log_list, logs)
    {
        if (log->usr_info->mem && log->usr_info->mem->read != log->usr_info->mem->write) {
            modem_log_wakeup_event(jiffies_to_msecs(HZ));
            if ((&log->wq) != NULL) {
                wake_up_interruptible(&log->wq);
            }
        }
    }
}

/*
 * modem_log_ipc_handler - wake up waitquque
 */
void modem_log_icc_handler(u32 data)
{ /*lint --e{715} suppress data not referenced*/
    modem_log_pr_debug("icc = 0x%x\n", data);
    modem_log_wakeup_all();
}

/*
 * modem_log_notify - modem log pm notify
 */
s32 modem_log_notify(struct notifier_block *notify_block, unsigned long mode, void *unused)
{ /*lint --e{715} suppress notify_block&unused not referenced*/
    struct logger_log *log = NULL;

    modem_log_pr_debug("entry\n");

    switch (mode) {
        case PM_POST_HIBERNATION:
            list_for_each_entry(log, &modem_log_list, logs)
            {
                if ((log->usr_info->mem) && (log->usr_info->mem->read != log->usr_info->mem->write) &&
                    (log->usr_info->mem->app_is_active)) {
                    modem_log_wakeup_event(jiffies_to_msecs(HZ));
                    if ((&log->wq) != NULL) {
                        wake_up_interruptible(&log->wq);
                    }
                }
            }
            break;
        default:
            break;
    }

    return 0;
}

/*
 * bsp_modem_log_register - tell modem log which user information is
 * @usr_info: information which modem log need to know
 * Returns 0 if success
 */
s32 bsp_modem_log_register(struct log_usr_info *usr_info)
{
    s32 ret = 0;
    struct logger_log *log = NULL;

    if (unlikely(NULL == usr_info)) { /*lint !e730: (Info -- Boolean argument to function)*/
        ret = (s32)MODEM_LOG_NO_USR_INFO;
        modem_log_pr_err("no user info to register\n");
        goto out;
    }

    log = kzalloc(sizeof(*log), GFP_KERNEL);
    if (log == NULL) {
        ret = (s32)MODEM_LOG_NO_MEM;
        goto out;
    }

    log->usr_info = usr_info;

    log->misc.minor = MISC_DYNAMIC_MINOR;
    log->misc.name = kstrdup(usr_info->dev_name, GFP_KERNEL);
    if (unlikely(log->misc.name == NULL)) { /*lint !e730: (Info -- Boolean argument to function)*/
        ret = (s32)MODEM_LOG_NO_MEM;
        goto out_free_log;
    }

    log->misc.fops = &modem_log_fops;
    log->misc.parent = NULL;

    init_waitqueue_head(&log->wq);
    mutex_init(&log->mutex);

    INIT_LIST_HEAD(&log->logs);
    list_add_tail(&log->logs, &modem_log_list);

    /* finally, initialize the misc device for this log */
    ret = misc_register(&log->misc);
    if (unlikely(ret)) { /*lint !e730: (Info -- Boolean argument to function)*/
        modem_log_pr_err("failed to register misc device for log '%s'!\n", log->misc.name); /*lint !e429*/
        goto out_rm_node;
    }

    modem_log_pr_debug("created log '%s'\n", log->misc.name);
    return 0; /*lint !e429*/
out_rm_node:
    list_del(&log->logs);
out_free_log:
    kfree(log);
out:
    return ret;
}

/*
 * bsp_modem_log_fwrite_trigger - trigger file write from ring buffer to log record file
 * @usr_info: information regitered to modem log
 */
void bsp_modem_log_fwrite_trigger(struct log_usr_info *usr_info)
{
    struct logger_log *log = NULL;

    if (usr_info == NULL) {
        modem_log_pr_err("no user info\n");
        return;
    }

    log = get_modem_log_from_name(usr_info->dev_name);
    if (log == NULL) {
        modem_log_pr_err("no device log\n");
        return;
    }

    /* if reader is not ready, no need to wakeup waitqueue */
    if (usr_info->mem && usr_info->mem->app_is_active) { /*lint !e456*/
        modem_log_wake_lock();         /*lint !e454*/
        if ((&log->wq) != NULL) {
            wake_up_interruptible(&log->wq);
        }
    }
    modem_log_pr_debug("%s\n", __func__); /*lint !e456*/
} /*lint !e454*/

/*
 * modem_log_fwrite_trigger_force - wakeup acore and trigger file write from ring buffer to log record file
 */
void modem_log_fwrite_trigger_force(void)
{
    modem_log_wakeup_all();
}
EXPORT_SYMBOL(modem_log_fwrite_trigger_force); /*lint !e19 */
                                               /*
 * bsp_modem_log_init - modem log init
 */
static s32 pm_om_icc_handler(u32 channel_id, u32 len, void *context)
{
    pm_om_icc_data_type data = 0;
    s32 ret;

    ret = bsp_icc_read(channel_id, (u8 *)&data, sizeof(data));
    if (sizeof(data) != (u32)ret) {
        modem_log_pr_err("icc read fail: 0x%x\n", ret);
        return ret;
    }
    switch (data) {
        case MODEM_DATA_OK:
                modem_log_icc_handler(data);
            break;
        case MODEM_ICC_CHN_READY:
            icc_channel_ready = 1;
            if (icc_channel_pending) {
                channel_id = PM_OM_ICC_ACORE_CHN_ID;
                ret = bsp_icc_send(ICC_CPU_MODEM, channel_id, (u8 *)&data, (u32)sizeof(data));
                if ((s32)sizeof(data) != ret) {
                    modem_log_pr_err("icc[0x%x] send fail: 0x%x\n", channel_id, ret);
                    return ret;
                }
                icc_channel_pending = 0;
            }
            break;
        default:
            break;
    }
    return 0;
}
int modem_log_init(void)
{
    u32 channel_id;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    g_modem_log.wake_lock = wakeup_source_register(NULL, "modem_log_wake");
    if (g_modem_log.wake_lock == NULL) {
        modem_log_pr_err("wakeup_source_register modem_log_wake fail!\n");
        return -1;
    }
#else
    wakeup_source_init(&g_modem_log.wake_lock, "modem_log_wake");
#endif

    g_modem_log.pm_notify.notifier_call = modem_log_notify;
    register_pm_notifier(&g_modem_log.pm_notify);

    channel_id = PM_OM_ICC_ACORE_CHN_ID;
    if (bsp_icc_event_register(channel_id, pm_om_icc_handler, NULL, NULL, NULL)) {
        goto icc_err;
    }
    g_modem_log.init_flag = 1;
    modem_log_pr_err("init ok\n");

    return 0;
icc_err:
    modem_log_pr_err("icc chn =%d err\n", channel_id);
    return -1;
}

#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
module_init(modem_log_init);
#endif
