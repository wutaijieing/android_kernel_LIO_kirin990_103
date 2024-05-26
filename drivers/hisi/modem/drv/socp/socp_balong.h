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

#ifndef _SOCP_BALONG_H
#define _SOCP_BALONG_H

#ifndef DIAG_SYSTEM_5G

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ͷ�ļ�����
 */
#include "bsp_memmap.h"
#include "soc_interrupts.h"
#include "product_config.h"
#include "hi_socp.h"
#include "bsp_socp.h"
#include "bsp_print.h"
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#define THIS_MODU mod_socp

#define SOCP_VIRT_PHY(virt) (virt)
#define SOCP_PHY_VIRT(phy) (phy)

typedef int (*socp_task_entry)(void *data);

/*
 * �Ĵ�������,ƫ�Ƶ�ַ
 */
#define SOCP_REG_GBLRST (HI_SOCP_GLOBAL_SRST_CTRL_OFFSET)
#define SOCP_REG_ENCRST (HI_SOCP_ENC_SRST_CTRL_OFFSET)
#define SOCP_REG_DECRST (HI_SOCP_DEC_SRST_CTRL_OFFSET)
#define SOCP_REG_ENCSTAT (HI_SOCP_ENC_CH_STATUS_OFFSET)
#define SOCP_REG_DECSTAT (HI_SOCP_DEC_CH_STATUS_OFFSET)
#define SOCP_REG_CLKCTRL (HI_SOCP_CLK_CTRL_OFFSET)
#define SOCP_REG_PRICFG (HI_SOCP_PRIOR_CFG_OFFSET)
#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
#define SOCP_REG_DEC_INT_TIMEOUT (HI_SOCP_DEC_INT_TIMEOUT_OFFSET)
#endif
#define SOCP_REG_INTTIMEOUT (HI_SOCP_INT_TIMEOUT_OFFSET)
#define SOCP_REG_BUFTIMEOUT (HI_SOCP_BUFFER_TIMEOUT_OFFSET)
#define SOCP_REG_DEC_PKTLEN (HI_SOCP_DEC_PKT_LEN_CFG_OFFSET)
#define SOCP_REG_GBL_INTSTAT (HI_SOCP_GLOBAL_INT_STATUS_OFFSET)

/* ������ �жϼĴ��� */
#define SOCP_REG_ENC_MASK0 (HI_SOCP_ENC_CORE0_MASK0_OFFSET)
#define SOCP_REG_ENC_RAWINT0 (HI_SOCP_ENC_CORE0_RAWINT0_OFFSET)
#define SOCP_REG_ENC_INTSTAT0 (HI_SOCP_ENC_CORE0_INT0_OFFSET)
#define SOCP_REG_APP_MASK1 (HI_SOCP_ENC_CORE0_MASK1_OFFSET)
#define SOCP_REG_MODEM_MASK1 (HI_SOCP_ENC_CORE1_MASK1_OFFSET)
#define SOCP_REG_ENC_RAWINT1 (HI_SOCP_ENC_RAWINT1_OFFSET)
#define SOCP_REG_APP_INTSTAT1 (HI_SOCP_ENC_CORE0_INT1_OFFSET)
#define SOCP_REG_MODEM_INTSTAT1 (HI_SOCP_ENC_CORE1_INT1_OFFSET)
#define SOCP_REG_ENC_MASK2 (HI_SOCP_ENC_CORE0_MASK2_OFFSET)
#define SOCP_REG_ENC_RAWINT2 (HI_SOCP_ENC_CORE0_RAWINT2_OFFSET)
#define SOCP_REG_ENC_INTSTAT2 (HI_SOCP_ENC_CORE0_INT2_OFFSET)
#define SOCP_REG_APP_MASK3 (HI_SOCP_ENC_CORE0_MASK3_OFFSET)
#define SOCP_REG_MODEM_MASK3 (HI_SOCP_ENC_CORE1_MASK3_OFFSET)
#define SOCP_REG_ENC_RAWINT3 (HI_SOCP_ENC_RAWINT3_OFFSET)
#define SOCP_REG_APP_INTSTAT3 (HI_SOCP_ENC_CORE0_INT3_OFFSET)
#define SOCP_REG_MODEM_INTSTAT3 (HI_SOCP_ENC_CORE1_INT3_OFFSET)
#define SOCP_REG_ENC_CORE1_MASK0 (HI_SOCP_ENC_CORE1_MASK0_OFFSET)
#define SOCP_REG_ENC_CORE1_INT0 (HI_SOCP_ENC_CORE1_INT0_OFFSET)
#define SOCP_REG_ENC_CORE1_MASK2 (HI_SOCP_ENC_CORE1_MASK2_OFFSET)
#define SOCP_REG_ENC_CORE1_INT2 (HI_SOCP_ENC_CORE1_INT2_OFFSET)

/* �������жϼĴ��� */
#define SOCP_REG_DEC_CORE0MASK0 (HI_SOCP_DEC_CORE0_MASK0_OFFSET)
#define SOCP_REG_DEC_CORE1MASK0 (HI_SOCP_DEC_CORE1_MASK0_OFFSET)
#define SOCP_REG_DEC_RAWINT0 (HI_SOCP_DEC_RAWINT0_OFFSET)
#define SOCP_REG_DEC_CORE0ISTAT0 (HI_SOCP_DEC_CORE0_INT0_OFFSET)
#define SOCP_REG_DEC_CORE1ISTAT0 (HI_SOCP_DEC_CORE1_INT0_OFFSET)
#define SOCP_REG_DEC_MASK1 (HI_SOCP_DEC_CORE0_MASK1_OFFSET)
#define SOCP_REG_DEC_RAWINT1 (HI_SOCP_DEC_CORE0_RAWINT1_OFFSET)
#define SOCP_REG_DEC_INTSTAT1 (HI_SOCP_DEC_CORE0_INT1_OFFSET)
#define SOCP_REG_DEC_CORE0MASK2 (HI_SOCP_DEC_CORE0_MASK2_OFFSET)
#define SOCP_REG_DEC_CORE1MASK2 (HI_SOCP_DEC_CORE1NOTE_MASK2_OFFSET)
#define SOCP_REG_DEC_RAWINT2 (HI_SOCP_DEC_RAWINT2_OFFSET)
#define SOCP_REG_DEC_CORE0ISTAT2 (HI_SOCP_DEC_CORE0NOTE_NT2_OFFSET)
#define SOCP_REG_DEC_CORE1ISTAT2 (HI_SOCP_DEC_CORE1NOTE_INT2_OFFSET)
#define SOCP_REG_DEC_CORE1_MASK1 (HI_SOCP_DEC_CORE1_MASK1_OFFSET)
#define SOCP_REG_DEC_CORE1_INT1 (HI_SOCP_DEC_CORE1_INT1_OFFSET)

