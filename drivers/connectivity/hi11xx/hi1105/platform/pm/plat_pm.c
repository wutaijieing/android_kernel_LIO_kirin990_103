

/* ͷ�ļ����� */
#include <linux/module.h> /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/kernel.h>

#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/tty.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>

#include "board.h"
#include "ssi_common.h"
#include "bfgx_dev.h"
#include "plat_debug.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "factory.h"
#include "plat_pm.h"
#include "plat_pm_wlan.h"
#include "plat_pm_gt.h"
#include "bfgx_data_parse.h"
#include "lpcpu_feature.h"
#include "securec.h"
#include "oam_ext_if.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "wireless_patch.h"
#endif

#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "oal_schedule.h"
#include "plat_firmware.h"
#include "bfgx_exception_rst.h"

#ifdef _PRE_SHARE_BUCK_SURPORT
#ifndef CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
#include <huawei_platform/sensor/hw_comm_pmic.h>
#endif
#endif
#include "hci_interface.h"

#define BFGX_SUBSYS_STATE_FROMAT_LEN 200

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_PM_C

#ifdef _PRE_HI_DRV_GPIO
gpio_ext_func *g_hisi_gpio_func = NULL;
#endif

#ifdef _PRE_H2D_GPIO_WKUP
#define WAIT_CFG_GPIO_WKUP_MODE_TIME 50
#endif

#ifdef _PRE_CONFIG_HISI_S3S4_POWER_STATE
struct s_pm_wal_host_handler g_pm_wal_host_handler = {NULL, NULL};
int g_wifi_is_enable = 0;
int g_bfgx_is_enable = 0;
#endif

#if defined(_PRE_S4_FEATURE)
void oal_free_irq_in_s4(void)
{
    uint32_t dev_id = HCC_EP_WIFI_DEV;
    hcc_bus *pst_bus = NULL;
    hcc_bus_dev *pst_bus_dev = NULL;
    uint32_t bfg_irq;
    hi110x_board_info *board = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);

    ps_print_info("free_irq_in_s4..\n");

    pst_bus_dev = hcc_get_bus_dev(dev_id);
    if (pst_bus_dev != NULL) {
        pst_bus = pst_bus_dev->cur_bus;
        ps_print_info("bus type: %s\n", pst_bus->name);
    } else {
        ps_print_err("pst_bus_dev is null\n");
        return;
    }

    // free wlan irq
    free_irq(pst_bus_dev->wlan_irq, pst_bus_dev);
    // free bfgx irq
    board = get_hi110x_board_info();
    if (board == NULL) {
        ps_print_err("board is null.\n");
        return;
    }

    bfg_irq = (uint32_t)board->wakeup_irq[B_SYS];
    ps_print_info("bfg_irq is %u.\n", bfg_irq);
    free_irq(bfg_irq, (void*)pm_data);
    return;
}
void request_wlan_gpio_irq_in_s4(hcc_bus_dev *pst_bus_dev)
{
    int32_t ret;
    hcc_bus *pst_bus = NULL;

    if (pst_bus_dev == NULL) {
        return;
    }

    pst_bus = pst_bus_dev->cur_bus;
    ps_print_info("request_irq_in_s4 bus type: %s\n", pst_bus->name);

    ret = request_irq(pst_bus_dev->wlan_irq, hcc_bus_wlan_gpio_irq, IRQF_TRIGGER_RISING | IRQF_DISABLED,
        "wifi_gpio_intr", pst_bus_dev);
    if (ret < 0) {
        ps_print_err("wlan_gpio_irq_request_for_s4 request_irq fail.\n");
        return;
    }
    disable_irq_nosync(pst_bus_dev->wlan_irq);
    // irq disabled default
    pst_bus_dev->irq_stat = 1;
    return;
}
void request_bfgx_gpio_irq_in_s4(void)
{
    hi110x_board_info *board = NULL;
    uint32_t bfg_irq;
    int32_t ret;
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);

    board = get_hi110x_board_info();
    if (board == NULL) {
        return;
    }

    bfg_irq = (uint32_t)board->wakeup_irq[B_SYS];
    ps_print_info("request bfgx irq %u in s4\n", bfg_irq);
    ret = request_irq(bfg_irq, bfg_wake_host_isr, IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
        "bfgx_wake_host", (void *)pm_data);
    if (ret < 0) {
        ps_print_err("bfgx_power_on_request_irq_s4 fail.\n");
        return;
    }
    disable_irq_nosync(bfg_irq);
    if (pm_data != NULL) {
        pm_data->ul_irq_stat = 1; /* irq disabled default. */
    }

    return;
}
void oal_request_irq_in_s4(void)
{
    uint32_t dev_id = HCC_EP_WIFI_DEV;
    hcc_bus *pst_bus = NULL;
    hcc_bus_dev *pst_bus_dev = NULL;

    ps_print_info("request_irq_in_s4..\n");

    pst_bus_dev = hcc_get_bus_dev(dev_id);
    if (pst_bus_dev != NULL) {
        pst_bus = pst_bus_dev->cur_bus;
        ps_print_info("request_irq_in_s4 bus type: %s\n", pst_bus->name);
    } else {
        ps_print_err("pst_bus_dev is null\n");
        return;
    }

    // request wlan gpio irq
    request_wlan_gpio_irq_in_s4(pst_bus_dev);

    // request bfgx gpio irq
    request_bfgx_gpio_irq_in_s4();
    return;
}
int set_board_s4(unsigned long mode)
{
    int ret = -1;
    if (mode == PM_HIBERNATION_PREPARE) {
        // free interrupt(GPIO/PCIE)
        oal_free_irq_in_s4();
        // gpio deinit
        suspend_board_gpio_in_s4();
    } else if (mode == PM_POST_HIBERNATION) {
        // gpio init
        resume_board_gpio_in_s4();
        // interrupt request (GPIO/PCIE) and disable request
        oal_request_irq_in_s4();
    } else {
        ps_print_info("not set 32khz clk in other mode\n");
    }
    return ret;
}
#endif

#ifdef _PRE_CONFIG_HISI_S3S4_POWER_STATE
void pm_host_walcb_register(work_cb suspend_cb, work_cb resume_cb)
{
    g_pm_wal_host_handler.pf_wal_host_suspend_work_func = suspend_cb;
    g_pm_wal_host_handler.pf_wal_host_resume_work_func = resume_cb;
}

EXPORT_SYMBOL(pm_host_walcb_register);
void resume_hi110x_wifi(void)
{
    int ret = FAILURE;
    pm_s3s4_chr_info_stru pm_s3s4_chr_info = { 0 };

    if (oal_likely(g_pm_wal_host_handler.pf_wal_host_resume_work_func)) {
        ps_print_info("resume_hi110x_wifi::wal_host_resume_work\n");
        ret = g_pm_wal_host_handler.pf_wal_host_resume_work_func();
        if (ret != SUCCESS) {
            pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_WIFI_RESUME_FAIL;
        }
    } else {
        ps_print_err("resume_hi110x_wifi::wal_host_resume_work NULL\n");
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_WIFI_RESUME_HANDLE_NULL;
    }

    if (ret != SUCCESS) {
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID,
                        (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
    }
}

int resume_hi110x_bfgx(void)
{
#ifdef _PRE_CONFIG_S3_HCI_DEV_OPT
    int ret;
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);
    pm_s3s4_chr_info_stru pm_s3s4_chr_info = { 0 };

    if (pm_data == NULL) {
        ps_print_err("resume_hi110x_bfgx::pm_get_drvdata return null\n");
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_BT_RESUME_HANDLE_NULL;
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID,
                        (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
        return -FAILURE;
    }

    ret = external_hci_dev_do_open(pm_data->st_bt_dev.hdev);
    ps_print_info("[%s]hci_dev_do_open in resume_hi110x_bfgx\n", index2name(pm_data->index));
    if (ret != SUCCESS) {
        ps_print_err("[%s]hci_dev_do_open fail in resume_hi110x_bfgx\n", index2name(pm_data->index));
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_BT_RESUME_FAIL;
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID,
                        (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
    }
    return ret;
#else
    ps_print_info("resume_hi110x_bfgx::hw_bt_ioctl register\n");
    hw_bt_ioctl(NULL, BT_IOCTL_HCISETPROTO, 0);
    return SUCCESS;
#endif
}

void resume_hi110x(void)
{
    int ret;
    ps_print_info("resume_hi110x\n");

    if (g_wifi_is_enable) {
        resume_hi110x_wifi();
    } else {
        ps_print_info("wifi is disable,so no need call wal_host_resume_work in resume_hi110x\n");
    }

    if (g_bfgx_is_enable) {
        ret = resume_hi110x_bfgx();
        if (ret != SUCCESS) {
            return;
        }
    } else {
        ps_print_info("bfgx is disable,so no need call hw_bt_ioctl register in resume_hi110x\n");
    }
}

int suspend_hi110x_bfgx(void)
{
#ifdef _PRE_CONFIG_S3_HCI_DEV_OPT
    int ret;
    pm_s3s4_chr_info_stru pm_s3s4_chr_info = { 0 };
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);

    if (pm_data == NULL) {
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_BT_SUSPEND_HANDLE_NULL;
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID, (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
        return -FAILURE;
    }

    ret = hci_dev_do_close(pm_data->st_bt_dev.hdev);
    ps_print_info("[%s]hci_dev_do_close in suspend_hi110x_bfgx\n", index2name(pm_data->index));
    if (ret != SUCCESS) {
        ps_print_err("[%s]hci_dev_do_close fail in suspend_hi110x_bfgx\n", index2name(pm_data->index));
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_BT_SUSPEND_FAIL;
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID, (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
    } else {
        g_bfgx_is_enable = POWER_ON;
    }
    return ret;
#else
    ps_print_info("suspend_hi110x_bfgx::hw_bt_ioctl unregister\n");
    hw_bt_ioctl(NULL, BT_IOCTL_HCIUNSETPROTO, 0);
    g_bfgx_is_enable = POWER_ON;
    return SUCCESS;
#endif
}

void suspend_hi110x_wifi(void)
{
    int ret;
    pm_s3s4_chr_info_stru pm_s3s4_chr_info = { 0 };

    if (oal_likely(g_pm_wal_host_handler.pf_wal_host_suspend_work_func)) {
        ps_print_info("suspend_hi110x_wifi::wal_host_suspend_work\n");
        ret = g_pm_wal_host_handler.pf_wal_host_suspend_work_func();
        if (ret != SUCCESS) {
            pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_WIFI_SUSPEND_FAIL;
        }
    } else {
        ps_print_err("suspend_hi110x_wifi::wal_host_suspend_work NULL\n");
        pm_s3s4_chr_info.us_s3s4_status = PM_S3S4_CHR_WIFI_SUSPEND_HANDLE_NULL;
        ret = -FAILURE;
    }

    if (ret != SUCCESS) {
        chr_exception_p(CHR_PLATFORM_S3S4_EVENTID, (uint8_t *)(&pm_s3s4_chr_info), sizeof(pm_s3s4_chr_info_stru));
    } else {
        g_wifi_is_enable = POWER_ON;
    }
}

void suspend_hi110x(void)
{
    int ret;
    ps_print_info("suspend_hi110x\n");

    if (board_get_sys_enable_gpio_val(B_SYS)) {
        ret = suspend_hi110x_bfgx();
        if (ret != SUCCESS) {
            return;
        }
    } else {
        ps_print_info("bfgx is disable,so no need call hw_bt_ioctl unregister in suspend_hi110x\n");
        g_bfgx_is_enable = POWER_OFF;
    }

    if (board_get_sys_enable_gpio_val(W_SYS)) {
        suspend_hi110x_wifi();
    } else {
        ps_print_info("wifi is disable,so no need call wal_host_suspend_work in suspend_hi110x\n");
        g_wifi_is_enable = POWER_OFF;
    }
}
#endif

#ifdef _PRE_HOST_SUSPEND_UART_POWERDOWN
#define WAIT_TTY_CLOSE_TIME 200
#define WAIT_POST_SUSPEND_SLEEP_TIME 100

static int send_disallow_msg_via_tty(struct pm_drv_data *pm_data, struct ps_core_s *ps_core_d)
{
    int ret;
    ps_print_info("[%s]host resume start!\n", index2name(pm_data->index));

#ifdef _PRE_SUSPEND_AP
    if (!os_str_cmp(g_st_board_info.uart_port[BUART], "/dev/ttyXRUSB0")) {
        ps_print_info("uart port is /dev/ttyXRUSB0, wait 100ms\n");
        msleep(WAIT_POST_SUSPEND_SLEEP_TIME);
    }
#endif

    if (bfgx_is_shutdown() == false) {
        open_tty_drv(ps_core_d);

        ret = prepare_to_visit_node(ps_core_d);
        if (ret < 0) {
            ps_print_err("[%s]wake up device fail, bring to reset work\n", index2name(pm_data->index));
            plat_exception_handler(SUBSYS_BFGX, THREAD_BT, BFGX_WAKEUP_FAIL);
            return ret;
        }

        queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);

        post_to_visit_node(ps_core_d);
    }
    return 0;
}

static void release_tty_after_send_msg(struct ps_core_s *ps_core_d)
{
    release_tty_drv(ps_core_d);
    msleep(WAIT_TTY_CLOSE_TIME);
}
#endif

#ifdef _PRE_CONFIG_HISI_S3S4_POWER_STATE
static int pf_suspend_notify(struct notifier_block *notify_block, unsigned long mode, void *unused)
{
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);
    if (pm_data == NULL) {
        ps_print_err("[BUART] pm_data is NULL!\n");
        return IRQ_NONE;
    }

    switch (mode) {
        case PM_POST_SUSPEND:
        case PM_POST_HIBERNATION:
            ps_print_info("[BUART] S3S4 resume now!\n");
#ifdef _PRE_S4_FEATURE
            set_board_s4(mode);
#endif
            resume_hi110x();
            break;

        case PM_SUSPEND_PREPARE:
        case PM_HIBERNATION_PREPARE:
            ps_print_info("[BUART] S3S4 suspend now!\n");
            suspend_hi110x();
#ifdef _PRE_S4_FEATURE
            set_board_s4(mode);
#endif
            break;
        default:
            break;
    }
    return 0;
}

static int pf_gnss_sr_notify(struct notifier_block *notify_block, unsigned long mode, void *unused)
{
    struct pm_drv_data *pm_data = pm_get_drvdata(GUART);
    if (pm_data == NULL) {
        ps_print_err("[GUART] pm_data is NULL!\n");
        return IRQ_NONE;
    }

    switch (mode) {
        case PM_POST_SUSPEND:
        case PM_POST_HIBERNATION:
            ps_print_info("[GUART] S3S4 resume now!\n");
#ifdef _PRE_S4_FEATURE
            set_board_s4(mode);
#endif
            resume_hi110x();
            break;

        case PM_SUSPEND_PREPARE:
        case PM_HIBERNATION_PREPARE:
            ps_print_info("[GUART] S3S4 suspend now!\n");
            suspend_hi110x();
#ifdef _PRE_S4_FEATURE
            set_board_s4(mode);
#endif
            break;
        default:
            break;
    }
    return 0;
}

#else
/*
 * Function: suspend_notify
 * Description: suspend notify call back
 * Ruturn: 0 -- success
 */
static int pf_suspend_notify(struct notifier_block *notify_block, unsigned long mode, void *unused)
{
    int32_t ret;
    struct pm_drv_data *pm_data = NULL;

    struct ps_core_s *ps_core_d = ps_get_core_reference(BUART);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("[BUART] ps_core_d is NULL\n");
        return -EINVAL;
    }
    pm_data = ps_core_d->pm_data;

    switch (mode) {
        case PM_POST_SUSPEND:
#ifdef _PRE_SUSPORT_OEMINFO
            ret = is_hitv_miniproduct();
            if (ret == 0) {
                resume_wlan_wakeup_host_gpio();
            }
#endif
#ifdef _PRE_SUSPEND_AP
            ps_print_info("[BUART] host resume , gpio level %d!\n", board_get_dev_wkup_host_state(B_SYS));
#else
            ps_print_info("[BUART] host resume , cnt %d!\n", atomic_read(&ps_core_d->sr_cnt));
#endif
            if (atomic_read(&ps_core_d->sr_cnt) > 0) {
                up(&ps_core_d->sr_wake_sema);
                atomic_set(&ps_core_d->sr_cnt, 0);
            }

#ifdef _PRE_HOST_SUSPEND_UART_POWERDOWN
            ret = send_disallow_msg_via_tty(pm_data, ps_core_d);
            if (ret != 0) {
                return ret;
            }
#endif
            break;
        case PM_SUSPEND_PREPARE:
#ifdef _PRE_HOST_SUSPEND_UART_POWERDOWN
            release_tty_after_send_msg(ps_core_d);
#endif
            atomic_set(&ps_core_d->sr_cnt, 1);
            ret = down_trylock(&ps_core_d->sr_wake_sema);
            if (ret) {
                atomic_set(&ps_core_d->sr_cnt, 0);
                ps_print_err("[BUART], get wakeup sem error %d\n", ret);
            }
            ps_print_info("[BUART] host suspend now \n");
            break;
        case PM_POST_HIBERNATION:
        case PM_HIBERNATION_PREPARE: {
#if defined(_PRE_S4_FEATURE)
            set_board_s4(mode);
#endif
            ps_print_info("[BUART] host std %s now \n",
                          (mode == PM_HIBERNATION_PREPARE) ? "suspend" : "resume");
            break;
        }
        default:
            break;
    }
    return 0;
}

