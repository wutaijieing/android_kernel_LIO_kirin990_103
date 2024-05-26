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

#include <linux/clk.h>
#include <of.h>
#include <osl_thread.h>
#include "deflate.h"
#include "socp_ind_delay.h"

#undef THIS_MODU
#define THIS_MODU mod_deflate

struct deflate_ctrl_info g_deflate_ctrl;
struct deflate_debug_info g_deflate_debug;
struct deflate_abort_info g_strDeflateAbort;
u32 g_deflate_enable_state = 0;

extern DRV_DEFLATE_CFG_STRU g_deflate_nv_ctrl;
#define DEFDST_TASK_PRO 81

/*
 * 函 数 名: deflate_get_data_buffer
 * 功能描述: 获取空闲缓冲区的数据
 * 输入参数:  ring_buffer       待查询的环形buffer
 *                 p_rw_buff         输出的环形buffer
 * 输出参数: 无
 * 返 回 值:  无
 */
void deflate_get_data_buffer(deflate_ring_buf_s *ring_buffer, deflate_buffer_rw_s *p_rw_buff)
{
    if (ring_buffer->read <= ring_buffer->write) {
        /* 写指针大于读指针，直接计算 */
        p_rw_buff->pBuffer = (char *)(uintptr_t)(ring_buffer->Start + (u32)ring_buffer->read);
        p_rw_buff->u32Size = (u32)(ring_buffer->write - ring_buffer->read);
        p_rw_buff->pRbBuffer = (char *)BSP_NULL;
        p_rw_buff->u32RbSize = 0;
    } else {
        /* 读指针大于写指针，需要考虑回卷 */
        p_rw_buff->pBuffer = (char *)(uintptr_t)(ring_buffer->Start + (u32)ring_buffer->read);
        p_rw_buff->u32Size = ((u32)((u64)ring_buffer->End - ((u64)ring_buffer->Start + ring_buffer->read)) + 1);
        p_rw_buff->pRbBuffer = (char *)(uintptr_t)ring_buffer->Start;
        p_rw_buff->u32RbSize = ring_buffer->write;
    }
}
/*
 * 函 数 名: deflate_read_done
 * 功能描述: 更新缓冲区的读指针
 * 输入参数:  ring_buffer       待更新的环形buffer
 *                 size          更新的数据长度
 * 输出参数: 无
 * 返 回 值:  无
 */
void deflate_read_done(deflate_ring_buf_s *ring_buffer, u32 size)
{
    ring_buffer->read += size;
    if (ring_buffer->read > (u32)(ring_buffer->End - ring_buffer->Start)) {
        ring_buffer->read -= ring_buffer->length;
    }
}
/*
 * 函 数 名: deflate_debug
 * 功能描述:读取寄存器
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void deflate_debug(void)
{
    /*lint -save -e732*/
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_GLOBALCTRL, g_strDeflateAbort.deflate_global);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_IBUFTIMEOUTCFG, g_strDeflateAbort.ibuf_timeout_config);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_RAWINT, g_strDeflateAbort.raw_int);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_INT, g_strDeflateAbort.int_state);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, g_strDeflateAbort.int_mask);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_TFRTIMEOUTCFG, g_strDeflateAbort.thf_timeout_config);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_STATE, g_strDeflateAbort.deflate_time);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_ABORTSTATERECORD, g_strDeflateAbort.abort_state_record);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEBUG_CH, g_strDeflateAbort.debug_chan);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFREMAINTHCFG, g_strDeflateAbort.obuf_thrh);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFRPTR, g_strDeflateAbort.read_addr);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFWPTR, g_strDeflateAbort.write_addr);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDST_BUFADDR_L, g_strDeflateAbort.start_low_addr);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDST_BUFADDR_H, g_strDeflateAbort.start_high_addr);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFDEPTH, g_strDeflateAbort.buff_size);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFTHRH, g_strDeflateAbort.int_thrh);
    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFOVFTIMEOUT, g_strDeflateAbort.over_timeout_en);
    DEFLATE_REG_READ(SOCP_REG_SOCP_MAX_PKG_BYTE_CFG, g_strDeflateAbort.pkg_config);
    DEFLATE_REG_READ(SOCP_REG_DEFLATE_OBUF_DEBUG, g_strDeflateAbort.obuf_debug);
    /*lint -restore +e732*/
}
/*
 * 函 数 名: deflate_set
 * 功能描述:deflate配置接口，当配置socp目的端时候调用
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 配置成功与否的标识码
 */