/* ������ͨ��buffer�Ĵ��� */

/* ����Դͨ��buffer�Ĵ��� */
#define SOCP_REG_ENCSRC_BUFWPTR(m) (HI_SOCP_ENC_SRC_BUFM_WPTR_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_BUFRPTR(m) (HI_SOCP_ENC_SRC_BUFM_RPTR_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_BUFCFG0(m) (HI_SOCP_ENC_SRC_BUFM_DEPTH_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_BUFCFG1(m) (HI_SOCP_ENC_SRC_BUFM_CFG_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQWPTR(m) (HI_SOCP_ENC_SRC_RDQ_WPTR_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQRPTR(m) (HI_SOCP_ENC_SRC_RDQ_RPTR_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQCFG(m) (HI_SOCP_ENC_SRC_RDQ_CFG_0_OFFSET + m * 0x40)
#ifdef FEATURE_SOCP_ADDR_64BITS
#define SOCP_REG_ENCSRC_BUFADDR_L(m) (HI_SOCP_ENC_SRC_BUFM_ADDR_L_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_BUFADDR_H(m) (HI_SOCP_ENC_SRC_BUFM_ADDR_H_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQADDR_L(m) (HI_SOCP_ENC_SRC_RDQ_BADDR_L_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQADDR_H(m) (HI_SOCP_ENC_SRC_RDQ_BADDR_H_0_OFFSET + m * 0x40)
#else
#define SOCP_REG_ENCSRC_BUFADDR(m) (HI_SOCP_ENC_SRC_BUFM_ADDR_0_OFFSET + m * 0x40)
#define SOCP_REG_ENCSRC_RDQADDR(m) (HI_SOCP_ENC_SRC_RDQ_BADDR_0_OFFSET + m * 0x40)
#endif
/* ����Ŀ��ͨ��buffer�Ĵ��� */
#define SOCP_REG_ENCDEST_BUFWPTR(n) (HI_SOCP_ENC_DEST_BUFN_WPTR_0_OFFSET + (n)*0x20)
#define SOCP_REG_ENCDEST_BUFRPTR(n) (HI_SOCP_ENC_DEST_BUFN_RPTR_0_OFFSET + (n)*0x20)
#define SOCP_REG_ENCDEST_BUFCFG(n) (HI_SOCP_ENC_DEST_BUFN_DEPTH_0_OFFSET + n * 0x20)
#define SOCP_REG_ENCDEST_BUFTHRH(n) (HI_SOCP_ENC_DEST_BUFN_THRH_0_OFFSET + n * 0x20)
#define SOCP_REG_ENCDEST_BUFTHRESHOLD(n) (HI_SOCP_ENC_INT_THRESHOLD_0_OFFSET + n * 0x20)

#define SOCP_REG_ENCDEST_SBCFG(n) (HI_SOCP_ENC_DEST_SB_CFG_OFFSET + n * 0x20)

/* ע��!   ���¼Ĵ�����v204֮ǰʹ�� */
#define SOCP_REG_ENCDEST_BUFSIDEBAND(n) (HI_SOCP_ENC_DEST_BUFN_SIDBAND_OFFSET + n * 0x20)
// //////////////////////////////////////////////////////////////////////////////////////////
#ifdef FEATURE_SOCP_ADDR_64BITS
#define SOCP_REG_ENCDEST_BUFADDR_L(n) (HI_SOCP_ENC_DEST_BUFN_ADDR_L_0_OFFSET + (n)*0x20)
#define SOCP_REG_ENCDEST_BUFADDR_H(n) (HI_SOCP_ENC_DEST_BUFN_ADDR_H_0_OFFSET + (n)*0x20)
#else
#define SOCP_REG_ENCDEST_BUFADDR(n) (HI_SOCP_ENC_DEST_BUFN_ADDR_0_OFFSET + (n)*0x20)
#endif

/* ������ͨ��buffer�Ĵ��� */
/* ����Դͨ��buffer�Ĵ��� */
#define SOCP_REG_DECSRC_BUFWPTR(x) (HI_SOCP_DEC_SRC_BUFX_WPTR_0_OFFSET + x * 0x40)
#define SOCP_REG_DECSRC_BUFRPTR(x) (HI_SOCP_DEC_SRC_BUFX_RPTR_0_OFFSET + x * 0x40)
#define SOCP_REG_DECSRC_BUFCFG0(x) (HI_SOCP_DEC_SRC_BUFX_CFG0_0_OFFSET + x * 0x40)
#define SOCP_REG_DEC_BUFSTAT0(x) (HI_SOCP_DEC_BUFX_STATUS0_0_OFFSET + x * 0x40)
#define SOCP_REG_DEC_BUFSTAT1(x) (HI_SOCP_DEC_BUFX_STATUS1_0_OFFSET + x * 0x40)
/* ע��!   ���������Ĵ�����v204֮ǰʹ�� */
#define SOCP_REG_DECSRC_BUFSIDEBAND(x) (HI_SOCP_DEC_SRC_BUFX_SIDEBAND_OFFSET + x * 0x40)
#define SOCP_REG_DECDEST_BUFSIDEBAND(y) (HI_SOCP_DEC_DEST_BUFY_SIDEBAND_OFFSET + y * 0x10)
// //////////////////////////////////////////////////////////////////////////////////////////
#ifdef FEATURE_SOCP_ADDR_64BITS
#define SOCP_REG_DECSRC_BUFADDR_L(x) (HI_SOCP_DEC_SRC_BUFX_ADDR_L_0_OFFSET + x * 0x40)
#define SOCP_REG_DECSRC_BUFADDR_H(x) (HI_SOCP_DEC_SRC_BUFX_ADDR_H_0_OFFSET + x * 0x40)
#else
#define SOCP_REG_DECSRC_BUFADDR(x) (HI_SOCP_DEC_SRC_BUFX_ADDR_0_OFFSET + x * 0x40)
#endif
/* ����Ŀ��ͨ��buffer�Ĵ��� */
#ifdef FEATURE_SOCP_ADDR_64BITS
#define SOCP_REG_DECDEST_BUFWPTR(y) (HI_SOCP_DEC_DEST_BUFY_WPTR_0_OFFSET + y * 0x20)
#define SOCP_REG_DECDEST_BUFRPTR(y) (HI_SOCP_DEC_DEST_BUFY_RPTR_0_OFFSET + y * 0x20)
#define SOCP_REG_DECDEST_BUFADDR_L(y) (HI_SOCP_DEC_DEST_BUFY_ADDR_L_0_OFFSET + y * 0x20)
#define SOCP_REG_DECDEST_BUFADDR_H(y) (HI_SOCP_DEC_DEST_BUFY_ADDR_H_0_OFFSET + y * 0x20)
#define SOCP_REG_DECDEST_BUFCFG(y) (HI_SOCP_DEC_DEST_BUFY_CFG0_0_OFFSET + y * 0x20)
#else
#define SOCP_REG_DECDEST_BUFWPTR(y) (HI_SOCP_DEC_DEST_BUFY_WPTR_0_OFFSET + y * 0x10)
#define SOCP_REG_DECDEST_BUFRPTR(y) (HI_SOCP_DEC_DEST_BUFY_RPTR_0_OFFSET + y * 0x10)
#define SOCP_REG_DECDEST_BUFADDR(y) (HI_SOCP_DEC_DEST_BUFY_ADDR_0_OFFSET + y * 0x10)
#define SOCP_REG_DECDEST_BUFCFG(y) (HI_SOCP_DEC_DEST_BUFY_CFG0_0_OFFSET + y * 0x10)
#endif