/*
 * Function: suspend_notify
 * Description: suspend notify call back
 * Ruturn: 0 -- success
 */
static int pf_gnss_sr_notify(struct notifier_block *notify_block, unsigned long mode, void *unused)
{
    int32_t ret;
    struct pm_drv_data *pm_data = NULL;

    struct ps_core_s *ps_core_d = ps_get_core_reference(GUART);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("[GUART] ps_core_d is NULL\n");
        return -EINVAL;
    }
    pm_data = ps_core_d->pm_data;

    switch (mode) {
        case PM_POST_SUSPEND:
            ps_print_info("[GUART] host resume , cnt %d!\n", atomic_read(&ps_core_d->sr_cnt));
            if (atomic_read(&ps_core_d->sr_cnt) > 0) {
                up(&ps_core_d->sr_wake_sema);
                atomic_set(&ps_core_d->sr_cnt, 0);
            }

#ifdef _PRE_HOST_SUSPEND_UART_POWERDOWN
            ret = send_disallow_msg_via_tty(pm_data, ps_core_d);
            if (ret != 0) {
                return ret;
            }
#endif
            break;
        case PM_SUSPEND_PREPARE:
#ifdef _PRE_HOST_SUSPEND_UART_POWERDOWN
            release_tty_after_send_msg(ps_core_d);
#endif
            atomic_set(&ps_core_d->sr_cnt, 1);
            ret = down_trylock(&ps_core_d->sr_wake_sema);
            if (ret) {
                atomic_set(&ps_core_d->sr_cnt, 0);
                ps_print_err("[GUART], get wakeup sem error %d\n", ret);
            }
            ps_print_info("[GUART] host suspend now \n");
            break;
        case PM_POST_HIBERNATION:
        case PM_HIBERNATION_PREPARE: {
#if defined(_PRE_S4_FEATURE)
            set_board_s4(mode);
#endif
            ps_print_info("[BUART] host std %s now \n",
                          (mode == PM_HIBERNATION_PREPARE) ? "suspend" : "resume");
            break;
        }
        default:
            break;
    }
    return 0;
}
#endif

static struct notifier_block g_pf_bt_sr_notifier = {
    .notifier_call = pf_suspend_notify,
    .priority = INT_MIN,
};

static struct notifier_block g_pf_gnss_sr_notifier = {
    .notifier_call = pf_gnss_sr_notify,
    .priority = INT_MIN,
};

static void ps_pm_notifier_register(int index)
{
    int ret;

    if (index == BUART) {
        ret = register_pm_notifier(&g_pf_bt_sr_notifier);
        if (ret < 0) {
            ps_print_warning("%s : register bt pm_notifier failed, ret = %d!\n", __func__, ret);
        }
    } else if (index == GUART) {
        ret = register_pm_notifier(&g_pf_gnss_sr_notifier);
        if (ret < 0) {
            ps_print_warning("%s : register gnss pm_notifier failed, ret = %d!\n", __func__, ret);
        }
    }
}

static struct pm_drv_data *g_pm_drv_data[UART_BUTT] = {NULL, NULL};

struct pm_drv_data *pm_get_drvdata(uint32_t index)
{
    return (index < UART_BUTT) ? g_pm_drv_data[index] : NULL;
}

static void pm_set_drvdata(struct pm_drv_data *data, int32_t index)
{
    if (index < UART_BUTT) {
        g_pm_drv_data[index] = data;
        data->index =  index;
    }
}

static void pm_clr_drvdata(uint32_t index)
{
    if (index < UART_BUTT) {
        g_pm_drv_data[index] = NULL;
    }
}


static struct ps_core_s *pm_get_core(struct pm_drv_data *pm_data)
{
    return pm_data->ps_core_data;
}


#ifdef CONFIG_HUAWEI_DSM
/*
 * �� �� ��  : hw_110x_dsm_client_notify
 * ��������  : DMD�¼��ϱ�
 * �� �� ֵ  : ��ʼ������ֵ���ɹ���ʧ��ԭ��
 */
static struct dsm_dev g_dsm_wifi = {
    .name = "dsm_wifi",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = DMD_EVENT_BUFF_SIZE,
};

static struct dsm_dev g_dsm_bt = {
    .name = "dsm_bt",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = DMD_EVENT_BUFF_SIZE,
};

static struct dsm_client *g_dsm_wifi_client = NULL;
static struct dsm_client *g_dsm_bt_client = NULL;

static inline struct dsm_client* get_dsm_client(int sub_sys)
{
    struct dsm_client *client_buf = NULL;

    if ((sub_sys == SYSTEM_TYPE_WIFI) || (sub_sys == SYSTEM_TYPE_PLATFORM)) {
        client_buf = g_dsm_wifi_client;
    } else if (sub_sys == SYSTEM_TYPE_BT) {
        client_buf = g_dsm_bt_client;
    }

    return client_buf;
}

void hw_110x_register_dsm_client(void)
{
    if (g_dsm_wifi_client == NULL) {
        g_dsm_wifi_client = dsm_register_client(&g_dsm_wifi);
    }

    if (g_dsm_bt_client == NULL) {
        g_dsm_bt_client = dsm_register_client(&g_dsm_bt);
    }
}

void hw_110x_unregister_dsm_client(void)
{
    if (g_dsm_wifi_client != NULL) {
        dsm_unregister_client(g_dsm_wifi_client, &g_dsm_wifi);
        g_dsm_wifi_client = NULL;
    }
    if (g_dsm_bt_client != NULL) {
        dsm_unregister_client(g_dsm_bt_client, &g_dsm_bt);
        g_dsm_bt_client = NULL;
    }
}
#define LOG_BUF_SIZE 512
static int g_last_dsm_id = 0;

void hw_110x_dsm_client_notify(int sub_sys, int dsm_id, const char *fmt, ...)
{
    char buf[LOG_BUF_SIZE] = {0};
    struct dsm_client *dsm_client_buf = NULL;
    va_list ap;
    int32_t ret = 0;

    dsm_client_buf = get_dsm_client(sub_sys);
    if (dsm_client_buf == NULL) {
        oam_error_log1(0, 0, "dsm subsys[%d] didn't suport\n", sub_sys);
        return;
    }

    if (dsm_client_buf != NULL) {
        if (fmt != NULL) {
            va_start(ap, fmt);
            ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, ap);
            va_end(ap);
            if (ret < 0) {
                oam_error_log1(0, 0, "vsnprintf_s fail, line[%d]\n", __LINE__);
                return;
            }
        } else {
            oam_error_log1(0, 0, "dsm_client_buf is null, line[%d]\n", __LINE__);
            return;
        }
    }

    if (!dsm_client_ocuppy(dsm_client_buf)) {
        dsm_client_record(dsm_client_buf, buf);
        dsm_client_notify(dsm_client_buf, dsm_id);
        g_last_dsm_id = dsm_id;
        oam_error_log1(0, OAM_SF_PWR, "wifi dsm_client_notify success,dsm_id=%d", dsm_id);
        oal_io_print("[I]wifi dsm_client_notify success,dsm_id=%d[%s]\n", dsm_id, buf);
    } else {
        oam_error_log2(0, OAM_SF_PWR, "wifi dsm_client_notify failed,last_dsm_id=%d dsm_id=%d", g_last_dsm_id, dsm_id);
        oal_io_print("[E]wifi dsm_client_notify failed,last_dsm_id=%d dsm_id=%d\n", g_last_dsm_id, dsm_id);

        // retry dmd record
        dsm_client_unocuppy(dsm_client_buf);
        if (!dsm_client_ocuppy(dsm_client_buf)) {
            dsm_client_record(dsm_client_buf, buf);
            dsm_client_notify(dsm_client_buf, dsm_id);
            oam_error_log1(0, OAM_SF_PWR, "wifi dsm_client_notify success,dsm_id=%d", dsm_id);
            oal_io_print("[I]wifi dsm notify success, dsm_id=%d[%s]\n", dsm_id, buf);
        } else {
            oam_error_log1(0, OAM_SF_PWR, "wifi dsm client ocuppy, dsm notify failed, dsm_id=%d", dsm_id);
            oal_io_print("[E]wifi dsm client ocuppy, dsm notify failed, dsm_id=%d\n", dsm_id);
        }
    }
}
EXPORT_SYMBOL(hw_110x_dsm_client_notify);

#endif

STATIC void bfgx_dev_state_set(struct pm_drv_data *pm_data, uint8_t on)
{
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return;
    }
    ps_print_warning("[%s]bfgx_dev_state_set:%d --> %d\n", index2name(pm_data->index), pm_data->bfgx_dev_state, on);
    pm_data->bfgx_dev_state = on;
}

STATIC int32_t bfgx_dev_state_get(struct pm_drv_data *pm_data)
{
    return pm_data->bfgx_dev_state;
}

STATIC void bfgx_uart_state_set(struct pm_drv_data *pm_data, uint8_t uart_state)
{
    ps_print_warning("[%s]bfgx_uart_state_set:%d-->%d", index2name(pm_data->index), pm_data->uart_state, uart_state);
    pm_data->uart_state = uart_state;
}

STATIC char bfgx_uart_state_get(struct pm_drv_data *pm_data)
{
    return pm_data->uart_state;
}

int32_t bfgx_uart_get_baud_rate(struct pm_drv_data *pm_data)
{
    struct ps_plat_s *ps_plat_d = NULL;

    ps_get_plat_reference(&ps_plat_d);
    if (unlikely(ps_plat_d == NULL)) {
        ps_print_err("ps_plat_d is NULL\n");
        return -EINVAL;
    }

    return ps_plat_d->baud_rate;
}