u32 deflate_set(u32 dst_chan_id, deflate_chan_config_s *deflate_attr)
{
    unsigned long start;
    unsigned long end;
    u32 buf_thrhold;
    u32 buf_length;
    u32 chan_id;
    u32 threshold;

    struct deflate_ctrl_info *p_chan = NULL;
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }
    g_deflate_debug.deflate_dst_set_cnt++;
    if (NULL == deflate_attr) {
        socp_error("the parameter is NULL!\n");
        return DEFLATE_ERR_NULL;
    }

    /* 判断参数有效性 */
    chan_id = DEFLATE_REAL_CHAN_ID(dst_chan_id);
    if (chan_id > DEFLATE_MAX_ENCDST_CHN) {
        socp_error("the chan id(0x%x) is invalid!\n", chan_id);
        return DEFLATE_ERR_INVALID_PARA;
    }
    start = (uintptr_t)deflate_attr->sCoderSetDstBuf.pucOutputStart;
    end = (uintptr_t)deflate_attr->sCoderSetDstBuf.pucOutputEnd;
    buf_thrhold = deflate_attr->sCoderSetDstBuf.u32Threshold;  // socp寄存器是kbyte为单位,deflate是byte为单位
    threshold = 32 * 1024;                                               // 阈值溢出,芯片建议该值在32KBytes以上

    buf_length = (u32)((end - start) + 1);

    if (0 != (buf_length % 8)) {
        socp_error("the parameter is not 8 bytes aligned!\n");
        return DEFLATE_ERR_NOT_8BYTESALIGN;
    }

    /* 如果经过配置则不能再次配置 */
    p_chan = &g_deflate_ctrl;

    if (!p_chan->set_state) {
        /* 写入起始地址到目的buffer起始地址寄存器 */
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDST_BUFADDR_L, (u32)start);
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDST_BUFADDR_H, (u32)((u64)start >> 32));
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFRPTR, 0);
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFRPTR, 0);
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFDEPTH, buf_length);
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFREMAINTHCFG, threshold);  // 阈值溢出
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFTHRH, buf_thrhold);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 22, 4, chan_id);

        /* 在g_strDeflateStat中保存参数 */
        p_chan->chan_id = dst_chan_id;
        p_chan->threshold = threshold;
        p_chan->deflate_dst_chan.Start = start;
        p_chan->deflate_dst_chan.End = end;
        p_chan->deflate_dst_chan.write = 0;
        p_chan->deflate_dst_chan.read = 0;
        p_chan->deflate_dst_chan.length = buf_length;

        /* 表明该通道已经配置 */
        p_chan->set_state = DEFLATE_CHN_SET;
    } else {
        socp_crit("Deflate set finished!\n");
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_ctrl_clear
 * 功能描述: 全局变量g_strDeflateCtrl清0
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 成功标识符
 */
u32 deflate_ctrl_clear(u32 dst_chan_id)
{
    u32 chan_id;
    chan_id = DEFLATE_REAL_CHAN_ID(dst_chan_id);
    if (chan_id > DEFLATE_MAX_ENCDST_CHN) {
        socp_error("the chan id is invalid!\n");
        return DEFLATE_ERR_INVALID_PARA;
    }
    /* deflate中断标志初始化 */
    g_deflate_ctrl.deflate_int_dst_ovf = 0;
    g_deflate_ctrl.deflate_int_dst_tfr = 0;
    g_deflate_ctrl.deflate_int_dst_thrh_ovf = 0;
    g_deflate_ctrl.deflate_int_dst_work_abort = 0;

    /* deflate目的通道属性初始化 */
    g_deflate_ctrl.set_state = 0;
    g_deflate_ctrl.threshold = 0;

    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_enable
 * 功能描述:deflate使能接口,供SOCP非压缩转压缩调用
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 成功标识符
 */
u32 deflate_enable(u32 dst_chan_id)
{
    u32 chan_id;

    chan_id = DEFLATE_REAL_CHAN_ID(dst_chan_id);
    if (chan_id > DEFLATE_MAX_ENCDST_CHN) {
        socp_error("the chan id is invalid!\n");
        return DEFLATE_ERR_INVALID_PARA;
    }
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 清中断，开中断 */
        bsp_deflate_data_send_manager(COMPRESS_ENABLE_STATE);
        /* 测试清中断，屏蔽阈值中断和上溢中断 */

        /* 使能deflate */
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 21, 1, 1);
    } else {
        socp_error("Deflate Enable failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_unable
 * 功能描述:deflate不使能接口,供SOCP压缩转非压缩场景调用
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 成功与否标识码
 */
u32 deflate_disable(u32 dst_chan_id)
{
    u32 deflate_idle_state;
    u32 chan_id;

    chan_id = DEFLATE_REAL_CHAN_ID(dst_chan_id);
    if (chan_id > DEFLATE_MAX_ENCDST_CHN) {
        socp_error("the chan id is invalid!\n");
        return DEFLATE_ERR_INVALID_PARA;
    }
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }

    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 立即压缩 */
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 20, 1, 1);

        /*lint -save -e732*/
        /* 读deflate全局状态寄存器，获取deflate工作状态 */
        DEFLATE_REG_READ(SOCP_REG_DEFLATE_GLOBALCTRL, deflate_idle_state);
        while (!(deflate_idle_state & DEFLATE_WORK_STATE)) {
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_GLOBALCTRL, deflate_idle_state);
            msleep(1);
        }
        /*lint -save -e845*/
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 21, 1, 0);
        /*lint -restore +e845*/

        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFRPTR, 0);
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFWPTR, 0);

        /* 清压缩中断状态，屏蔽中断状态 */
        bsp_deflate_data_send_manager(COMPRESS_DISABLE_STATE);
    } else {
        socp_error("Deflate Unable failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_register_read_cb
 * 功能描述: deflate 注册读回调接口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值:成功与否标识码
 */
u32 deflate_register_read_cb(deflate_read_cb read_cb)
{
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }

    /* deflate是否已经配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 设置对应通道的回调函数 */
        g_deflate_ctrl.read_cb = read_cb;
        g_deflate_debug.deflate_reg_readcb_cnt++;
    } else {
        socp_error("Deflate readCB  failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_register_event_cb
 * 功能描述:deflate 注册event回调接口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 成功与否标识码
 */
u32 deflate_register_event_cb(deflate_event_cb event_cb)
{
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }

    /* deflate是否已经配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 设置对应通道的回调函数 */
        g_deflate_ctrl.event_cb = event_cb;
        g_deflate_debug.deflate_reg_eventcb_cnt++;
    } else {
        socp_error("Deflate RegiseEventCB  failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_read_data_done
 * 功能描述: deflate read done接口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 成功与否标识码
 */
u32 deflate_read_data_done(u32 data_len)
{
    /*lint -save -e438*/
    deflate_buffer_rw_s rw_buff;
    u32 phy_addr;
    u32 curmodestate;
    unsigned long lock_flag;

    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }
    curmodestate = DEFLATE_REG_GETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 27, 1);
    if (1 == curmodestate) {
        socp_crit("deflate ind delay mode is cycle 0x%x!\n", curmodestate);
        return BSP_OK;
    }

    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        g_deflate_debug.deflate_readdone_etr_cnt++;
        if (0 == data_len) {
            g_deflate_debug.deflate_readdone_zero_cnt++;
        } else {
            g_deflate_debug.deflate_readdone_valid_cnt++;
        }
        DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFRPTR, phy_addr);
        g_deflate_ctrl.deflate_dst_chan.read = phy_addr;
        DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFWPTR, phy_addr);
        /*lint -restore +e732*/
        g_deflate_ctrl.deflate_dst_chan.write = phy_addr;
        deflate_get_data_buffer(&g_deflate_ctrl.deflate_dst_chan, &rw_buff);
        if (rw_buff.u32Size + rw_buff.u32RbSize < data_len) {
            socp_error("rw_buff is not enough, data_len=0x%x!\n", data_len);

            /*lint -save -e550 -e845*/
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
            DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 0);

            DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
            DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 1, 0);
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);

            g_deflate_debug.deflate_readdone_fail_cnt++;
            return DEFLATE_ERR_INVALID_PARA;
        }
        /* 更新读指针 */
        deflate_read_done(&g_deflate_ctrl.deflate_dst_chan, data_len);
        /* 写入读指针到读指针寄存器 */
        phy_addr = g_deflate_ctrl.deflate_dst_chan.read;
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFRPTR, phy_addr);

        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        bsp_deflate_data_send_continue();
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        /*lint -restore +e550 +e845*/
        g_deflate_debug.deflate_readdone_suc_cnt++;
    } else {
        socp_error("Readdatadone failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }

    return DEFLATE_OK;
    /*lint -restore +e438*/
}
/*
 * 函 数 名: deflate_get_read_buffer
 * 功能描述: 获取deflate数据空间
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值:成功与否标识码
 */
u32 deflate_get_read_buffer(deflate_buffer_rw_s *p_rw_buff)
{
    u32 phy_addr;
    /* 判断参数有效性 */
    if (NULL == p_rw_buff) {
        socp_error("the parameter is NULL!\n");
        return DEFLATE_ERR_NULL;
    }
    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }

    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        g_deflate_debug.deflate_get_readbuf_etr_cnt++;
        /* 根据读写指针获取buffer */
        DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFRPTR, phy_addr);
        g_deflate_ctrl.deflate_dst_chan.read = phy_addr;

        DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFWPTR, phy_addr);
        g_deflate_ctrl.deflate_dst_chan.write = phy_addr;

        deflate_get_data_buffer(&g_deflate_ctrl.deflate_dst_chan, p_rw_buff);

        g_deflate_debug.deflate_get_readbuf_suc_cnt++;
    } else {
        socp_error("deflate set failed!\n");
        return DEFLATE_ERR_SET_INVALID;
    }

    return DEFLATE_OK;
}