/* DEBUG�Ĵ��� */
#define SOCP_REG_ENCCD_DBG0 (HI_SOCP_ENC_CD_DBG0_OFFSET)
#define SOCP_REG_ENCCD_DBG1 (HI_SOCP_ENC_CD_DBG1_OFFSET)
#define SOCP_REG_ENCIBUF_DBG0 (HI_SOCP_ENC_IBUF_DBG0_OFFSET)
#define SOCP_REG_ENCIBUF_DBG1 (HI_SOCP_ENC_IBUF_DBG1_OFFSET)
#define SOCP_REG_ENCIBUF_DBG2 (HI_SOCP_ENC_IBUF_DBG2_OFFSET)
#define SOCP_REG_ENCIBUF_DBG3 (HI_SOCP_ENC_IBUF_DBG3_OFFSET)
#define SOCP_REG_ENCOBUF_DBG0 (HI_SOCP_ENC_OBUF_DBG0_OFFSET)
#define SOCP_REG_ENCOBUF_DBG1 (HI_SOCP_ENC_OBUF_DBG1_OFFSET)
#define SOCP_REG_ENCOBUF_DBG2 (HI_SOCP_ENC_OBUF_DBG2_OFFSET)
#define SOCP_REG_ENCOBUF_DBG3 (HI_SOCP_ENC_OBUF_DBG3_OFFSET)
#define SOCP_REG_DECIBUF_DBG0 (HI_SOCP_DEC_IBUF_DBG0_OFFSET)
#define SOCP_REG_DECIBUF_DBG1 (HI_SOCP_DEC_IBUF_DBG1_OFFSET)
#define SOCP_REG_DECIBUF_DBG2 (HI_SOCP_DEC_IBUF_DBG2_OFFSET)
#define SOCP_REG_DECIBUF_DBG3 (HI_SOCP_DEC_IBUF_DBG3_OFFSET)
#define SOCP_REG_DECOBUF_DBG0 (HI_SOCP_DEC_OBUF_DBG0_OFFSET)
#define SOCP_REG_DECOBUF_DBG1 (HI_SOCP_DEC_OBUF_DBG1_OFFSET)
#define SOCP_REG_DECOBUF_DBG2 (HI_SOCP_DEC_OBUF_DBG2_OFFSET)
#define SOCP_REG_DECOBUF_DBG3 (HI_SOCP_DEC_OBUF_DBG3_OFFSET)

#define SOCP_REG_ENCOBUF_PKTNUM(x) (HI_SOCP_ENC_OBUF_0_PKT_NUM_OFFSET + x * 0x04)

/* �汾�Ĵ��� */
#define SOCP_REG_SOCP_VERSION (HI_SOCP_SOCP_VERSION_OFFSET)
#define SOCP_201_VERSION (0x201)
#define SOCP_203_VERSION (0x203) /* v203�汾SOCPʱ�ӵ�����������λ����Ϊ3.4ms */
#define SOCP_204_VERSION (0x204) /* v204�汾SOCPʱ�ӵ�����������λ����Ϊ3.4ms */
#define SOCP_205_VERSION (0x205)
#define SOCP_206_VERSION (0x206)
#define SOCP_CLK_RATIO(n) (n * 10 / 34) /* ʱ�ӵ����󣬼�����λ�Ļ���� */

/* BBP ͨ���Ĵ��� */
#define BBP_REG_LOG_ADDR(m) (0x0200 + 0x10 * m)
#define BBP_REG_LOG_WPTR(m) (0x0204 + 0x10 * m)
#define BBP_REG_LOG_RPTR(m) (0x0208 + 0x10 * m)
#define BBP_REG_LOG_CFG(m) (0x020C + 0x10 * m)
#define BBP_REG_DS_ADDR (0x0280)
#define BBP_REG_DS_WPTR (0x0284)
#define BBP_REG_DS_RPTR (0x0288)
#define BBP_REG_DS_CFG (0x028C)
#define BBP_REG_PTR_ADDR (0x0290)
#define BBP_REG_CH_EN (0x0294)
#define BBP_REG_PKT_CNT (0x0298)
#define BBP_REG_CH_STAT (0x029C)
#define BBP_REG_LOG_EN (0x02B8)

/*
 * ͨ�����ֵ����
 */
#define SOCP_TOTAL_ENCSRC_CHN (0x20)

#define BBP_MAX_CHN (0x09)
#define BBP_MAX_LOG_CHN (0x08)
#define DSP_MAX_CHN (0x02)
#define SOCP_FIXED_MAX_CHAN (BBP_MAX_CHN + DSP_MAX_CHN)
#define SOCP_MAX_ENCSRC_CHN (0x20) /* ����Դͨ�� */
#define SOCP_MAX_ENCDST_CHN (0x07)
#define SOCP_MAX_DECSRC_CHN (0x04)
#define SOCP_MAX_DECDST_CHN (0x10)
#define SOCP_CCORE_ENCSRC_CHN_BASE (4)
#define SOCP_CCORE_ENCSRC_CHN_NUM (5)

/* LTE DSP/BBPͨ�� */
#define SOCP_DSPLOG_CHN (0x0e)                 // 14
#define SOCP_BBPLOG_CHN (SOCP_DSPLOG_CHN + 2)  // 16��ʼ����8��
#define SOCP_BBPDS_CHN (SOCP_BBPLOG_CHN + 8)   // 24
#define SOCP_FIXCHN_BASE (SOCP_DSPLOG_CHN)
#define SOCP_FIXCHN_END (SOCP_BBPDS_CHN)

