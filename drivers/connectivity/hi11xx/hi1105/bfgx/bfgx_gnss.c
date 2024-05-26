

/* Include Head file */
#include "bfgx_gnss.h"

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include "plat_debug.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "bfgx_dev.h"
#include "bfgx_ioctrl.h"
#include "bfgx_exception_rst.h"
#include "bfgx_data_parse.h"
#include "platform_common_clk.h"

/*
* 平台处理逻辑：
* sleep_vote ==1 && frc_awake == 0  :  sleep
* sleep_vote == 1 && frc_awake == 1:   work
* sleep_vote == 0 && frc_awake == 1:   work
* sleep_vote == 0 && frc_awake == 0 :  5s后无GNSS数据收发，视为gnss业务异常，补投票， sleep
*/
STATIC int32_t gnss_set_frc_awake(struct pm_drv_data *pm_data, uint32_t frc)
{
    int32_t ret;
    ret = prepare_to_visit_node(pm_data->ps_core_data);
    if (ret < 0) {
        return ret;
    }
    ps_print_info("[%s]gnss_frc_awake %d!\n", index2name(pm_data->index), frc);

    pm_data->gnss_frc_awake = (uint32_t)(frc & 0x1);

    post_to_visit_node(pm_data->ps_core_data);
    return ret;
}

STATIC void gnss_lowpower_vote(struct pm_drv_data *pm_data, uint32_t vote)
{
    if (oal_unlikely(pm_data == NULL)) {
        return;
    }
    ps_print_info("[%s]gnss_lowpower_vote %d!\n", index2name(pm_data->index), vote);
    if (vote == GNSS_AGREE_SLEEP) {
        if (pm_data->bfgx_lowpower_enable == BFGX_PM_DISABLE) {
            ps_print_warning("[%s]gnss low power disabled!\n", index2name(pm_data->index));
            return;
        }
        if (pm_data->bfgx_dev_state_get(pm_data) == BFGX_SLEEP) {
            ps_print_warning("[%s]gnss proc: dev has been sleep, not allow dev slp\n", index2name(pm_data->index));
            return;
        }

        /* set the flag to 1 means gnss request sleep */
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);

        /* if bt and fm are both shutdown ,we will pull down gpio directly */
        if (!timer_pending(&pm_data->bfg_timer) && (gnss_get_lowpower_state(pm_data) == GNSS_AGREE_SLEEP)) {
            ps_print_info("[%s]gnss_lowpower_vote sumit sleep work!\n", index2name(pm_data->index));

            if (queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work) != true) {
                ps_print_info("[%s]queue_work send_allow_sleep_work not return true\n", index2name(pm_data->index));
            }
        }
    } else if (vote == GNSS_NOT_AGREE_SLEEP) {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);
    } else {
        ps_print_err("PLAT_GNSS_SLEEP_VOTE_CMD invalid arg !\n");
    }
}

STATIC int32_t gnss_sepreted_skb_data_read(struct sk_buff_head *read_queue, uint32_t uart)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint8_t seperate_tag;

    ps_core_d = ps_get_core_reference(uart);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    oal_spin_lock(&ps_core_d->bfgx_info[BFGX_GNSS].rx_lock);
    do {
        if ((skb = ps_skb_dequeue(ps_core_d, RX_GNSS_QUEUE)) == NULL) {
            oal_spin_unlock(&ps_core_d->bfgx_info[BFGX_GNSS].rx_lock);
            if (read_queue->qlen != 0) {
                // 没有找到last包，skb queue就空了
                ps_print_err("skb dequeue error, qlen=%x!\n", read_queue->qlen);
                return -EFAULT;
            } else {
                ps_print_info("gnss read no data!\n");
                return 0;
            }
        }

        seperate_tag = skb->data[skb->len - 1];
        if ((seperate_tag != GNSS_SEPER_TAG_INIT) && (seperate_tag != GNSS_SEPER_TAG_LAST)) {
            ps_print_err("seperate_tag=%x not support\n", seperate_tag);
            seperate_tag = GNSS_SEPER_TAG_LAST;
        }

        skb_queue_tail(read_queue, skb);
    } while (seperate_tag == GNSS_SEPER_TAG_INIT);
    oal_spin_unlock(&ps_core_d->bfgx_info[BFGX_GNSS].rx_lock);

    return 0;
}