/*
 * 函 数 名: deflate_int_handler
 * 功能描述:deflate中断处理接口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 deflate_int_handler(void)
{
    /*lint -save -e438*/
    u32 is_handle = false;
    u32 init_state = 0;
    unsigned long lock_flag;
    u32 mask;
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        DEFLATE_REG_READ(SOCP_REG_DEFLATE_INT, init_state);
        /* 阈值传输中断 */
        if (init_state & DEFLATE_TFR_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.deflate_int_dst_tfr |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        }
        /* buffer阈值溢出 */
        else if (init_state & DEFLATE_THROVF_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.deflate_int_dst_thrh_ovf |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        }
        /* buffer上溢中断 */
        else if (init_state & DEFLATE_OVF_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.deflate_int_dst_ovf |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        }
        /* 异常中断 */
        else if (init_state & DEFLATE_WORK_ABORT_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.deflate_int_dst_work_abort |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        }
        /* 循环模式中断 */
        else if (init_state & DEFLATE_CYCLE_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.int_deflate_cycle |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        }
        /* 阻塞模式中断 */
        else if (init_state & DEFLATE_NOCYCLE_MASK) {
            spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
            DEFLATE_REG_READ(SOCP_REG_DEFLATE_INTMASK, mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_INTMASK, init_state | mask);
            DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_RAWINT, init_state);
            g_deflate_ctrl.int_deflate_no_cycle |= init_state;
            spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
            is_handle = TRUE;
        } else {
            is_handle = FALSE;
        }
    }
    if (is_handle) {
        osl_sem_up(&g_deflate_ctrl.task_sem);
    }
    /*lint -restore +e438*/
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_cycle
 * 功能描述: deflate 循环模式处理
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void deflate_cycle(void)
{
    unsigned long lock_flag;
    /* 检测通道是否配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 屏蔽别的中断 */
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0x07);
        /* 打开阻塞中断 */
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 10, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 10, 1, 0);
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        if (g_deflate_ctrl.event_cb) {
            (void)g_deflate_ctrl.event_cb(g_deflate_ctrl.chan_id, DEFLATE_EVENT_CYCLE, 0);
        }
    }
    return;
}
void deflate_nocycle(void)
{
    unsigned long lock_flag;

    /* 检测通道是否配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        /* 打开别的中断 */
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 0);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0);
        /* 打开循环模式中断 */
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 9, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 9, 1, 0);
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        if (g_deflate_ctrl.event_cb) {
            (void)g_deflate_ctrl.event_cb(g_deflate_ctrl.chan_id, DEFLATE_EVENT_NOCYCLE, 0);
        }
    }
    return;
}
void deflate_thresholdovf(void)
{
    /* 检测通道是否配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        if (g_deflate_ctrl.event_cb) {
            g_deflate_debug.deflate_task_thrh_ovf_cb_ori_cnt++;
            (void)g_deflate_ctrl.event_cb(g_deflate_ctrl.chan_id, DEFLATE_EVENT_THRESHOLD_OVERFLOW, 0);
            g_deflate_debug.deflate_task_thrh_ovf_cb_cnt++;
        }
        if (g_deflate_ctrl.read_cb)

        {
            g_deflate_debug.deflate_task_thrh_ovf_cb_ori_cnt++;
            (void)g_deflate_ctrl.read_cb(g_deflate_ctrl.chan_id);
            g_deflate_debug.deflate_task_thrh_ovf_cb_cnt++;
        }
    }

    return;
}
void deflate_ovf(void)
{
    /* 检测通道是否配置 */
    if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
        if (g_deflate_ctrl.event_cb) {
            g_deflate_debug.deflate_task_ovf_cb_ori_cnt++;
            (void)g_deflate_ctrl.event_cb(g_deflate_ctrl.chan_id, DEFLATE_EVENT_OVERFLOW, 0);
            g_deflate_debug.deflate_task_ovf_cb_cnt++;
        }
        if (g_deflate_ctrl.read_cb) {
            g_deflate_debug.deflate_task_ovf_cb_ori_cnt++;
            (void)g_deflate_ctrl.read_cb(g_deflate_ctrl.chan_id);
            g_deflate_debug.deflate_task_trf_cb_cnt++;
        }
    }
    return;
}
/*
 * 函 数 名: deflate_task
 * 功能描述: deflate 任务处理接口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
int deflate_task(void *para)
{
    u32 int_trf_state = 0;
    u32 int_ovf_state = 0;
    u32 int_thrh_ovf_state = 0;
    u32 int_cycle_state = 0;
    u32 int_no_cycle_state = 0;
    u32 int_work_abort_state = 0;
    unsigned long lock_flag;

    while (1) {
        osl_sem_down(&g_deflate_ctrl.task_sem);
        /*lint -save -e550*/
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);

        int_trf_state = g_deflate_ctrl.deflate_int_dst_tfr;
        g_deflate_ctrl.deflate_int_dst_tfr = 0;

        int_thrh_ovf_state = g_deflate_ctrl.deflate_int_dst_thrh_ovf;
        g_deflate_ctrl.deflate_int_dst_thrh_ovf = 0;

        int_ovf_state = g_deflate_ctrl.deflate_int_dst_ovf;
        g_deflate_ctrl.deflate_int_dst_ovf = 0;

        int_cycle_state = g_deflate_ctrl.int_deflate_cycle;
        g_deflate_ctrl.int_deflate_cycle = 0;

        int_no_cycle_state = g_deflate_ctrl.int_deflate_no_cycle;
        g_deflate_ctrl.int_deflate_no_cycle = 0;

        int_work_abort_state = g_deflate_ctrl.deflate_int_dst_work_abort;
        g_deflate_ctrl.deflate_int_dst_work_abort = 0;

        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        /*lint -restore +e550*/
        /* 处理传输中断 */
        if (int_trf_state) {
            /* 检测通道是否配置 */
            if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
                if (g_deflate_ctrl.read_cb) {
                    g_deflate_debug.deflate_task_trf_cb_ori_cnt++;
                    (void)g_deflate_ctrl.read_cb(g_deflate_ctrl.chan_id);
                    g_deflate_debug.deflate_task_trf_cb_cnt++;
                }
            }
        }
        /* 处理目的 buffer 阈值溢出中断 */
        if (int_thrh_ovf_state) {
            deflate_thresholdovf();
        }
        /* 处理buffer上溢中断 */
        if (int_ovf_state) {
            deflate_ovf();
        }
        if (int_cycle_state) {
            deflate_cycle();
        }
        if (int_no_cycle_state) {
            deflate_nocycle();
        }
        /* 处理异常中断 */
        if (int_work_abort_state) {
            /* 检测通道是否配置 */
            if (DEFLATE_CHN_SET == g_deflate_ctrl.set_state) {
                if (g_deflate_ctrl.event_cb) {
                    g_deflate_debug.deflate_task_int_workabort_cb_ori_cnt++;
                    (void)g_deflate_ctrl.event_cb(g_deflate_ctrl.chan_id, DEFLATE_EVENT_WORK_ABORT, 0);
                    deflate_debug();
                    g_deflate_debug.deflate_task_int_workabort_cb_cnt++;
                }
            }
        }
    }

    return 0;
}