/* ��ַ��ȷ�� */
#define BBP_REG_ARM_BASEADDR HI_BBP_DMA_BASE_ADDR_VIRT
#define TENSILICA_CORE0_DRAM_ADDR (0x49f80000) /* DSP��ַ��ȷ�� */
#define SOCP_REG_BASEADDR (g_socp_stat.base_addr)

/* SOCP BBP����ʹ�õ�Ԥ���ڴ�ռ�궨�� */
#define PBXA9_DRAM_BBPDS_VIRT (IO_ADDRESS(DDR_SOCP_ADDR)) /* �õ�ַ��Ҫȷ�� */
#define PBXA9_DRAM_BBPDS_PHYS (DDR_SOCP_ADDR)             /* �õ�ַ��Ҫȷ�� */
#define PBXA9_DRAM_BBPDS_SIZE (DDR_SOCP_SIZE)             /* �ÿռ��С��Ҫȷ�� */

#define INT_LVL_SOCP INT_LVL_SOCP0

/* ϵͳ������ */
#define SOCP_BBPDS_CHN_ADDR (PBXA9_DRAM_BBPDS_PHYS)
#define SOCP_BBPDS_CHN_SIZE (PBXA9_DRAM_BBPDS_SIZE)

#define SOCP_BBPLOG_CHN_SIZE (0x2000)

#define SOCP_BBPLOG0_CHN_SIZE (0x2000 * 8)
#define SOCP_FIXEDID_BASE (SOCP_DSPLOG_CHN - SOCP_FIXCHN_BASE)

#define SOCP_DSPLOG_DST_BUFID ((s32)1)
#define SOCP_BBPLOG_DST_BUFID ((s32)1)
#define SOCP_BBPDS_DST_BUFID ((s32)1)

// #define SOCP_CODER_DST_TIMEOUT_DEFAULT 0
// #define SOCP_CODER_DST_TIMEOUT_LONG    1
// #define SOCP_CODER_DST_TIMEOUT_SHORT   2

/* log2.0 2014-03-19 Begin: */
/* SOCP�������������������ӳ�ʱ�䣬10msΪ��λ */
#define SOCP_LOG_FLUSH_MAX_OVER_TIME 10
/* log2.0 2014-03-19 End: */
/*
 * �ṹ����
 */

/* ͨ��״̬�ṹ�壬������ */
typedef struct  {
    unsigned long Start;
    unsigned long End;
    u32 write;
    u32 read;
    u32 length;
    u32 idle_size;
} socp_ring_buff_s;

typedef struct  {
    u32 chan_id;
    u32 chan_en;
    u32 dst_chan_id;
    u32 bypass_en;
    u32 alloc_state; /* ͨ���Ѿ���û�з���ı�ʶ */
    u32 last_rd_size;
    u32 rd_threshold;
    SOCP_ENCSRC_CHNMODE_ENUM_UIN32 chan_mode; /* ���ݽṹ���� */
    SOCP_CHAN_PRIORITY_ENUM_UIN32 priority;
    SOCP_DATA_TYPE_ENUM_UIN32 data_type;
    SOCP_DATA_TYPE_EN_ENUM_UIN32 data_type_en;
    SOCP_ENC_DEBUG_EN_ENUM_UIN32 debug_en;
    socp_ring_buff_s enc_src_buff;
    socp_ring_buff_s rd_buff;
    socp_event_cb event_cb;
    socp_rd_cb rd_cb;
} socp_enc_src_chan_s;

typedef struct socp_compress {
    int bcompress;
    socp_compress_ops_stru ops;
} socp_compress_stru;

typedef struct  {
    u32 chan_id;
    u32 set_state; /* ͨ���Ѿ���û�����õı�ʶ */
    u32 threshold;    /* ��ֵ */
    socp_compress_stru struCompress;
    socp_ring_buff_s enc_dst_buff;
    SOCP_EVENT_ENUM_UIN32 chan_event;
    socp_event_cb event_cb;
    socp_read_cb read_cb;
    u32 buf_thrhold;
} socp_encdst_chan_s;

typedef struct  {
    u32 chan_id;
    u32 chan_en;
    u32 set_state; /* ͨ���Ѿ���û�����õı�ʶ */
    u32 rd_threshold;
    SOCP_DATA_TYPE_EN_ENUM_UIN32 data_type_en;
    SOCP_DECSRC_CHNMODE_ENUM_UIN32 chan_mode; /* ���ݽṹ���� */
    socp_ring_buff_s dec_src_buff;
    socp_ring_buff_s dec_rd_buff;
    socp_event_cb event_cb;
    socp_rd_cb rd_cb;
} socp_decsrc_chan_s;

typedef struct  {
    u32 chan_id;
    u32 alloc_state;
    SOCP_DATA_TYPE_ENUM_UIN32 data_type;
    socp_ring_buff_s dec_dst_buff;
    socp_event_cb event_cb;
    socp_read_cb read_cb;
} socp_decdst_chan_s;

/* ȫ��״̬�ṹ�� */
typedef struct  {
    s32 init_flag;
    unsigned long base_addr;
    unsigned long armBaseAddr;
    unsigned long tensiAddr;
    unsigned long bbpDsAddr;
    struct semaphore u32EncSrcSemID;
    struct semaphore u32EncDstSemID;
    struct semaphore dec_src_sem_id;
    struct semaphore dec_dst_sem_id;
    unsigned long u32EncSrcTskID;
    unsigned long u32EncDstTskID;
    unsigned long dec_src_task_id;
    unsigned long dec_dst_task_id;
    u32 int_enc_src_header;
    u32 u32IntEncSrcRD;
    u32 int_enc_dst_tfr;
    u32 int_enc_dst_ovf;
    u32 int_enc_dst_thrh_ovf;
    u32 int_dec_src_err;
    u32 int_dec_dst_trf;
    u32 int_dec_dst_ovf;
    socp_compress_isr compress_isr;
    socp_enc_src_chan_s enc_src_chan[SOCP_MAX_ENCSRC_CHN];
    socp_encdst_chan_s enc_dst_chan[SOCP_MAX_ENCDST_CHN];
    socp_decsrc_chan_s dec_src_chan[SOCP_MAX_DECSRC_CHN];
    socp_decdst_chan_s dec_dst_chan[SOCP_MAX_DECDST_CHN];
} socp_gbl_state_s;

/* ����ѹ��ö�� */
enum socp_enc_dst_output_compress_e {
    SOCP_NO_COMPRESS = 0,
    SOCP_COMPRESS,
};

/* ѹ��ʹ��ö�� */
enum socp_compress_enable_e {
    SOCP_COMPRESS_DISABLE = 0,
    SOCP_COMPRESS_ENBALE,

};
typedef unsigned int socp_compress_enable_e;

