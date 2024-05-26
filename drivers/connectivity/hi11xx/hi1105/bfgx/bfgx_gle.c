

/* Include Head file */
#include "bfgx_gle.h"

#include <linux/platform_device.h>

#include "plat_debug.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "bfgx_dev.h"
#include "bfgx_ioctrl.h"
#include "bfgx_exception_rst.h"
#include "bfgx_data_parse.h"
#include "platform_common_clk.h"

STATIC struct bt_data_combination g_gle_data_combination = {0};
STATIC struct platform_device *g_hw_ps_gle_device = NULL;
STATIC atomic_t g_gle_oam_cnt = ATOMIC_INIT(0);
struct platform_device *get_gle_platform_device(void)
{
    return g_hw_ps_gle_device;
}

STATIC int32_t gle_open(void)
{
    int32_t ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata(service_get_bus_id(BFGX_GLE));
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -EINVAL;
    }

    // gle 未加入上电配置，先关闭低功耗
    pm_data->bfgx_lowpower_enable = BFGX_PM_DISABLE;

    ps_core_d = pm_data->ps_core_data;
    if (alloc_seperted_rx_buf(ps_core_d, BFGX_GLE, g_bfgx_rx_max_frame[BFGX_GLE],
                              VMALLOC) != BFGX_POWER_SUCCESS) {
        return -ENOMEM;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_GLE];

    // gpio 控制上电，需要重构
    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        ret = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

    if (prepare_to_visit_node(ps_core_d) != BFGX_POWER_SUCCESS) {
        ret = BFGX_POWER_WAKEUP_FAIL;
        goto bfgx_wakeup_fail;
    }

    if (bfgx_open_cmd_send(BFGX_GLE, pm_data->index) != BFGX_POWER_SUCCESS) {
        ps_print_err("gle open cmd fail\n");
        ret = BFGX_POWER_OPEN_CMD_FAIL;
        goto bfgx_open_cmd_fail;
    }

    /* 单红外没有低功耗 */
    if (oal_atomic_read(&g_ir_only_mode) == 0) {
        mod_timer(&pm_data->bfg_timer, jiffies + msecs_to_jiffies(BT_SLEEP_TIME));
        oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
    }

    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_OPEN);
    post_to_visit_node(ps_core_d);

    if (oal_unlikely(ret != BFGX_POWER_SUCCESS)) {
        free_seperted_rx_buf(ps_core_d, BFGX_GLE, VMALLOC);
    }

bfgx_open_cmd_fail:
    post_to_visit_node(ps_core_d);
bfgx_wakeup_fail:
bfgx_power_on_fail:
    release_tty_drv(ps_core_d);
    ps_print_err("gle open cmd fail, need adapter fail process\n");

    return ret;
}

STATIC int32_t hw_gle_open(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_top* pm_top_data = pm_get_top();

    if (unlikely((inode == NULL) || (filp == NULL))) {
        ps_print_err("%s param is error", __func__);
        return -EINVAL;
    }

    ps_print_info("open gle");

    mutex_lock(&(pm_top_data->host_mutex));
    ret = gle_open();
    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;
}