STATIC int32_t gnss_read_data_to_user(struct sk_buff_head *read_queue, char __user *buf, size_t count)
{
    struct sk_buff *skb = NULL;
    int32_t data_len;
    int32_t copy_cnt;
    uint32_t ret;

    copy_cnt = 0;
    while (read_queue->qlen > 0) {
        skb = skb_dequeue(read_queue);
        if (unlikely(skb == NULL)) {
            ps_print_err("copy dequeue error, copy_cnt=%x\n", copy_cnt);
            return -EINVAL;
        }

        if (unlikely(skb->len <= 1)) {
            ps_print_err("skb len error,skb->len=%x,copy_cnt=%x,count=%x\n", skb->len, copy_cnt, (uint32_t)count);
            goto data_error;
        }

        data_len = (int32_t)(skb->len - 1);
        if (unlikely(data_len + copy_cnt > count)) {
            ps_print_err("copy total len error,skb->len=%x,tag=%x,copy_cnt=%x,read_cnt=%x\n",
                         skb->len, skb->data[skb->len - 1], copy_cnt, (uint32_t)count);
            goto data_error;
        }

        ret = copy_to_user(buf + copy_cnt, skb->data, data_len);
        if (unlikely(ret != 0)) {
            ps_print_err("copy_to_user err,ret=%x,dest=%p,src=%p,tag:%x,count1=%x,copy_cnt=%x,read_cnt=%x\n",
                         ret, buf + copy_cnt, skb->data, skb->data[skb->len - 1], data_len, copy_cnt, (uint32_t)count);
            goto data_error;
        }

        copy_cnt += data_len;
        kfree_skb(skb);
    }

    return copy_cnt;

data_error:
    kfree_skb(skb);
    return -EFAULT;
}


#ifdef HAVE_HISI_GNSS
/*
 * Prototype    : hw_gnss_open
 * Description  : functions called from above gnss hal,when open gnss file
 * input        : "/dev/hwgnss"
 * output       : return 0 --> open is ok
 *                return !0--> open is false
 */
STATIC int32_t hw_gnss_open(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_drv_data *pm_data = NULL;
    struct pm_top* pm_top_data = pm_get_top();

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GNSS));
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        ps_print_err("%s param is error", __func__);
        return -EINVAL;
    }

    pm_data = ps_core_d->pm_data;
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -EINVAL;
    }

    mutex_lock(&(pm_top_data->host_mutex));

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);

    ret = hw_bfgx_open(BFGX_GNSS);

    oal_atomic_set(&(ps_core_d->bfgx_info[BFGX_GNSS].read_delay), GNSS_READ_DEFAULT_TIME);

    if (ret != BFGX_POWER_SUCCESS) {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    }

    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;
}

/*
 * Prototype    : hw_gnss_poll
 * Description  : called by gnss func from hal;
 *                check whether or not allow read and write
 */