int32_t bfgx_pm_feature_set(struct pm_drv_data *pm_data)
{
#define FEATURE_SET_DELAY  50
    struct ps_core_s *ps_core_d = NULL;
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_core_d = pm_get_core(pm_data);
    if (ps_core_d == NULL) {
        ps_print_err("ps_core_d is NULL\n");
        return -FAILURE;
    }

    if (pm_data->bfgx_pm_ctrl_enable == BFGX_PM_DISABLE) {
        ps_print_info("[%s]bfgx platform pm ctrl disable\n", index2name(pm_data->index));
        oal_msleep(FEATURE_SET_DELAY);
        return SUCCESS;
    }

    if (pm_data->bfgx_lowpower_enable == BFGX_PM_ENABLE) {
        ps_print_info("[%s]bfgx platform pm enable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_PL_ENABLE_PM);
    } else {
        ps_print_info("[%s]bfgx platform pm disable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_PL_DISABLE_PM);
    }

    if (pm_data->bfgx_bt_lowpower_enable == BFGX_PM_ENABLE) {
        ps_print_info("[%s]bfgx bt pm enable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_BT_ENABLE_PM);
    } else {
        ps_print_info("[%s]bfgx bt pm disable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_BT_DISABLE_PM);
    }

    if (pm_data->bfgx_gnss_lowpower_enable == BFGX_PM_ENABLE) {
        ps_print_info("[%s]bfgx gnss pm enable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_GNSS_ENABLE_PM);
    } else {
        ps_print_info("[%s]bfgx gnss pm disable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_GNSS_DISABLE_PM);
    }

    if (pm_data->bfgx_nfc_lowpower_enable == BFGX_PM_ENABLE) {
        ps_print_info("[%s]bfgx nfc pm enable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_NFC_ENABLE_PM);
    } else {
        ps_print_info("[%s]bfgx nfc pm disable\n", index2name(pm_data->index));
        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_NFC_DISABLE_PM);
    }

    oal_msleep(FEATURE_SET_DELAY);

    return SUCCESS;
}

/*
 * Prototype    : bfg_wake_lock
 * Description  : control bfg wake lock
 * Input        : uint8_t on: 0 means wake unlock ,1 means wake lock.
 */
static void bfg_wake_lock(struct pm_drv_data *pm_data)
{
    oal_wakelock_stru *pst_bfg_wake_lock;
    unsigned long flags;

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return;
    }

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
    pst_bfg_wake_lock = &pm_data->bus_wake_lock;

    oal_spin_lock_irq_save(&pst_bfg_wake_lock->lock, &flags);
    if (oal_wakelock_active(pst_bfg_wake_lock) == 0) {
        __pm_stay_awake(&pst_bfg_wake_lock->st_wakelock);
        pst_bfg_wake_lock->locked_addr = (uintptr_t)_RET_IP_;
        pst_bfg_wake_lock->lock_count++;
        if (oal_unlikely(pst_bfg_wake_lock->debug)) {
            printk(KERN_INFO "wakelock[%s] lockcnt:%lu <==%pf\n",
                   pst_bfg_wake_lock->st_wakelock.name, pst_bfg_wake_lock->lock_count, (void *)_RET_IP_);
        }

        gps_ilde_sleep_vote(1);
        ps_print_info("bfg_wakelock active[%d],cnt %lu\n",
                      oal_wakelock_active(pst_bfg_wake_lock), pst_bfg_wake_lock->lock_count);
    }
    oal_spin_unlock_irq_restore(&pst_bfg_wake_lock->lock, &flags);
#endif

    return;
}

static void bfg_wake_unlock(struct pm_drv_data *pm_data)
{
    oal_wakelock_stru *pst_bfg_wake_lock;
    unsigned long flags;

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return;
    }

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
    pst_bfg_wake_lock = &pm_data->bus_wake_lock;

    oal_spin_lock_irq_save(&pst_bfg_wake_lock->lock, &flags);

    if (oal_wakelock_active(pst_bfg_wake_lock)) {
        pst_bfg_wake_lock->lock_count--;
        __pm_relax(&pst_bfg_wake_lock->st_wakelock);
        pst_bfg_wake_lock->locked_addr = 0UL;

        if (oal_unlikely(pst_bfg_wake_lock->debug)) {
            printk(KERN_INFO "wakeunlock[%s] lockcnt:%lu <==%pf\n", pst_bfg_wake_lock->st_wakelock.name,
                   pst_bfg_wake_lock->lock_count, (void *)_RET_IP_);
        }

        gps_ilde_sleep_vote(0);
        ps_print_info("[%s]bfg_wakelock active[%d],cnt %lu\n",
                      index2name(pm_data->index),
                      oal_wakelock_active(pst_bfg_wake_lock),
                      pst_bfg_wake_lock->lock_count);
    } else {
        ps_print_info("[%s]bfg_wakelock not active,cnt %lu\n",
                      index2name(pm_data->index), pst_bfg_wake_lock->lock_count);
    }
    oal_spin_unlock_irq_restore(&pst_bfg_wake_lock->lock, &flags);
#endif

    return;
}

#ifndef _PRE_H2D_GPIO_WKUP
static int32_t ps_change_baud_rate_retry(struct ps_core_s *ps_core_d, long baud_rate, uint8_t flowctl_status)
{
    const uint32_t set_baud_retry = 3;
    int ret = OAL_TRUE;
    uint32_t j = set_baud_retry;
    while (ps_change_uart_baud_rate(ps_core_d, baud_rate, flowctl_status) != 0) {
        ps_print_warning("change uart rate fail,left retry cnt:%d,do retry\n", j);
        declare_dft_trace_key_info("change uart rate fail", OAL_DFT_TRACE_FAIL);
        if (--j) {
            oal_msleep(SLEEP_100_MSEC);
        } else {
            ps_print_err("change uart rate %ld fail,retried %u but not succ\n",
                         baud_rate, set_baud_retry);
            ret = OAL_FALSE;
            break;
        }
    }

    return ret;
}
#endif

static int32_t process_host_wkup_dev_fail(struct ps_core_s *ps_core_d, struct pm_drv_data *pm_data)
{
    unsigned long flags;
    int bwkup_gpio_val;
#ifndef _PRE_H2D_GPIO_WKUP
    unsigned long baud_rate;
#endif
    if (!oal_is_err_or_null(ps_core_d->tty) && tty_chars_in_buffer(ps_core_d->tty)) {
        ps_print_info("tty tx buf is not empty\n");
    }

    bwkup_gpio_val = oal_gpio_get_value(pm_data->wakeup_host_gpio);
    ps_print_info("[%s]bfg still NOT wkup, gpio level:%d\n", index2name(pm_data->index), bwkup_gpio_val);

    if (bwkup_gpio_val == 0) {
        declare_dft_trace_key_info("bfg wakeup fail", OAL_DFT_TRACE_EXCEP);
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
        } else {
            (void)ssi_dump_err_regs(SSI_ERR_BFG_WAKE_UP_FAIL);
        }
        return OAL_FALSE;
    } else {
        ps_print_info("[%s]bfg wakeup ack lost, complete it\n", index2name(pm_data->index));
        oal_spin_lock_irq_save(&pm_data->wakelock_protect_spinlock, &flags);
        bfg_wake_lock(pm_data);
        bfgx_dev_state_set(pm_data, BFGX_ACTIVE);
        complete(&pm_data->dev_ack_comp);
        oal_spin_unlock_irq_restore(&pm_data->wakelock_protect_spinlock, &flags);
#ifndef _PRE_H2D_GPIO_WKUP
        // set default baudrate. should not try again if failed, return succ
        baud_rate = ps_get_default_baud_rate(ps_core_d);
        (void)ps_change_baud_rate_retry(ps_core_d, baud_rate, FLOW_CTRL_ENABLE);
#endif
        queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);
        return OAL_TRUE;
    }

    return OAL_FALSE;
}

#ifdef _PRE_H2D_GPIO_WKUP
static int32_t host_wkup_dev_via_gpio(struct ps_core_s *ps_core_d, struct pm_drv_data *pm_data)
{
    const uint32_t wkup_retry = 3;
    int32_t ret;
    unsigned long timeleft;
    int bwkup_gpio_val = 0;
    uint32_t i;

    for (i = 0; i < wkup_retry; i++) {
        bwkup_gpio_val = oal_gpio_get_value(pm_data->wakeup_host_gpio);
        ps_print_info("bfg wakeup dev,try %u,cur gpio level:%u\n", i + 1, bwkup_gpio_val);

        board_host_wakeup_bfg_set(GPIO_HIGHLEVEL);
        mdelay(1);
        board_host_wakeup_bfg_set(GPIO_LOWLEVEL);

        timeleft = wait_for_completion_timeout(&pm_data->dev_ack_comp,
                                               msecs_to_jiffies(WAIT_WKUP_DEVACK_TIMEOUT_MSEC));
        if (timeleft || (bfgx_dev_state_get(pm_data) == BFGX_ACTIVE)) {
            bwkup_gpio_val = oal_gpio_get_value(pm_data->wakeup_host_gpio);
            ps_print_info("bfg wkup OK, gpio level:%d\n", bwkup_gpio_val);

            return OAL_TRUE;
        } else {
            ret = process_host_wkup_dev_fail(ps_core_d, pm_data);
            if (ret == OAL_TRUE) {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}
#else
static int32_t host_wkup_dev_via_uart(struct ps_core_s *ps_core_d, struct pm_drv_data *pm_data)
{
    const uint32_t wkup_retry = 3;
    int32_t ret;
    unsigned long timeleft;
    uint8_t zero_num = 0;
    int bwkup_gpio_val = 0;
    uint32_t i;
    unsigned long baud_rate;

    // prepare baudrate
    ret = ps_change_baud_rate_retry(ps_core_d, WKUP_DEV_BAUD_RATE, FLOW_CTRL_DISABLE);
    if (ret != OAL_TRUE) {
        return OAL_FALSE;
    }

    for (i = 0; i < wkup_retry; i++) {
        bwkup_gpio_val = oal_gpio_get_value(pm_data->wakeup_host_gpio);
        ps_print_info("[%s]bfg wakeup dev,try %u,gpio level:%u\n", index2name(pm_data->index), i + 1, bwkup_gpio_val);
        /* uart write long zero to wake up device */
        ps_write_tty(ps_core_d, &zero_num, sizeof(uint8_t));

        timeleft = wait_for_completion_timeout(&pm_data->dev_ack_comp,
                                               msecs_to_jiffies(WAIT_WKUP_DEVACK_TIMEOUT_MSEC));
        if (timeleft || (bfgx_dev_state_get(pm_data) == BFGX_ACTIVE)) {
            bwkup_gpio_val = oal_gpio_get_value(pm_data->wakeup_host_gpio);
            ps_print_info("[%s]bfg wkup OK, gpio level:%d\n", index2name(pm_data->index), bwkup_gpio_val);
            // set default baudrate. should not try again if failed, return succ
            baud_rate = ps_get_default_baud_rate(ps_core_d);
            (void)ps_change_baud_rate_retry(ps_core_d, baud_rate, FLOW_CTRL_ENABLE);
            return OAL_TRUE;
        } else {
            ret = process_host_wkup_dev_fail(ps_core_d, pm_data);
            if (ret == OAL_TRUE) {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}
#endif

STATIC void host_do_wkup(struct ps_core_s *ps_core_d, struct pm_drv_data *pm_data)
{
    int ret;

    oal_reinit_completion(pm_data->dev_ack_comp);
#ifdef _PRE_H2D_GPIO_WKUP
    ret = host_wkup_dev_via_gpio(ps_core_d, pm_data);
    if (ret != OAL_TRUE) {
        ps_print_err("[%s]host gpio wkup bfg fail\n", index2name(pm_data->index));
    }
#else
    /* begin to wake up device via uart rxd */
    ret = host_wkup_dev_via_uart(ps_core_d, pm_data);
    if (ret != OAL_TRUE) {
        ps_print_info("[%s]host wkup bfg fail\n", index2name(pm_data->index));
    }
#endif
}

/*
 * Prototype    : bfg_sleep_wakeup
 * Description  : function for bfg device wake up
 */
STATIC void host_wkup_dev_work(oal_work_stru *work)
{
#define WAIT_RESUME_TIMEOUT (5 * HZ)
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = container_of(work, struct pm_drv_data, wkup_dev_work);
    if (pm_data == NULL) {
        return;
    }

    ps_print_info("[%s]dev:%d,uart:%d\n", index2name(pm_data->index),
                  bfgx_dev_state_get(pm_data), bfgx_uart_get_baud_rate(pm_data));

    ps_core_d = pm_data->ps_core_data;
    if (ps_core_d == NULL) {
        return;
    }

    if (ps_core_d->tty_have_open == false) {
#ifdef _PRE_PM_TTY_OFF
        if (open_tty_drv(ps_core_d) < 0) {
            ps_print_err("open tty failed\n");
            return;
        }
#else
        ps_print_err("%s,tty is closed skip!\n", __func__);
        return;
#endif
    }
    /* if B send work item of wkup_dev before A's work item finished, then
     * B should not do actual wkup operation.
     */
    if (bfgx_dev_state_get(pm_data) == BFGX_ACTIVE) {
        if (oal_waitqueue_active(&pm_data->host_wkup_dev_comp.wait)) {
            ps_print_info("it seems like dev ack with NoSleep\n");
        } else { /* Ŀǰ����һ��host_mutex����������case��Ӧ���ڣ����͹���ģ�鲻Ӧ�����ⲿ */
            ps_print_info("B do wkup_dev work item after A do it but not finished\n");
        }
        complete_all(&pm_data->host_wkup_dev_comp);
        return;
    }

    host_do_wkup(ps_core_d, pm_data);
}

static int32_t bfg_wait_tty_resume(struct ps_core_s *ps_core_d)
{
#define MAX_TTYRESUME_LOOPCNT 300

#ifdef ASYNCB_SUSPENDED
    uint32_t loop_tty_resume_cnt = 0;
#endif

    if ((ps_core_d->tty) && (ps_core_d->tty->port)) {
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
        while (tty_port_suspended(ps_core_d->tty->port)) {
#ifdef ASYNCB_SUSPENDED
            if (loop_tty_resume_cnt++ >= MAX_TTYRESUME_LOOPCNT) {
                ps_print_err("tty is not ready, state:%d!\n", tty_port_suspended(ps_core_d->tty->port));
                return OAL_FALSE;
            }
#endif
            oal_msleep(SLEEP_10_MSEC);
        }
#ifdef ASYNCB_SUSPENDED
        ps_print_info("tty state:0x%x,loop_cnt:%d\n", tty_port_suspended(ps_core_d->tty->port), loop_tty_resume_cnt);
#endif
#else
        ps_print_info("tty port flag 0x%x\n", (unsigned int)ps_core_d->tty->port->flags);
#ifdef ASYNCB_SUSPENDED
        while (test_bit(ASYNCB_SUSPENDED, (volatile unsigned long *)&(ps_core_d->tty->port->flags))) {
            if (loop_tty_resume_cnt++ >= MAX_TTYRESUME_LOOPCNT) {
                ps_print_err("tty is not ready, flag is 0x%x!\n", (unsigned int)ps_core_d->tty->port->flags);
                return OAL_FALSE;
            }
            oal_msleep(SLEEP_10_MSEC);
        }
#endif
#endif
    } else {
        ps_print_err("tty has not inited\n");
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


static int32_t disallow_msg_send_check(struct pm_drv_data *pm_data)
{
    struct ps_core_s *ps_core_d = NULL;
#ifdef PLATFORM_DEBUG_ENABLE
    struct st_exception_info *pst_exception_data = NULL;
#endif

    ps_core_d = pm_get_core(pm_data);
    if (ps_core_d == NULL) {
        return -FAILURE;
    }

    /* wait for uart resume */
    if (down_timeout(&ps_core_d->sr_wake_sema, WAIT_RESUME_TIMEOUT) == -ETIME) {
        ps_print_err("[%s] uart not resume!sr_cnt %d\n", index2name(pm_data->index), atomic_read(&ps_core_d->sr_cnt));
        return -FAILURE;;
    }

    up(&ps_core_d->sr_wake_sema);

#ifdef _PRE_PM_TTY_OFF
    if (ps_core_d->tty_have_open == false) {
        if (open_tty_drv(ps_core_d) < 0) {
            ps_print_err("open tty failed!\n");
            return -FAILURE;
        }
    }
#endif

    /*
     * ��ֹhost˯������±�dev���ѽ���gpio�жϺ�ֱ���������·���Ϣ��
     * ��ʱuart���ܻ�û��ready,��������ȴ�tty resume֮����·���Ϣ
     */
    if (bfg_wait_tty_resume(ps_core_d) == OAL_FALSE) {
        return -FAILURE;
    }

    /* ����������case: ���ύdisallow sleep workǰwork queque�в�����һ��allow sleep workδ����
    ����disallow��Ϣ���͵�ʱ������allow sleep��Ϣ�ȷ���ȥ�ˣ�оƬ�Ѿ�˯�ߣ�disallow��Ϣδ���ͳɹ� */
    if (bfgx_dev_state_get(pm_data) != BFGX_ACTIVE) {
        ps_print_err("[%s]device not in active, no msg could send,commit a wkup work.\n", index2name(pm_data->index));
        queue_work(pm_data->wkup_dev_workqueue, &pm_data->wkup_dev_work);
        return -FAILURE;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    // ��λ������ʱDFR��־λ����������������uart����
    pst_exception_data = get_exception_info_reference();
    if (pst_exception_data != NULL) {
        pst_exception_data->debug_beat_flag  = 1;
    }
#endif

    return SUCCESS;
}

#ifdef _PRE_H2D_GPIO_WKUP
int32_t host_send_cfg_gpio_wkup_dev_msg(struct pm_drv_data *pm_data)
{
    struct ps_core_s *ps_core_d = NULL;
    unsigned long timeleft;
    int32_t subchip_type = get_hi110x_subchip_type();

    ps_core_d = pm_get_core(pm_data);
    if (ps_core_d == NULL) {
        return -FAILURE;;
    }

    if (subchip_type == BOARD_VERSION_HI1103) {
        oal_reinit_completion(pm_data->gpio_wkup_dev_ack_comp);

        ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_ENABLE_GPIO_WKUP_BFG);

        timeleft = wait_for_completion_timeout(
            &pm_data->gpio_wkup_dev_ack_comp,
            msecs_to_jiffies(WAIT_CFG_GPIO_WKUP_MODE_TIME));
        if (!timeleft) {
            ps_uart_state_dump(ps_core_d);
            ps_print_err("wait cfg wakup dev mode ack timeout\n");
            return -FAILURE;
        }
        // ��hostͶƱ֮ǰ���ñ��
        atomic_set(&pm_data->cfg_wkup_dev_flag, 1);
    }
    return SUCCESS;
}
#endif

void host_send_disallow_msg(oal_work_stru *work)
{
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = container_of(work, struct pm_drv_data, send_disallow_msg_work);
    unsigned long flags;

    if (pm_data == NULL) {
        return;
    }

    ps_print_info("[%s]%s\n", index2name(pm_data->index), __func__);

    ps_core_d = pm_get_core(pm_data);
    if (ps_core_d == NULL) {
        return;
    }

    if (disallow_msg_send_check(pm_data) < 0) {
        ps_print_info("host skip send disallow message\n");
        return;
    }

    /* ����uart����,�·�disallow sleep��Ϣ,������� */
    oal_spin_lock_irq_save(&pm_data->uart_state_spinlock, &flags);
    bfgx_uart_state_set(pm_data, UART_READY);
    oal_spin_unlock_irq_restore(&pm_data->uart_state_spinlock, &flags);

    if (oal_atomic_read(&g_ir_only_mode) == 0) {
        if (ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_DISALLOW_SLP) != 0) {
            ps_print_info("[%s]SYS_CFG_DISALLOW_SLP MSG send fail, retry\n", index2name(pm_data->index));
            oal_msleep(SLEEP_10_MSEC);
            queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);
            return;
        }
    }

    /*
     * �����������������dev wkup hostû������, ֻ�Ǳ�֤��host wkup dev�Ĳ���һ��
     * ע�����Ҫ��host wkup devǰ��ҪINIT���������
     */
    complete_all(&pm_data->host_wkup_dev_comp);

    if (oal_atomic_read(&g_ir_only_mode) == 0) {
        /* if any of BFNI is open, we should mod timer. */
        if ((!bfgx_other_subsys_all_shutdown(pm_data, BFGX_GNSS)) ||
            (gnss_get_lowpower_state(pm_data) == GNSS_AGREE_SLEEP)) {
            mod_timer(&pm_data->bfg_timer, jiffies + msecs_to_jiffies(PLATFORM_SLEEP_TIME));
            oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
            ps_print_info("[%s]mod_timer:host_send_disallow_msg\n", index2name(pm_data->index));
        }
        pm_data->operate_beat_timer(BEAT_TIMER_RESET);
    }

    if (atomic_read(&pm_data->bfg_needwait_devboot_flag) == NEED_SET_FLAG) {
        complete(&pm_data->dev_bootok_ack_comp);
    }
}

STATIC int32_t agree_allow_bfg_sleep(struct ps_core_s *ps_core_d, struct pm_drv_data *pm_data)
{
    /* if someone is visiting the dev_node */
    if (atomic_read(&ps_core_d->node_visit_flag) > 0) {
        ps_print_info("[%s]someone visit node, not send allow sleep msg\n", index2name(pm_data->index));
        return OAL_FALSE;
    }

    if ((gnss_get_lowpower_state(pm_data) == GNSS_NOT_AGREE_SLEEP) || (ps_chk_tx_queue_empty(ps_core_d) == false)) {
        ps_print_info("[%s]tx queue not empty, or sleep vote[%d], frc awake[%d]\n", index2name(pm_data->index),
                      atomic_read(&pm_data->gnss_sleep_flag), pm_data->gnss_frc_awake);
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

STATIC int32_t bfg_enter_sleep_check(struct pm_drv_data *pm_data)
{
    struct ps_core_s *ps_core_d = pm_get_core(pm_data);
    struct st_exception_info *pst_exception_data = get_exception_info_reference();
    if (pst_exception_data == NULL) {
        ps_print_err("get exception info reference is error\n");
        return -FAILURE;
    }
    if (in_plat_exception_reset() == OAL_TRUE) {
        ps_print_err("[%s]plat is doing dfr not allow sleep\n", index2name(pm_data->index));
        mod_timer(&pm_data->bfg_timer, jiffies + msecs_to_jiffies(PLATFORM_SLEEP_TIME));
        oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
        return -FAILURE;
    }

    if (ps_core_d->tty_have_open == false) {
        ps_print_info("[%s]tty has closed, not send msg to dev\n",
                      index2name(pm_data->index));
#ifdef _PRE_PM_TTY_OFF
        if (open_tty_drv(ps_core_d) < 0) {
            ps_print_err("open tty failed!\n");
            return -FAILURE;
        }
#else
        return -FAILURE;
#endif
    }

#ifdef _PRE_H2D_GPIO_WKUP
    if (atomic_read(&pm_data->cfg_wkup_dev_flag) != 1) {
        ps_print_err("not cfg gpio wkup dev mode!\n");
        return -FAILURE;
    }
#endif
    return SUCCESS;
}

STATIC void host_allow_bfg_sleep_cmd(struct pm_drv_data *pm_data)
{
    unsigned long flags;
    unsigned long timeleft;
    int allow_sleep =  OAL_FALSE;
    struct ps_core_s *ps_core_d = pm_get_core(pm_data);
    /*
     * we need reinit completion cnt as 0, to prevent such case:
     * 1)host allow dev sleep, dev ack with OK, cnt=1,
     * 2)device wkup host,
     * 3)host allow dev sleep,
     * 4)host wkup dev, it will wait dev_ack succ immediately since cnt==1,
     * 5)dev ack with ok, cnt=2,
     * this case will cause host wait dev_ack invalid.
     */
    oal_reinit_completion(pm_data->dev_slp_comp);

    if (ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_ALLOWDEV_SLP) != 0) {
        ps_print_info("[%s]SYS_CFG_ALLOWDEV_SLP MSG send fail\n", index2name(pm_data->index));
    } else {
#ifdef _PRE_HI_DRV_GPIO
        hitv_gpio_direction_output(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_LOWLEVEL);
#endif
    }

    timeleft = wait_for_completion_timeout(&pm_data->dev_slp_comp, msecs_to_jiffies(WAIT_DEVACK_TIMEOUT_MSEC));

    oal_spin_lock_irq_save(&pm_data->wakelock_protect_spinlock, &flags);
    if (timeleft && (pm_data->bfgx_dev_state == BFGX_SLEEP)) {
        allow_sleep = OAL_TRUE;
        pm_data->operate_beat_timer(BEAT_TIMER_DELETE);
        bfg_wake_unlock(pm_data);
    }
    oal_spin_unlock_irq_restore(&pm_data->wakelock_protect_spinlock, &flags);

    if (allow_sleep) {
        ps_print_info("wait device sleep ack complete, timeleft = %ld\n", timeleft);
#ifdef _PRE_PM_TTY_OFF
        release_tty_drv(ps_core_d);
#endif
    } else {
        ps_print_info("[%s]wait device sleep ack timeout[%d], timeleft[%ld], dev state[%d]\n",
                      index2name(pm_data->index), WAIT_DEVACK_TIMEOUT_MSEC, timeleft, pm_data->bfgx_dev_state);
    }
}

STATIC void host_allow_bfg_sleep(oal_work_stru *work)
{
    unsigned long flags;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = container_of(work, struct pm_drv_data, send_allow_sleep_work);
    if (pm_data == NULL) {
        return;
    }

    ps_print_info("[%s]%s\n", index2name(pm_data->index), __func__);

    if (bfg_enter_sleep_check(pm_data) != SUCCESS) {
        ps_print_err("bfg sleep check failed!\n");
        return;
    }

    oal_spin_lock_irq_save(&pm_data->uart_state_spinlock, &flags);
    ps_core_d = pm_get_core(pm_data);
    if (agree_allow_bfg_sleep(ps_core_d, pm_data) != OAL_TRUE) {
        oal_spin_unlock_irq_restore(&pm_data->uart_state_spinlock, &flags);
        mod_timer(&pm_data->bfg_timer, jiffies + msecs_to_jiffies(PLATFORM_SLEEP_TIME));
        oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
        return;
    }

    /* ����device״̬Ϊ˯��̬����host����dev���֮ǰ(��dev����hostǰ)uart������ */
    ps_print_info("[%s]set UART_NOT_READY,BFGX_SLEEP\n", index2name(pm_data->index));
    bfgx_uart_state_set(pm_data, UART_NOT_READY);
    bfgx_dev_state_set(pm_data, BFGX_SLEEP);
    /* clear mod cnt */
    oal_atomic_set(&pm_data->bfg_timer_mod_cnt, 0);
    pm_data->bfg_timer_mod_cnt_pre = 0;

    oal_spin_unlock_irq_restore(&pm_data->uart_state_spinlock, &flags);

    mod_timer(&pm_data->dev_ack_timer, jiffies + msecs_to_jiffies(WAIT_DEVACK_MSEC));

    host_allow_bfg_sleep_cmd(pm_data);

    return;
}

/*
 * Prototype    : bfg_check_timer_work
 * Description  : check bfg timer is work fine
 * input        : ps_core_d
 */
void bfg_check_timer_work(struct pm_drv_data *pm_data)
{
    uint32_t temp_timer_mod_cnt;

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return;
    }

    temp_timer_mod_cnt = (uint32_t)oal_atomic_read(&pm_data->bfg_timer_mod_cnt);
    /* 10s��û��������bfg timer ����:ֱ���ύallow to sleep work */
    if ((pm_data->bfg_timer_mod_cnt_pre == temp_timer_mod_cnt) && (temp_timer_mod_cnt != 0)
        && (gnss_get_lowpower_state(pm_data) == GNSS_AGREE_SLEEP)
        && (pm_data->bfgx_lowpower_enable == BFGX_PM_ENABLE)) {
        if (time_after(jiffies, pm_data->bfg_timer_check_time)) {
            declare_dft_trace_key_info("bfg_timer not work in 10s", OAL_DFT_TRACE_FAIL);
            if (queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work) != true) {
                ps_print_info("[%s]queue_work send_allow_sleep_work not return true\n",
                              index2name(pm_data->index));
            } else {
                ps_print_info("[%s]timer_state(%d),queue_work send_allow_sleep_work succ\n",
                              index2name(pm_data->index), timer_pending(&pm_data->bfg_timer));
            }
        }
    } else {
        pm_data->bfg_timer_mod_cnt_pre = temp_timer_mod_cnt;
        pm_data->bfg_timer_check_time = jiffies + msecs_to_jiffies(PL_CHECK_TIMER_WORK);
    }
}

static int32_t bfg_timer_expire_param_check(struct pm_drv_data *pm_data)
{
    if (unlikely(pm_data == NULL)) {
        ps_print_err("pm_data is null\n");
        return OAL_FALSE;
    }

    if (pm_data->bfgx_lowpower_enable == BFGX_PM_DISABLE) {
        ps_print_dbg("lowpower function disabled\n");
        return OAL_FALSE;
    }
    if (bfgx_dev_state_get(pm_data) == BFGX_SLEEP) {
        ps_print_dbg("dev has been sleep\n");
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

/*
 * Prototype    : bfg_timer_expire
 * Description  : bfg timer expired function
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
OAL_STATIC void bfg_timer_expire(unsigned long data)
#else
OAL_STATIC void bfg_timer_expire(struct timer_list *t)
#endif
{
    struct ps_core_s *ps_core_d = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    struct pm_drv_data *pm_data = (struct pm_drv_data *)(uintptr_t)data;
#else
    struct pm_drv_data *pm_data = from_timer(pm_data, t, bfg_timer);
#endif
    uint32_t gnss_state;

    if (bfg_timer_expire_param_check(pm_data) == OAL_FALSE) {
        return;
    }

    ps_core_d = pm_data->ps_core_data;
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is NULL\n");
        return;
    }

    gnss_state = gnss_get_lowpower_state(pm_data);
    if (gnss_state == GNSS_AGREE_SLEEP) {
        if (queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work) != true) {
            ps_print_info("[%s]queue_work send_allow_sleep_work not return true\n",
                          index2name(pm_data->index));
        }
        pm_data->gnss_votesleep_check_cnt = 0;
        pm_data->rx_pkt_gnss_pre = 0;
    } else if (gnss_state == GNSS_NOT_AGREE_SLEEP) {
        /* GNSS NOT AGREE SLEEP ,Check it */
        if (pm_data->rx_pkt_gnss_pre != ps_core_d->bfgx_info[BFGX_GNSS].rx_pkt_num) {
            pm_data->rx_pkt_gnss_pre = ps_core_d->bfgx_info[BFGX_GNSS].rx_pkt_num;
            pm_data->gnss_votesleep_check_cnt = 0;

            mod_timer(&pm_data->bfg_timer, jiffies +  msecs_to_jiffies(PLATFORM_SLEEP_TIME));
            oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
        } else {
            pm_data->gnss_votesleep_check_cnt++;
            if (pm_data->gnss_votesleep_check_cnt >= PL_CHECK_GNSS_VOTE_CNT) {
                ps_print_err("[%s]gnss_votesleep_check_cnt %d,set GNSS_AGREE_SLEEP\n",
                             index2name(pm_data->index), pm_data->gnss_votesleep_check_cnt);
                atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
                queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work);

                pm_data->gnss_votesleep_check_cnt = 0;
                pm_data->rx_pkt_gnss_pre = 0;
            } else {
                mod_timer(&pm_data->bfg_timer, jiffies +  msecs_to_jiffies(PLATFORM_SLEEP_TIME));
                oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
            }
        }
    }
}

int32_t host_wkup_dev(struct pm_drv_data *pm_data)
{
    unsigned long timeleft;
    struct ps_core_s *ps_core_d = NULL;

    if (unlikely(pm_data == NULL)) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_core_d = pm_get_core(pm_data);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("[%s]ps_core_d or tty is NULL\n", index2name(pm_data->index));
        return -EINVAL;
    }
    if (pm_data->bfgx_lowpower_enable == BFGX_PM_DISABLE) {
        return 0;
    }
    ps_print_dbg("wkup start\n");

    oal_reinit_completion(pm_data->host_wkup_dev_comp);
    queue_work(pm_data->wkup_dev_workqueue, &pm_data->wkup_dev_work);
    timeleft = wait_for_completion_timeout(&pm_data->host_wkup_dev_comp, msecs_to_jiffies(WAIT_WKUPDEV_MSEC));
    if (!timeleft) {
        ps_print_err("[%s]wait wake up dev timeout\n", index2name(pm_data->index));
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_BFG_WKUP_DEV);

        return -ETIMEDOUT;
    }
    ps_print_dbg("wkup over\n");

    return 0;
}

/*
 * Prototype    : bfgx_other_subsys_all_shutdown
 * Description  : check other subsystem is shutdown or not
 * Input        : uint8_t type: means one system to check
 * Return       : 0 - other subsystem are all shutdown
 *                1 - other subsystem are not all shutdown
 */
int32_t bfgx_other_subsys_all_shutdown(struct pm_drv_data *pm_data, uint8_t subsys)
{
    int32_t i = 0;
    struct ps_core_s *ps_core_d = pm_get_core(pm_data);

    if (ps_core_d == NULL) {
        ps_print_err("ps_core_d is NULL\n");
        return -EINVAL;
    }

    for (i = 0; i < BFGX_BUTT; i++) {
        if (i == subsys) {
            continue;
        }

        if (atomic_read(&ps_core_d->bfgx_info[i].subsys_state) == POWER_STATE_OPEN) {
            return false;
        }
    }

    return true;
}

/*
 * Prototype    : bfgx_print_subsys_state
 * Description  : check all sub system state
 */
void bfgx_print_subsys_state(void)
{
    int32_t i = 0;
    int32_t total;
    int32_t count = 0;
    char print_str[BFGX_SUBSYS_STATE_FROMAT_LEN];

    struct ps_core_s *ps_core_d = ps_get_core_reference(BUART);
    if (ps_core_d == NULL) {
        ps_print_err("ps_core_d is NULL\n");
        return;
    }

    total = 0;

    for (i = 0; i < BFGX_BUTT; i++) {
        if (atomic_read(&ps_core_d->bfgx_info[i].subsys_state) == POWER_STATE_OPEN) {
            total = snprintf_s(print_str + count, sizeof(print_str) - count, sizeof(print_str) - count - 1,
                               "%s:%s ", ps_core_d->bfgx_info[i].name, "on ");
            if (total < 0) {
                oal_io_print("log str format err line[%d]\n", __LINE__);
                return;
            }
            count += total;
        } else {
            total = snprintf_s(print_str + count, sizeof(print_str) - count, sizeof(print_str) - count - 1,
                               "%s:%s ", ps_core_d->bfgx_info[i].name, "off");
            if (total < 0) {
                oal_io_print("log str format err line[%d]\n", __LINE__);
                return;
            }
            count += total;
        }
    }

    ps_print_err("%s\n", print_str);
}

void bfgx_gpio_intr_enable(struct pm_drv_data *pm_data, uint32_t ul_en)
{
    unsigned long flags;
    uint32_t irq;
    oal_spin_lock_irq_save(&pm_data->bfg_irq_spinlock, &flags);
    irq = (uint32_t)pm_data->bfg_irq;
    if (ul_en) {
        /* ����֧���жϿ���Ƕ�� */
        if (pm_data->ul_irq_stat) {
            enable_irq(irq);
            pm_data->ul_irq_stat = 0;
        }
    } else {
        if (!pm_data->ul_irq_stat) {
            disable_irq_nosync(irq);
            pm_data->ul_irq_stat = 1;
        }
    }
    oal_spin_unlock_irq_restore(&pm_data->bfg_irq_spinlock, &flags);
}

bool bfgx_is_boot_up(struct pm_drv_data *pm_data)
{
    const int check_interval = 50;
    int retry = 20; // 1���ӣ�����20��
    while (oal_gpio_get_value(pm_data->wakeup_host_gpio) != GPIO_HIGHLEVEL) {
        if (retry-- <= 0) {
            return false;
        }
        msleep(check_interval);
    }
    return true;
}

static int32_t bfgx_wait_bootup(struct pm_drv_data *pm_data)
{
    unsigned long timeleft;
    int32_t error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_core_d = pm_get_core(pm_data);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    /* WAIT_BFGX_BOOTOK_TIME:���ʱ��ĿǰΪ1s����1s�������������Ҫ��ע */
    timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
    if (!timeleft) {
        ps_uart_state_dump(ps_core_d);
        if (wlan_is_shutdown()) {
            ps_print_err("[%s]wifi off, wait bfgx boot up ok timeout\n", index2name(pm_data->index));
            error = BFGX_POWER_WIFI_OFF_BOOT_UP_FAIL;
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP_WITH_WIFI_OFF);
        } else {
            ps_print_err("[%s]wifi on, wait bfgx boot up ok timeout\n", index2name(pm_data->index));
            error = BFGX_POWER_WIFI_ON_BOOT_UP_FAIL;
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP_WITH_WIFI_ON);
        }
    }

    return error;
}

STATIC int32_t bfgx_dev_power_on(struct pm_drv_data *pm_data)
{
    int32_t error;
    struct ps_core_s *ps_core_d = NULL;
    hi110x_board_info *bd_info = NULL;
    uint32_t sys;

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        return BFGX_POWER_FAILED;
    }
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_core_d = pm_get_core(pm_data);
    if (unlikely(ps_core_d == NULL)) {
        return BFGX_POWER_FAILED;
    }
    /* ��ֹHost˯�� */
    oal_wake_lock(&pm_data->bus_wake_lock);

    oal_reinit_completion(pm_data->dev_bootok_ack_comp);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);
    sys = (pm_data->index == BUART) ? B_SYS : G_SYS;
    error = bd_info->bd_ops.bfgx_dev_power_on(sys);
    if (error != BFGX_POWER_SUCCESS) {
        goto bfgx_power_on_fail;
    }

    error = bfgx_wait_bootup(pm_data);
    if (error != BFGX_POWER_SUCCESS) {
        goto bfgx_power_on_fail;
    }

    atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
#ifdef _PRE_H2D_GPIO_WKUP
    /* host����bfg�������gpio��ʽ����Ҫ���������1103 */
    error = host_send_cfg_gpio_wkup_dev_msg(pm_data);
    if (error != SUCCESS) {
        goto bfgx_power_on_fail;
    }
#endif

    if (oal_atomic_read(&g_ir_only_mode) == 0) {
        if (wlan_is_shutdown()) {
            ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_NOTIFY_WIFI_CLOSE);
        } else {
            ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_NOTIFY_WIFI_OPEN);
        }

        bfgx_pm_feature_set(pm_data);
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
    oal_wake_unlock(&pm_data->bus_wake_lock);
    return error;
}

