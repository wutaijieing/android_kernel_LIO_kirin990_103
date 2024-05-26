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

#ifndef _MDRV_OM_H_
#define _MDRV_OM_H_

#ifdef _cplusplus
extern "C"
{
#endif
/**
 *  @brief   可维可测模块对外头文件
 *  @file    mdrv_om.h
 *  @author  xxx
 *  @version v1.0
 *  @date    2019.11.23
 *  @note    修改记录(版本号|修订日期|说明)
 * <ul><li>v1.0|2019.05.30|创建文件</li></ul>
 *  @since
 */
#include "mdrv_om_common.h"



/**
* @brief 不同组件错误码的宏定义
*/
#define DRV_NR_BSP_MODID(ERROR_CODE)              ((OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_BSP))| ERROR_CODE)/**<NR DRV */
#define DRV_NR_MSP_MODID(ERROR_CODE)              ((OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_MSP))| ERROR_CODE)
#define PAM_NR_OSA_MODID(ERROR_CODE)              (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_OSA)| ERROR_CODE)
#define PAM_NR_OAM_MODID(ERROR_CODE)              (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_OAM)| ERROR_CODE)
#define NAS_NR_MODID(ERROR_CODE)                  (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_NAS)| ERROR_CODE)
#define AS_NR_MODID(ERROR_CODE)                   (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_AS)| ERROR_CODE)
#define PS_NR_MODID(ERROR_CODE)                   (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_PS)| ERROR_CODE)
#define PHY_NR_MODID(ERROR_CODE)                  (OM_MODID_HEAD(NR_SYSTEM_AP,MODULE_PHY)| ERROR_CODE)
#define DRV_LR_BSP_MODID(ERROR_CODE)              (ERROR_CODE)
#define DRV_LR_MSP_MODID(ERROR_CODE)              (ERROR_CODE)
#define PAM_LR_OSA_MODID(ERROR_CODE)              (ERROR_CODE)
#define PAM_LR_OAM_MODID(ERROR_CODE)              (ERROR_CODE)
#define NAS_LR_MODID(ERROR_CODE)                  (ERROR_CODE)
#define AS_LR_MODID(ERROR_CODE)                   (ERROR_CODE)
#define PS_LR_MODID(ERROR_CODE)                   (ERROR_CODE)
#define PHY_LR_MODID(ERROR_CODE)                  (ERROR_CODE)

#ifdef _cplusplus
}
#endif
#endif