STATIC uint32_t hw_gle_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32_t mask = 0;

    ps_print_dbg("%s\n", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (unlikely((ps_core_d == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_GLE].rx_wait, wait);

    ps_print_dbg("%s, recive gnss me data\n", __func__);

    if (ps_core_d->bfgx_info[BFGX_GLE].rx_queue.qlen) { /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

STATIC ssize_t hw_gle_read(struct file *filp, char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint16_t count1;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (unlikely((ps_core_d == NULL) || (buf == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if ((skb = ps_skb_dequeue(ps_core_d, RX_GLE_QUEUE)) == NULL) {
        ps_print_warning("bt read skb queue is null!\n");
        return 0;
    }

    /* read min value from skb->len or count */
    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1)) {
        ps_print_err("copy_to_user is err!\n");
        ps_restore_skbqueue(ps_core_d, skb, RX_GLE_QUEUE);
        return -EFAULT;
    }

    /* have read count1 byte */
    skb_pull(skb, count1);

    /* if skb->len = 0: read is over */
    if (skb->len == 0) { /* curr skb data have read to user */
        kfree_skb(skb);
    } else { /* if don,t read over; restore to skb queue */
        ps_restore_skbqueue(ps_core_d, skb, RX_GLE_QUEUE);
    }

    return count1;
}

STATIC int32_t hw_gle_data_send(struct bt_data_combination *data_combination,
                                       struct ps_core_s *ps_core_d,
                                       const char __user *buf,
                                       size_t count)
{
    struct sk_buff *skb = NULL;
    uint16_t total_len;

    total_len = count + data_combination->len + sizeof(struct ps_packet_head) +
                sizeof(struct ps_packet_end);

    skb = ps_alloc_skb(total_len);
    if (skb == NULL) {
        ps_print_err("ps alloc skb mem fail\n");
        return -EFAULT;
    }

    if (copy_from_user(&skb->data[sizeof(struct ps_packet_head) +
                       data_combination->len], buf, count)) {
        ps_print_err("copy_from_user from bt is err\n");
        kfree_skb(skb);
        return -EFAULT;
    }

    if (data_combination->len == 1) {
        skb->data[sizeof(struct ps_packet_head)] = data_combination->type;
    }

    ps_add_packet_head(skb->data, GLE_MSG, total_len);
    ps_skb_enqueue(ps_core_d, skb, TX_HIGH_QUEUE);
    queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);

    ps_core_d->bfgx_info[BFGX_GLE].tx_pkt_num++;

    return count;
}

STATIC ssize_t priv_gle_write(struct file *filp, const char __user *buf, size_t count,
                              struct bt_data_combination *data_comb)
{
    struct ps_core_s *ps_core_d = NULL;
    ssize_t ret = 0;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (hw_bfgx_write_check(filp, buf, ps_core_d, BFGX_GLE) < 0) {
        return -EINVAL;
    }

    if (count > GLE_TX_MAX_FRAME) {
        ps_print_err("bt skb len is too large!\n");
        return -EINVAL;
    }

    /* if high queue num > MAX_NUM and don't write */
    if (oal_netbuf_list_len(&ps_core_d->tx_high_seq) > TX_HIGH_QUE_MAX_NUM) {
        ps_print_err("gle tx high seqlen large than MAXNUM\n");
        return 0;
    }

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0) {
        ps_print_err("prepare work fail, bring to reset work\n");
        plat_exception_handler(SUBSYS_BFGX, THREAD_GLE, BFGX_WAKEUP_FAIL);
        return ret;
    }

    oal_wake_lock_timeout(&ps_core_d->pm_data->bt_wake_lock, DEFAULT_WAKELOCK_TIMEOUT);

    /* modify expire time of uart idle timer */
    oal_atomic_inc(&ps_core_d->pm_data->bfg_timer_mod_cnt);
    mod_timer(&ps_core_d->pm_data->bfg_timer, jiffies + msecs_to_jiffies(GLE_SLEEP_TIME));

    ret = hw_gle_data_send(data_comb, ps_core_d, buf, count);

    post_to_visit_node(ps_core_d);
    return ret;
}

STATIC ssize_t hw_gle_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t ret = 0;
    uint8_t type = 0;
    uint8_t __user *puser = (uint8_t __user *)buf;

    /* 因为蓝牙有组合发送，需要加锁保护 */
    mutex_lock(&(g_gle_data_combination.comb_mutex));

    /* 数据流不允许连续下发两次type */
    if ((g_gle_data_combination.len == BT_TYPE_DATA_LEN) && (count == BT_TYPE_DATA_LEN)) {
        g_gle_data_combination.len = 0;
        mutex_unlock(&(g_gle_data_combination.comb_mutex));
        ps_print_err("two consecutive type write!\n");
        return -EFAULT;
    }

    /* BT数据分两次下发，先发数据类型，长度固定为1Byte，然后发数据，需要在驱动中组合起来发给device */
    if ((count == BT_TYPE_DATA_LEN) && (puser != NULL)) {
        get_user(type, puser);
        g_gle_data_combination.type = type;
        g_gle_data_combination.len = count;
        mutex_unlock(&(g_gle_data_combination.comb_mutex));
        return count;
    }

    ret = priv_gle_write(filp, buf, count, &g_gle_data_combination);

    g_gle_data_combination.len = 0;
    mutex_unlock(&(g_gle_data_combination.comb_mutex));
    return ret;
}

STATIC int32_t hw_gle_release(struct inode *inode, struct file *filp)
{
    int32_t ret;
    struct pm_top* pm_top_data = pm_get_top();
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    mutex_lock(&(pm_top_data->host_mutex));

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (unlikely((ps_core_d == NULL) || (ps_core_d->pm_data == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }
    pm_data = ps_core_d->pm_data;
    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_GLE];

    if (atomic_read(&pst_bfgx_data->subsys_state) == POWER_STATE_SHUTDOWN) {
        ps_print_warning("[%s]%s has closed! It's Not necessary to send msg to device\n",
                         index2name(pm_data->index), service_get_name(BFGX_GLE));
        return BFGX_POWER_SUCCESS;
    }
    oal_wait_queue_wake_up_interrupt(&pst_bfgx_data->rx_wait);

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0) {
        /* 唤醒失败，bfgx close时的唤醒失败不进行DFR恢复 */
        ps_print_err("[%s]prepare work FAIL\n", index2name(pm_data->index));
    }

    ret = bfgx_close_cmd_send(BFGX_GLE, pm_data->index);
    if (ret < 0) {
        /* 发送close命令失败，不进行DFR，继续进行下电流程，DFR恢复延迟到下次open时或者其他业务运行时进行 */
        ps_print_err("[%s]bfgx close cmd fail\n", index2name(pm_data->index));
    }

    if (uart_bfgx_close_cmd(ps_core_d) != SUCCESS) {
        /* bfgx self close fail, go on */
        ps_print_err("gnss me self close fail\n");
    }

    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);

    if (release_tty_drv(ps_core_d) != SUCCESS) {
        /* 代码执行到此处，说明六合一所有业务都已经关闭，无论tty是否关闭成功，device都要下电 */
        ps_print_err("close gnss me tty is err!");
    }

    post_to_visit_node(ps_core_d);
    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_SHUTDOWN);

    free_seperted_rx_buf(ps_core_d, BFGX_GLE, VMALLOC);
    ps_kfree_skb(ps_core_d, g_bfgx_rx_queue[BFGX_GLE]);

    pst_bfgx_data->rx_pkt_num = 0;
    pst_bfgx_data->tx_pkt_num = 0;

    pm_data->uart_state = UART_NOT_READY;
    pm_data->bfgx_dev_state = BFGX_SLEEP;

    // need adapter bfg_wake_unlock

    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;
}

