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


#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include "osl_types.h"
#include "bsp_rfile.h"
#include "bsp_icc.h"
#include "bsp_slice.h"
#include "osl_sem.h"
#include "osl_thread.h"
#include <securec.h>

#include "socp_balong.h"

#define THIS_MODU mod_socp
#define SOCP_ROOT_PATH MODEM_LOG_ROOT "/socp/"

socp_debug_info_s g_socp_debug_info;
extern u32 g_socp_debug_trace_cfg;

struct socp_debug_icc_stru {
    char ops_cmd[32];
};

typedef void (*socp_debug_ops)(void);

struct socp_debug_proc {
    char ops_cmd[32];
    socp_debug_ops ops_func;
};

struct socp_debug_ctrl_stru {
    osl_sem_id task_sem;
    OSL_TASK_ID task_id;
};
struct socp_debug_ctrl_stru g_socp_debug_ctrl;

void socp_debug_help(void)
{
    socp_crit("help           :socp_debug_help\n");
    socp_crit("ap_count_clean :socp_debug_ap_count_clean\n");
    socp_crit("ap_count_store :socp_debug_ap_count_store\n");
    socp_crit("cp_count_store :socp_debug_cp_count_store\n");
    socp_crit("cp_count_clean :socp_debug_cp_count_clean\n");
    socp_crit("count_print    :socp_debug_print_count\n");
    socp_crit("reg_store      :socp_debug_reg_store\n");
    socp_crit("all_store      :socp_debug_all_store\n");
    socp_crit("all_clean      :socp_debug_all_clean\n");
}

/* cov_verified_start */
void socp_debug_send_cmd(char *cmd)
{
    return;
}
void socp_debug_ap_count_clean(void)
{
    return;
}

void socp_debug_count_store(char *p, int len)
{
    return;
}
void socp_debug_ap_count_store(void)
{
    char p[] = "ApCount_";
    socp_debug_count_store(p, strlen(p));
}
void socp_debug_reg_store(void)
{
    return;
}
void socp_debug_cp_count_store(void)
{
    char p[] = "CpCount_";
    socp_debug_count_store(p, (int)strlen(p));
}
void socp_debug_cp_count_clean(void)
{
    return;
}
void socp_debug_print_count(void)
{
    socp_help();
}

void socp_debug_all_store(void)
{
    socp_debug_ap_count_store();
    socp_debug_cp_count_store();
    socp_debug_reg_store();
}

void socp_debug_all_clean(void)
{
    socp_debug_ap_count_clean();
    socp_debug_cp_count_clean();
}

struct socp_debug_proc g_socp_ops[] = {
    { "help", socp_debug_help },
    { "ap_count_clean", socp_debug_ap_count_clean },
    { "ap_count_store", socp_debug_ap_count_store },
    { "reg_store", socp_debug_reg_store },
    { "cp_count_store", socp_debug_cp_count_store },
    { "cp_count_clean", socp_debug_cp_count_clean },
    { "count_print", socp_debug_print_count },
    { "all_store", socp_debug_all_store },
    { "all_clean", socp_debug_all_clean },
};

int socp_debug_icc_msg_callback(u32 chanid, u32 len, void *pdata)
{
    u32 channel_id = ICC_CHN_ACORE_CCORE_MIN << 16 | IFC_RECV_FUNC_SOCP_DEBUG;
    if (channel_id != chanid)
        return 0;

    socp_crit("enter here:\n");

    osl_sem_up(&g_socp_debug_ctrl.task_sem);
    return 0;
}
int socp_debug_icc_task(void *para)
{
    return 0;
}

/*lint -save -e745*/
int socp_debug_init(void)
{
    return 0;
}

/*
 * 函 数 名: socp_debug_set_reg_bits
 * 功能描述: 测试socp功能时调用，用于设置socp寄存器
 * 输入参数:   reg
 *             pos
 *             bits
 *             val
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_debug_set_reg_bits(u32 reg, u32 pos, u32 bits, u32 val)
{
    SOCP_REG_SETBITS(reg, pos, bits, val);
}

/*
 * 函 数 名: socp_debug_get_reg_bits
 * 功能描述: 测试socp功能时调用，用于设置socp寄存器
 * 输入参数:   reg
 *             pos
 *             bit
 *             val
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_debug_get_reg_bits(u32 reg, u32 pos, u32 bits)
{
    return SOCP_REG_GETBITS(reg, pos, bits);
}

/*
 * 函 数 名: socp_debug_write_reg
 * 功能描述: 测试socp功能时调用，用于读取寄存器值
 * 输入参数:   reg
 *             result
 * 输出参数: 无
 * 返 回 值: 寄存器值
 */