typedef struct  {
    u32 alloc_enc_src_cnt;    /* SOCP�������Դͨ���Ĵ��� */
    u32 alloc_enc_src_suc_cnt; /* SOCP�������Դͨ���ɹ��Ĵ��� */
    u32 socp_set_enc_dst_cnt;      /* SOCP���ñ���Ŀ��ͨ���Ĵ��� */
    u32 socp_set_enc_dst_suc_cnt;   /* SOCP���ñ���Ŀ��ͨ���ɹ��Ĵ��� */
    u32 socp_set_dec_src_cnt;      /* SOCP���ý���Դͨ���Ĵ��� */
    u32 socp_set_dec_src_suc_cnt;    /* SOCP���ý���Դͨ���ɹ��Ĵ��� */
    u32 socp_alloc_dec_dst_cnt;    /* SOCP�������Ŀ��ͨ���Ĵ��� */
    u32 socp_alloc_dec_dst_suc_cnt; /* SOCP�������Ŀ��ͨ���ɹ��Ĵ��� */
    u32 socp_app_etr_int_cnt;      /* ����APP�жϵĴ��� */
    u32 socp_app_suc_int_cnt;      /* ����APP�жϴ���ɹ��Ĵ��� */
} socp_debug_gbl_state_s;

typedef struct  {
    u32 free_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];            /* SOCP�ͷű���Դͨ���ɹ��Ĵ��� */
    u32 soft_reset_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];       /* SOCP��λ����Դͨ���Ĵ��� */
    u32 start_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];           /* SOCP��������Դͨ���Ĵ��� */
    u32 stop_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];            /* SOCP�رձ���Դͨ���Ĵ��� */
    u32 reg_event_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];        /* SOCPע�����Դͨ���¼��Ĵ��� */
    u32 get_wbuf_enter_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];      /* SOCP����Դͨ�����Ի��дbuffer�Ĵ��� */
    u32 get_wbuf_suc_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];      /* SOCP����Դͨ���ɹ����дbuffer�Ĵ��� */
    u32 write_done_enter_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];  /* SOCP����Դͨ������д��buffer�Ĵ��� */
    u32 write_done_suc_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];  /* SOCP����Դͨ���ɹ�д��buffer�Ĵ��� */
    u32 write_done_fail_enc_src_cnt[SOCP_MAX_ENCSRC_CHN]; /* SOCP����Դͨ��д��bufferʧ�ܵĴ��� */
    u32 reg_rd_cb_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];         /* SOCPע�����Դͨ��RDbuffer�ص������Ĵ��� */
    u32 get_rd_buff_enc_src_enter_cnt[SOCP_MAX_ENCSRC_CHN];     /* SOCP����Դͨ�����Ի��Rdbuffer�Ĵ��� */
    u32 get_rd_buff_suc_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];     /* SOCP����Դͨ���ɹ����Rdbuffer�Ĵ��� */
    u32 read_rd_done_enter_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];   /* SOCP����Դͨ������д��Rdbuffer�Ĵ��� */
    u32 read_rd_suc_enc_src_done_cnt[SOCP_MAX_ENCSRC_CHN];   /* SOCP����Դͨ���ɹ�д��Rdbuffer�Ĵ��� */
    u32 read_rd_done_fail_enc_src_cnt[SOCP_MAX_ENCSRC_CHN];  /* SOCP����Դͨ��д��Rdbufferʧ�ܵĴ��� */
    u32 enc_src_task_head_cb_cnt[SOCP_MAX_ENCSRC_CHN];       /* �����лص�����Դͨ����ͷ�����жϴ�������ɵĴ��� */
    u32 enc_src_task_head_cb_ori_cnt[SOCP_MAX_ENCSRC_CHN];    /* �����лص�����Դͨ����ͷ�����жϴ��������� */
    u32 enc_src_task_rd_cb_cnt[SOCP_MAX_ENCSRC_CHN];         /* �����лص�����Դͨ��Rd ����жϴ�������ɵĴ��� */
    u32 enc_src_task_rd_cb_ori_cnt[SOCP_MAX_ENCSRC_CHN];      /* �����лص�����Դͨ��Rd ����жϴ��������� */
    u32 enc_src_task_isr_head_int_cnt[SOCP_MAX_ENCSRC_CHN];      /* ISR�н������Դͨ����ͷ�����жϴ��� */
    u32 enc_src_task_isr_rd_int_cnt[SOCP_MAX_ENCSRC_CHN];        /* ISR�������Դͨ��Rd ����жϴ��� */
} socp_debug_enc_src_s;