STATIC int32_t hw_gle_oam_open(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_print_info("%s", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    oal_atomic_inc(&g_gle_oam_cnt);
    ps_print_info("%s g_me_oam_cnt=%d\n", __func__, oal_atomic_read(&g_gle_oam_cnt));

    oal_atomic_set(&ps_core_d->dbg_recv_dev_log, 1);

    oal_atomic_set(&ps_core_d->dbg_read_delay, DBG_READ_DEFAULT_TIME);

    return 0;
}

STATIC ssize_t hw_gle_oam_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    uint16_t count1 = 0;
    long timeout;

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
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

STATIC int32_t hw_gle_oam_release(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_print_info("%s", __func__);

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    oal_atomic_dec(&g_gle_oam_cnt);
    ps_print_info("%s g_me_oam_cnt=%d", __func__, oal_atomic_read(&g_gle_oam_cnt));
    if (oal_atomic_read(&g_gle_oam_cnt) == 0) {
        oal_wait_queue_wake_up_interrupt(&ps_core_d->rx_dbg_wait);
        atomic_set(&ps_core_d->dbg_recv_dev_log, 0);
        ps_kfree_skb(ps_core_d, RX_DBG_QUEUE);
    }

    return 0;
}

STATIC ssize_t hw_gle_excp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
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

STATIC long hw_gle_excp_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
    int32_t ret = 0;

    struct ps_core_s *ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_GLE));
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