u32 socp_debug_read_reg(u32 reg)
{
    u32 result = 0;

    SOCP_REG_READ(reg, result);

    return result;
}

/*
 * 函 数 名: socp_debug_read_reg
 * 功能描述: 测试socp功能时调用，用于读取寄存器值
 * 输入参数:   reg
 *             result
 * 输出参数: 无
 * 返 回 值: 返回码
 */
u32 socp_debug_write_reg(u32 reg, u32 data)
{
    u32 result = 0;

    SOCP_REG_WRITE(reg, data);
    SOCP_REG_READ(reg, result);

    if (result == data) {
        return 0;
    }

    return (u32)-1;
}

/*
 * 函 数 名: socp_help
 * 功能描述: 获取socp打印信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_help(void)
{
    socp_crit("\r socp_show_debug_gbl\n");
    socp_crit("\r socp_show_enc_src_chan_curn");
    socp_crit("\r socp_show_enc_src_chan_add\n");
    socp_crit("\r socp_show_enc_src_chan_add\n");
    socp_crit("\r socp_show_enc_dst_chan_cur\n");
    socp_crit("\r socp_show_enc_dst_chan_add\n");
    socp_crit("\r socp_show_enc_dst_chan_all\n");
    socp_crit("\r socp_show_dec_src_chan_cur\n");
    socp_crit("\r socp_show_dec_src_chan_add\n");
    socp_crit("\r socp_show_dec_src_chan_all\n");
    socp_crit("\r socp_show_dec_dst_chan_cur\n");
    socp_crit("\r socp_show_dec_dst_chan_add\n");
    socp_crit("\r socp_show_dec_dst_chan_all\n");
    socp_crit("\r socp_show_ccore_head_err_cnt\n");
    socp_crit("\r socp_debug_cnt_show\n");
}

/*
 * 函 数 名: socp_show_debug_gbl
 * 功能描述: 显示全局debug 计数信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_show_debug_gbl(void)
{
    socp_debug_gbl_state_s *socp_debug_gbl_info = NULL;

    socp_debug_gbl_info = &g_socp_debug_info.socp_debug_gbl;

    socp_crit(" SOCP global status:\n");
    socp_crit(" g_socp_stat.base_addr): 0x%x\n", (s32)g_socp_stat.base_addr);
    socp_crit(" socp_debug_gbl_info->alloc_enc_src_cnt: 0x%x\n", (s32)socp_debug_gbl_info->alloc_enc_src_cnt);
    socp_crit(" socp_debug_gbl_info->alloc_enc_src_suc_cnt: 0x%x\n", (s32)socp_debug_gbl_info->alloc_enc_src_suc_cnt);
    socp_crit(" socp_debug_gbl_info->socp_set_enc_dst_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_set_enc_dst_cnt);
    socp_crit(" socp_debug_gbl_info->socp_set_enc_dst_suc_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_set_enc_dst_suc_cnt);
    socp_crit(" socp_debug_gbl_info->socp_set_dec_src_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_set_dec_src_cnt);
    socp_crit(" socp_debug_gbl_info->socp_set_dec_src_suc_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_set_dec_src_suc_cnt);
    socp_crit(" socp_debug_gbl_info->socp_alloc_dec_dst_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_alloc_dec_dst_cnt);
    socp_crit(" socp_debug_gbl_info->socp_alloc_dec_dst_suc_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_alloc_dec_dst_suc_cnt);
    socp_crit(" socp_debug_gbl_info->socp_app_etr_int_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_app_etr_int_cnt);
    socp_crit(" socp_debug_gbl_info->socp_app_suc_int_cnt: 0x%x\n", (s32)socp_debug_gbl_info->socp_app_suc_int_cnt);
}

/* cov_verified_stop */

/*
 * 函 数 名: socp_show_enc_src_chan_cur
 * 功能描述: 打印编码源通道当前属性
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */



u32 socp_show_enc_src_chan_cur(u32 real_chan_id)
{
    socp_crit("EncSrc channel 0x%x status:\n", real_chan_id);
    socp_crit("chan_id:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].chan_id);
    socp_crit("alloc_state:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].alloc_state);
    socp_crit("chan_en:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].chan_en);
    socp_crit("dst_chan_id:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].dst_chan_id);
    socp_crit("priority:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].priority);
    socp_crit("bypass_en:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].bypass_en);
    socp_crit("chan_mode:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].chan_mode);
    socp_crit("data_type:\t\t%d\n", g_socp_stat.enc_src_chan[real_chan_id].data_type);
    socp_crit("enc_src_buff.read:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].enc_src_buff.read);
    socp_crit("enc_src_buff.write:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].enc_src_buff.write);
    socp_crit("enc_src_buff.length:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].enc_src_buff.length);
    if (SOCP_ENCSRC_CHNMODE_LIST == g_socp_stat.enc_src_chan[real_chan_id].chan_mode) {
        socp_crit("rd_buff.read:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].rd_buff.read);
        socp_crit("rd_buff.write:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].rd_buff.write);
        socp_crit("rd_buff.length:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].rd_buff.length);
        socp_crit("rd_threshold:\t\t0x%x\n", g_socp_stat.enc_src_chan[real_chan_id].rd_threshold);
    }

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_enc_src_chan_add
 * 功能描述: 打印编码源通道累计统计值
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_enc_src_chan_add(u32 real_chan_id)
{
    socp_debug_enc_src_s *socp_add_debug_enc_src = &g_socp_debug_info.socp_debug_encsrc;

    socp_crit("EncSec channelc 0x%x cnt:\n", real_chan_id);
    socp_crit("free_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->free_enc_src_cnt[real_chan_id]);
    socp_crit("start_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->start_enc_src_cnt[real_chan_id]);
    socp_crit("stop_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->stop_enc_src_cnt[real_chan_id]);
    socp_crit("soft_reset_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->soft_reset_enc_src_cnt[real_chan_id]);
    socp_crit("reg_event_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->reg_event_enc_src_cnt[real_chan_id]);
    socp_crit("get_wbuf_enter_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->get_wbuf_enter_enc_src_cnt[real_chan_id]);
    socp_crit("get_wbuf_suc_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->get_wbuf_suc_enc_src_cnt[real_chan_id]);
    socp_crit("write_done_enter_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->write_done_enter_enc_src_cnt[real_chan_id]);
    socp_crit("write_done_suc_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->write_done_suc_enc_src_cnt[real_chan_id]);
    socp_crit("write_done_fail_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->write_done_fail_enc_src_cnt[real_chan_id]);
    socp_crit("reg_rd_cb_enc_src_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->reg_rd_cb_enc_src_cnt[real_chan_id]);
    socp_crit("get_rd_buff_enc_src_enter_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->get_rd_buff_enc_src_enter_cnt[real_chan_id]);
    socp_crit("get_rd_buff_suc_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->get_rd_buff_suc_enc_src_cnt[real_chan_id]);
    socp_crit("read_rd_done_enter_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->read_rd_done_enter_enc_src_cnt[real_chan_id]);
    socp_crit("read_rd_suc_enc_src_done_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->read_rd_suc_enc_src_done_cnt[real_chan_id]);
    socp_crit("read_rd_done_fail_enc_src_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->read_rd_done_fail_enc_src_cnt[real_chan_id]);
    socp_crit("enc_src_task_isr_head_int_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->enc_src_task_isr_head_int_cnt[real_chan_id]);
    socp_crit("enc_src_task_head_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->enc_src_task_head_cb_ori_cnt[real_chan_id]);
    socp_crit("enc_src_task_head_cb_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->enc_src_task_head_cb_cnt[real_chan_id]);
    socp_crit("enc_src_task_isr_rd_int_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->enc_src_task_isr_rd_int_cnt[real_chan_id]);
    socp_crit("enc_src_task_rd_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_src->enc_src_task_rd_cb_ori_cnt[real_chan_id]);
    socp_crit("enc_src_task_rd_cb_cnt: 0x%x\n", (s32)socp_add_debug_enc_src->enc_src_task_rd_cb_cnt[real_chan_id]);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_enc_src_chan_add
 * 功能描述: 打印所有编码源通道信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_show_enc_src_chan_all(void)
{
    u32 i;

    for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++) {
        (void)socp_show_enc_src_chan_cur(i);
        (void)socp_show_enc_src_chan_add(i);
    }

    return;
}

/*
 * 函 数 名: socp_show_enc_dst_chan_cur
 * 功能描述: 打印编码目的通道信息
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_enc_dst_chan_cur(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);

    socp_crit("EncDst channel 0x%x status:\n", unique_id);
    socp_crit("chan_id:%d\n", g_socp_stat.enc_dst_chan[real_chan_id].chan_id);
    socp_crit("set_state:%d\n", g_socp_stat.enc_dst_chan[real_chan_id].set_state);
    socp_crit("enc_dst_buff.read:0x%x\n", g_socp_stat.enc_dst_chan[real_chan_id].enc_dst_buff.read);
    socp_crit("enc_dst_buff.write:0x%x\n", g_socp_stat.enc_dst_chan[real_chan_id].enc_dst_buff.write);
    socp_crit("enc_dst_buff.length:0x%x\n", g_socp_stat.enc_dst_chan[real_chan_id].enc_dst_buff.length);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_enc_dst_chan_add
 * 功能描述: 打印编码目的通道累计统计值
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_enc_dst_chan_add(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);
    socp_debug_encdst_s *socp_add_debug_enc_dst = &g_socp_debug_info.socp_debug_enc_dst;

    socp_crit("EncDst channel 0x%x Cnt:\n", unique_id);
    socp_crit("socp_reg_event_encdst_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_reg_event_encdst_cnt[real_chan_id]);
    socp_crit("socp_reg_readcb_encdst_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_reg_readcb_encdst_cnt[real_chan_id]);
    socp_crit("socp_get_read_buff_encdst_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_get_read_buff_encdst_etr_cnt[real_chan_id]);
    socp_crit("socp_get_read_buff_encdst_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_get_read_buff_encdst_suc_cnt[real_chan_id]);
    socp_crit("socp_read_done_encdst_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_read_done_encdst_etr_cnt[real_chan_id]);
    socp_crit("socp_read_done_encdst_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_read_done_encdst_suc_cnt[real_chan_id]);
    socp_crit("socp_read_done_encdst_fail_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_read_done_encdst_fail_cnt[real_chan_id]);
    socp_crit("socp_read_done_zero_encdst_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_read_done_zero_encdst_cnt[real_chan_id]);
    socp_crit("socp_read_done_vld_encdst_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_read_done_vld_encdst_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_isr_trf_int_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_encdst_task_isr_trf_int_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_trf_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_encdst_task_trf_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_trf_cb_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_encdst_task_trf_cb_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_isr_ovf_int_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_encdst_task_isr_ovf_int_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_ovf_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_encdst_task_ovf_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_ovf_cb_cnt: 0x%x\n", (s32)socp_add_debug_enc_dst->socp_encdst_task_ovf_cb_cnt[real_chan_id]);
    socp_crit("socp_encdst_isr_thrh_ovf_int_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_encdst_isr_thrh_ovf_int_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_thrh_ovf_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_encdst_task_thrh_ovf_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_encdst_task_thrh_ovf_cb_cnt: 0x%x\n",
              (s32)socp_add_debug_enc_dst->socp_encdst_task_thrh_ovf_cb_cnt[real_chan_id]);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_enc_dst_chan_all
 * 功能描述: 打印编码目的通道信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_show_enc_dst_chan_all(void)
{
    u32 i;
    u32 unique_id = 0;

    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
        unique_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
        (void)socp_show_enc_dst_chan_cur(unique_id);
        (void)socp_show_enc_dst_chan_add(unique_id);
    }

    return;
}
#ifdef SOCP_DECODE_ENABLE
/*
 * 函 数 名: socp_show_dec_src_chan_cur
 * 功能描述: 打印解码源通道信息
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_dec_src_chan_cur(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);

    socp_crit("DecSrc channel 0x%x status:\n", unique_id);
    socp_crit("chan_id:%d\n", g_socp_stat.dec_src_chan[real_chan_id].chan_id);
    socp_crit("set_state:%d\n", g_socp_stat.dec_src_chan[real_chan_id].set_state);
    socp_crit("chan_en:%d\n", g_socp_stat.dec_src_chan[real_chan_id].chan_en);
    socp_crit("chan_mode:%d\n", g_socp_stat.dec_src_chan[real_chan_id].chan_mode);
    socp_crit("dec_src_buff.read:0x%x\n", g_socp_stat.dec_src_chan[real_chan_id].dec_src_buff.read);
    socp_crit("dec_src_buff.write:0x%x\n", g_socp_stat.dec_src_chan[real_chan_id].dec_src_buff.write);
    socp_crit("dec_src_buff.length:0x%x\n", g_socp_stat.dec_src_chan[real_chan_id].dec_src_buff.length);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_dec_src_chan_add
 * 功能描述: 打印解码源通道累计统计值
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_dec_src_chan_add(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);
    socp_debug_decsrc_s *socp_add_debug_dec_src = &g_socp_debug_info.socp_debug_dec_src;

    socp_crit("DecSrc channel 0x%x Cnt:\n", unique_id);
    socp_crit("socp_soft_reset_decsrc_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_soft_reset_decsrc_cnt[real_chan_id]);
    socp_crit("socp_soft_start_decsrc_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_soft_start_decsrc_cnt[real_chan_id]);
    socp_crit("socp_stop_start_decsrc_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_stop_start_decsrc_cnt[real_chan_id]);
    socp_crit("socp_reg_event_decsrc_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_reg_event_decsrc_cnt[real_chan_id]);
    socp_crit("socp_get_wbuf_decsrc_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_get_wbuf_decsrc_etr_cnt[real_chan_id]);
    socp_crit("socp_get_wbuf_decsrc_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_get_wbuf_decsrc_suc_cnt[real_chan_id]);
    socp_crit("socp_write_done_decsrc_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_write_done_decsrc_etr_cnt[real_chan_id]);
    socp_crit("socp_write_done_decsrc_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_write_done_decsrc_suc_cnt[real_chan_id]);
    socp_crit("socp_write_done_decsrc_fail_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_write_done_decsrc_fail_cnt[real_chan_id]);
    socp_crit("socp_decsrc_isr_err_int_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_decsrc_isr_err_int_cnt[real_chan_id]);
    socp_crit("socp_decsrc_task_err_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_src->socp_decsrc_task_err_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_decsrc_task_err_cb_cnt: 0x%x\n", (s32)socp_add_debug_dec_src->socp_decsrc_task_err_cb_cnt[real_chan_id]);
    return BSP_OK;
}

/*
 * 函 数 名: socp_show_dec_src_chan_all
 * 功能描述: 打印解码源通道信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_show_dec_src_chan_all(void)
{
    u32 i;
    u32 unique_id = 0;

    for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++) {
        unique_id = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN, i);
        (void)socp_show_dec_src_chan_cur(unique_id);
        (void)socp_show_dec_src_chan_add(unique_id);
    }

    return;
}

/*
 * 函 数 名: socp_show_dec_dst_chan_cur
 * 功能描述: 打印解码目的通道信息
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_dec_dst_chan_cur(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);

    socp_crit("DecDst channel 0x%x status:\n", unique_id);
    socp_crit("chan_id:%d\n", g_socp_stat.dec_dst_chan[real_chan_id].chan_id);
    socp_crit("alloc_state:%d\n", g_socp_stat.dec_dst_chan[real_chan_id].alloc_state);
    socp_crit("data_type:%d\n", g_socp_stat.dec_dst_chan[real_chan_id].data_type);
    socp_crit("dec_dst_buff.read:0x%x\n", g_socp_stat.dec_dst_chan[real_chan_id].dec_dst_buff.read);
    socp_crit("dec_dst_buff.write:0x%x\n", g_socp_stat.dec_dst_chan[real_chan_id].dec_dst_buff.write);
    socp_crit("dec_dst_buff.length:0x%x\n", g_socp_stat.dec_dst_chan[real_chan_id].dec_dst_buff.length);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_dec_dst_chan_add
 * 功能描述: 打印解码目的通道累计统计值
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 socp_show_dec_dst_chan_add(u32 unique_id)
{
    u32 real_chan_id = SOCP_REAL_CHAN_ID(unique_id);
    socp_debug_decdst_s *socp_add_debug_dec_dst = &g_socp_debug_info.socp_debug_dec_dst;

    socp_crit("DecDst channel 0x%x Cnt:\n", unique_id);
    socp_crit("socp_free_decdst_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_free_decdst_cnt[real_chan_id]);
    socp_crit("socp_reg_event_decdst_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_reg_event_decdst_cnt[real_chan_id]);
    socp_crit("socp_reg_readcb_decdst_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_reg_readcb_decdst_cnt[real_chan_id]);
    socp_crit("socp_get_readbuf_decdst_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_get_readbuf_decdst_etr_cnt[real_chan_id]);
    socp_crit("socp_get_readbuf_decdst_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_get_readbuf_decdst_suc_cnt[real_chan_id]);
    socp_crit("socp_read_done_decdst_etr_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_read_done_decdst_etr_cnt[real_chan_id]);
    socp_crit("socp_read_done_decdst_suc_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_read_done_decdst_suc_cnt[real_chan_id]);
    socp_crit("socp_read_done_decdst_fail_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_read_done_decdst_fail_cnt[real_chan_id]);
    socp_crit("socp_read_done_zero_decdst_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_read_done_zero_decdst_cnt[real_chan_id]);
    socp_crit("socp_read_done_vld_decdst_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_read_done_vld_decdst_cnt[real_chan_id]);
    socp_crit("socp_decdst_isr_trf_int_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_decdst_isr_trf_int_cnt[real_chan_id]);
    socp_crit("socp_decdst_task_trf_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_decdst_task_trf_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_decdst_task_trf_cb_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_decdst_task_trf_cb_cnt[real_chan_id]);
    socp_crit("socp_decdst_isr_ovf_int_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_decdst_isr_ovf_int_cnt[real_chan_id]);
    socp_crit("socp_decdst_task_ovf_cb_ori_cnt: 0x%x\n",
              (s32)socp_add_debug_dec_dst->socp_decdst_task_ovf_cb_ori_cnt[real_chan_id]);
    socp_crit("socp_decdst_task_ovf_cb_cnt: 0x%x\n", (s32)socp_add_debug_dec_dst->socp_decdst_task_ovf_cb_cnt[real_chan_id]);

    return BSP_OK;
}

/*
 * 函 数 名: socp_show_dec_dst_chan_all
 * 功能描述: 打印解码目的通道信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_show_dec_dst_chan_all(void)
{
    u32 i;
    u32 unique_id = 0;

    for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
        unique_id = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN, i);
        (void)socp_show_dec_dst_chan_cur(unique_id);
        (void)socp_show_dec_dst_chan_add(unique_id);
    }

    return;
}

EXPORT_SYMBOL(socp_show_dec_src_chan_cur);
EXPORT_SYMBOL(socp_show_dec_src_chan_add);
EXPORT_SYMBOL(socp_show_dec_src_chan_all);
EXPORT_SYMBOL(socp_show_dec_dst_chan_cur);
EXPORT_SYMBOL(socp_show_dec_dst_chan_add);
EXPORT_SYMBOL(socp_show_dec_dst_chan_all);
#endif

/*
 * 函 数 名: socp_debug_cnt_show
 * 功能描述: 显示debug 计数信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_debug_cnt_show(void)
{
    socp_show_debug_gbl();
    socp_show_enc_src_chan_all();
    socp_show_enc_dst_chan_all();
#ifdef SOCP_DECODE_ENABLE
    socp_show_dec_src_chan_all();
    socp_show_dec_dst_chan_all();
#endif
}

void socp_debug_set_trace(u32 v)
{
    g_socp_debug_trace_cfg = v;
}
#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
module_init(socp_debug_init);
#endif
EXPORT_SYMBOL(socp_debug_help);
EXPORT_SYMBOL(socp_debug_all_store);
EXPORT_SYMBOL(socp_debug_all_clean);
EXPORT_SYMBOL(socp_debug_set_reg_bits);
EXPORT_SYMBOL(socp_debug_get_reg_bits);
EXPORT_SYMBOL(socp_debug_read_reg);
EXPORT_SYMBOL(socp_debug_write_reg);
EXPORT_SYMBOL(socp_help);
EXPORT_SYMBOL(socp_show_debug_gbl);
EXPORT_SYMBOL(socp_show_enc_src_chan_cur);
EXPORT_SYMBOL(socp_show_enc_src_chan_add);
EXPORT_SYMBOL(socp_show_enc_src_chan_all);
EXPORT_SYMBOL(socp_show_enc_dst_chan_cur);
EXPORT_SYMBOL(socp_show_enc_dst_chan_add);
EXPORT_SYMBOL(socp_show_enc_dst_chan_all);
EXPORT_SYMBOL(socp_debug_cnt_show);