typedef struct  {
    u32 socp_reg_event_encdst_cnt[SOCP_MAX_ENCDST_CHN];        /* SOCPע�����Ŀ��ͨ���¼��Ĵ��� */
    u32 socp_reg_readcb_encdst_cnt[SOCP_MAX_ENCDST_CHN];       /* SOCPע�����Ŀ��ͨ�������ݻص������Ĵ��� */
    u32 socp_get_read_buff_encdst_etr_cnt[SOCP_MAX_ENCDST_CHN];   /* SOCP����Ŀ��ͨ�����Զ����ݵĴ��� */
    u32 socp_get_read_buff_encdst_suc_cnt[SOCP_MAX_ENCDST_CHN];   /* SOCP����Ŀ��ͨ���ɹ������ݵĴ��� */
    u32 socp_read_done_encdst_etr_cnt[SOCP_MAX_ENCDST_CHN];   /* SOCP����Ŀ��ͨ������д��Ŀ��buffer�Ĵ��� */
    u32 socp_read_done_encdst_suc_cnt[SOCP_MAX_ENCDST_CHN];   /* SOCP����Ŀ��ͨ��д��Ŀ��buffer�ɹ��Ĵ��� */
    u32 socp_read_done_encdst_fail_cnt[SOCP_MAX_ENCDST_CHN];  /* SOCP����Ŀ��ͨ��д��Ŀ��bufferʧ�ܵĴ��� */
    u32 socp_read_done_zero_encdst_cnt[SOCP_MAX_ENCDST_CHN];  /* SOCP����Ŀ��ͨ��д��Ŀ��buffer size ����0 �Ĵ��� */
    u32 socp_read_done_vld_encdst_cnt[SOCP_MAX_ENCDST_CHN]; /* SOCP����Ŀ��ͨ��д��Ŀ��buffer size ������0 �Ĵ��� */
    u32 socp_encdst_task_trf_cb_cnt[SOCP_MAX_ENCDST_CHN];        /* �����лص�����Ŀ��ͨ����������жϴ�������ɵĴ��� */
    u32 socp_encdst_task_trf_cb_ori_cnt[SOCP_MAX_ENCDST_CHN];     /* �����лص�����Ŀ��ͨ����������жϴ������Ĵ��� */
    u32 socp_encdst_task_ovf_cb_cnt[SOCP_MAX_ENCDST_CHN];        /* �����лص�����Ŀ��ͨ��buf ����жϴ�������ɵĴ��� */
    u32 socp_encdst_task_ovf_cb_ori_cnt[SOCP_MAX_ENCDST_CHN];     /* �����лص�����Ŀ��ͨ��buf ����жϴ��������� */
    u32 socp_encdst_task_isr_trf_int_cnt[SOCP_MAX_ENCDST_CHN];       /* ISR�н������Ŀ��ͨ����������жϴ��� */
    u32 socp_encdst_task_isr_ovf_int_cnt[SOCP_MAX_ENCDST_CHN];       /* ISR�н������Ŀ��ͨ��buf ����жϴ��� */
    u32 socp_encdst_task_thrh_ovf_cb_cnt[SOCP_MAX_ENCDST_CHN]; /* �����лص�����Ŀ��ͨ��buf��ֵ����жϴ�������ɵĴ��� */
    u32 socp_encdst_task_thrh_ovf_cb_ori_cnt[SOCP_MAX_ENCDST_CHN]; /* �����лص�����Ŀ��ͨ��buf��ֵ����жϴ��������� */
    u32 socp_encdst_isr_thrh_ovf_int_cnt[SOCP_MAX_ENCDST_CHN];   /* ISR�н������Ŀ��ͨ��buf��ֵ����жϴ��� */

    /* log2.0 2014-03-19 Begin: */
    BSP_U32 socp_encdst_soft_overtime_ori_cnt[SOCP_MAX_ENCDST_CHN]; /* SOCP����Ŀ��ͨ�������ʱ������� */
    BSP_U32 socp_encdst_soft_overtime_cnt[SOCP_MAX_ENCDST_CHN];    /* SOCP����Ŀ��ͨ�������ʱ������ɴ��� */
                                                                  /* log2.0 2014-03-19 End: */
    u32 socp_encdst_mode_change_fail_cnt[SOCP_MAX_ENCDST_CHN];      /* SOCP����Ŀ��ͨ��ģʽ�л�ʧ�ܼ��� */
} socp_debug_encdst_s;

typedef struct  {
    u32 socp_soft_reset_decsrc_cnt[SOCP_MAX_DECSRC_CHN];       /* SOCP��λ����Դͨ���Ĵ��� */
    u32 socp_soft_start_decsrc_cnt[SOCP_MAX_DECSRC_CHN];           /* SOCP��������Դͨ���Ĵ��� */
    u32 socp_stop_start_decsrc_cnt[SOCP_MAX_DECSRC_CHN];            /* SOCP�رս���Դͨ���Ĵ��� */
    u32 socp_reg_event_decsrc_cnt[SOCP_MAX_DECSRC_CHN];        /* SOCPע�����Դͨ���¼��Ĵ��� */
    u32 socp_get_wbuf_decsrc_etr_cnt[SOCP_MAX_DECSRC_CHN];      /* SOCP����Դͨ�����Ի��дbuffer�Ĵ��� */
    u32 socp_get_wbuf_decsrc_suc_cnt[SOCP_MAX_DECSRC_CHN];      /* SOCP����Դͨ�����дbuffer�ɹ��Ĵ��� */
    u32 socp_write_done_decsrc_etr_cnt[SOCP_MAX_DECSRC_CHN];  /* SOCP����Դͨ������д��buffer�Ĵ��� */
    u32 socp_write_done_decsrc_suc_cnt[SOCP_MAX_DECSRC_CHN];  /* SOCP����Դͨ��д��buffer�ɹ��Ĵ��� */
    u32 socp_write_done_decsrc_fail_cnt[SOCP_MAX_DECSRC_CHN]; /* SOCP����Դͨ��д��bufferʧ�ܵĴ��� */
    u32 socp_reg_rdcb_decsrc_cnt[SOCP_MAX_DECSRC_CHN];         /* SOCPע�����Դͨ��RDbuffer�ص������Ĵ��� */
    u32 socp_get_rdbuf_decsrc_etr_cnt[SOCP_MAX_DECSRC_CHN];     /* SOCP����Դͨ�����Ի��Rdbuffer�Ĵ��� */
    u32 socp_get_rdbuf_decsrc_suc_cnt[SOCP_MAX_DECSRC_CHN];     /* SOCP����Դͨ�����Rdbuffer�ɹ��Ĵ��� */
    u32 socp_read_rd_done_decsrc_etr_cnt[SOCP_MAX_DECSRC_CHN];   /* SOCP����Դͨ������д��Rdbuffer�Ĵ��� */
    u32 socp_read_rd_done_decsrc_suc_cnt[SOCP_MAX_DECSRC_CHN];   /* SOCP����Դͨ��д��Rdbuffer�ɹ��Ĵ��� */
    u32 socp_read_rd_done_decsrc_fail_cnt[SOCP_MAX_DECSRC_CHN];  /* SOCP����Դͨ��д��Rdbufferʧ�ܵĴ��� */
    u32 socp_decsrc_isr_err_int_cnt[SOCP_MAX_DECSRC_CHN];       /* ISR�н������Դͨ�������жϴ��� */
    u32 socp_decsrc_task_err_cb_cnt[SOCP_MAX_DECSRC_CHN];        /* �����е��ý���Դͨ�������жϴ�������ɵĴ��� */
    u32 socp_decsrc_task_err_cb_ori_cnt[SOCP_MAX_DECSRC_CHN];     /* �����е��ý���Դͨ�������жϴ��������� */
} socp_debug_decsrc_s;