STATIC const struct file_operations g_hw_gle_oam_fops = {
    .owner = THIS_MODULE,
    .open = hw_gle_oam_open,
    .read = hw_gle_oam_read,
    .release = hw_gle_oam_release,
};

STATIC const struct file_operations g_hw_gle_excp_fops = {
    .owner = THIS_MODULE,
    .read = hw_gle_excp_read,
    .unlocked_ioctl = hw_gle_excp_ioctl,
};

STATIC struct miscdevice g_hw_gle_oam_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwmeoam",
    .fops = &g_hw_gle_oam_fops,
};

STATIC struct miscdevice g_hw_gle_excp_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwmeexcp",
    .fops = &g_hw_gle_excp_fops,
};

STATIC struct hw_ps_plat_data g_hisi_gle_platform_data = {
    .dev_name = "/dev/ttyAMA5",
    .flow_cntrl = FLOW_CTRL_ENABLE,
    .baud_rate = DEFAULT_BAUD_RATE,
    .ldisc_num = N_HW_GNSS,
    .suspend = NULL,
    .resume = NULL,
};

STATIC const struct file_operations g_hw_gle_fops = {
    .owner = THIS_MODULE,
    .open = hw_gle_open,
    .write = hw_gle_write,
    .read = hw_gle_read,
    .poll = hw_gle_poll,
    .unlocked_ioctl = NULL,
    .release = hw_gle_release,
};

STATIC struct miscdevice g_hw_gle_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwgle",
    .fops = &g_hw_gle_fops,
};


#ifdef _PRE_CONFIG_USE_DTS
STATIC int32_t ps_gle_misc_dev_register(void)
{
    int32_t err;

    if (is_bfgx_support() != OAL_TRUE) {
        /* don't support bfgx */
        ps_print_info("bfgx disabled, misc dev register bypass\n ");
        return 0;
    }

    mutex_init(&(g_gle_data_combination.comb_mutex));
    err = misc_register(&g_hw_gle_device);
    if (err != 0) {
        ps_print_err("Failed to register gle inode\n ");
        return -EFAULT;
    }

    return 0;
}

STATIC void ps_gle_misc_dev_unregister(void)
{
    if (is_bfgx_support() != OAL_TRUE) {
        ps_print_info("bfgx disabled, misc dev unregister bypass\n ");
        return;
    }

#ifdef HAVE_HISI_GNSS
    misc_deregister(&g_hw_gle_device);
    ps_print_info("misc gnss me device have removed\n");
#endif
}

STATIC void ps_gle_baudrate_init(void)
{
    g_hisi_gle_platform_data.baud_rate = UART_BAUD_RATE_2M;
    ps_print_info("ps gle init baudrate=%ld\n", g_hisi_gle_platform_data.baud_rate);
    return;
}

STATIC int32_t gle_misc_dev_register(void)
{
    int32_t ret;

    ret = ps_gle_misc_dev_register();
    if (ret != 0) {
        ps_print_err("Failed to register misc dev, ret = %d\n", ret);
        return ret;
    }

    ret = misc_register(&g_hw_gle_oam_device);
    if (ret != 0) {
        ps_print_err("Failed to register hwmeoam inode, ret = %d\n", ret);
        goto err_register_meoam;
    }

    ret = misc_register(&g_hw_gle_excp_device);
    if (ret != 0) {
        ps_print_err("Failed to register hwmeexcp inode, ret = %d\n", ret);
        goto err_register_meexcp;
    }

    return OAL_SUCC;

err_register_meexcp:
    misc_deregister(&g_hw_gle_oam_device);
err_register_meoam:
    ps_gle_misc_dev_unregister();
    return ret;
}