STATIC void bfgx_timer_clear(struct pm_drv_data *pm_data)
{
    ps_print_info("[%s]delete sleep timer and work!\n", index2name(pm_data->index));
    pm_data->operate_beat_timer(BEAT_TIMER_DELETE);
    cancel_work_sync(&pm_data->send_allow_sleep_work);
    del_timer_sync(&pm_data->bfg_timer);
    oal_atomic_set(&pm_data->bfg_timer_mod_cnt, 0);
    pm_data->bfg_timer_mod_cnt_pre = 0;
}
/*
 * Prototype    : bfgx_dev_power_off
 * Description  : bfg device power off function
 * Return       : 0 means succeed,-1 means failed
 */
STATIC int32_t bfgx_dev_power_off(struct pm_drv_data *pm_data)
{
    int32_t error = SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    uint32_t sys;
    hi110x_board_info *bd_info = NULL;
    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        return -FAILURE;
    }

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_core_d = pm_get_core(pm_data);
    if (unlikely(ps_core_d == NULL)) {
        return -FAILURE;
    }

    /* ������û������ */
    if (oal_atomic_read(&g_ir_only_mode) == 0) {
        bfgx_timer_clear(pm_data);
    }

    /* �µ缴����ɣ���Ҫ�ڴ�ʱ�����´��ϵ�Ҫ�ȴ�device�ϵ�ɹ���flag */
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);
    sys = (pm_data->index == BUART) ? B_SYS : G_SYS;
    bd_info->bd_ops.bfgx_dev_power_off(sys);

    ps_core_d->rx_pkt_sys = 0;
    ps_core_d->rx_pkt_oml = 0;