typedef struct  {
    u32 socp_free_decdst_cnt[SOCP_MAX_DECDST_CHN];            /* SOCP�ͷŽ���Ŀ��ͨ���ɹ��Ĵ��� */
    u32 socp_reg_event_decdst_cnt[SOCP_MAX_DECDST_CHN];        /* SOCPע�����Ŀ��ͨ���¼��Ĵ��� */
    u32 socp_reg_readcb_decdst_cnt[SOCP_MAX_DECDST_CHN];       /* SOCPע�����Ŀ��ͨ�������ݻص������Ĵ��� */
    u32 socp_get_readbuf_decdst_etr_cnt[SOCP_MAX_DECDST_CHN];   /* SOCP����Ŀ��ͨ�����Զ����ݵĴ��� */
    u32 socp_get_readbuf_decdst_suc_cnt[SOCP_MAX_DECDST_CHN];   /* SOCP����Ŀ��ͨ�������ݳɹ��Ĵ��� */
    u32 socp_read_done_decdst_etr_cnt[SOCP_MAX_DECDST_CHN];   /* SOCP����Ŀ��ͨ������д��Ŀ��buffer�Ĵ��� */
    u32 socp_read_done_decdst_suc_cnt[SOCP_MAX_DECDST_CHN];   /* SOCP����Ŀ��ͨ��д��Ŀ��buffer�ɹ��Ĵ��� */
    u32 socp_read_done_decdst_fail_cnt[SOCP_MAX_DECDST_CHN];  /* SOCP����Ŀ��ͨ��д��Ŀ��bufferʧ�ܵĴ��� */
    u32 socp_read_done_zero_decdst_cnt[SOCP_MAX_DECDST_CHN];  /* SOCP����Ŀ��ͨ��д��Ŀ��buffer size ����0 �Ĵ��� */
    u32 socp_read_done_vld_decdst_cnt[SOCP_MAX_DECDST_CHN]; /* SOCP����Ŀ��ͨ��д��Ŀ��buffer size ������0 �Ĵ��� */
    u32 socp_decdst_isr_trf_int_cnt[SOCP_MAX_DECDST_CHN];       /* ISR�н������Ŀ��ͨ����������жϴ��� */
    u32 socp_decdst_task_trf_cb_cnt[SOCP_MAX_DECDST_CHN];        /* �����лص�����Ŀ��ͨ����������жϴ�������ɵĴ��� */
    u32 socp_decdst_task_trf_cb_ori_cnt[SOCP_MAX_DECDST_CHN];     /* �����лص�����Ŀ��ͨ����������жϴ��������� */
    u32 socp_decdst_isr_ovf_int_cnt[SOCP_MAX_DECDST_CHN];       /* ISR�н������Ŀ��ͨ��buf ����жϴ��� */
    u32 socp_decdst_task_ovf_cb_cnt[SOCP_MAX_DECDST_CHN];        /* �����н������Ŀ��ͨ��buf ����жϴ�������ɵĴ��� */
    u32 socp_decdst_task_ovf_cb_ori_cnt[SOCP_MAX_DECDST_CHN];     /* �����н������Ŀ��ͨ��buf ����жϴ��������� */
} socp_debug_decdst_s;

/* ������Ϣ */
typedef struct {
    socp_debug_gbl_state_s socp_debug_gbl;
    socp_debug_enc_src_s socp_debug_encsrc;
    socp_debug_encdst_s socp_debug_enc_dst;
    socp_debug_decsrc_s socp_debug_dec_src;
    socp_debug_decdst_s socp_debug_dec_dst;
} socp_debug_info_s;

typedef enum {
    SOCP_DST_CHAN_CFG_ADDR = 0,
    SOCP_DST_CHAN_CFG_SIZE,
    SOCP_DST_CHAN_CFG_TIME,
    SOCP_DST_CHAN_CFG_RSV,
    SOCP_DST_CHAN_CFG_BUTT
} socp_dst_chan_cfg_e;


/*
 * �Ĵ���λ����
 */

/*
 * Bit masks for registers: ENCSTAT,DECSTAT, channel state
 */
#define SOCP_CHN_BUSY ((s32)1) /* ͨ��æ */
#define SOCP_CHN_IDLE ((s32)0) /* ͨ���� */

/*
 * ͨ���򿪺궨�壬�����ڱ���Դͨ���ͽ���Ŀ��ͨ��
 */
#define SOCP_CHN_ALLOCATED ((s32)1)   /* ͨ���Ѿ����� */
#define SOCP_CHN_UNALLOCATED ((s32)0) /* ͨ��û�з��� */

/*
 * ͨ��ʹ�ܺ궨�壬ͬʱ�����ڱ���Դͨ���ͽ���Դͨ��
 */
#define SOCP_CHN_ENABLE ((s32)1)  /* ͨ���� */
#define SOCP_CHN_DISABLE ((s32)0) /* ͨ���ر� */

/*
 * ͨ���Ƿ��Ѿ�����
 */
#define SOCP_CHN_SET ((s32)1)   /* ͨ���Ѿ����� */
#define SOCP_CHN_UNSET ((s32)0) /* ͨ��û������ */
/*
 * ������·����ʹ��λ
 */
#define SOCP_ENCSRC_BYPASS_ENABLE ((s32)1)  /* ͨ����·ʹ�� */
#define SOCP_ENCSRC_BYPASS_DISABLE ((s32)0) /* ͨ����·û��ʹ�� */

/*
 * ������·����ʹ��λ
 */
#define SOCP_DECSRC_DEBUG_ENBALE ((s32)1)  /* ͨ��DEBUG ʹ�� */
#define SOCP_DECSRC_DEBUG_DISBALE ((s32)0) /* ͨ��DEBUG û��ʹ�� */

#define SOCP_CORE0_DEC_OUTOVFINT_MASK ((s32)(1))      /* ����core0 Ŀ��BUFFER����ж� */
#define SOCP_DEC_INERRINT_MASK ((s32)(1 << 1))        /* ��������������ж� */
#define SOCP_CORE0_DEC_TFRINT_MASK ((s32)(1 << 2))    /* ����core0 ��������ж� */
#define SOCP_CORE1_DEC_TFRINT_MASK ((s32)(1 << 3))    /* ����core1 ��������ж� */
#define SOCP_CORE1_DEC_OUTOVFINT_MASK ((s32)(1 << 4)) /* ����core1 Ŀ��BUFFER����ж� */
#define SOCP_CORE0_ENC_MODESWT_MASK ((s32)(1 << 6))   /* ����core0 ����Ŀ��bufferģʽ�л���� */
#define SOCP_CORE1_ENC_MODESWT_MASK ((s32)(1 << 7))   /* ����core1 ����Ŀ��bufferģʽ�л���� */
#define SOCP_MODEM_ENC_RDINT_MASK ((s32)(1 << 10))    /* ����RDBUFFER����ж� */
#define SOCP_APP_ENC_RDINT_MASK ((s32)(1 << 11))      /* ����APPCPU��ѯ����ͨ��Rd���ȫ���ж� */
#define SOCP_APP_ENC_OUTOVFINT_MASK ((s32)(1 << 12))  /* ����RDBUFFER����ж� */
#define SOCP_MODEM_ENC_FLAGINT_MASK ((s32)(1 << 13))  /* ����MODEMCPU��ͷ�������ж� */
#define SOCP_APP_ENC_FLAGINT_MASK ((s32)(1 << 14))    /* ����APP  CPU��ͷ�������ж� */
#define SOCP_APP_ENC_TFRINT_MASK ((s32)(1 << 15))     /* ����ͨ������ȫ���ж� */
#define SOCP_DEC_SRCINT_NUM (6)                       /* ����MODEMCPU��ͷ�������ж� */

