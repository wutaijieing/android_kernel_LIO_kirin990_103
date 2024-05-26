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
 
 /**
 *  @brief   socp模块在ACORE上的对外头文件
 *  @file    mdrv_socp.h
 *  @author  yaningbo
 *  @version v1.0
 *  @date    2019.11.29
 *  @note    修改记录(版本号|修订日期|说明) 
 *  <ul><li>v1.0|2012.11.29|创建文件</li></ul>
 *  <ul><li>v2.0|2019.11.29|接口自动化</li></ul>
 *  @since   
 */

#ifndef __MDRV_SOCP_H__
#define __MDRV_SOCP_H__

#include "mdrv_socp_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


#include "mdrv_socp_common.h"


/**
 * @brief socp rd描述符数据结构
*/
typedef struct
{
    unsigned long long data_addr;
    unsigned short data_len;
    unsigned short data_type;
    unsigned int reservd;
}socp_rd_data_s;

/**
 * @brief socp 环形buffer结构
*/
typedef struct {
    char *buffer;
    char *rb_buffer;
    unsigned int size;
    unsigned int rb_size;
}socp_buffer_rw_s;

/**
 * @brief socp coder dest buff config
 */
typedef struct {
    void *vir_buffer;              /**< socp coder dest channel buffer virtual address */
    unsigned long phy_buffer_addr; /**< socp coder dest channel buffer phyical address */
    unsigned int buffer_size;      /**< socp coder dest channel buffer size */
    unsigned int over_time;        /**< socp coder dest channel buffer timeout */
    unsigned int log_on_flag;      /**< socp coder dest channel buffer log is on flag */
    unsigned int cur_time_out;     /**< socp coder dest channel buffer current timeout cost */
} socp_encdst_buf_log_cfg_s;

/**
 * @brief socp deflate压缩功能使能
 *
 * @par 描述:
 * socp deflate压缩功能使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id , 目的通道ID。
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_compress_enable(unsigned int dst_chan_id);
/**
 * @brief socp deflate压缩功能去使能
 *
 * @par 描述:
 * socp deflate压缩功能去使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id , 目的通道ID。
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_compress_disable(unsigned int dst_chan_id);

/**
 * @brief socp deflate压缩功能去使能
 *
 * @par 描述:
 * socp deflate压缩功能去使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id , 目的通道ID。
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */

int mdrv_deflate_set_ind_mode(SOCP_IND_MODE_ENUM mode);
/**
 * @brief socp deflate压缩功能去使能
 *
 * @par 描述:
 * socp deflate压缩功能去使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id , 目的通道ID。
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_compress_status(void);

/**
 * @brief socp deflate压缩功能去使能
 *
 * @par 描述:
 * socp deflate压缩功能去使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[in]  dst_chan_id , 目的通道ID。
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_deflate_get_log_ind_mode(unsigned int *eMode);

/**
 * @brief 获取log上报模式
 *
 * @par 描述:
 * 获取log上报模式
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[out]  mode, log上报模式
	mode：0，直接上报模式
	mode：1，延迟上报模式
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_get_log_ind_mode(unsigned int *mode);

/**
 * @brief bbp采数内存开关使能
 *
 * @par 描述:
 * bbp采数内存开关使能
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 0:  操作成功。
 * @retval 非0:  操作失败。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_socp_bbpmemenable(void);
/**
 * @brief 获取socp目的端中断计数
 *
 * @par 描述:
 * 获取socp ind目的端传输中断和上溢中断计数
 *
 * @param[out]  trf_info, 传输中断计数
 * @param[out]  trh_ovf_info, 上溢中断计数
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 无
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_SocpEncDstQueryIntInfo(unsigned int *trf_info, unsigned int *trh_ovf_info);
/**
 * @brief 清除socp目的端中断信息
 *
 * @par 描述:
 * 清除socp目的端中断信息
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 *
 * @retval 无
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_clear_socp_encdst_int_info(void);

/**
 * @brief 本接口用于查询刷新后LOG2.0 SOCP超时配置信息
 *
 * @par 描述:
 * 本接口用于查询刷新后LOG2.0 SOCP超时配置信息
 *
 * @attention
 * <ul><li>NA</li></ul>
 *
 * @param[out]  cfg   log配置的信息
 *
 * @retval 0:  获取成功。
 * @retval 非0：获取失败。
 *
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
unsigned int mdrv_socp_get_log_cfg(socp_encdst_buf_log_cfg_s *cfg);

#ifdef __cplusplus
}
#endif
#endif
