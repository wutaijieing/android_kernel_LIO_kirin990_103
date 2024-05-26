/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
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

/*lint --e{528,537,715} */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/rtc.h>

#include <asm/system_misc.h>

#include <product_config.h>
#include <mdrv_sysboot.h>
#include <mdrv_chg.h>
#include <bsp_icc.h>
#include <bsp_onoff.h>
#include "power_exchange.h"
#include "mdrv_chg.h"
#include <securec.h>
#include <hi_sysboot.h>
#include "power_off_mbb.h"
#include <bsp_print.h>
#define THIS_MODU mod_onoff
static unsigned int g_modem_lr_state = 0;
static unsigned int g_modem_nr_state = 0;

/*
 * ��������: ���ڻ�ȡ����ģʽ
 */
int bsp_start_mode_get(void)
{
    return DRV_START_MODE_NORMAL;
}

/*
 * ��������: C�˺˼�ͨ�ź���
 */
static void bsp_power_icc_send_state(void)
{
    int ret;
    int mode;
    u32 icc_channel_id = (ICC_CHN_IFC << 16) | IFC_RECV_FUNC_ONOFF;

    mode = bsp_start_mode_get();

    ret = bsp_icc_send(ICC_CPU_MODEM, icc_channel_id, (u8 *)&mode, (u32)sizeof(mode));
    if (ret != (int)sizeof(mode)) {
        bsp_err("send len(%x) != expected len(%lu)\n", ret, (unsigned long)sizeof(mode));
    }
}
/*
 * ��������: 5GCPU״̬��ȡ�ӿں���
 */
int mdrv_sysboot_is_nr_modem_ready(void)
{
    return g_modem_nr_state;
}
/*
 * ��������: 4GCPU״̬��ȡ�ӿں���
 */
int mdrv_sysboot_is_modem_ready(void)
{
    return g_modem_lr_state;
}

/*
 * ��������: C�˺˼�ص�����
 */
static s32 bsp_power_ctrl_read_cb(u32 channel_id, u32 len, void *context)
{
    int ret = 0;
    int read_len;
    tag_ctrl_msg_s msg;
    read_len = bsp_icc_read(channel_id, (u8 *)&msg, (u32)sizeof(tag_ctrl_msg_s));
    if (read_len != (int)sizeof(tag_ctrl_msg_s)) {
        bsp_err("read len(%x) != expected len(%lu)\n", read_len, (unsigned long)sizeof(tag_ctrl_msg_s));
        return -1;
    }

    bsp_info("[onoff]bsp_power_ctrl_read_cb is called, msg: 0x%x\n", msg.pwr_type);

    switch (msg.pwr_type) {
        case E_POWER_ON_MODE_GET:
            bsp_power_icc_send_state();
            break;
        case E_MODE_NRCPU_READY: {
            g_modem_nr_state = 1;
            bsp_err("bsp_power_ctrl_read_cb modem nrcpu ready\n");
            break;
        }
        case E_MODE_LRCPU_READY: {
            g_modem_lr_state = 1;
            bsp_err("bsp_power_ctrl_read_cb modem lrcpu ready\n");
            break;
        }
        case E_POWER_SHUT_DOWN:
            /* Here is to judge whether to shut down or restart */
            if (msg.reason == DRV_SHUTDOWN_RESET) {
                bsp_drv_power_reboot();
            } else {
                drv_shut_down(msg.reason, POWER_OFF_MONITOR_TIMEROUT);
            }
            break;
        case E_POWER_POWER_OFF:
            /* To shut down right now */
            drv_shut_down(DRV_SHUTDOWN_POWER_KEY, 0);
            break;
        case E_POWER_POWER_REBOOT:
            bsp_drv_power_reboot();
            break;
        default:
            bsp_err("invalid ctrl by ccore\n");
    }

    return ret;
}

/*
 * �� �� ��  : his_boot_probe
 * ��������  : power on
 * �������  :
 * �������  : ��
 * �� �� ֵ  :
 * ���ú���  :
 * ��������  :
 */
static int __init his_boot_probe(struct platform_device *pdev)
{
    int ret;
    ret = bsp_icc_event_register(ICC_CHN_IFC << 16 | IFC_RECV_FUNC_ONOFF, (read_cb_func)bsp_power_ctrl_read_cb, NULL,
                                 NULL, NULL);
    if (ret != 0) {
        bsp_err("icc event register failed.\n");
        return ret;
    }


    return 0;
}

static struct platform_device g_his_boot_dev = {
    .name = "his_boot",
    .id = 0,
    .dev = {
    .init_name = "his_boot",
    }, /* lint !e785 */
}; /* lint !e785 */

static struct platform_driver g_his_boot_drv = {
    .probe      = his_boot_probe,
    .driver     = {
        .name   = "his_boot",
        .owner  = THIS_MODULE, /* lint !e64 */
        .probe_type = PROBE_FORCE_SYNCHRONOUS,

    }, /* lint !e785 */
}; /* lint !e785 */

int his_boot_init(void)
{
    int ret;

    bsp_debug("his_boot_init.\n");

    ret = platform_device_register(&g_his_boot_dev);
    if (ret) {
        bsp_debug("register his_boot device failed.\n");
        return ret;
    }

    ret = platform_driver_register(&g_his_boot_drv); /*lint !e64*/
    if (ret) {
        bsp_debug("register his_boot driver failed.\n");
        platform_device_unregister(&g_his_boot_dev);
    }

    return ret;
}
#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
late_initcall(his_boot_init);
#endif
