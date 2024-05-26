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

/*
 * 1 ͷ�ļ�����
 */
#include <product_config.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include "osl_sem.h"
#include "osl_spinlock.h"
#include <bsp_slice.h>
#include "bsp_dump.h"
#include "bsp_trace.h"
#include "bsp_print.h"
#include "bsp_socp.h"
#include "bsp_ipc.h"
#include "bsp_hardtimer.h"
#include <bsp_modem_log.h>
#include <securec.h>
#include "bsp_om_log.h"
#include "bsp_rfile.h"
#include "bsp_pcie.h"

#define OLD_MATCH ((u32)1)
#define NEW_MATCH ((u32)0)
/* new print start */
#define THIS_MODU mod_print
u32 g_print_close = 0;
struct bsp_syslevel_ctrl g_print_sys_level = { BSP_P_ERR, BSP_P_INFO };

struct bsp_print_tag g_print_tag[MODU_MAX] = {
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
    {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO}, {BSP_P_INFO},
};/* new print end */

#ifdef ENABLE_BUILD_OM
bsp_log_swt_cfg_s g_mod_peint_level_info[BSP_MODU_MAX] = {
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}
};
static DEFINE_SPINLOCK(g_om_log_spinlock);
#define __LOG_BUF_LEN (1 << 17)
static char __logbuffer[__LOG_BUF_LEN] = {0};
static u32 __buf_index = 0;
#define LOG_BUF_MASK (__LOG_BUF_LEN - 1)
#define LOG_BUF(idx) (__logbuffer[idx & LOG_BUF_MASK])

void bsp_log2buf(const char *buf, u32 len)
{
    unsigned long flags;
    u32 i = 0;
    char timeslice[16] = {0};
    u32 length = 0;
    length = snprintf_s(timeslice, sizeof(timeslice), sizeof(timeslice) - 1, "[%-8x]", bsp_get_slice_value());
    if (length < 0)
        printk(KERN_ERR "snprintf_s err!\n");

    spin_lock_irqsave(&g_om_log_spinlock, flags);
    length = strlen(timeslice);

    /* ��¼ʱ��� */
    for (i = 0; i < length; i++) {
        LOG_BUF(__buf_index) = *(timeslice + i);
        __buf_index++;
    }
    /* ��¼buf���� */
    length = len;
    for (i = 0; i < length; i++) {
        LOG_BUF(__buf_index) = *(buf + i);
        __buf_index++;
    }
    spin_unlock_irqrestore(&g_om_log_spinlock, flags);
    return;
}

void bsp_store_log_buf(char *buf, u32 len)
{
    u32 i = 0;
    u32 tempindex = 0;
    u32 length = 0;
    unsigned long flags;
    if ((NULL == buf) || (!len))
        return;

    if (len > __LOG_BUF_LEN)
        length = __LOG_BUF_LEN;
    else
        length = len;

    spin_lock_irqsave(&g_om_log_spinlock, flags);
    if (__buf_index < (__LOG_BUF_LEN - 1)) {
        tempindex = 0;
    } else {
        tempindex = (__buf_index + __LOG_BUF_LEN - length) & LOG_BUF_MASK;
    }
    for (i = 0; i < length; i++) {
        *(buf + i) = LOG_BUF(tempindex);
        tempindex = (tempindex + 1) & LOG_BUF_MASK;
    }
    spin_unlock_irqrestore(&g_om_log_spinlock, flags);
    return;
}
/*
 * ��������: ��ѯģ�鵱ǰ���õĴ�ӡ����
 * �������: mod_id:����ѯģ��ID
 * �������: ��
 * �� �� ֵ: ��ӡ����
 */
u32 bsp_log_module_cfg_get(bsp_module_e mod_id)
{
    if (mod_id >= BSP_MODU_MAX) {
        return BSP_ERR_LOG_INVALID_MODULE;
    }

    return g_mod_peint_level_info[mod_id].print_level;
}

/*
 * ��������: ���õ���ģ��Ĵ�ӡ����
 * �������: mod_id:ģ��ID
 *           print_level: ��ӡ����
 * �������: ��
 * �� �� ֵ: BSP_OK ���óɹ������� ����ʧ��
 */
u32 bsp_mod_level_set(bsp_module_e mod_id, u32 print_level)
{
    if (mod_id >= BSP_MODU_MAX) {
        return BSP_ERR_LOG_INVALID_MODULE;
    }

    if (print_level > BSP_LOG_LEVEL_MAX) {
        return BSP_ERR_LOG_INVALID_LEVEL;
    }

    g_mod_peint_level_info[mod_id].print_level = print_level;

    return BSP_OK;
}

/*
 * ��������: ��������ģ��Ĵ�ӡ����
 * �������: print_level: ��ӡ����
 * �������: ��
 * �� �� ֵ: BSP_OK ���óɹ������� ����ʧ��
 */
