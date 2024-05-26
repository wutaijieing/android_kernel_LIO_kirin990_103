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
#ifndef _DEFLATE_H
#define _DEFLATE_H
#endif

#include <product_config.h>

#include "osl_types.h"
#include "osl_sem.h"

#include "osl_spinlock.h"
#include "osl_thread.h"
#include "acore_nv_stru_drv.h"
#include "bsp_print.h"
#include "bsp_socp.h"

#ifdef DIAG_SYSTEM_5G
#include "socp_balong_3.x.h"
#else
#include "socp_balong.h"
#endif

#include "hi_deflate.h"
#include <nv_stru_drv.h>
#include <nv_id_drv.h>
#include <bsp_nvim.h>

#define DEFLATE_NULL (void *)0
#define DEFLATE_OK (0)
#define DEFLATE_ERROR (1)
#define DEFLATE_ERR_SET_FAIL (2)
#define DEFLATE_ERR_SET_INVALID (3)
#define DEFLATE_ERR_INVALID_PARA (4)
#define DEFLATE_ERR_NOT_INIT (5)
#define DEFLATE_ERR_NULL (6)
#define DEFLATE_ERR_NOT_8BYTESALIGN (7)
#define DEFLATE_ERR_INVALID_CHAN (8)
#define DEFLATE_ERR_MEM_NOT_ENOUGH (9)
#define DEFLATE_ERR_IND_MODE (10)
#define DEFLATE_ERR_GET_CYCLE (11)
#define DEFLATE_ERR_SET_CPS_MODE (12)

#define DEFLATE_CODER_DEST_CHAN (0x01)
#define DEFLATE_MAX_ENCDST_CHN (0x07)

// #define DEFLATE_TIMEOUT_DEFLATY        (0x927c0)  // 10min
#define DEFLATE_TIMEOUT_DEFLATY (0xFFFF)
#define DEFLATE_TIMEOUT_INDIRECT (0x0a)  // 10ms

#define deflate_crit socp_crit
#define deflate_error socp_error

typedef socp_ring_buff_s deflate_ring_buf_s;
struct deflate_ctrl_info {
    u32 init_flag;
    u32 chan_id;
    void *base_addr;
    u32 set_state; /* 通道已经或没有配置的标识 */
    u32 threshold;    /* 阈值 */
    socp_event_cb event_cb;
    socp_read_cb read_cb;
    osl_sem_id task_sem;
    OSL_TASK_ID taskid;
    spinlock_t int_spin_lock;
    u32 init_state;

    deflate_ring_buf_s deflate_dst_chan;
    u32 deflate_int_dst_tfr;
    u32 deflate_int_dst_thrh_ovf;
    u32 deflate_int_dst_ovf;
    u32 deflate_int_dst_work_abort;
    u32 int_deflate_cycle;
    u32 int_deflate_no_cycle;
};
extern struct deflate_ctrl_info g_deflate_ctrl;

typedef SOCP_CODER_DEST_CHAN_S deflate_chan_config_s;
typedef SOCP_BUFFER_RW_STRU deflate_buffer_rw_s;
typedef socp_read_cb deflate_read_cb;

typedef socp_event_cb deflate_event_cb;
/* DEFLATE_REG Base address of Module's Register */
enum deflate_state_e {
    DEFLATE_IDLE = 0,    /* DEFLATE处于空闲态 */
    DEFLATE_BUSY,        /* DEFLATE处忙 */
    DEFLATE_UNKNOWN_BUTT /*  未知状态 */
};
enum deflate_read_state_e {
    DEFLATE_READ_DONE = 0,    /* DEFLATE 读完成 */
    DEFLATE_READ_GO,          /* DEFLATE 读进行中 */
    DEFLATE_READ_UNKNOWN_BUTT /*  未知状态 */
};

#define SOCP_REG_DEFLATE_INFORMATION HI_SOCP_REG_DEFLATE_INFORMATION           /* deflate压缩模块版本信息 */
#define SOCP_REG_DEFLATE_GLOBALCTRL HI_SOCP_REG_DEFLATE_GLOBALCTRL             /* SOCP deflate全局控制寄存器 */
#define SOCP_REG_DEFLATE_IBUFTIMEOUTCFG HI_SOCP_REG_DEFLATE_IBUFTIMEOUTCFG     /* 压缩模块ibuf超时计数器配置 */
#define SOCP_REG_DEFLATE_RAWINT HI_SOCP_REG_DEFLATE_RAWINT                     /* 原始中断寄存器 */
#define SOCP_REG_DEFLATE_INT HI_SOCP_REG_DEFLATE_INT                           /* 中断状态寄存器 */
#define SOCP_REG_DEFLATE_INTMASK HI_SOCP_REG_DEFLATE_INTMASK                   /* 中断屏蔽寄存器 */
#define SOCP_REG_DEFLATE_TFRTIMEOUTCFG HI_SOCP_REG_DEFLATE_TFRTIMEOUTCFG       /* 压缩数据块传输超时计数器配置阈值 */
#define SOCP_REG_DEFLATE_STATE HI_SOCP_REG_DEFLATE_STATE                       /* 压缩模块操作控制 */
#define SOCP_REG_DEFLATE_ABORTSTATERECORD HI_SOCP_REG_DEFLATE_ABORTSTATERECORD /* 压缩异常状态记录 */
#define SOCP_REG_DEFLATEDEBUG_CH HI_SOCP_REG_DEFLATEDEBUG_CH                   /* 压缩模块bug通道 */
#define SOCP_REG_DEFLATEDEST_BUFREMAINTHCFG HI_SOCP_REG_DEFLATEDEST_BUFREMAINTHCFG /* 压缩通路目的buffer溢出中断阈值寄存器 */