STATIC uint32_t hw_gnss_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32_t mask = 0;

    ps_print_dbg("%s\n", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GNSS));
    if (unlikely((ps_core_d == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_GNSS].rx_wait, wait);

    ps_print_dbg("%s, recive gnss data\n", __func__);

    if (ps_core_d->bfgx_info[BFGX_GNSS].rx_queue.qlen) { /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/*
 * Prototype    : hw_gnss_read
 * Description  : functions called from above gnss hal,read count data to buf
 */
STATIC ssize_t hw_gnss_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    struct sk_buff_head read_queue;
    int32_t copy_cnt;

    if (unlikely(buf == NULL)) {
        ps_print_err("buf is NULL\n");
        return -EINVAL;
    }

    skb_queue_head_init(&read_queue);

    copy_cnt = gnss_sepreted_skb_data_read(&read_queue, service_get_bus_id(BFGX_GNSS));
    if (copy_cnt < 0) {
        goto read_error;
    }

    copy_cnt = gnss_read_data_to_user(&read_queue, buf, count);
    if (copy_cnt < 0) {
        goto read_error;
    }

    return copy_cnt;

read_error:
    while ((skb = skb_dequeue(&read_queue)) != NULL) {
        ps_print_err("free skb: len=%x, tag=%x\n", skb->len, skb->data[skb->len - 1]);
        kfree_skb(skb);
    }

    return -EFAULT;
}

/*
 * Prototype    : hw_gnss_write
 * Description  : functions called from above gnss hal,write count data to buf
 */
STATIC ssize_t hw_gnss_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    int32_t ret = 0;
    struct pm_drv_data *pm_data = NULL;
    PS_PRINT_FUNCTION_NAME;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GNSS));
    if (oal_unlikely((ps_core_d == NULL) || (ps_core_d->pm_data == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    pm_data = ps_core_d->pm_data;

    ret = hw_bfgx_write_check(filp, buf, ps_core_d, BFGX_GNSS);
    if (ret < 0) {
        return ret;
    }

    if (count > GNSS_TX_MAX_FRAME) {
        ps_print_err("err:gnss packet is too large!\n");
        return -EINVAL;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (oal_netbuf_list_len(&ps_core_d->tx_low_seq) > TX_LOW_QUE_MAX_NUM) {
        return 0;
    }

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);
    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0) {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        ps_print_err("prepare work fail, bring to reset work\n");
        plat_exception_handler(SUBSYS_BFGX, THREAD_GNSS, BFGX_WAKEUP_FAIL);
        return ret;
    }

    oal_wake_lock_timeout(&ps_core_d->pm_data->gnss_wake_lock, DEFAULT_WAKELOCK_TIMEOUT);

    /* to divide up packet function and tx to tty work */
    if (ps_h2d_tx_data(ps_core_d, BFGX_GNSS, buf, count) < 0) {
        ps_print_err("hw_gnss_write is err\n");
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        post_to_visit_node(ps_core_d);
        return -EFAULT;
    }

    ps_core_d->bfgx_info[BFGX_GNSS].tx_pkt_num++;

    post_to_visit_node(ps_core_d);

    return count;
}

/*
 * Prototype    : hw_gnss_ioctl
 * Description  : called by gnss func from hal when open power gpio or close power gpio
 */
STATIC long hw_gnss_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
    struct ps_core_s *ps_core_d = NULL;
    int32_t __user *puser;
    int32_t coeff_para[COEFF_NUM];

    PS_PRINT_FUNCTION_NAME;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GNSS));
    if (unlikely((ps_core_d == NULL) || (file == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    switch (cmd) {
        case GNSS_SET_READ_TIME:
            if (arg < GNSS_MAX_READ_TIME) { /* set timeout for gnss read function */
                oal_atomic_set(&(ps_core_d->bfgx_info[BFGX_GNSS].read_delay), arg);
            } else {
                ps_print_err("arg is too large!\n");
                return -EINVAL;
            }
            break;
        case PLAT_GNSS_DCXO_SET_PARA_CMD:
            puser = (int32_t __user *)arg;
            if (copy_from_user(coeff_para, puser, COEFF_NUM * sizeof(int32_t))) {
                ps_print_err("[dcxo] get gnss update para error\n");
                return -EINVAL;
            }
            update_dcxo_coeff(coeff_para, COEFF_NUM * sizeof(int32_t));
            break;
        case PLAT_GNSS_SLEEP_VOTE_CMD:
            gnss_lowpower_vote(ps_core_d->pm_data, arg);
            break;
        case PLAT_GNSS_FRC_AWAKE_CMD:
            gnss_set_frc_awake(ps_core_d->pm_data, arg);
            break;
        default:
            ps_print_warning("cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return 0;
}

/*
 * Prototype    : hw_gnss_release
 * Description  : called by gnss func from hal when close gnss inode
 */
STATIC int32_t hw_gnss_release(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_drv_data *pm_data = pm_get_drvdata(service_get_bus_id(BFGX_GNSS));
    struct pm_top* pm_top_data = pm_get_top();

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((inode == NULL) || (filp == NULL))) {
        ps_print_err("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&(pm_top_data->host_mutex));

    ret = hw_bfgx_close(BFGX_GNSS);

    oal_wake_unlock_force(&pm_data->gnss_wake_lock);

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    pm_data->gnss_frc_awake = 0;

    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;
}

STATIC const struct file_operations g_hw_gnss_fops = {
    .owner = THIS_MODULE,
    .open = hw_gnss_open,
    .write = hw_gnss_write,
    .read = hw_gnss_read,
    .poll = hw_gnss_poll,
    .unlocked_ioctl = hw_gnss_ioctl,
    .release = hw_gnss_release,
};

struct miscdevice g_hw_gnss_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwgnss",
    .fops = &g_hw_gnss_fops,
};

int32_t hw_gnss_misc_register(void)
{
    int32_t err;
    err = misc_register(&g_hw_gnss_device);
    if (err != 0) {
        ps_print_err("Failed to register gnss inode\n ");
    }
    return err;
}

void hw_gnss_misc_unregister(void)
{
    misc_deregister(&g_hw_gnss_device);
}

#endif

STATIC struct platform_device *g_hw_ps_me_device = NULL;
STATIC atomic_t g_me_oam_cnt = ATOMIC_INIT(0);
struct platform_device *get_me_platform_device(void)
{
    return g_hw_ps_me_device;
}

STATIC int32_t hw_gnss_me_open(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_drv_data *pm_data = pm_get_drvdata(service_get_bus_id(BFGX_ME));
    struct pm_top* pm_top_data = pm_get_top();

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((inode == NULL) || (filp == NULL))) {
        ps_print_err("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&(pm_top_data->host_mutex));

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);

    ret = gnss_me_open();
    if (ret != BFGX_POWER_SUCCESS) {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    }

    mutex_unlock(&(pm_top_data->host_mutex));
    return ret;
}

STATIC uint32_t hw_gnss_me_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32_t mask = 0;

    ps_print_dbg("%s\n", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely((ps_core_d == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_GNSS].rx_wait, wait);

    ps_print_dbg("%s, recive gnss me data\n", __func__);

    if (ps_core_d->bfgx_info[BFGX_GNSS].rx_queue.qlen) { /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

STATIC long hw_gnss_me_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
    {
        struct ps_core_s *ps_core_d = NULL;
        int32_t __user *puser;
        int32_t coeff_para[COEFF_NUM];

        PS_PRINT_FUNCTION_NAME;

        ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
        if (unlikely((ps_core_d == NULL) || (file == NULL))) {
            ps_print_err("ps_core_d is NULL\n");
            return -EINVAL;
        }

        switch (cmd) {
            case GNSS_SET_READ_TIME:
                if (arg < GNSS_MAX_READ_TIME) { /* set timeout for gnss read function */
                    oal_atomic_set(&(ps_core_d->bfgx_info[BFGX_GNSS].read_delay), arg);
                } else {
                    ps_print_err("arg is too large!\n");
                    return -EINVAL;
                }
                break;
            case PLAT_GNSS_DCXO_SET_PARA_CMD:
                puser = (int32_t __user *)arg;
                if (copy_from_user(coeff_para, puser, COEFF_NUM * sizeof(int32_t))) {
                    ps_print_err("[dcxo] get gnss update para error\n");
                    return -EINVAL;
                }
                update_dcxo_coeff(coeff_para, COEFF_NUM * sizeof(int32_t));
                break;
            case PLAT_GNSS_SLEEP_VOTE_CMD:
                gnss_lowpower_vote(ps_core_d->pm_data, arg);
                break;
            case PLAT_GNSS_FRC_AWAKE_CMD:
                gnss_set_frc_awake(ps_core_d->pm_data, arg);
                break;
            default:
                ps_print_warning("cmd = %d not find\n", cmd);
                return -EINVAL;
        }

        return 0;
    }


STATIC ssize_t hw_gnss_me_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    struct sk_buff_head read_queue;
    int32_t copy_cnt;

    if (unlikely(buf == NULL)) {
        ps_print_err("buf is NULL\n");
        return -EINVAL;
    }

    skb_queue_head_init(&read_queue);

    copy_cnt = gnss_sepreted_skb_data_read(&read_queue, service_get_bus_id(BFGX_ME));
    if (copy_cnt < 0) {
        goto read_error;
    }

    copy_cnt = gnss_read_data_to_user(&read_queue, buf, count);
    if (copy_cnt < 0) {
        goto read_error;
    }

    return copy_cnt;

read_error:
    while ((skb = skb_dequeue(&read_queue)) != NULL) {
        ps_print_err("free skb: len=%x, tag=%x\n", skb->len, skb->data[skb->len - 1]);
        kfree_skb(skb);
    }

    return -EFAULT;
}

STATIC ssize_t hw_gnss_me_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct pm_drv_data *pm_data = NULL;
    int32_t ret = 0;

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely((ps_core_d == NULL) || (buf == NULL) || (ps_core_d->pm_data == NULL))) {
        ps_print_err("ps_core_d or buf is NULL\n");
        return -EINVAL;
    }

    if (atomic_read(&ps_core_d->bfgx_info[BFGX_GNSS].subsys_state) == POWER_STATE_SHUTDOWN) {
        ps_print_warning("gnss me has closed! It's Not necessary to send msg to device\n");
        return -EINVAL;
    }

    if (count > GNSS_TX_MAX_FRAME) {
        ps_print_err("err:gnss me packet is too large!\n");
        return -EINVAL;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (oal_netbuf_list_len(&ps_core_d->tx_low_seq) > TX_LOW_QUE_MAX_NUM) {
        return 0;
    }
    pm_data = ps_core_d->pm_data;
    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0) {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        ps_print_err("prepare work fail, bring to reset work\n");
        plat_exception_handler(SUBSYS_BFGX, THREAD_GNSS, BFGX_WAKEUP_FAIL);
        return ret;
    }

    oal_wake_lock_timeout(&pm_data->gnss_wake_lock, DEFAULT_WAKELOCK_TIMEOUT);

    /* to divide up packet function and tx to tty work */
    if (ps_h2d_tx_data(ps_core_d, BFGX_GNSS, buf, count) < 0) {
        ps_print_err("hw_gnss_me_write is err\n");
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        post_to_visit_node(ps_core_d);
        return -EFAULT;
    }

    ps_core_d->bfgx_info[BFGX_GNSS].tx_pkt_num++;

    post_to_visit_node(ps_core_d);

    return (ssize_t)count;
}

STATIC int32_t hw_gnss_me_release(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_drv_data *pm_data = pm_get_drvdata(service_get_bus_id(BFGX_ME));
    struct pm_top* pm_top_data = pm_get_top();

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -EINVAL;
    }

    mutex_lock(&(pm_top_data->host_mutex));

    ret = gnss_me_close();

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    pm_data->gnss_frc_awake = 0;

    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;
}

STATIC int32_t hw_meoam_open(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_print_info("%s", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    oal_atomic_inc(&g_me_oam_cnt);
    ps_print_info("%s g_me_oam_cnt=%d\n", __func__, oal_atomic_read(&g_me_oam_cnt));

    oal_atomic_set(&ps_core_d->dbg_recv_dev_log, 1);

    oal_atomic_set(&ps_core_d->dbg_read_delay, DBG_READ_DEFAULT_TIME);

    return 0;
}

STATIC ssize_t hw_meoam_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    uint16_t count1 = 0;
    long timeout;

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely((ps_core_d == NULL) || (buf == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (oal_netbuf_list_len(&ps_core_d->rx_dbg_seq) == 0) {
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        timeout = oal_wait_event_interruptible_timeout_m(ps_core_d->rx_dbg_wait,
            (oal_netbuf_list_len(&ps_core_d->rx_dbg_seq) > 0),
            (long)msecs_to_jiffies(oal_atomic_read(&ps_core_d->dbg_read_delay)));
        if (!timeout) {
            ps_print_dbg("debug read time out!\n");
            return -ETIMEDOUT;
        }
    }

    if ((skb = ps_skb_dequeue(ps_core_d, RX_DBG_QUEUE)) == NULL) {
        ps_print_dbg("dbg read no data!\n");
        return -ETIMEDOUT;
    }

    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1)) {
        ps_print_err("debug copy_to_user is err!\n");
        ps_restore_skbqueue(ps_core_d, skb, RX_DBG_QUEUE);
        return -EFAULT;
    }

    skb_pull(skb, count1);

    if (skb->len == 0) {
        kfree_skb(skb);
    } else {
        ps_restore_skbqueue(ps_core_d, skb, RX_DBG_QUEUE);
    }

    return count1;
}

STATIC int32_t hw_meoam_release(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_print_info("%s", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    oal_atomic_dec(&g_me_oam_cnt);
    ps_print_info("%s g_me_oam_cnt=%d", __func__, oal_atomic_read(&g_me_oam_cnt));
    if (oal_atomic_read(&g_me_oam_cnt) == 0) {
        oal_wait_queue_wake_up_interrupt(&ps_core_d->rx_dbg_wait);
        atomic_set(&ps_core_d->dbg_recv_dev_log, 0);
        ps_kfree_skb(ps_core_d, RX_DBG_QUEUE);
    }

    return 0;
}

STATIC ssize_t hw_meexcp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    uint16_t count1;
    memdump_info_t *memdump_t = &g_gcpu_memdump_cfg;

    if (unlikely(buf == NULL)) {
        ps_print_err("buf is NULL\n");
        return -EINVAL;
    }

    if ((skb = skb_dequeue(&memdump_t->quenue)) == NULL) {
        return 0;
    }

    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1)) {
        ps_print_err("copy_to_user is err!\n");
        skb_queue_head(&memdump_t->quenue, skb);
        return -EFAULT;
    }

    skb_pull(skb, count1);

    if (skb->len == 0) {
        kfree_skb(skb);
    } else {
        skb_queue_head(&memdump_t->quenue, skb);
    }

    return count1;
}

