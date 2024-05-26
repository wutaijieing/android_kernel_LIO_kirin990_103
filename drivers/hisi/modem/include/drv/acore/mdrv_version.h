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
 *  @brief   version模块在acore上的对外头文件
 *  @file    mdrv_version.h
 *  @author  zhaoxiaofeng
 *  @version v1.0
 *  @date    2019.11.25
 *  @note    修改记录(版本号|修订日期|说明)
 *  <ul><li>v1.0|2019.11.25|创建文件</li></ul>
 *  @since
 */
#ifndef _MDRV_ACORE_VERSION_H_
#define _MDRV_ACORE_VERSION_H_

#ifdef _cplusplus
extern "C"
{
#endif
/**
* @brief 编译日期字符串的长度
*/
#define BUILD_DATE_LEN 12
/**
* @brief 编译时间字符串的长度
*/
#define BUILD_TIME_LEN 12

/**
* @brief bsp_version_memversion_ctrl接口操作类型，0表示读取
*/
#define VERIONREADMODE 0

/**
* @brief bsp_version_memversion_ctrl接口操作类型，1表示写入
*/
#define VERIONWRITEMODE 1

/**
* @brief 硬件版本类型（acore）
*/
typedef enum{
    BOARD_TYPE_BBIT = 0,  /**<0表示BBIT*/
    BOARD_TYPE_SFT,       /**<1表示SFT*/
    BOARD_TYPE_ASIC,      /**<2表示ASIC*/
    BOARD_TYPE_MAX
}board_actual_type_e;

/**
* @brief 芯片版本类型（acore）
*/
typedef enum{
     PV500_CHIP = 0,         /**<0表示PV500*/
     V7R1_CHIP = 1,          /**<1表示V7R1*/
     PV500_PILOT_CHIP,       /**<2表示PV500_PILOT*/
     V7R1_PILOT_CHIP,        /**<3表示V7R1_PILOT*/
     V7R2_CHIP =5,           /**<5表示V7R2*/
     V8R1_PILOT_CHIP = 0x8,  /**<8表示V8R1_PILOT*/
}bsp_chip_type_e;

/**
* @brief 产品信息结构体（acore）
*/
typedef struct  mproduct_info
{
    unsigned char *product_name;       /**<产品名称字符串指针*/
    unsigned int   product_name_len;   /**<产品名称字符创的长度*/
    bsp_chip_type_e chip_type;         /**<芯片版本类型*/
    unsigned int  platform_info ;      /**<平台版本类型*/
    board_actual_type_e board_a_type;  /**<硬件版本类型*/
    unsigned short chip_id;            /**<芯片ID*/
} mproduct_info_s;

/* 芯片Modem 的软件版本信息*/
/**
* @brief 软件版本信息结构体
*/
typedef  struct  msw_ver_info
{
    unsigned short  vver_no;
    unsigned short  rver_no;
    unsigned short  cver_no;
    unsigned short  bver_no;
    unsigned short  spc_no;
    unsigned short  custom_nov;
    unsigned int    product_no;
    unsigned char  build_date[BUILD_DATE_LEN];  /**<编译日期*/
    unsigned char  build_time[BUILD_TIME_LEN];  /**<编译时间*/
} msw_ver_info_s;

/**
* @brief 硬件版本信息结构体（acore）
*/
typedef  struct  mhw_ver_info
{
    int  hw_index;                  /**<硬件版本号，即productid*/
    unsigned int  hw_id_sub;        /**<硬件子版本号*/
    unsigned char *hw_full_ver;     /**<硬件版本信息 MBB使用*/
    unsigned int  hw_full_ver_len;  /**<硬件版本信息长度*/
    unsigned char *hw_name;         /**<外部产品名称  MBB使用*/
    unsigned int  hw_name_len;      /**<外部产品名称长度*/
    unsigned char *hw_in_name;      /**<内部产品名称  MBB使用*/
    unsigned int  hw_in_name_len;   /**<内部产品名称长度*/
} mhw_ver_info_s;

/**
* @brief modem版本信息结构体（acore）
*/
typedef struct  modem_ver_info
{
    mproduct_info_s  product_info;  /**<产品版本信息*/
    msw_ver_info_s  sw_ver_info;    /**<软件版本信息*/
    mhw_ver_info_s  hw_ver_info;    /**<硬件版本信息*/
} modem_ver_info_s;

/**
* @brief 组件写入或读取的版本信息枚举（acore）
*/
typedef enum
{
    VER_BOOTLOAD = 0,
    VER_BOOTROM = 1,
    VER_NV = 2,
    VER_VXWORKS = 3,     /**<3表示VXWORKS*/
    VER_DSP = 4,
    VER_PRODUCT_ID = 5,  /**<5表示PRODUCT_ID*/
    VER_WBBP = 6,
    VER_PS = 7,          /**<7表示PS*/
    VER_OAM = 8,
    VER_GBBP = 9,
    VER_SOC = 10,
    VER_HARDWARE = 11,   /**<11表示HARDWARE*/
    VER_SOFTWARE = 12,   /**<12表示SOFTWARE*/
    VER_MEDIA = 13,
    VER_APP = 14,
    VER_ASIC = 15,
    VER_RF = 16,
    VER_PMU = 17,
    VER_PDM = 18,        /**<18表示PDM*/
    VER_PRODUCT_INNER_ID = 19,
    VER_INFO_NUM = 20
} comp_type_e;

/**
 * @brief 获取版本信息接口（acore）
 *
 * @par 描述:
 * 该接口返回版本信息结构体的地址，版本信息包含芯片、硬件、软件等版本信息，
 * 可查看modem_ver_info_s结构体成员。
 *
 * @attention
 * <ul><li>版本信息较多，使用时请注意查看modem_ver_info_s结构体及其成员的注释</li></ul>
 *
 * @retval 返回版本信息全局变量的指针，不可用时返回NULL。
 * @par 依赖:
 * <ul><li>mdrv_version.h：该接口声明所在的头文件。</li></ul>
 *
 * @see modem_ver_info_s
 */
const modem_ver_info_s *mdrv_ver_get_info(void);

#ifdef _cplusplus
}
#endif
#endif