void deflate_set_dst_threshold(bool mode)
{
    u32 buf_len;
    u32 thrh;

    DEFLATE_REG_READ(SOCP_REG_DEFLATEDEST_BUFDEPTH, buf_len);
    if (mode == true) /* true为需要打开延时上报的场景 */
    {
        thrh = (buf_len >> 2) * 3;
    } else {
        thrh = 0x1000;
    }

    DEFLATE_REG_WRITE(SOCP_REG_DEFLATEDEST_BUFTHRH, thrh);

    socp_crit(" set encdst thrh success! 0x%x\n", thrh);

    return;
}

/*
 * 函 数 名: deflate_set_cycle_mode
 * 功能描述: SOCP循环模式设置
 * 输入参数: 通道号、模式
 * 输出参数: 无
 * 返 回 值: 无
 */

s32 deflate_set_cycle_mode(u32 cycle)
{
    u32 mode_state;
    u32 waittime = 10000;
    unsigned long lock_flag;

    mode_state = DEFLATE_REG_GETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 27, 1);
    if ((0 == cycle || 1 == cycle) && mode_state) {
        u32 i;
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 26, 1, 0);
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        for (i = 0; i < waittime; i++) {
            mode_state = DEFLATE_REG_GETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 27, 1);
            if (0 == mode_state) {
                break;
            }
        }

        if (waittime == i) {
            socp_error("set encdst cycle off timeout!\n");
            return DEFLATE_ERR_GET_CYCLE;
        }
        bsp_deflate_data_send_manager(COMPRESS_ENABLE_STATE);
        return DEFLATE_OK;
    } else if ((2 == cycle) && (!mode_state)) {
        u32 i;

        bsp_deflate_data_send_manager(COMPRESS_DISABLE_STATE);
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 26, 1, 1);
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
        for (i = 0; i < waittime; i++) {
            mode_state = DEFLATE_REG_GETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 27, 1);
            if (1 == mode_state) {
                break;
            }
        }

        if (waittime == i) {
            socp_error("set encdst cycle on timeout!\n");
            return DEFLATE_ERR_GET_CYCLE;
        }
    }

    return BSP_OK;
}
/*
 * 函 数 名: deflate_set_time
 * 功能描述: DEFLATE时间设置
 * 输入参数: 通道号、模式
 * 输出参数: 无
 * 返 回 值: 无
 */