#define SOCP_TIMEOUT_TRF_LONG_MAX (0x927c0)  // 10min
#define SOCP_TIMEOUT_TRF_LONG_MIN (0x0a)     // 10ms
#define SOCP_TIMEOUT_TRF_SHORT_VAL (0x0a)    // 10ms

#define SOCP_DEC_PKTLGTH_MAX (0x04)  // dec:4096, ��λΪKB
#define SOCP_DEC_PKTLGTH_MIN (0x06)  // dec:6, ��λΪ�ֽ�
#define SOCP_TIMEOUT_MAX (0xffff)
#define SOCP_DEC_MAXPKT_MAX (0x1000)
#define SOCP_DEC_MINPKT_MAX (0x1f)
#define SOCP_ENC_SRC_BUFLGTH_MAX (0x7ffffff)
#define SOCP_ENC_SRC_RDLGTH_MAX (0xffff)
#define SOCP_ENC_DST_BUFLGTH_MAX (0x1fffff)
#define SOCP_ENC_DST_TH_MAX (0x7ff)
#define SOCP_DEC_SRC_BUFLGTH_MAX (0xffff)
#define SOCP_DEC_SRC_RDLGTH_MAX (0xffff)
#define SOCP_DEC_DST_BUFLGTH_MAX (0xffff)
#define SOCP_DEC_DST_TH_MAX (0xff)

// ����Ŀ��buffer�����ж�״̬�Ĵ���16-22λָʾ��ֵ����ж�
#define SOCP_ENC_DST_BUFF_THRESHOLD_OVF_MASK (0x007f0000)
#define SOCP_ENC_DST_BUFF_OVF_MASK (0x0000007f)
// ��ֵ�жϼĴ�����ʼλ
#define SOCP_ENC_DST_BUFF_THRESHOLD_OVF_BEGIN (16)

#define SOCP_RESET_TIME (1000)
#define SOCP_GBLRESET_TIME (100)

extern socp_gbl_state_s g_socp_stat;

/*
 * ��������
 */
s32 socp_check_init(void);
s32 socp_check_buf_addr(unsigned long start, unsigned long end);
s32 socp_check_chan_type(u32 para, u32 type);
s32 socp_check_chan_id(u32 para, u32 id);
s32 socp_check_encsrc_chan_id(u32 id);
s32 socp_check_8bytes_align(unsigned long para);
s32 socp_check_chan_priority(u32 para);
s32 socp_check_data_type(u32 para);
s32 socp_check_encsrc_alloc(u32 id);
s32 socp_check_encdst_set(u32 id);
s32 socp_check_decsrc_set(u32 id);
s32 socp_check_decdst_alloc(u32 id);
s32 socp_check_data_type_en(u32 param);
s32 socp_check_enc_debug_en(u32 param);
s32 socp_soft_free_encdst_chan(u32 enc_dst_chan_id);
s32 socp_soft_free_decsrc_chan(u32 dec_src_chan_id);
s32 socp_reset_dec_chan(u32 chan_id);
s32 socp_get_index(u32 size, u32 *index);
s32 socp_can_sleep(void);
s32 bsp_socp_encdst_set_cycle(u32 chanid, u32 cycle);
void socp_encdst_overflow_info(void);
/* log2.0 2014-03-19 Begin: */
u32 socp_is_encdst_chan_empty(void);
/* log2.0 2014-03-19 End: */
void socp_debug_set_reg_bits(u32 reg, u32 pos, u32 bits, u32 val);
u32 socp_debug_get_reg_bits(u32 reg, u32 pos, u32 bits);
u32 socp_debug_read_reg(u32 reg);
u32 socp_debug_write_reg(u32 reg, u32 data);

void socp_help(void);

#define BSP_REG_SETBITS(base, reg, pos, bits, val)                                        \
    (BSP_REG(base, reg) = (BSP_REG(base, reg) & (~((((u32)1 << (bits)) - 1) << (pos)))) | \
                          ((u32)((val) & (((u32)1 << (bits)) - 1)) << (pos)))
#define BSP_REG_GETBITS(base, reg, pos, bits) ((BSP_REG(base, reg) >> (pos)) & (((u32)1 << (bits)) - 1))

#define SOCP_REG_WRITE(reg, data) (writel(data, (volatile void *)(uintptr_t)(g_socp_stat.base_addr + reg)))
#define SOCP_REG_READ(reg, result) (result = readl((const volatile void *)(uintptr_t)(g_socp_stat.base_addr + reg)))

#define SOCP_REG_SETBITS(reg, pos, bits, val) BSP_REG_SETBITS(g_socp_stat.base_addr, reg, pos, bits, val)
#define SOCP_REG_GETBITS(reg, pos, bits) BSP_REG_GETBITS(g_socp_stat.base_addr, reg, pos, bits)

#define BBP_REG_WRITE(reg, data) (writel(data, (volatile void *)(g_socp_stat.armBaseAddr + reg)))
#define BBP_REG_READ(reg, result) (result = readl((const volatile void *)(g_socp_stat.armBaseAddr + reg)))

#define BBP_REG_SETBITS(reg, pos, bits, val) BSP_REG_SETBITS(g_socp_stat.armBaseAddr, reg, pos, bits, val)
#define BBP_REG_GETBITS(reg, pos, bits) BSP_REG_GETBITS(g_socp_stat.armBaseAddr, reg, pos, bits)
/**************************************************************************/

#define SOCP_FLUSH_CACHE(ptr, size) (0)
#define SOCP_INVALID_CACHE(ptr, size) (0)

#define socp_crit(fmt, ...) printk(KERN_ERR "[%s]:" fmt, BSP_MOD(THIS_MODU), ##__VA_ARGS__)
#define socp_error(fmt, ...) \
    printk(KERN_ERR "[%s]:<%s %d>" fmt, BSP_MOD(THIS_MODU), __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define BIT_N(n) (0x01 << (n))
#define SOCP_DEBUG_READ_DONE BIT_N(0)
#define SOCP_DEBUG_READ_CB BIT_N(1)

#define SOCP_DEBUG_TRACE(ulSwitch, ARG1, ARG2, ARG3, ARG4) do { \
    if (FALSE != (g_socp_debug_trace_cfg & ulSwitch)) {                  \
        socp_error("0x%x, 0x%x, 0x%x, 0x%x\n", ARG1, ARG2, ARG3, ARG4); \
    }                                                                   \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* end #ifndef DIAG_SYSTEM_5G */
#endif
