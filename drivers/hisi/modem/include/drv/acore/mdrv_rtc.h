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
 *  @brief   rtc模块在fusion上的对外头文件
 *  @file    mdrv_rtc.h
 *  @author  zhaixiaojun
 *  @version v1.0
 *  @date    2019.12.04
 *  <ul><li>v1.0|2019.12.04|创建文件</li></ul>
 *  @since
 */
#ifndef __MDRV_RTC_H__
#define __MDRV_RTC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
     *                           Attention                           *
******************************************************************************
* Description : Driver for rtc
* Core        : Acore、Ccore
* Header File : the following head files need to be modified at the same time
*             : /acore/mdrv_rtc.h
*             : /ccore/fusion/mdrv_rtc.h
*             : /ccore/ccpu/mdrv_rtc.h
******************************************************************************/

/**
 * @brief 设置时间
 *
 * @par 描述:
 * 设置tm中包含的时间信息
 *
 * @param[in]  tm, 需要设置的时间信息
 *
 * @retval 0,表示函数执行成功。
 * @retval !=0,表示函数执行失败。

 * @par 依赖:
 * <ul><li>mdrv_rtc.h：该接口声明所在的头文件。</li></ul>
 *
 * @see struct rtc_time
 */

unsigned int mdrv_rtc_get_unix_time_value(void);

/*****************************************************************************
       *                        Attention   END                        *
*****************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