s32 deflate_set_time(u32 mode)
{
    if ((SOCP_IND_MODE_DIRECT != mode) && (SOCP_IND_MODE_DELAY != mode) && (SOCP_IND_MODE_CYCLE != mode)) {
        socp_error("mode error !\n");
        return DEFLATE_ERR_IND_MODE;
    }
    if (SOCP_IND_MODE_DIRECT == mode) {
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_TFRTIMEOUTCFG, DEFLATE_TIMEOUT_INDIRECT);
        return DEFLATE_OK;

    } else if (SOCP_IND_MODE_DELAY == mode) {
        DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_TFRTIMEOUTCFG, DEFLATE_TIMEOUT_DEFLATY);
        return DEFLATE_OK;

    } else {
        socp_crit("cycle mode need not set time !\n");
        return DEFLATE_OK;
    }
}
/*
 * 函 数 名: bsp_deflate_data_send_manager
 * 功能描述: deflate编码目的端上报数据
 * 输入参数: enc_dst_chan_id: 编码目的端通道号
 *           b_enable: 中断使能
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_deflate_data_send_manager(u32 b_enable)
{
    unsigned long lock_flag;

    if ((COMPRESS_DISABLE_STATE == b_enable) && (g_deflate_enable_state == COMPRESS_ENABLE_STATE)) {
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 1);

        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0x07);

        g_deflate_enable_state = COMPRESS_DISABLE_STATE;
        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
    } else if ((COMPRESS_ENABLE_STATE == b_enable) && (g_deflate_enable_state == COMPRESS_DISABLE_STATE)) {
        spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 0);

        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0);
        g_deflate_enable_state = COMPRESS_ENABLE_STATE;

        spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
    } else {
        socp_crit(" deflate has enabled.\n");
    }
}
/*
 * 函 数 名: bsp_deflate_data_send_continue
 * 功能描述: deflate目的端数据上报使能
 * 注    意: 该函数调用时，需要调用者保证同步
 * 输入参数:        b_enable: 中断使能
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_deflate_data_send_continue(void)
{
    if (COMPRESS_ENABLE_STATE == g_deflate_enable_state) {
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 2, 1, 1);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 2, 1, 0);

        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 3, 0x07);
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0);
    } else {
        socp_crit("deflate g_socp_enable_state = %d\n", g_deflate_enable_state);
    }
}

static void deflate_global_ctrl_init(void)
{
    spin_lock_init(&g_deflate_ctrl.int_spin_lock);
    osl_sem_init(0, &g_deflate_ctrl.task_sem);

    /* deflate中断标志初始化 */
    g_deflate_ctrl.deflate_int_dst_ovf = 0;
    g_deflate_ctrl.deflate_int_dst_tfr = 0;
    g_deflate_ctrl.deflate_int_dst_thrh_ovf = 0;
    g_deflate_ctrl.deflate_int_dst_work_abort = 0;

    /* deflate目的通道属性初始化 */
    g_deflate_ctrl.set_state = 0;
    g_deflate_ctrl.threshold = 0;
}