#ifdef _PRE_H2D_GPIO_WKUP
    atomic_set(&pm_data->cfg_wkup_dev_flag, 0);
#endif

    bfg_wake_unlock(pm_data);

    return error;
}

uint32_t gnss_get_lowpower_state(struct pm_drv_data *pm_data)
{
    if (pm_data->gnss_frc_awake) {
        return GNSS_FRC_AWAKE;
    } else {
        return (uint32_t)atomic_read(&pm_data->gnss_sleep_flag);
    }
}

int32_t gnss_me_open(void)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    struct pm_drv_data *pm_data = NULL;

    ps_print_info("open gnss me\n");

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely(ps_core_d == NULL)) {
        return BFGX_POWER_FAILED;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_GNSS];
    if (atomic_read(&pst_bfgx_data->subsys_state) == POWER_STATE_OPEN) {
        ps_print_warning("gnss me has opened! It's Not necessary to send msg to device\n");
        return BFGX_POWER_SUCCESS;
    }
    pm_data = ps_core_d->pm_data;
    if (!bfgx_is_boot_up(pm_data)) {
        ps_print_err("gcpu is not boot up!\n");
        return BFGX_POWER_FAILED;
    }

    if (alloc_seperted_rx_buf(ps_core_d, BFGX_GNSS, GNSS_RX_MAX_FRAME, VMALLOC) != 0) {
        ps_print_err("gnss me alloc failed! len=%d\n", GNSS_RX_MAX_FRAME);
        return BFGX_POWER_FAILED;
    }

    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("gnss me open tty fail!\n");
        goto open_tty_fail;
    }

    oal_reinit_completion(pm_data->dev_bootok_ack_comp);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    bfgx_gpio_intr_enable(pm_data, OAL_TRUE);

    if (bfgx_wait_bootup(ps_core_d->pm_data) != BFGX_POWER_SUCCESS) {
        ps_print_err("gnss me open wait bootup fail!\n");
        goto open_cmd_fail;
    }

    if (bfgx_open_cmd_send(BFGX_GNSS, pm_data->index) != BFGX_POWER_SUCCESS) {
        ps_print_err("bfgx open cmd fail\n");
        goto open_cmd_fail;
    }
    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_OPEN);

    return BFGX_POWER_SUCCESS;

open_cmd_fail:
    bfgx_uart_state_set(pm_data, UART_NOT_READY);
    bfgx_dev_state_set(pm_data, BFGX_SLEEP);
    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);
open_tty_fail:
    free_seperted_rx_buf(ps_core_d, BFGX_GNSS, VMALLOC);
    return BFGX_POWER_FAILED;
}

int32_t gnss_me_close(void)
{
    int32_t ret;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    struct pm_drv_data *pm_data = NULL;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_ME));
    if (unlikely(ps_core_d == NULL)) {
        return BFGX_POWER_FAILED;
    }
    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_GNSS];

    if (atomic_read(&pst_bfgx_data->subsys_state) == POWER_STATE_SHUTDOWN) {
        ps_print_warning("gnss me has closed! It's Not necessary to send msg to device\n");
        return BFGX_POWER_SUCCESS;
    }

    pm_data = ps_core_d->pm_data;

    ps_print_info("close gnss me\n");

    oal_wait_queue_wake_up_interrupt(&pst_bfgx_data->rx_wait);

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0) {
        /* ����ʧ�ܣ�bfgx closeʱ�Ļ���ʧ�ܲ�����DFR�ָ� */
        ps_print_err("[%s]prepare work FAIL\n", index2name(pm_data->index));
    }

    bfgx_timer_clear(pm_data);

    ret = bfgx_close_cmd_send(BFGX_GNSS, pm_data->index);
    if (ret < 0) {
        /* ����close����ʧ�ܣ�������DFR�����������µ����̣�DFR�ָ��ӳٵ��´�openʱ��������ҵ������ʱ���� */
        ps_print_err("bfgx close cmd fail\n");
    }
    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_SHUTDOWN);

    if (uart_bfgx_close_cmd(ps_core_d) != SUCCESS) {
        /* bfgx self close fail, go on */
        ps_print_err("gnss me self close fail\n");
    }

    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);

    if (release_tty_drv(ps_core_d) != SUCCESS) {
        /* ����ִ�е��˴���˵������һ����ҵ���Ѿ��رգ�����tty�Ƿ�رճɹ���device��Ҫ�µ� */
        ps_print_err("close gnss me tty is err!");
    }

    post_to_visit_node(ps_core_d);

    /* �µ缴����ɣ���Ҫ�ڴ�ʱ�����´��ϵ�Ҫ�ȴ�device�ϵ�ɹ���flag */
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    free_seperted_rx_buf(ps_core_d, BFGX_GNSS, VMALLOC);
    ps_kfree_skb(ps_core_d, RX_GNSS_QUEUE);

    bfgx_uart_state_set(pm_data, UART_NOT_READY);
    bfgx_dev_state_set(pm_data, BFGX_SLEEP);
    bfg_wake_unlock(pm_data);

    return BFGX_POWER_SUCCESS;
}