STATIC void gle_misc_dev_unregister(void)
{
    misc_deregister(&g_hw_gle_excp_device);
    misc_deregister(&g_hw_gle_oam_device);
    ps_gle_misc_dev_unregister();
}

STATIC int32_t ps_gle_probe(struct platform_device *pdev)
{
    struct hw_ps_plat_data *pdata = NULL;
    struct ps_plat_s *ps_plat_d = NULL;
    int32_t err;
    hi110x_board_info *bd_info = get_hi110x_board_info();
    uint32_t uart = service_get_bus_id(BFGX_GLE);

    ps_gle_baudrate_init();

    strncpy_s(g_hisi_gle_platform_data.dev_name, sizeof(g_hisi_gle_platform_data.dev_name),
              bd_info->uart_port[uart], sizeof(g_hisi_gle_platform_data.dev_name) - 1);
    g_hisi_gle_platform_data.dev_name[sizeof(g_hisi_gle_platform_data.dev_name) - 1] = '\0';

    pdev->dev.platform_data = &g_hisi_gle_platform_data;
    pdata = &g_hisi_gle_platform_data;

    g_hw_ps_gle_device = pdev;

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

    if (gle_misc_dev_register() != 0) {
        goto err_misc_dev;
    }

    ps_print_suc("%s is success!\n", __func__);

    return 0;

err_misc_dev:
    ps_core_exit(ps_plat_d->core_data, uart);
err_core_init:
    kfree(ps_plat_d);
    ps_plat_d = NULL;
    return -EFAULT;
}

STATIC int32_t ps_gle_remove(struct platform_device *pdev)
{
    struct ps_plat_s *ps_plat_d = NULL;
    uint32_t uart = service_get_bus_id(BFGX_GLE);

    gle_misc_dev_unregister();
    ps_plat_d = dev_get_drvdata(&pdev->dev);
    if (ps_plat_d == NULL) {
        ps_print_err("ps_plat_d is null\n");
        return -EFAULT;
    }

    ps_core_exit(ps_plat_d->core_data, uart);
    kfree(ps_plat_d);
    ps_plat_d = NULL;

    return 0;
}

STATIC int32_t ps_gle_suspend(struct platform_device *pdev, oal_pm_message_t state)
{
    return 0;
}

STATIC int32_t ps_gle_resume(struct platform_device *pdev)
{
    return 0;
}

STATIC struct of_device_id g_hi110x_ps_gle_match_table[] = {
    {
        .compatible = DTS_COMP_HI110X_PS_GLE_NAME,
        .data = NULL,
    },
    {},
};
#endif

STATIC struct platform_driver g_ps_gle_platform_driver = {
#ifdef _PRE_CONFIG_USE_DTS
    .probe = ps_gle_probe,
    .remove = ps_gle_remove,
    .suspend = ps_gle_suspend,
    .resume = ps_gle_resume,
#endif
    .driver = {
        .name = "hisi_gle",
        .owner = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = g_hi110x_ps_gle_match_table,
#endif
    },
};

int32_t hw_ps_gle_init(void)
{
    int32_t ret;

    ret = platform_driver_register(&g_ps_gle_platform_driver);
    if (ret) {
        ps_print_err("Unable to register platform gle driver.\n");
    }

    return ret;
}

void hw_ps_gle_exit(void)
{
    platform_driver_unregister(&g_ps_gle_platform_driver);
}

MODULE_DESCRIPTION("Public serial Driver for huawei gle chips");
MODULE_LICENSE("GPL");