static void deflate_global_reset(void)
{
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 1, 1, 1); /* deflate软复位 */
}


s32 deflate_init(void)
{
    s32 ret;
    struct clk *cDeflate = NULL;
    struct device_node *dev = NULL;
    // struct socp_enc_dst_log_cfg *cDeflateConfig;
    socp_compress_ops_stru ops;
    unsigned long lock_flag;

    ret = bsp_nvm_read(NV_ID_DRV_DEFLATE, (u8 *)&g_deflate_nv_ctrl, sizeof(DRV_DEFLATE_CFG_STRU));
    if (ret) {
        g_deflate_nv_ctrl.deflate_enable = 0;
        deflate_error("deflate read nv fail!\n");
        return ret;
    }
    if (!g_deflate_nv_ctrl.deflate_enable) {
        deflate_crit("deflate_init:deflate is diabled!\n");
        return DEFLATE_OK;
    }

    deflate_crit("[init]start\n");

    if (BSP_TRUE == g_deflate_ctrl.init_flag) {
        deflate_error("deflate init already!\n");
        return DEFLATE_OK;
    }

    /* 解析dts,获取deflate基地址，并映射 */
    dev = of_find_compatible_node(NULL, NULL, "hisilicon,deflate_balong_app");
    if (NULL == dev) {
        deflate_error("deflate dev find failed!\n");
        return DEFLATE_ERROR;
    }
    g_deflate_ctrl.base_addr = (void *)of_iomap(dev, 0);
    if (NULL == g_deflate_ctrl.base_addr) {
        deflate_error("deflate base addr is error!\n");
        return DEFLATE_ERROR;
    }

    deflate_global_ctrl_init(); /* deflate控制块初始化 */

    /* 打开defalte时钟 */
    cDeflate = clk_get(NULL, "clk_socp_deflat");
    if (IS_ERR(cDeflate)) {
        deflate_error("deflate prepare clk fail!\n");
        return DEFLATE_ERROR;
    }
    clk_prepare(cDeflate);
    if (BSP_OK != clk_enable(cDeflate)) {
        deflate_error("deflate clk enable failed!\n");
        return DEFLATE_ERROR;
    }

    deflate_global_reset(); /* deflate全局软复位 */

    /* 创建deflate任务 */
    ret = osl_task_init("deflateProc", DEFDST_TASK_PRO, 4096, (OSL_TASK_FUNC)deflate_task, NULL,
                        &g_deflate_ctrl.taskid);
    if (BSP_OK != ret) {
        deflate_error("create task failed!\n");
        return DEFLATE_ERROR;
    }

    /*lint -save -e845*/
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 10, 8, 0x5); /* 5*socp编码包压缩作为触发压缩条件之一 */
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 18, 1, 0);   /* 编码包阈值条件默认不使能 */
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 0, 1, 0);    /* bypass模式不使能 */

    DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_TFRTIMEOUTCFG, DEFLATE_TIMEOUT_INDIRECT); /* 默认超时寄存器设置为立即上报=10ms */
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATEDEST_BUFOVFTIMEOUT, 31, 1, 0);           /* 设置buffer溢出不丢数 */
    DEFLATE_REG_WRITE(SOCP_REG_DEFLATE_IBUFTIMEOUTCFG, DEFLATE_TIMEOUT_INDIRECT); /* ibuf超时配置 */
    DEFLATE_REG_WRITE(SOCP_REG_SOCP_MAX_PKG_BYTE_CFG, 0x2000); /* socp最大包长设置为8k,满足socp源端数据包为4k情况下极限编码场景 */

    spin_lock_irqsave(&g_deflate_ctrl.int_spin_lock, lock_flag);
    /* 清原始中断 */
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 1, 2, 0x03);
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 4, 1, 1);
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_RAWINT, 6, 5, 0x1f);
    /* 屏蔽中断 */
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 1, 2, 0x03);
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 4, 1, 1);
    DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_INTMASK, 6, 3, 0x07);
    spin_unlock_irqrestore(&g_deflate_ctrl.int_spin_lock, lock_flag);
    /*lint -restore +e845*/

    ops.isr = deflate_int_handler;
    ops.register_event_cb = deflate_register_event_cb;
    ops.register_read_cb = deflate_register_read_cb;
    ops.enable = deflate_enable;
    ops.disable = deflate_disable;
    ops.set = deflate_set;
    ops.getbuffer = deflate_get_read_buffer;
    ops.readdone = deflate_read_data_done;
    ops.clear = deflate_ctrl_clear;
    bsp_socp_register_compress(&ops);

    /* 设置初始化状态 */
    g_deflate_ctrl.init_flag = BSP_TRUE;
    deflate_crit("[init]ok\n");
    return DEFLATE_OK;
}