STATIC long hw_meexcp_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
    int32_t ret = 0;

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    switch (cmd) {
        case PLAT_ME_DUMP_FILE_READ_CMD:
            ret = plat_bfgx_dump_rotate_cmd_read(ps_core_d, arg);
            break;

        default:
            ps_print_warning("hw_meoam_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return ret;
}

STATIC const struct file_operations g_hw_gnss_me_fops = {
    .owner = THIS_MODULE,
    .open = hw_gnss_me_open,
    .write = hw_gnss_me_write,
    .read = hw_gnss_me_read,
    .poll = hw_gnss_me_poll,
    .unlocked_ioctl = hw_gnss_me_ioctl,
    .release = hw_gnss_me_release,
};

STATIC const struct file_operations g_hw_meoam_fops = {
    .owner = THIS_MODULE,
    .open = hw_meoam_open,
    .read = hw_meoam_read,
    .release = hw_meoam_release,
};

STATIC const struct file_operations g_hw_meexcp_fops = {
    .owner = THIS_MODULE,
    .read = hw_meexcp_read,
    .unlocked_ioctl = hw_meexcp_ioctl,
};

#ifdef HAVE_HISI_GNSS
STATIC struct miscdevice g_hw_gnss_me_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwgnss_me",
    .fops = &g_hw_gnss_me_fops,
};
#endif