static void firmware_download_fail_proc(struct pm_top *pm_top_data, int which_cfg)
{
    ps_print_err("firmware download fail!\n");
    declare_dft_trace_key_info("patch_download_fail", OAL_DFT_TRACE_FAIL);
    if (which_cfg == BFGX_CFG) {
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_BFG_FW_DOWN);
    } else {
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_WIFI_FW_DOWN);
    }

    if (pm_top_data->wlan_pm_info->pst_bus->bus_type == HCC_BUS_SDIO) {
        (void)ssi_dump_err_regs(SSI_ERR_FIRMWARE_DOWN_SDIO_FAIL);
    } else {
        (void)ssi_dump_err_regs(SSI_ERR_FIRMWARE_DOWN_FAIL);
    }

    if (hi11xx_get_os_build_variant() != HI1XX_OS_BUILD_VARIANT_USER) {
        /* dump bootloader rw dtcm */
        ssi_read_reg_info_test(HI1103_BOOTLOAD_DTCM_BASE_ADDR, HI1103_BOOTLOAD_DTCM_SIZE, 1, SSI_RW_DWORD_MOD);
    }
}


/*
 * Description  : ����ǰ������
 */
static int pre_download_process(struct pm_top *pm_top_data, firmware_downlaod_privfunc priv_func)
{
    int32_t ret;
    /* firmware_cfg_init(sdio) function should just be called once */
    if (!test_bit(FIRMWARE_CFG_INIT_OK, &pm_top_data->firmware_cfg_init_flag)) {
        ps_print_info("firmware_cfg_init begin\n");
        ret = firmware_cfg_init();
        if (ret) {
            ps_print_err("firmware_cfg_init failed, ret:%d!\n", ret);
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_CFG_FAIL_FIRMWARE_DOWN);
            return ret;
        }

        ps_print_info("firmware_cfg_init OK\n");
        set_bit(FIRMWARE_CFG_INIT_OK, &pm_top_data->firmware_cfg_init_flag);
    }

    /* do some private command before load cfg */
    if (priv_func != NULL) {
        ret = priv_func();
        if (ret) {
            ps_print_err("priv_func failed, ret:%d!\n", ret);
            return ret;
        }
    }

    return SUCCESS;
}

/*
* wifiû��ʱ, ��Ҫ��ʼ��bus�����򣬹ر�wifi�͹��ģ��ȼ�����ɺ��ٿ���wifi�͹���
*/
static int firmware_download_bus_prepare(hcc_bus *bus)
{
    int32_t ret;

    if (wlan_is_shutdown() || (in_plat_exception_reset() == OAL_TRUE)) {
        ret = hcc_bus_reinit(bus);
        if (ret != OAL_SUCC) {
            ps_print_err("sdio reinit failed, ret:%d!\n", ret);
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_FW_SDIO_INIT);
            return -FAILURE;
        }
        if (bus->bus_dev->dev_id == HCC_EP_WIFI_DEV) {
            wlan_pm_init_dev();
        } else if (bus->bus_dev->dev_id == HCC_EP_GT_DEV) {
            gt_pm_init_dev();
        } else if (bus->bus_dev->dev_id == HCC_EP_WIFI_DEV) {
            wlan_pm_init_dev();
        }
    } else {
        wlan_pm_disable();
    }
    return SUCCESS;
}

static int plat_firmware_download(uint32_t which_cfg, firmware_downlaod_privfunc priv_func,
                                  hcc_bus *bus)
{
    int32_t ret;
    struct pm_top *pm_top_data = pm_get_top();

    ps_print_info("enter firmware_download_function\n");

    if (pm_top_data == NULL || bus == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (which_cfg >= CFG_FILE_TOTAL) {
        ps_print_err("cfg file index [%d] outof range\n", which_cfg);
        return -FAILURE;
    }

    hcc_bus_and_wake_lock(bus);

    if (firmware_download_bus_prepare(bus) != SUCCESS) {
        hcc_bus_and_wake_unlock(bus);
        return -FAILURE;
    }

    ps_print_info("firmware_download begin\n");

    ret = pre_download_process(pm_top_data, priv_func);
    if (ret != SUCCESS) {
        hcc_bus_and_wake_unlock(bus);
        return ret;
    }

    ret = firmware_download(which_cfg, bus);
    if (ret < 0) {
        hcc_bus_and_wake_unlock(bus);
        if (ret != -OAL_EINTR) {
            firmware_download_fail_proc(pm_top_data, which_cfg);
        } else {
            /* download firmware interrupt */
            ps_print_info("firmware download interrupt!\n");
            declare_dft_trace_key_info("patch_download_interrupt", OAL_DFT_TRACE_FAIL);
        }
        return ret;
    }
    declare_dft_trace_key_info("patch_download_ok", OAL_DFT_TRACE_SUCC);

    hcc_bus_and_wake_unlock(bus);

    if (!wlan_is_shutdown()) {
        wlan_pm_enable();
    }

    ps_print_info("firmware_download success\n");
    return SUCCESS;
}

/*
 * Prototype    : firmware_download_function
 * Description  : download wlan patch
 * Return       : 0 means succeed,-1 means failed
 */
int firmware_download_function_priv(uint32_t which_cfg, firmware_downlaod_privfunc priv_func,
                                    hcc_bus *bus)
{
    int32_t ret;
    unsigned long long total_time;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_time = 0;
    static unsigned long long count = 0;

    start_time = ktime_get();

    ret = plat_firmware_download(which_cfg, priv_func, bus);
    if (ret != SUCCESS) {
        return ret;
    }

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);
    if (total_time > max_time) {
        max_time = total_time;
    }

    count++;
    ps_print_warning("download firmware, count [%llu], current time [%llu]us, max time [%llu]us\n",
                     count, total_time, max_time);
    return SUCCESS;
}

static uint32_t firware_download_get_cfg(uint32_t which_cfg)
{
    uint32_t cfg = which_cfg;
    if (hi110x_is_emu() != OAL_TRUE) {
        return cfg;
    }
    if (which_cfg == BFGX_AND_WIFI_CFG) { // ��WIFI ������BFG
        cfg = WIFI_CFG;
    } else {
        ps_print_err("only support wifi which_cfg=%u\n", which_cfg);
        cfg = CFG_FILE_TOTAL;
    }
    return cfg;
}

int firmware_download_function(uint32_t which_cfg, hcc_bus *bus)
{
    return firmware_download_function_priv(firware_download_get_cfg(which_cfg), NULL, bus);
}

bool wlan_is_shutdown(void)
{
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }

    return ((wlan_pm_info->wlan_power_state == POWER_STATE_SHUTDOWN) ? true : false);
}

bool bfgx_is_shutdown(void)
{
    struct ps_core_s *ps_core_d = ps_get_core_reference(BUART);
    if (ps_core_d == NULL) {
        ps_print_info("bfgx not init\n");
        return true;
    }

    return ps_chk_bfg_active(ps_core_d) ? false : true;
}
EXPORT_SYMBOL(bfgx_is_shutdown);

bool gle_is_shutdown(void)
{
    struct ps_core_s *ps_core_d = ps_get_core_reference(GUART);
    if (ps_core_d == NULL) {
        ps_print_info("bfgx not init\n");
        return true;
    }

    return ps_chk_bfg_active(ps_core_d) ? false : true;
}
EXPORT_SYMBOL(gle_is_shutdown);

int32_t wifi_power_fail_process(int32_t error)
{
    int32_t ret = WIFI_POWER_FAIL;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }

    // emu ƽ̨�����ϵ��쳣���µ磬�������ⶨλ
    if (hi110x_is_emu() == OAL_TRUE) {
        ps_print_err("emu skip power fail proces\n");
        return -FAILURE;
    }

    ps_print_err("wifi power fail, error=[%d]\n", error);

    switch (error) {
        case WIFI_POWER_SUCCESS:
        case WIFI_POWER_PULL_POWER_GPIO_FAIL:
            break;

        /* BFGX off��wifi firmware download fail��wait boot up fail��ֱ�ӷ���ʧ�ܣ��ϲ����ԣ�����DFR */
        case WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL:
            if (oal_trigger_bus_exception(wlan_pm_info->pst_bus, OAL_TRUE) == OAL_TRUE) {
                /* exception is processing, can't power off */
                ps_print_info("bfgx off,sdio exception is working\n");
                break;
            }
            ps_print_info("bfgx off,set wlan_power_state to shutdown\n");
            oal_wlan_gpio_intr_enable(hbus_to_dev(wlan_pm_info->pst_bus), OAL_FALSE);
            wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
            ret = -OAL_EINTR;
            board_power_off(W_SYS);
            break;
        case WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT:
            ret = -OAL_EINTR;
            board_power_off(W_SYS);
            break;
            /* ��ϵͳ�жϵ��쳣, �޸ķ���ֵ���� */
        case WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL:
        case WIFI_POWER_BFGX_OFF_PULL_WLEN_FAIL:
        case WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL:
        case WIFI_POWER_ON_CALI_FAIL:
            board_power_off(W_SYS);
            break;
        /* BFGX on��wifi�ϵ�ʧ�ܣ�����ȫϵͳ��λ��wifi���η���ʧ�ܣ��ϲ����� */
        case WIFI_POWER_BFGX_ON_BOOT_UP_FAIL:
            if (oal_trigger_bus_exception(wlan_pm_info->pst_bus, OAL_TRUE) == OAL_TRUE) {
                /* exception is processing, can't power off */
                ps_print_info("bfgx on,sdio exception is working\n");
                break;
            }
            ps_print_info("bfgx on,set wlan_power_state to shutdown\n");
            oal_wlan_gpio_intr_enable(hbus_to_dev(wlan_pm_info->pst_bus), OAL_FALSE);
            wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
            if (plat_power_fail_exception_info_set(SUBSYS_WIFI, THREAD_WIFI, WIFI_POWERON_FAIL) ==
                WIFI_POWER_SUCCESS) {
                bfgx_system_reset();
                plat_power_fail_process_done();
            } else {
                ps_print_err("wifi power fail, set exception info fail\n");
            }
            break;
        case WIFI_POWER_BFGX_DERESET_WCPU_FAIL:
        case WIFI_POWER_BFGX_ON_FIRMWARE_DOWNLOAD_FAIL:
        case WIFI_POWER_BFGX_ON_PULL_WLEN_FAIL:
            if (plat_power_fail_exception_info_set(SUBSYS_WIFI, THREAD_WIFI, WIFI_POWERON_FAIL) ==
                WIFI_POWER_SUCCESS) {
                bfgx_system_reset();
                plat_power_fail_process_done();
            } else {
                ps_print_err("wifi power fail, set exception info fail\n");
            }
            break;

        default:
            ps_print_err("error is undefined, error=[%d]\n", error);
            break;
    }

    return ret;
}

static void wifi_notify_bfgx_status(uint8_t status)
{
    int32_t ret;

    struct ps_core_s *ps_core_d = ps_get_core_reference(BUART);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return;
    }

    if (!bfgx_is_shutdown()) {
        ret = prepare_to_visit_node(ps_core_d);
        if (ret < 0) {
            ps_print_err("prepare work fail, bring to reset work\n");
            plat_exception_handler(SUBSYS_BFGX, THREAD_IDLE, BFGX_WAKEUP_FAIL);
            return;
        }

        ps_tx_sys_cmd(ps_core_d, SYS_MSG, status);

        post_to_visit_node(ps_core_d);
    }
}

#ifdef _PRE_SHARE_BUCK_SURPORT
#ifndef CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
/*
*  �� �� ��  : buck_mode_get
*  ��������  : ��ȡbuck����
*  ����ֵ:
*  0:  ȫ����buck
*  1:  I2C���ƶ�������buck
*  2:  GPIO���ƶ�������buck
*  3:  host���ƹ�������buck��ѹ
*/
OAL_STATIC uint8_t buck_mode_get(void)
{
    return  (((g_st_board_info.buck_param) & BUCK_MODE_MASK) >> BUCK_MODE_OFFSET);
}

/*
*  �� �� ��  : buck_number_get
*  ��������  : ��ȡ��ͬ��Ʒ��buck���
*  ����ֵ:
*/
OAL_STATIC uint8_t buck_number_get(void)
{
    return  (((g_st_board_info.buck_param) & BUCK_NUMBER_MASK) >> BUCK_NUMBER_OFFSET);
}

OAL_STATIC void buck_pmic_cfg(uint16_t buck_control_flag, uint8_t high_vset)
{
#define BUCK_1_P15_V    1150000
#define BUCK_1_P05_V    1050000
#ifndef WBG_PMIC_REQ
#define WBG_PMIC_REQ 8
#endif
    struct hw_comm_pmic_cfg_t fp_pmic_ldo_set;
    uint8_t buck_num = buck_number_get();
    if (buck_num == BUCK_NUMBER_AE || buck_num == BUCK_NUMBER_ONNT) {
        fp_pmic_ldo_set.pmic_num = buck_num;
    } else {
        ps_print_info("share buck mode, invalid buck num cfg[%d], check dts & ini\n", buck_num);
        return;
    }
    fp_pmic_ldo_set.pmic_power_type = VOUT_BUCK_1;
    if (high_vset) {
        fp_pmic_ldo_set.pmic_power_voltage = BUCK_1_P15_V;
    } else {
        fp_pmic_ldo_set.pmic_power_voltage = BUCK_1_P05_V;
    }
    /* �򿪻�ر� 0/1 */
    if (buck_control_flag) {
        fp_pmic_ldo_set.pmic_power_state = 1;
    } else {
        fp_pmic_ldo_set.pmic_power_state = 0;
    }

    ps_print_info("share buck mode, buck_flag[0x%x],pmic_num[%d],pmic_power_type[%d],power_state[%d],vset[%d]\n",
                  buck_control_flag, fp_pmic_ldo_set.pmic_num, fp_pmic_ldo_set.pmic_power_type,
                  fp_pmic_ldo_set.pmic_power_state, fp_pmic_ldo_set.pmic_power_voltage);

    hw_pmic_power_cfg(WBG_PMIC_REQ, &fp_pmic_ldo_set);
}
#endif
#endif


