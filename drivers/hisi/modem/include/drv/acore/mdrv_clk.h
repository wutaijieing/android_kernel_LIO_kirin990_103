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
 *  @brief   clk模块在acore上的对外头文件
 *  @file    mdrv_clk.h
 *  @author  maluping
 *  @version v1.0
 *  @date    2019.11.27
 *  @note    修改记录(版本号|修订日期|说明)
 *  <ul><li>v1.0|2019.11.27|创建文件</li></ul>
 *  @since
 */
#ifndef __MDRV_ACORE_CLK_H__
#define __MDRV_ACORE_CLK_H__

#include "mdrv_public.h"

#ifdef __cplusplus
extern "C" {
#endif

 /**
* @brief  acc 类型枚举
*/
enum mdrv_clk_acc_type {
    CLK_HDLC,
    CLK_ACC_TYPE_MAX
};

/**
 * @brief 打开时钟
 *
 * @par 描述:
 * 根据acc类型打开对应时钟
 *
 * @attention
 * <ul><li>超出acc类型范围会报错。</li></ul>
 *
 * @param[in]  acc_clk_index , acc类型值。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @par 依赖:
 * <ul><li>mdrv_clk.h：该接口声明所在的头文件。</li></ul>
 *
 * @see mdrv_clk_acc_type
 */
int mdrv_acc_clk_enable(enum mdrv_clk_acc_type acc_clk_index);
/**
 * @brief 关闭时钟
 *
 * @par 描述:
 * 根据acc类型关闭对应时钟
 *
 * @attention
 * <ul><li>超出acc类型范围会报错。</li></ul>
 *
 * @param[in]  acc_clk_index , acc类型值。
 *
 * @par 依赖:
 * <ul><li>mdrv_clk.h：该接口声明所在的头文件。</li></ul>
 *
 * @see mdrv_clk_acc_type
 */
void mdrv_acc_clk_disable(enum mdrv_clk_acc_type acc_clk_index);
/**
 * @brief 查询时钟状态
 *
 * @par 描述:
 * 根据acc类型查询时钟状态
 *
 * @attention
 * <ul><li>超出acc类型范围会报错。</li></ul>
 *
 * @param[in]  acc_clk_index , acc类型值。
 *
 * @retval 1,时钟是打开的。
 * @retval 0,时钟是关闭的。
 * @retval -1,无该时钟节点。
 *
 * @par 依赖:
 * <ul><li>mdrv_clk.h：该接口声明所在的头文件。</li></ul>
 *
 * @see mdrv_clk_acc_type
 */
int mdrv_acc_clk_is_enabled(enum mdrv_clk_acc_type acc_clk_index);

#ifdef __cplusplus
}
#endif

#endif