STATIC struct miscdevice g_hw_meoam_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwmeoam",
    .fops = &g_hw_meoam_fops,
};

STATIC struct miscdevice g_hw_meexcp_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwmeexcp",
    .fops = &g_hw_meexcp_fops,
};

STATIC struct hw_ps_plat_data g_hisi_me_platform_data = {
    .dev_name = "/dev/ttyAMA5",
    .flow_cntrl = FLOW_CTRL_ENABLE,
    .baud_rate = DEFAULT_BAUD_RATE,
    .ldisc_num = N_HW_GNSS,
    .suspend = NULL,
    .resume = NULL,
};

#ifdef _PRE_CONFIG_USE_DTS
STATIC int32_t ps_me_misc_dev_register(void)
{
#ifdef HAVE_HISI_GNSS
    int32_t err;
#endif

    if (is_bfgx_support() != OAL_TRUE) {
        /* don't support bfgx */
        ps_print_info("bfgx disabled, misc dev register bypass\n ");
        return 0;
    }

#ifdef HAVE_HISI_GNSS
    err = misc_register(&g_hw_gnss_me_device);
    if (err != 0) {
        ps_print_err("Failed to register gnss inode\n ");
        return -EFAULT;
    }
#endif

    return 0;
}

STATIC void ps_me_misc_dev_unregister(void)
{
    if (is_bfgx_support() != OAL_TRUE) {
        ps_print_info("bfgx disabled, misc dev unregister bypass\n ");
        return;
    }

#ifdef HAVE_HISI_GNSS
    misc_deregister(&g_hw_gnss_me_device);
    ps_print_info("misc gnss me device have removed\n");
#endif
}