s32 deflate_stop(u32 dst_chan_id)
{
    u32 real_chan_id;
    u32 chan_type;

    /* 判断是否已经初始化 */
    if (!g_deflate_ctrl.init_flag) {
        socp_error("the module has not been initialized!\n");
        return DEFLATE_ERR_NOT_INIT;
    }

    /* 判断通道ID是否有效 */
    real_chan_id = DEFLATE_REAL_CHAN_ID(dst_chan_id);
    chan_type = DEFLATE_REAL_CHAN_TYPE(dst_chan_id);

    /* 编码通道 */
    if (DEFLATE_CODER_DEST_CHAN == chan_type) {
        if (real_chan_id < DEFLATE_MAX_ENCDST_CHN) {
            if (DEFLATE_CHN_SET != g_deflate_ctrl.set_state) {
                socp_error("encoder src not allocated!\n");
                return DEFLATE_ERR_INVALID_CHAN;
            }
        } else {
            socp_error("enc dst 0x%x is valid!\n", dst_chan_id);
            return DEFLATE_ERROR;
        }

        /* 停止deflate */
        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 21, 1, 0);
        /*lint -restore +e845*/

        /* 复位通道 */

        DEFLATE_REG_SETBITS(SOCP_REG_DEFLATE_GLOBALCTRL, 1, 1, 1);
    }
    return DEFLATE_OK;
}
/*
 * 函 数 名: deflate_help
 * 功能描述:deflate 打印信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void deflate_help(void)
{
    socp_crit("deflate_show_debug_gbl: check global cnt info(src channel,config and interrupt)\n");
    socp_crit("deflate_show_dest_chan_cur: compress dst chan info\n");
}
void deflate_show_debug_gbl(void)
{
    socp_crit("\r DEFLATE global status:\n");
    socp_crit("\r g_deflate_debug.deflate_dst_set_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_dst_set_cnt);
    socp_crit("\r g_deflate_debug.deflate_dst_set_suc_cnt : 0x%x\n", (s32)g_deflate_debug.deflate_dst_set_suc_cnt);
    socp_crit("\r g_deflate_debug.deflate_reg_readcb_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_reg_readcb_cnt);
    socp_crit("\r g_deflate_debug.deflate_reg_eventcb_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_reg_eventcb_cnt);
    socp_crit("\r g_deflate_debug.deflate_get_readbuf_etr_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_get_readbuf_etr_cnt);
    socp_crit("\r g_deflate_debug.deflate_get_readbuf_suc_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_get_readbuf_suc_cnt);
    socp_crit("\r g_deflate_debug.deflate_readdone_etr_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_readdone_etr_cnt);
    socp_crit("\r g_deflate_debug.deflate_readdone_zero_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_readdone_zero_cnt);
    socp_crit("\r g_deflate_debug.deflate_readdone_valid_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_readdone_valid_cnt);
    socp_crit("\r g_deflate_debug.deflate_readdone_fail_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_readdone_fail_cnt);
    socp_crit("\r g_deflate_debug.deflate_readdone_suc_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_readdone_suc_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_trf_cb_ori_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_task_trf_cb_ori_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_trf_cb_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_task_trf_cb_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_ovf_cb_ori_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_task_ovf_cb_ori_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_ovf_cb_cnt: 0x%x\n", (s32)g_deflate_debug.deflate_task_ovf_cb_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_thrh_ovf_cb_ori_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_task_thrh_ovf_cb_ori_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_thrh_ovf_cb_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_task_thrh_ovf_cb_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_int_workabort_cb_ori_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_task_int_workabort_cb_ori_cnt);
    socp_crit("\r g_deflate_debug.deflate_task_int_workabort_cb_cnt: 0x%x\n",
              (s32)g_deflate_debug.deflate_task_int_workabort_cb_cnt);
}
u32 deflate_show_dest_chan_cur(void)
{
    socp_crit("g_deflate_ctrl.init_flag:0x%x\n", g_deflate_ctrl.init_flag);
    socp_crit("g_deflate_ctrl.chan_id:0x%x\n", g_deflate_ctrl.chan_id);
    socp_crit("g_deflate_ctrl.set_state:%d\n", g_deflate_ctrl.set_state);
    socp_crit("g_deflate_ctrl.deflate_dst_chan.read:0x%x\n", g_deflate_ctrl.deflate_dst_chan.read);
    socp_crit("g_deflate_ctrl.deflate_dst_chan.write:0x%x\n", g_deflate_ctrl.deflate_dst_chan.write);
    socp_crit("g_deflate_ctrl.deflate_dst_chan.length:0x%x\n", g_deflate_ctrl.deflate_dst_chan.length);
    return BSP_OK;
}

static struct platform_driver modem_deflate_drv = {
    .driver     = {
        .name     = "modem_deflate",
        .owner    = (struct module *)(unsigned long)THIS_MODULE,
    },
};

static struct platform_device modem_deflate_device = {
    .name = "modem_deflate",
    .id = 0,
    .dev = {
    .init_name = "modem_deflate",
    },
};

int deflate_dev_init(void)
{
    u32 ret;
    if (!g_deflate_nv_ctrl.deflate_enable) {
        socp_error("deflate is diabled!\n");
        return DEFLATE_OK;
    }
    ret = (u32)platform_device_register(&modem_deflate_device);
    if (ret) {
        socp_crit("modem_deflate_device fail !\n");
        return -1;
    }

    ret = (u32)platform_driver_register(&modem_deflate_drv);
    if (ret) {
        socp_crit("modem_deflate_drv fail !\n");
        platform_device_unregister(&modem_deflate_device);
        return -1;
    }
    return 0;
}

#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
device_initcall(deflate_dev_init);
module_init(deflate_init);
#endif
EXPORT_SYMBOL(deflate_set_dst_threshold);
EXPORT_SYMBOL(deflate_set_time);
EXPORT_SYMBOL(deflate_set_cycle_mode);
EXPORT_SYMBOL(deflate_dev_init);
EXPORT_SYMBOL(deflate_init);