#ifdef FEATURE_SOCP_ADDR_64BITS
#define SOCP_REG_DEFLATEDEST_BUFRPTR HI_SOCP_REG_DEFLATE_DST_BUFRPTR_OFSSET /* 压缩通路目的buffer读指针寄存器 */
#define SOCP_REG_DEFLATEDEST_BUFWPTR HI_SOCP_REG_DEFLATE_DST_BUFWPTR_OFSSET /* 压缩通路目的buffer写指针寄存器 */
#define SOCP_REG_DEFLATEDST_BUFADDR_L HI_SOCP_REG_DEFLATE_DST_BUFADDR_LOW   /* 压缩目的buffer起始地址低32位 */
#define SOCP_REG_DEFLATEDST_BUFADDR_H HI_SOCP_REG_DEFLATE_DST_BUFADDR_HIGH  /* 压缩目的buffer起始地址高32位 */
#else
#define SOCP_REG_DEFLATEDEST_BUFRPTR HI_SOCP_REG_DEFLATEDEST_BUFRPTR /* 压缩通路目的buffer读指针寄存器 */
#define SOCP_REG_DEFLATEDEST_BUFWPTR HI_SOCP_REG_DEFLATEDEST_BUFWPTR /* 压缩通路目的buffer写指针寄存器 */
#define SOCP_REG_DEFLATEDEST_BUFADDR HI_SOCP_REG_DEFLATEDEST_BUFADDR /* 压缩通路目的buffer起始地址寄存器 */
#endif
#define SOCP_REG_DEFLATEDEST_BUFDEPTH HI_SOCP_REG_DEFLATEDEST_BUFDEPTH           /* 压缩通路目的buffer深度寄存器 */
#define SOCP_REG_DEFLATEDEST_BUFTHRH HI_SOCP_REG_DEFLATEDEST_BUFTHRH             /* 传输阈值中断配置寄存器 */
#define SOCP_REG_DEFLATEDEST_BUFOVFTIMEOUT HI_SOCP_REG_DEFLATEDEST_BUFOVFTIMEOUT /* 压缩目的BUFFER溢出超时配置寄存器 */
#define SOCP_REG_SOCP_MAX_PKG_BYTE_CFG HI_SOCP_REG_SOCP_MAX_PKG_BYTE_CFG         /* socp最大包长字节阈值配置 */
#define SOCP_REG_DEFLATE_OBUF_DEBUG HI_SOCP_REG_DEFLATE_OBUF_DEBUG               /* 压缩目的buffer DEBUG */

#define SOCP_REG_DEFLATE_COM_PKG_NUM HI_SOCP_REG_DEFLATE_COM_PKG_NUM /* 压缩完成的包个数 */
#ifdef DIAG_SYSTEM_5G
#define SOCP_REG_DEFLATE_COMPELTE_TIMEOUT HI_SOCP_REG_DEFLATE_DST_COMPLETE_TIMEOUT /* 传输完成超时寄存器 */
#endif

#define DEFLATE_DRX_BACKUP_DDR_ADDR (SHM_BASE_ADDR + SHM_OFFSET_DEFLATE)
#define DEFLATE_REG_ADDR_DRX(addr) (addr + g_deflate_ctrl.base_addr)
#define DEFLATE_DRX_REG_GBLRST_NUM (18)