/* ��������buck���µ�ӿ� */
void buck_power_ctrl(uint8_t enable, uint8_t subsys)
{
#ifdef _PRE_SHARE_BUCK_SURPORT
#ifndef CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
    if (subsys > PLAT_SUBSYS) {
        return ;
    }

    if (buck_mode_get() == EXT_BUCK_HOST_CTL) {
        uint8_t high_vset = OAL_TRUE;

        if (enable) {
            g_st_board_info.buck_control_flag |= (1 << subsys);
        } else {
            g_st_board_info.buck_control_flag &= ~(1 << subsys);
        }

        /* ��ƽ̨Ĭ���ϵ磬����WIFI/BT�򿪣�1.15v */
        if ((g_st_board_info.buck_control_flag == HI110X_PLAT_SUB_MASK) ||
            (g_st_board_info.buck_control_flag & HI110X_BT_SUB_MASK) ||
            (g_st_board_info.buck_control_flag & HI110X_WIFI_SUB_MASK)) {
            high_vset = OAL_TRUE;
        } else {
            high_vset = OAL_FALSE;
        }

        buck_pmic_cfg(g_st_board_info.buck_control_flag, high_vset);
    } else {
        ps_print_info("NOT share buck mode[0x%x], do nothing\n", g_st_board_info.buck_param);
    }
#endif
#endif
    return ;
}

static void wlan_power_on_timedebug(ktime_t start_time)
{
    unsigned long long total_time;
    ktime_t  end_time, trans_time;
    static unsigned long long max_download_time = 0;
    static unsigned long long num = 0;

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);
    if (total_time > max_download_time) {
        max_download_time = total_time;
    }

    num++;
    ps_print_warning("power on, count [%llu], current time [%llu]us, max time [%llu]us\n",
                     num, total_time, max_download_time);
}

int32_t wlan_power_on(void)
{
    int32_t error;
    ktime_t start_time;

    hi110x_board_info *bd_info = NULL;

    ps_print_info("wlan power on!\n");

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        return -FAILURE;
    }

    /* wifi�ϵ�ʱ���������򿪣�����Ҫ�رյ����⣬����ȫpatch */
    if (oal_atomic_read(&g_ir_only_mode) != 0) {
        if (hw_ir_only_open_other_subsys() != BFGX_POWER_SUCCESS) {
            ps_print_err("ir only mode,but close ir only mode fail!\n");
            return -FAILURE;
        }
    }

    start_time = ktime_get();

    if (hcc_bus_exception_is_busy(hcc_get_bus(HCC_EP_WIFI_DEV)) == OAL_TRUE) {
        declare_dft_trace_key_info("open_fail_exception_is_busy", OAL_DFT_TRACE_FAIL);

        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_ON_EXCP);
        return -FAILURE;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    if (is_dfr_test_en(WIFI_POWER_ON_FAULT)) {
        error = WIFI_POWER_BFGX_DERESET_WCPU_FAIL;
        ps_print_warning("dfr test WIFI_POWER_ON_FAULT enable\n");
        goto wifi_power_fail;
    }
#endif

    buck_power_ctrl(OAL_TRUE, WIFI_SUBSYS);

    error = bd_info->bd_ops.wlan_power_on();
    if (error != WIFI_POWER_SUCCESS) {
        goto wifi_power_fail;
    }

    wifi_notify_bfgx_status(SYS_CFG_NOTIFY_WIFI_OPEN);

    wlan_power_on_timedebug(start_time);

    return WIFI_POWER_SUCCESS;

wifi_power_fail:
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_WIFI_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_UP_WIFI);
    return wifi_power_fail_process(error);
}

int32_t wlan_power_off(void)
{
    int32_t error;
    hi110x_board_info *bd_info = NULL;

    ps_print_info("wlan power off!\n");

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        ps_print_err("board info is err\n");
        return -FAILURE;
    }

    wifi_notify_bfgx_status(SYS_CFG_NOTIFY_WIFI_CLOSE);

    error = bd_info->bd_ops.wlan_power_off();

    buck_power_ctrl(OAL_FALSE, WIFI_SUBSYS);

    return error;
}

int32_t wlan_power_off_complete(void)
{
    uint32_t i, timeout_cnt;
    int32_t ret = -OAL_EFAIL;
    int gpio_level;

    timeout_cnt = hi110x_get_emu_timeout(ACK_CHECK_MAX_CNT);
    gpio_level = 0;
    for (i = 0; i < timeout_cnt; i++) {
        msleep(ACK_CHECK_WAIT_TIME);
        gpio_level = board_get_dev_wkup_host_state(W_SYS);
        if (gpio_level == 1) {
            ret = OAL_SUCC;
            break;
        }
    }

    oal_print_hi11xx_log(HI11XX_LOG_WARN, "wlan wkup gpio level[%d], timeout_cnt[%d]", gpio_level, i);
    return ret;
}

int32_t gt_power_on(void)
{
    int32_t error;
    ktime_t start_time;

    hi110x_board_info *bd_info = NULL;

    ps_print_info("gt power on!\n");

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        return -FAILURE;
    }

    start_time = ktime_get();
    if (hcc_bus_exception_is_busy(hcc_get_bus(HCC_EP_GT_DEV)) == OAL_TRUE) {
        declare_dft_trace_key_info("open_fail_exception_is_busy", OAL_DFT_TRACE_FAIL);

        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_ON_EXCP);
        return -FAILURE;
    }

    if (bd_info->bd_ops.gt_power_on != NULL) {
        error = bd_info->bd_ops.gt_power_on();
        if (error != GT_POWER_SUCCESS) {
            goto gt_power_fail;
        }
    }

    wlan_power_on_timedebug(start_time);

    return GT_POWER_SUCCESS;

gt_power_fail:
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_WIFI_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_UP_WIFI);
    return gt_power_fail_process(error);
}

int32_t gt_power_off(void)
{
    int32_t error;
    hi110x_board_info *bd_info = NULL;

    ps_print_info("gt power off!\n");

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        ps_print_err("board info is err\n");
        return -FAILURE;
    }

    error = bd_info->bd_ops.gt_power_off();

    return error;
}

int32_t gt_power_off_complete(void)
{
    return OAL_SUCC;
}

STATIC int32_t bfgx_power_on(struct pm_drv_data *pm_data, uint8_t subsys)
{
    int32_t ret = BFGX_POWER_SUCCESS;
    unsigned long long total_time;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_download_time = 0;
    static unsigned long long num = 0;

    start_time = ktime_get();

    buck_power_ctrl(OAL_TRUE, subsys);

    if (bfgx_other_subsys_all_shutdown(pm_data, subsys)) {
        ret = bfgx_dev_power_on(pm_data);
        if (ret != BFGX_POWER_SUCCESS) {
            return ret;
        }
    }

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);
    if (total_time > max_download_time) {
        max_download_time = total_time;
    }

    num++;
    ps_print_warning("[%s]power on, count [%llu], current time [%llu]us, max time [%llu]us\n",
                     index2name(pm_data->index), num, total_time, max_download_time);

    return BFGX_POWER_SUCCESS;
}

STATIC int32_t bfgx_power_off(struct pm_drv_data *pm_data, uint8_t subsys)
{
    int32_t ret = SUCCESS;

    ps_print_info("[%s]%s power off!\n", index2name(pm_data->index), service_get_name(subsys));

    if (bfgx_other_subsys_all_shutdown(pm_data, subsys)) {
        ret = bfgx_dev_power_off(pm_data);
    }

    buck_power_ctrl(OAL_FALSE, subsys);

    return ret;
}

/*
 * Prototype    : bfgx_dev_power_control
 * Description  : bfg power control function
 * Input        : int32_t flag: 1 means on, 0 means off
 *                uint8_t type: means one of bfg
 * Return       : 0 means succeed,-1 means failed
 */
STATIC int32_t bfgx_dev_power_control(struct pm_drv_data *pm_data, uint8_t subsys, uint8_t flag)
{
    int32_t ret = 0;

    if (flag == BFG_POWER_GPIO_UP) {
        ret = bfgx_power_on(pm_data, subsys);
        if (ret) {
            ps_print_err("[%s]bfgx power on is error!\n", index2name(pm_data->index));
        }
    } else if (flag == BFG_POWER_GPIO_DOWN) {
        ret = bfgx_power_off(pm_data, subsys);
        if (ret) {
            ps_print_err("[%s]bfgx power off is error!\n", index2name(pm_data->index));
        }
    } else {
        ps_print_err("invalid input data!\n");
        ret = -FAILURE;
    }

    return ret;
}

/*
 * Prototype    : bfg_wake_host_isr
 * Description  : functions called when bt wake host via GPIO
 */
irqreturn_t bfg_wake_host_isr(int irq, void *dev_id)
{
    unsigned long flags;
    struct pm_drv_data *pm_data = (struct pm_drv_data*)dev_id;

    if (!memcheck_is_working()) {
        return IRQ_NONE;
    }
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return IRQ_NONE;
    }

    ps_print_info("[%s]%s\n", index2name(pm_data->index), __func__);

    pm_data->bfg_wakeup_host++;

    pm_data->wakeup_src_debug = 1;

    if (oal_gpio_get_value(pm_data->wakeup_host_gpio) == GPIO_LOWLEVEL) {
        ps_print_info("bfg wakeup host gpio status is low\n");
    }

    oal_spin_lock_irq_save(&pm_data->wakelock_protect_spinlock, &flags);
    bfg_wake_lock(pm_data);
    bfgx_dev_state_set(pm_data, BFGX_ACTIVE);
    complete(&pm_data->dev_ack_comp);
    oal_spin_unlock_irq_restore(&pm_data->wakelock_protect_spinlock, &flags);

    queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);

    return IRQ_HANDLED;
}

#ifdef _PRE_HI_DRV_GPIO
void bfgx_wake_host(uint32_t para1)
{
    g_hisi_gpio_func->pfn_gpio_clear_bit_int(BFGX_WKAEUP_HOST_GPIO);
    ps_print_info("bgfx_wake_host entering.-------------add GpioClearBitInt shangshengyan\n");
    (void)bfg_wake_host_isr(0, (void *)pm_get_drvdata(BUART));
}
#endif

/* return 1 for wifi power on,0 for off. */
int32_t hi110x_get_wifi_power_stat(void)
{
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }
    return (wlan_pm_info->wlan_power_state != POWER_STATE_SHUTDOWN);
}
EXPORT_SYMBOL(hi110x_get_wifi_power_stat);

STATIC void low_power_remove(struct pm_drv_data *pm_data, int index)
{
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return;
    }

    free_irq((uint32_t)pm_data->bfg_irq, NULL);

    /* delete timer */
    del_timer_sync(&pm_data->bfg_timer);
    oal_atomic_set(&pm_data->bfg_timer_mod_cnt, 0);
    pm_data->bfg_timer_mod_cnt_pre = 0;

    del_timer_sync(&pm_data->dev_ack_timer);
    /* destory wake lock */
    oal_wake_lock_exit(&pm_data->bus_wake_lock);
    oal_wake_lock_exit(&pm_data->bt_wake_lock);
    oal_wake_lock_exit(&pm_data->gnss_wake_lock);
    /* free platform driver data struct */
    kfree(pm_data);

    pm_clr_drvdata(index);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
STATIC void devack_timer_expire(unsigned long data)
#else
STATIC void devack_timer_expire(struct timer_list *t)
#endif
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    struct pm_drv_data *pm_data = (struct pm_drv_data *)(uintptr_t)data;
#else
    struct pm_drv_data *pm_data = from_timer(pm_data, t, dev_ack_timer);
#endif
    if (unlikely(pm_data == NULL)) {
        ps_print_err("devack timer para is null\n");
        return;
    }

    ps_print_info("[%s]%s\n", index2name(pm_data->index), __func__);

    if (oal_gpio_get_value(pm_data->wakeup_host_gpio) == 1) { /* ������Ӧgpio�ܽŵ�ֵ */
        pm_data->uc_dev_ack_wait_cnt++;
        if (pm_data->uc_dev_ack_wait_cnt < WAIT_DEVACK_CNT) {
            mod_timer(&pm_data->dev_ack_timer, jiffies +  msecs_to_jiffies(WAIT_DEVACK_MSEC));
            return;
        }
        /* device doesn't agree to sleep */
        ps_print_info("[%s]device does not agree to sleep\n", index2name(pm_data->index));
        if (unlikely(pm_data->rcvdata_bef_devack_flag == 1)) {
            ps_print_info("[%s]device send data to host before dev rcv allow slp msg\n", index2name(pm_data->index));
            pm_data->rcvdata_bef_devack_flag = 0;
        }

        bfgx_dev_state_set(pm_data, BFGX_ACTIVE);
        bfgx_uart_state_set(pm_data, UART_READY);
        /*
         * we mod timer at any time, since we could get another chance to sleep
         * in exception case like:dev agree to slp after this ack timer expired
         */
        if ((!bfgx_other_subsys_all_shutdown(pm_data, BFGX_GNSS)) ||
            (gnss_get_lowpower_state(pm_data) == GNSS_AGREE_SLEEP)) {
            mod_timer(&pm_data->bfg_timer, jiffies + msecs_to_jiffies(BT_SLEEP_TIME));
            oal_atomic_inc(&pm_data->bfg_timer_mod_cnt);
        }

        complete(&pm_data->dev_slp_comp);
    } else {
        complete(&pm_data->dev_slp_comp);

        pm_data->uc_dev_ack_wait_cnt = 0;
    }
}

static void low_power_probe_timer_init(struct pm_drv_data *pm_data)
{
    /* init timer */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    init_timer(&pm_data->dev_ack_timer);
    pm_data->dev_ack_timer.function = devack_timer_expire;
    pm_data->dev_ack_timer.data = (uintptr_t)pm_data;
#else
    timer_setup(&pm_data->dev_ack_timer, devack_timer_expire, 0);
#endif
    pm_data->uc_dev_ack_wait_cnt = 0;

    /* init bfg data timer */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    init_timer(&pm_data->bfg_timer);
    pm_data->bfg_timer.function = bfg_timer_expire;
    pm_data->bfg_timer.data = (uintptr_t)pm_data;
#else
    timer_setup(&pm_data->bfg_timer, bfg_timer_expire, 0);
#endif
    oal_atomic_set(&pm_data->bfg_timer_mod_cnt, 0);
    pm_data->bfg_timer_mod_cnt_pre = 0;
    pm_data->bfg_timer_check_time = 0;
    pm_data->rx_pkt_gnss_pre = 0;
    pm_data->gnss_votesleep_check_cnt = 0;
}