STATIC void ps_me_baudrate_init(void)
{
    if (hi110x_is_asic() == VERSION_FPGA) {
        g_hisi_me_platform_data.baud_rate = UART_BAUD_RATE_2M;
    } else {
        g_hisi_me_platform_data.baud_rate = UART_BAUD_RATE_6M;
    }

    ps_print_info("ps me init baudrate=%ld\n", g_hisi_me_platform_data.baud_rate);
    return;
}

STATIC int32_t ps_me_probe(struct platform_device *pdev)
{
    struct hw_ps_plat_data *pdata = NULL;
    struct ps_plat_s *ps_plat_d = NULL;
    int32_t err;
    hi110x_board_info *bd_info = get_hi110x_board_info();
    uint32_t uart = service_get_bus_id(BFGX_ME);

    ps_me_baudrate_init();

    strncpy_s(g_hisi_me_platform_data.dev_name, sizeof(g_hisi_me_platform_data.dev_name),
              bd_info->uart_port[uart], sizeof(g_hisi_me_platform_data.dev_name) - 1);
    g_hisi_me_platform_data.dev_name[sizeof(g_hisi_me_platform_data.dev_name) - 1] = '\0';

    pdev->dev.platform_data = &g_hisi_me_platform_data;
    pdata = &g_hisi_me_platform_data;

    g_hw_ps_me_device = pdev;

    ps_plat_d = kzalloc(sizeof(struct ps_plat_s), GFP_KERNEL);
    if (ps_plat_d == NULL) {
        ps_print_err("no mem to allocate\n");
        return -ENOMEM;
    }
    dev_set_drvdata(&pdev->dev, ps_plat_d);

    err = ps_core_init(&ps_plat_d->core_data, uart);
    if (err != 0) {
        ps_print_err("me core init failed\n");
        goto err_core_init;
    }

    /* refer to itself */
    ps_plat_d->core_data->ps_plat = ps_plat_d;
    /* get reference of pdev */
    ps_plat_d->pm_pdev = pdev;

    /* copying platform data */
    if (strncpy_s(ps_plat_d->dev_name, sizeof(ps_plat_d->dev_name),
                  pdata->dev_name, HISI_UART_DEV_NAME_LEN - 1) != EOK) {
        ps_print_err("strncpy_s failed, please check!\n");
    }
    ps_plat_d->flow_cntrl = pdata->flow_cntrl;
    ps_plat_d->baud_rate = pdata->baud_rate;
    ps_plat_d->ldisc_num = pdata->ldisc_num;

    me_tty_recv = ps_core_recv;

    err = ps_me_misc_dev_register();
    if (err != 0) {
        goto err_misc_dev;
    }

    err = misc_register(&g_hw_meoam_device);
    if (err != 0) {
        ps_print_err("Failed to register hwmeoam inode\n");
        goto err_register_meoam;
    }

    err = misc_register(&g_hw_meexcp_device);
    if (err != 0) {
        ps_print_err("Failed to register hwmeexcp inode\n");
        goto err_register_meexcp;
    }

    ps_print_suc("%s is success!\n", __func__);

    return 0;

err_register_meexcp:
    misc_deregister(&g_hw_meoam_device);
err_register_meoam:
    ps_me_misc_dev_unregister();
err_misc_dev:
    ps_core_exit(ps_plat_d->core_data, uart);
err_core_init:
    kfree(ps_plat_d);
    ps_plat_d = NULL;
    return -EFAULT;
}