struct deflate_debug_info {
    u32 deflate_dst_set_cnt;               /* deflate目的buffer配置次数 */
    u32 deflate_dst_set_suc_cnt;            /* deflate目的buffer配置成功次数 */
    u32 deflate_reg_readcb_cnt;            /* 注册deflate目的通道读数据回调函数次数 */
    u32 deflate_reg_eventcb_cnt;           /* 注册deflate目的通道异常事件回调函数次数 */
    u32 deflate_get_readbuf_etr_cnt;        /* 尝试获取deflate目的buffer次数 */
    u32 deflate_get_readbuf_suc_cnt;        /* 获取deflate目的buffer成功次数 */
    u32 deflate_readdone_etr_cnt;          /* 尝试读取deflate目的数据次数 */
    u32 deflate_readdone_zero_cnt;         /* 尝试读取deflate目的数据长度等于0次数 */
    u32 deflate_readdone_valid_cnt;        /* 读取deflate目的数据长度不等于0次数 */
    u32 deflate_readdone_fail_cnt;         /* 读取deflate目的数据失败的次数 */
    u32 deflate_readdone_suc_cnt;          /* 读取deflate目的数据成功的次数 */
    u32 deflate_task_trf_cb_ori_cnt;          /* 处理传输中断任务的次数 */
    u32 deflate_task_trf_cb_cnt;             /* 处理完传输中断任务的次数 */
    u32 deflate_task_ovf_cb_ori_cnt;          /* 处理上溢中断的次数 */
    u32 deflate_task_ovf_cb_cnt;             /* 处理完上溢中断的次数 */
    u32 deflate_task_thrh_ovf_cb_ori_cnt; /* 处理阈值溢出的次数 */
    u32 deflate_task_thrh_ovf_cb_cnt;    /* 处理完阈值溢出的次数 */
    u32 deflate_task_int_workabort_cb_ori_cnt;  /* 处理异常的次数 */
    u32 deflate_task_int_workabort_cb_cnt;     /* 处理完异常的次数 */
};
struct deflate_abort_info {
    u32 deflate_global;
    u32 ibuf_timeout_config;
    u32 raw_int;
    u32 int_state;
    u32 int_mask;
    u32 thf_timeout_config;
    u32 deflate_time;
    u32 abort_state_record;
    u32 debug_chan;
    u32 obuf_thrh;
    u32 read_addr;
    u32 write_addr;
    u32 start_low_addr;
    u32 start_high_addr;
    u32 buff_size;
    u32 int_thrh;
    u32 over_timeout_en;
    u32 pkg_config;
    u32 obuf_debug;
    u32 u32Reserved;
};

enum tag_deflate_event_e {
    DEFLATE_EVENT_WORK_ABORT = 0x5,         /* 异常 */
    DEFLATE_EVENT_OVERFLOW = 0x6,           /* 目的buffer上溢 */
    DEFLATE_EVENT_THRESHOLD_OVERFLOW = 0x7, /* 目的buffer阈值溢出中断 */

    DEFLATE_EVENT_CYCLE = 0x1000,   /* 目的循环模式中断 */
    DEFLATE_EVENT_NOCYCLE = 0x1001, /* 目的阻塞模式中断 */
    DEFLATE_EVENT_BUTT
};

#define DEFLATE_CHN_SET ((s32)1) /* 通道已经配置 */
#define DEFLATE_WORK_ABORT_MASK ((s32)(1) << 2)
#define DEFLATE_THROVF_MASK ((s32)(1) << 6)
#define DEFLATE_OVF_MASK ((s32)(1) << 7)
#define DEFLATE_TFR_MASK ((s32)(1) << 8)
#define DEFLATE_CYCLE_MASK ((s32)(1) << 9)
#define DEFLATE_NOCYCLE_MASK ((s32)(1) << 10)
#define DEFLATE_WORK_STATE ((s32)(1) << 19)
#define DEFLATE_CHAN_DEF(chan_type, chan_id) ((chan_type << 16) | chan_id)
#define DEFLATE_REAL_CHAN_ID(unique_chan_id) (unique_chan_id & 0xFFFF)
#define DEFLATE_REAL_CHAN_TYPE(unique_chan_id) (unique_chan_id >> 16)

#define DEFLATE_REG_READ(reg, result) (result = readl((volatile void *)(g_deflate_ctrl.base_addr + reg)))
#define DEFLATE_REG_SETBITS(reg, pos, bits, val) BSP_REG_SETBITS(g_deflate_ctrl.base_addr, reg, pos, bits, val)
#define DEFLATE_REG_WRITE(reg, data) (writel(data, (volatile void *)(g_deflate_ctrl.base_addr + reg)))
#define DEFLATE_REG_GETBITS(reg, pos, bits) BSP_REG_GETBITS(g_deflate_ctrl.base_addr, reg, pos, bits)

s32 deflate_init(void);
u32 deflate_set(u32 dst_chan_id, deflate_chan_config_s *deflate_attr);
u32 deflate_ctrl_clear(u32 dst_chan_id);

u32 deflate_enable(u32 dst_chan_id);

u32 deflate_disable(u32 dst_chan_id);

u32 deflate_int_handler(void);

u32 deflate_read_data_done(u32 data_len);

u32 deflate_get_read_buffer(deflate_buffer_rw_s *p_rw_buff);

u32 deflate_register_read_cb(deflate_read_cb read_cb);
u32 deflate_register_event_cb(deflate_event_cb read_cb);
s32 deflate_set_time(u32 mode);
s32 deflate_set_cycle_mode(u32 cycle);
void deflate_set_dst_threshold(bool mode);
void bsp_deflate_data_send_manager(u32 b_enable);

void bsp_deflate_data_send_continue(void);