#ifdef _PRE_HI_DRV_GPIO
STATIC int hitv_bfgx_wkup_int_init(void)
{
    int ret;

    ret = g_hisi_gpio_func->pfn_gpio_dir_set_bit(BFGX_WKAEUP_HOST_GPIO, 1);
    ret |= g_hisi_gpio_func->pfn_gpio_set_int_type(BFGX_WKAEUP_HOST_GPIO, BFGX_HI_UNF_GPIO_INTTYPE_UP);
    ps_print_err("HI_DRV_MODULE_GetFunction BT_GPIO TYPE_UP %08x \n", ret);
    ret |= g_hisi_gpio_func->pfn_gpio_register_server_func(BFGX_WKAEUP_HOST_GPIO, bfgx_wake_host);
    ret |= g_hisi_gpio_func->pfn_gpio_clear_bit_int(BFGX_WKAEUP_HOST_GPIO);
    ret |= g_hisi_gpio_func->pfn_gpio_set_int_enable(BFGX_WKAEUP_HOST_GPIO, BFGX_INT_ENABLE);
    ps_print_info("HI_DRV_MODULE_GetFunction BT_GPIO pfnGpio %08x \n", ret);

    return ret;
}
#endif

static int low_power_irq_init(struct pm_drv_data *pm_data, int idx)
{
    int ret;

    oal_spin_lock_init(&pm_data->bfg_irq_spinlock);
    pm_data->bfg_irq = (idx == BUART) ? g_st_board_info.wakeup_irq[B_SYS] : g_st_board_info.wakeup_irq[G_SYS];
    pm_data->wakeup_host_gpio = (idx == BUART) ? g_st_board_info.dev_wakeup_host[B_SYS] :
                                                 g_st_board_info.dev_wakeup_host[G_SYS];
    pm_data->wakeup_src_debug = 0;

#ifndef _PRE_HI_DRV_GPIO
    ret = request_irq((uint32_t)pm_data->bfg_irq, bfg_wake_host_isr,
                      IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, "bfgx_wake_host", (void*)pm_data);
    if (ret < 0) {
        ps_print_err("[%s]couldn't acquire %s IRQ\n", index2name(pm_data->index), PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
        return OAL_FAIL;
    }

    disable_irq_nosync((uint32_t)pm_data->bfg_irq);
    pm_data->ul_irq_stat = 1; /* irq diabled default */
#else
    ret = HI_DRV_MODULE_GetFunction(HI_ID_GPIO, (void**)(&g_hisi_gpio_func)); /* TV using hisi interfaces */
    if (ret) {
        return OAL_FAIL;
    }

    ret = hitv_bfgx_wkup_int_init();
    pm_data->ul_irq_stat = 0; /* irq enabled default */
#endif

    return OAL_SUCC;
}

static int low_power_workq_init(struct pm_drv_data *pm_data, int idx)
{
    oal_workqueue_stru *host_wkup_dev_workq = NULL;
    /* create an ordered workqueue with @max_active = 1 & WQ_UNBOUND flag to wake up device */
    host_wkup_dev_workq = create_singlethread_workqueue("wkup_dev_workqueue");
    if (host_wkup_dev_workq == NULL) {
        ps_print_err("create wkup workqueue failed\n");
        return OAL_FAIL;
    }
    pm_data->wkup_dev_workqueue = host_wkup_dev_workq;
    INIT_WORK(&pm_data->wkup_dev_work, host_wkup_dev_work);
    INIT_WORK(&pm_data->send_disallow_msg_work, host_send_disallow_msg);
    INIT_WORK(&pm_data->send_allow_sleep_work, host_allow_bfg_sleep);

    /* init bfg wake lock */
    oal_wake_lock_init(&pm_data->bus_wake_lock, BFG_LOCK_NAME);
    oal_wake_lock_init(&pm_data->bt_wake_lock, BT_LOCK_NAME);
    oal_wake_lock_init(&pm_data->gnss_wake_lock, GNSS_LOCK_NAME);

    /* init spinlock */
    oal_spin_lock_init(&pm_data->uart_state_spinlock);
    oal_spin_lock_init(&pm_data->wakelock_protect_spinlock);

    /* init completion */
    init_completion(&pm_data->host_wkup_dev_comp);
    init_completion(&pm_data->dev_ack_comp);
    init_completion(&pm_data->dev_slp_comp);
    init_completion(&pm_data->dev_bootok_ack_comp);
#ifdef _PRE_H2D_GPIO_WKUP
    init_completion(&pm_data->gpio_wkup_dev_ack_comp);
#endif
    return OAL_SUCC;
}

STATIC int low_power_probe(struct pm_drv_data **pp_data, int idx)
{
    struct pm_drv_data *pm_data = NULL;

    pm_data = kzalloc(sizeof(struct pm_drv_data), GFP_KERNEL);
    if (pm_data == NULL) {
        ps_print_err("no mem to allocate pm_data\n");
        goto PMDATA_MALLOC_FAIL;
    }

    pm_data->rcvdata_bef_devack_flag = 0;
    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->bt_fake_close_flag = OAL_FALSE;
    pm_data->bfgx_pm_ctrl_enable = BFGX_PM_DISABLE;
    if (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG ||
        get_hi110x_subchip_type() == BOARD_VERSION_HI1161) {
        pm_data->bfgx_lowpower_enable = BFGX_PM_DISABLE;
    } else {
        pm_data->bfgx_lowpower_enable = BFGX_PM_ENABLE;
    }

    pm_data->bfgx_bt_lowpower_enable = BFGX_PM_ENABLE;
    pm_data->bfgx_gnss_lowpower_enable = BFGX_PM_DISABLE;
    pm_data->bfgx_nfc_lowpower_enable = BFGX_PM_DISABLE;

    pm_data->gnss_frc_awake = 0;
    atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    pm_data->uart_state = UART_NOT_READY;

    if (low_power_irq_init(pm_data, idx) != OAL_SUCC) {
        goto REQ_IRQ_FAIL;
    }

    if (low_power_workq_init(pm_data, idx) != OAL_SUCC) {
        goto CREATE_WORKQ_FAIL;
    }

    low_power_probe_timer_init(pm_data);

    /* set driver data */
    pm_set_drvdata(pm_data, idx);

    /* register host pm */
    ps_pm_notifier_register(idx);

    *pp_data = pm_data;
    return OAL_SUCC;

CREATE_WORKQ_FAIL:
    free_irq((uint32_t)pm_data->bfg_irq, NULL);
REQ_IRQ_FAIL:
    kfree(pm_data);
PMDATA_MALLOC_FAIL:
    return -ENOMEM;
}

struct pm_top* g_pm_top = NULL;

struct pm_top* pm_get_top(void)
{
    return g_pm_top;
}

int low_power_init(void)
{
    int ret;
    struct pm_top* pm_top_data;

    pm_top_data = kzalloc(sizeof(struct pm_top), GFP_KERNEL);
    if (pm_top_data == NULL) {
        ps_print_err("no mem to allocate pm_data\n");
        goto TOP_MALLOC_FAIL;
    }

    g_pm_top = pm_top_data;

    service_attribute_load();

    /* init mutex */
    mutex_init(&(pm_top_data->host_mutex));

    pm_top_data->firmware_cfg_init_flag = 0;

    if (is_wifi_support()) {
        pm_top_data->wlan_pm_info = wlan_pm_init();
        if (pm_top_data->wlan_pm_info == NULL) {
            ps_print_err("no mem to allocate wlan_pm_info\n");
            goto WLAN_INIT_FAIL;
        }
    }

    if (is_gt_support()) {
        pm_top_data->gt_pm_info = gt_pm_init();
        if (pm_top_data->gt_pm_info == NULL) {
            ps_print_err("no mem to allocate gt_pm_info\n");
            goto WLAN_INIT_FAIL;
        }
    }

    ret = hw_ps_init();
    if (ret != SUCCESS) {
        ps_print_err("low_power_init: hw_ps_init fail\n");
        goto BFG_INIT_FAIL;
    }

    ps_print_info("low_power_init: success\n");
    return ret;

BFG_INIT_FAIL:
    wlan_pm_exit();
WLAN_INIT_FAIL:
    kfree(g_pm_top);
    g_pm_top = NULL;
TOP_MALLOC_FAIL:
    return -ENOMEM;
}

void low_power_exit(void)
{
    hw_ps_exit();

    wlan_pm_exit();

    kfree(g_pm_top);

    g_pm_top = NULL;

    firmware_cfg_clear();
}

/*
 * Prototype    : ps_pm_register
 * Description  : register interface for 3 in 1
 * Input        : struct ps_pm_s *new_pm: interface want to register
 * Return       : 0 means succeed,-1 means failed
 */
int32_t ps_pm_register(struct ps_core_s *ps_core_d, int index)
{
    int ret;
    struct pm_drv_data *pm_data = NULL;
    struct pm_top* pm_top_data = pm_get_top();

    if (pm_top_data == NULL) {
        return -FAILURE;
    }

    ret = low_power_probe(&pm_data, index);
    if (ret != OAL_SUCC) {
        return -FAILURE;
    }

    if (index == BUART) {
        pm_top_data->bfg_pm_info = pm_data;
    } else {
        pm_top_data->gnss_pm_info = pm_data;
    }

    ps_core_d->pm_data = pm_data;

    pm_data->ps_core_data = ps_core_d;
    pm_data->change_baud_rate = ps_change_uart_baud_rate;
    pm_data->operate_beat_timer = mod_beat_timer;
    pm_data->bfg_wake_lock = bfg_wake_lock;
    pm_data->bfg_wake_unlock = bfg_wake_unlock;
    pm_data->bfgx_dev_state_get = bfgx_dev_state_get;
    pm_data->bfgx_dev_state_set = bfgx_dev_state_set;
    pm_data->bfg_power_set = bfgx_dev_power_control;
    pm_data->bfgx_uart_state_get = bfgx_uart_state_get;
    pm_data->bfgx_uart_state_set = bfgx_uart_state_set;
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
    pm_data->download_patch = bfg_patch_download_function;
    pm_data->recv_patch = bfg_patch_recv;
    pm_data->write_patch = ps_patch_write;
#endif

    ps_print_suc("pm registered over:%d!\n", index);

    return SUCCESS;
}

/*
 * Prototype    : ps_pm_unregister
 * Description  : unregister interface for 3 in 1
 * Input        : struct ps_pm_s *new_pm: interface want to unregister
 * Return       : 0 means succeed,-1 means failed
 */
int32_t ps_pm_unregister(struct ps_core_s *ps_core_d, int index)
{
    struct pm_drv_data *pm_data = ps_core_d->pm_data;

    pm_data->change_baud_rate = NULL;
    pm_data->operate_beat_timer = NULL;
    pm_data->bfg_wake_lock = NULL;
    pm_data->bfg_wake_unlock = NULL;
    pm_data->bfgx_dev_state_get = NULL;
    pm_data->bfgx_dev_state_set = NULL;
    pm_data->bfg_power_set = NULL;
    pm_data->bfgx_uart_state_set = NULL;
    pm_data->bfgx_uart_state_get = NULL;
    pm_data->ps_core_data = NULL;
    ps_core_d->pm_data = NULL;

    low_power_remove(pm_data, index);

    ps_print_suc("pm unregistered over:%d!\n", index);

    return SUCCESS;
}


#ifdef CONFIG_HI110X_GPS_SYNC
#define GNSS_SYNC_IOREMAP_SIZE 0x1000
static struct gnss_sync_data *g_gnss_sync_data = NULL;

struct gnss_sync_data *gnss_get_sync_data(void)
{
    return g_gnss_sync_data;
}

static void gnss_set_sync_data(struct gnss_sync_data *data)
{
    g_gnss_sync_data = data;
}

static int gnss_sync_probe(struct platform_device *pdev)
{
    struct gnss_sync_data *sync_info = NULL;
    struct device_node *np = pdev->dev.of_node;
    uint32_t addr_base;
    int32_t ret;
    uint32_t version = 0;

    ps_print_info("[GPS] gnss sync probe start\n");

    ret = of_property_read_u32(np, "version", &version);
    if (ret != SUCCESS) {
        ps_print_err("[GPS] get gnss sync version failed!\n");
        return -FAILURE;
    }

    ps_print_info("[GPS] gnss sync version %d\n", version);
    if (version == 0) {
        return SUCCESS;
    }

    sync_info = kzalloc(sizeof(struct gnss_sync_data), GFP_KERNEL);
    if (sync_info == NULL) {
        ps_print_err("[GPS] alloc memory failed\n");
        return -ENOMEM;
    }

    sync_info->version = version;

    ret = of_property_read_u32(np, "addr_base", &addr_base);
    if (ret != SUCCESS) {
        ps_print_err("[GPS] get gnss sync reg base failed!\n");
        ret = -FAILURE;
        goto sync_get_resource_fail;
    }

    sync_info->addr_base_virt = ioremap(addr_base, GNSS_SYNC_IOREMAP_SIZE);
    if (sync_info->addr_base_virt == NULL) {
        ps_print_err("[GPS] gnss sync reg ioremap failed!\n");
        ret = -ENOMEM;
        goto sync_get_resource_fail;
    }

    ret = of_property_read_u32(np, "addr_offset", &sync_info->addr_offset);
    if (ret != SUCCESS) {
        ps_print_err("[GPS] get gnss sync reg offset failed!\n");
        ret = -FAILURE;
        goto sync_get_resource_fail;
    }

    ps_print_info("[GPS] gnss sync probe is finished!\n");

    gnss_set_sync_data(sync_info);

    return SUCCESS;

sync_get_resource_fail:
    gnss_set_sync_data(NULL);
    kfree(sync_info);
    return ret;
}

static void gnss_sync_shutdown(struct platform_device *pdev)
{
    struct gnss_sync_data *sync_info = gnss_get_sync_data();
    ps_print_info("[GPS] gnss sync shutdown!\n");

    if (sync_info == NULL) {
        ps_print_err("[GPS] gnss sync info is NULL.\n");
        return;
    }

    gnss_set_sync_data(NULL);
    kfree(sync_info);
    return;
}

#define DTS_COMP_GNSS_SYNC_NAME "hisilicon,hisi_gps_sync"

static const struct of_device_id g_gnss_sync_match_table[] = {
    {
        .compatible = DTS_COMP_GNSS_SYNC_NAME,  // compatible must match with which defined in dts
        .data = NULL,
    },
    {},
};

static struct platform_driver g_gnss_sync_driver = {
    .probe = gnss_sync_probe,
    .suspend = NULL,
    .remove = NULL,
    .shutdown = gnss_sync_shutdown,
    .driver = {
        .name = "hisi_gps_sync",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(g_gnss_sync_match_table),  // dts required code
    },
};

int gnss_sync_init(void)
{
    int ret;
    ret = platform_driver_register(&g_gnss_sync_driver);
    if (ret) {
        ps_print_err("[GPS] unable to register gnss sync driver!\n");
    } else {
        ps_print_info("[GPS] gnss sync init ok!\n");
    }
    return ret;
}

void gnss_sync_exit(void)
{
    platform_driver_unregister(&g_gnss_sync_driver);
}
#endif