STATIC int32_t ps_me_remove(struct platform_device *pdev)
{
    struct ps_plat_s *ps_plat_d = NULL;

    misc_deregister(&g_hw_meexcp_device);
    misc_deregister(&g_hw_meoam_device);
    ps_me_misc_dev_unregister();

    ps_plat_d = dev_get_drvdata(&pdev->dev);
    if (ps_plat_d == NULL) {
        ps_print_err("ps_plat_d is null\n");
        return -EFAULT;
    }

    ps_core_exit(ps_plat_d->core_data, service_get_bus_id(BFGX_ME));
    kfree(ps_plat_d);
    ps_plat_d = NULL;

    return 0;
}

STATIC int32_t ps_me_suspend(struct platform_device *pdev, oal_pm_message_t state)
{
    return 0;
}

STATIC int32_t ps_me_resume(struct platform_device *pdev)
{
    return 0;
}

STATIC struct of_device_id g_hi110x_ps_me_match_table[] = {
    {
        .compatible = DTS_COMP_HI110X_PS_ME_NAME,
        .data = NULL,
    },
    {},
};
#endif

STATIC struct platform_driver g_ps_me_platform_driver = {
#ifdef _PRE_CONFIG_USE_DTS
    .probe = ps_me_probe,
    .remove = ps_me_remove,
    .suspend = ps_me_suspend,
    .resume = ps_me_resume,
#endif
    .driver = {
        .name = "hisi_me",
        .owner = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = g_hi110x_ps_me_match_table,
#endif
    },
};

int32_t hw_ps_me_init(void)
{
    int32_t ret;

    ret = platform_driver_register(&g_ps_me_platform_driver);
    if (ret) {
        ps_print_err("Unable to register platform me driver.\n");
    }

    return ret;
}

void hw_ps_me_exit(void)
{
    platform_driver_unregister(&g_ps_me_platform_driver);
}

MODULE_DESCRIPTION("Public serial Driver for huawei GNSS ME chips");
MODULE_LICENSE("GPL");