u32 bsp_log_level_set(bsp_log_level_e log_level)
{
    u32 mod_id = 0;

    if (log_level > BSP_LOG_LEVEL_MAX) {
        return BSP_ERR_LOG_INVALID_LEVEL;
    }

    for (mod_id = 0; mod_id < BSP_MODU_MAX; mod_id++) {
        g_mod_peint_level_info[mod_id].print_level = log_level;
    }

    return BSP_OK;
}

/*
 * ��������: ������ģ��Ĵ�ӡ��������ΪĬ��ֵ
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_log_level_reset(void)
{
    u32 i;

    for (i = 0; i < BSP_MODU_MAX; i++) {
        g_mod_peint_level_info[i].print_level = BSP_LOG_LEVEL_ERROR;
    }
}

/*
 * ��������: �����ӡ�������ӿ�
 * �������:  mod_id: ���ģ��
 *            log_level: ��ӡ����
 *            fmt :��ӡ�������
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_trace(bsp_log_level_e log_level, bsp_module_e mod_id, char *fmt, ...)
{
    char bsp_print_buffer[BSP_PRINT_BUF_LEN] = {
        '\0',
    };
    va_list arglist;
    /* ��ӡ������� */
    if (mod_id >= BSP_MODU_MAX) {
        return;
    }

    if (g_mod_peint_level_info[mod_id].print_level > log_level) {
        return;
    }
    /*lint -save -e530*/
    va_start(arglist, fmt);
    /*lint -restore +e530*/
#if (defined(BSP_CONFIG_PHONE_TYPE))
    (void)vsnprintf(bsp_print_buffer, (BSP_PRINT_BUF_LEN - 1), fmt, arglist); /* unsafe_function_ignore: vsnprintf */
#else
    if (vsnprintf_s(bsp_print_buffer, (BSP_PRINT_BUF_LEN - 1), (BSP_PRINT_BUF_LEN - 1), fmt, arglist) < 0) {
        va_end(arglist);
        return;
    }
#endif
    va_end(arglist);

    bsp_print_buffer[BSP_PRINT_BUF_LEN - 1] = '\0';

    printk(KERN_ERR "%s", bsp_print_buffer);
#if (FEATURE_HDS_PRINTLOG == FEATURE_ON)
    if (NULL != g_bsp_print_hook) {
        g_bsp_print_hook(mod_id, log_level, OLD_MATCH, bsp_print_buffer);
    }
#endif
    return;
}
EXPORT_SYMBOL_GPL(bsp_trace);
/* new print start */
/*
 * ��������: ��ѯģ�鵱ǰ���õĴ�ӡ����
 * �������: sel:sel = 0 close print; sel = 1 open print
 * �������: ��
 * �� �� ֵ: ��ӡ����
 */
void bsp_print_control(u32 sel)
{
    g_print_close = sel;
    return;
}
/*
 * ��������: ��ȡ����״̬
 * �������: NA
 * �������: ��
 * �� �� ֵ: ��ӡ����
 */
u32 bsp_get_print_status(void)
{
    return g_print_close;
}
/*
 * ��������: ϵͳ��������
 * �������: u32 console������̨��ӡ����u32 logbuf��default for acore
 * �������: BSP_OK/BSP_ERROR
 */
s32 logs(u32 console, u32 logbuf)
{
    if (console >= BSP_LEVEL_SUM || logbuf >= BSP_LEVEL_SUM)
        return BSP_ERROR;
    g_print_sys_level.logbuf_level = logbuf;
    g_print_sys_level.con_level = console;
    return BSP_OK;
}
/*
 * ��������: set all modules' level
 * �������: u32 level: mod_level
 * �������: NA
 */
void set_all_module(u32 level)
{
    int i = 0;
    if (level >= BSP_LEVEL_SUM)
        return;
    for (i = 0; i < MODU_MAX; i++)
        g_print_tag[i].modlevel = level;

    return;
}
/*
 * ��������: set module's level according to modid
 * �������: u32 modid: module's id, u32 level: mod_level
 * �������: BSP_OK/BSP_ERROR
 * �� �� ֵ: �ɹ�/ʧ��
 */
s32 logm(u32 modid, u32 level)
{
    if (MODU_MAX <= modid) {
        bsp_err("modid is error!\n");
        return BSP_ERROR;
    }
    if (level >= BSP_LEVEL_SUM) {
        bsp_err("level can't over 5!\n");
        return BSP_ERROR;
    }
    if (mod_all == modid) {
        set_all_module(level);
    } else
        g_print_tag[modid].modlevel = level;
    return BSP_OK;
}
/*
 * ��������: inquire module's level according to modid
 * �������: u32 modid: module's id
 * �������: NA
 */
void logc(u32 modid)
{
    if (MODU_MAX <= modid) {
        bsp_err("modid is error!\n");
        return;
    }
    bsp_err("con_level:%d logbuf_level:%d mod_level:%d\n\n", g_print_sys_level.con_level,
            g_print_sys_level.logbuf_level, g_print_tag[modid].modlevel);
    return;
}
/*
 * ��������: print
 * �������: u32 modid: module's id, BSP_LOG_LEVEL level: print level, char *fmt: string
 */
