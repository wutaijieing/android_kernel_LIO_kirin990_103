

/* Include Head file */
#include "gt_dev.h"
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include "plat_debug.h"
#include "bfgx_exception_rst.h"
#include "bfgx_dev.h"

memdump_info_t g_gt_memdump_cfg;

STATIC ssize_t hw_gt_excp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sk_buff *skb = NULL;
    uint16_t count1;
    memdump_info_t *memdump_t = &g_gt_memdump_cfg;

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

STATIC long hw_gt_excp_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
    int32_t ret = 0;

    if (file == NULL) {
        ps_print_err("file is null\n");
        return -EINVAL;
    }
    switch (cmd) {
        case PLAT_GT_DUMP_FILE_READ_CMD:
            ret = plat_gt_dump_rotate_cmd_read(arg);
            break;
        default:
            ps_print_warning("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }
    return ret;
}

STATIC const struct file_operations g_hw_gt_excp_fops = {
    .owner = THIS_MODULE,
    .read = hw_gt_excp_read,
    .unlocked_ioctl = hw_gt_excp_ioctl,
};

STATIC struct miscdevice g_hw_gt_excp_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwgtexcp",
    .fops = &g_hw_gt_excp_fops,
};

#ifdef _PRE_CONFIG_USE_DTS
STATIC int32_t ps_gt_probe(struct platform_device *pdev)
{
    int32_t err = misc_register(&g_hw_gt_excp_device);
    if (err != 0) {
        ps_print_err("Failed to register gt inode\n ");
        return -EFAULT;
    }
    ps_print_suc("%s is success!\n", __func__);

    return 0;
}

STATIC int32_t ps_gt_remove(struct platform_device *pdev)
{
    misc_deregister(&g_hw_gt_excp_device);
    return 0;
}

STATIC int32_t ps_gt_suspend(struct platform_device *pdev, oal_pm_message_t state)
{
    return 0;
}

STATIC int32_t ps_gt_resume(struct platform_device *pdev)
{
    return 0;
}

STATIC struct of_device_id g_hi110x_ps_gt_match_table[] = {
    {
        .compatible = DTS_COMP_HI110X_PS_GT_NAME,
        .data = NULL,
    },
    {},
};
#endif

STATIC struct platform_driver g_ps_gt_platform_driver = {
#ifdef _PRE_CONFIG_USE_DTS
    .probe = ps_gt_probe,
    .remove = ps_gt_remove,
    .suspend = ps_gt_suspend,
    .resume = ps_gt_resume,
#endif
    .driver = {
        .name = "hisi_gt",
        .owner = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = g_hi110x_ps_gt_match_table,
#endif
    },
};

int32_t hw_ps_gt_init(void)
{
    int32_t ret;
    ret = platform_driver_register(&g_ps_gt_platform_driver);
    if (ret) {
        ps_print_err("Unable to register platform gt driver.\n");
    }
    return ret;
}

void hw_ps_gt_exit(void)
{
    platform_driver_unregister(&g_ps_gt_platform_driver);
}
MODULE_DESCRIPTION("Public serial Driver for huawei gt chips");
MODULE_LICENSE("GPL");