void bsp_print(module_tag_e modid, enum bsp_log_level level, char *fmt, ...)
{
    char print_buffer[BSP_PRINT_BUF_LEN] = {
        '\0',
    };
    va_list arglist;
    if (modid >= MODU_MAX || BSP_PRINT_OFF == level) {
        return;
    }

    if (g_print_tag[modid].modlevel < level) {  // ���뼶�����ģ��Ĭ�ϼ��𣬷���
        return;
    }

    va_start(arglist, fmt);
#if (defined(BSP_CONFIG_PHONE_TYPE))
    (void)vsnprintf(print_buffer, (BSP_PRINT_BUF_LEN - 1), fmt, arglist); /* unsafe_function_ignore: vsnprintf */
#else
    if (vsnprintf_s(print_buffer, (BSP_PRINT_BUF_LEN - 1), (BSP_PRINT_BUF_LEN - 1), fmt, arglist) < 0) {
        va_end(arglist);
        return;
    }
#endif
    va_end(arglist);
    print_buffer[BSP_PRINT_BUF_LEN - 1] = '\0';

    (void)bsp_log2buf(print_buffer, strlen(print_buffer) + 1);

    if (g_print_sys_level.con_level >= level)
        (void)printk(KERN_ERR "%s", print_buffer);
    else if (g_print_sys_level.logbuf_level >= level)
        (void)printk(KERN_INFO "%s", print_buffer);
    else
        return;
#if (FEATURE_HDS_PRINTLOG == FEATURE_ON)
    if (NULL != g_bsp_print_hook) {
        g_bsp_print_hook(modid, level, NEW_MATCH, print_buffer);
    }
#endif
}
EXPORT_SYMBOL_GPL(bsp_print);
EXPORT_SYMBOL_GPL(logs);
EXPORT_SYMBOL_GPL(logm);
EXPORT_SYMBOL_GPL(logc);
EXPORT_SYMBOL_GPL(bsp_print_control);
/* new print end */

/* debug �ӿ� */
void bsp_log_show(void)
{
    bsp_err("trace level = %d\n", g_mod_peint_level_info[0].print_level);
}

void log_buff_info(void)
{
    struct log_mem_stru *log = NULL;

    log = (struct log_mem_stru *)bsp_dump_get_field_addr(DUMP_LRCCPU_DMESG);
    if (log != NULL) {
        bsp_err("BUFFER MAGIC    : 0x%x\n", log->log_info.magic);
        bsp_err("READ POINTER    : 0x%x\n", log->log_info.read);
        bsp_err("WRITE POINTER   : 0x%x\n", log->log_info.write);
        bsp_err("BUFFER LENGTH   : 0x%x\n", log->log_info.size);
        bsp_err("APP STATE       : 0x%x\n", log->log_info.app_is_active);
    }
}

#elif defined(ENABLE_BUILD_PRINT)
/*
 * ��������: print
 * �������: u32 modid: module's id, BSP_LOG_LEVEL level: print level, char *fmt: string
 */
void bsp_print(module_tag_e modid, enum bsp_log_level level, char *fmt, ...)
{
    char print_buffer[BSP_PRINT_BUF_LEN] = {
        '\0',
    };
    va_list arglist;
    if (modid >= MODU_MAX || BSP_PRINT_OFF == level) {
        return;
    }

    if (g_print_tag[modid].modlevel < level) {  // ���뼶�����ģ��Ĭ�ϼ��𣬷���
        return;
    }

    va_start(arglist, fmt);
#if (defined(BSP_CONFIG_PHONE_TYPE))
    (void)vsnprintf(print_buffer, (BSP_PRINT_BUF_LEN - 1), fmt, arglist); /* unsafe_function_ignore: vsnprintf */
#else
    if (vsnprintf_s(print_buffer, (BSP_PRINT_BUF_LEN - 1), (BSP_PRINT_BUF_LEN - 1), fmt, arglist) < 0) {
        return;
    }
#endif
    va_end(arglist);
    print_buffer[BSP_PRINT_BUF_LEN - 1] = '\0';

    if (g_print_sys_level.con_level >= level)
        (void)printk(KERN_ERR "%s", print_buffer);
    else if (g_print_sys_level.logbuf_level >= level)
        (void)printk(KERN_INFO "%s", print_buffer);
    else
        return;
}
EXPORT_SYMBOL(bsp_print);
#endif
u32 bsp_print_get_len_limit(void)
{
    return BSP_PRINT_LEN_LIMIT;
}
EXPORT_SYMBOL_GPL(bsp_print_get_len_limit);
EXPORT_SYMBOL(bsp_log_module_cfg_get);
EXPORT_SYMBOL(bsp_mod_level_set);
EXPORT_SYMBOL(bsp_log_level_set);
EXPORT_SYMBOL(bsp_log_level_reset);
EXPORT_SYMBOL(bsp_log_show);
EXPORT_SYMBOL(log_buff_info);
