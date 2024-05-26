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
 *  @brief   pcdev模块在acore上的对外头文件
 *  @file    mdrv_pcdev.h
 *  @author  guoyuan
 *  @version v1.0
 *  @date    2019.12.17
 *  @note    修改记录(版本号|修订日期|说明)
 *  <ul><li>v1.0|2019.12.17|创建文件</li></ul>
 *  @since
 */

#ifndef _MDRV_PCDEV_H_
#define _MDRV_PCDEV_H_

#ifdef _cplusplus
extern "C"
{
#endif

#include <linux/dma-mapping.h>

/**
* @brief diag口最大描述符个数
*/
#define MAX_DESC_NUM 4

/* IOCTL CMD 定义 */
/**
* @brief 配置写完成回调
*/
#define PCDEV_IOCTL_SET_WRITE_CB      0x7F001000
/**
* @brief 配置读完成回调
*/
#define PCDEV_IOCTL_SET_READ_CB       0x7F001001
/**
* @brief 配置事件完成回调
*/
#define PCDEV_IOCTL_SET_EVT_CB        0x7F001002
/**
* @brief 配置buf释放回调
*/
#define PCDEV_IOCTL_SET_FREE_CB       0x7F001003
/**
* @brief 配置带时间戳的写完成回调（diag专用）
*/
#define PCDEV_IOCTL_SET_WRITE_INFO_CB 0x7F001004
/**
* @brief 事件发送命令
*/
#define PCDEV_IOCTL_SEND_EVT          0x7F001005
/**
* @brief tty专用命令
*/
#define PCDEV_IOCTL_SEND_MSC_READ     0x7F001006
/**
* @brief modem口专用命令
*/
#define PCDEV_IOCTL_SEND_REL_IND      0x7F001007
/**
* @brief 阻塞写命令
*/
#define PCDEV_IOCTL_WRITE_ASYNC       0x7F001010
/**
* @brief 获取发送来的buf
*/
#define PCDEV_IOCTL_GET_RD_BUFF       0x7F001011
/**
* @brief 归还已读取的buf
*/
#define PCDEV_IOCTL_RETURN_BUFF       0x7F001012
/**
* @brief 重新配置收buf缓冲区
*/
#define PCDEV_IOCTL_RELLOC_READ_BUFF  0x7F001013
/**
* @brief 兼容usb，暂不使用
*/
#define PCDEV_IOCTL_SEND_BUFF_CAN_DMA 0x7F001014
/**
* @brief 配置读buf缓冲区（diag专用）
*/
#define PCDEV_IOCTL_SET_READ_BUFF     0x7F001015
/**
* @brief 兼容usb，暂不使用
*/
#define PCDEV_IOCTL_IS_IMPORT_DONE    0x7F001020
/**
* @brief 兼容usb，暂不使用
*/
#define PCDEV_IOCTL_WRITE_DO_COPY     0x7F001021
/**
* @brief modem口专用命令
*/
#define PCDEV_MODEM_IOCTL_SET_MSC_READ_CB 0x7F001030
/**
* @brief modem口专用命令
*/
#define PCDEV_MODEM_IOCTL_MSC_WRITE_CMD   0x7F001031
/**
* @brief modem口专用命令
*/
#define PCDEV_MODEM_IOCTL_SET_REL_IND_CB  0x7F001032
/**
* @brief modem口专用命令
*/
#define PCDEV_MODEM_IOCTL_SET_MSC_WRITE_CB  0x7F001033
/**
* @brief 配置hids连接状态（diag专用）
*/
#define PCDEV_MODEM_IOCTL_SET_DIAG_STATUS  0x7F001040

/**
* @brief 需要传输的buf信息结构体
*/
typedef struct pcdev_wr_async_info
{
    char *p_vaddr;           /**<虚拟地址 */
    char *p_paddr;           /**<物理地址 */
    unsigned int size;       /**<buf大小 */
    void* p_priv;            /**<私有字段，暂不使用 */
}pcdev_wr_async_info_s;

/**
* @brief 需要传输的带时间戳的buf信息结构体（diag专用）
*/
typedef struct pcdev_write_info
{
    char *p_vaddr;           /**<虚拟地址 */
    char *p_paddr;           /**<物理地址 */
    unsigned int size;       /**<buf大小 */
    unsigned int submit_time;/**<时间戳 */
    unsigned int complete_time;/**<时间戳 */
    unsigned int done_time;/**<时间戳 */
}pcdev_write_info_s;

/**
* @brief 接收buf配置结构体
*/
typedef struct pcdev_read_buf_info
{
    unsigned int buf_size;  /**<buf大小，diag专用 */
}pcdev_read_buf_info_s;

/**
* @brief 接收buf配置结构体（diag专用）
*/
typedef struct pcdev_read_buf_set_info
{
    dma_addr_t phy_buf_base[MAX_DESC_NUM];    /**<diag专用 */
    unsigned long long vir_buf_base[MAX_DESC_NUM];/**<diag专用 */
    unsigned int buf_size;/**<diag专用 */
    unsigned int buf_num;/**<diag专用 */
}pcdev_read_buf_set_info_s;

/**
* @brief 端口状态枚举结构体
*/
typedef enum pcdev_evt
{
    PCDEV_EVT_DEV_SUSPEND = 0,    /**<端口断开 */
    PCDEV_EVT_DEV_READY = 1,      /**<端口链接 */
    PCDEV_EVT_DEV_BOTTOM
}pcdev_evt_e;

/**
* @brief hids状态枚举结构体（diag专用）
*/
typedef enum pcdev_diag_mode
{
    PCDEV_DIAG_DISCONNECT = 0,    /**<hids断开 */
    PCDEV_DIAG_CONNECT = 1,       /**<hids连接 */
    PCDEV_DIAG_BOTTOM
}pcdev_diag_mode_e;

/* ACM MODEM 管角信号结构体定义*/
/**
* @brief modem口专用
*/
typedef struct pcdev_mdm_msc
{
    unsigned int op_dtr:1;          /**<DTR CHANGE FLAG*/
    unsigned int op_dsr:1;          /**<DSR CHANGE FLAG*/
    unsigned int op_cts:1;          /**<CTSCHANGE FLAG*/
    unsigned int op_rts:1;          /**<RTS CHANGE FLAG*/
    unsigned int op_ri:1;           /**<RI CHANGE FLAG*/
    unsigned int op_dcd:1;          /**<DCD CHANGE FLAG*/
    unsigned int op_fc:1;           /**<FC CHANGE FLAG*/
    unsigned int op_brk:1;          /**<BRK CHANGE FLAG*/
    unsigned int op_spare:24;       /**<reserve*/
    unsigned char ucdtr;            /**<DTR  VALUE*/
    unsigned char ucdsr;            /**<DSR  VALUE*/
    unsigned char uccts;            /**<DTS VALUE*/
    unsigned char ucrts;            /**<RTS  VALUE*/
    unsigned char ucri;             /**<RI VALUE*/
    unsigned char ucdcd;            /**<DCD  VALUE*/
    unsigned char ucfc;             /**<FC  VALUE*/
    unsigned char ucbrk;            /**<BRK  VALUE*/
    unsigned char ucbrk_len;        /**<BRKLEN VALUE*/
} pcdev_mdm_msc_stru;

/**
* @brief 写完成回调格式
*/
typedef void (*pcdev_write_done_cb_t)(char *vaddr, char *p_vaddr, int size);
/**
* @brief 带时间戳的写完成回调格式（diag专用）
*/
typedef void (*pcdev_write_info_done_cb_t)(pcdev_write_info_s *write_info);
/**
* @brief 读完成回调格式
*/
typedef void (*pcdev_read_done_cb_t)(void);
/**
* @brief 事件完成回调格式
*/
typedef void (*pcdev_event_cb_t)(pcdev_evt_e evt);
/**
* @brief 枚举完成回调格式
*/
typedef void (*pcdev_enum_done_cb_t)(void);
/**
* @brief 去枚举回调格式
*/
typedef void (*pcdev_disable_cb_t)(void);
/**
* @brief modem口专用接口
*/
typedef void (*pcdev_modem_msc_read_cb_t)(pcdev_mdm_msc_stru *modem_msc);
/**
* @brief modem口专用接口
*/
typedef void (*pcdev_modem_rel_ind_cb_t)(unsigned int enable);
/**
* @brief modem口专用接口
*/
typedef void (*pcdev_modem_msc_write_cb_t)(pcdev_mdm_msc_stru *modem_msc);


/**
 * @brief 注册通道枚举完成回调。
 *
 * @par 描述:
 * 注册回调函数。
 *
 * @param[in]  pfunc   通道枚举完成后将被调用的回调
 *
 * @retval <0，表示函数执行失败。
 * @retval >=0，表示函数执行成功，其值表示逻辑通道号所对应的物理通道号。
 *
 * @attention
 * <ul><li>请在初始化阶段完成注册。</li></ul>
 *
 * @par 依赖:
 * <ul><li>mdrv_pcdev.h：该接口声明所在的头文件。</li></ul>
 *
 * @see pcdev_enum_done_cb_t
 */
unsigned int mdrv_pcdev_reg_enumdonecb(pcdev_enum_done_cb_t pfunc);

/**
 * @brief 注册通道去枚举通知回调。
 *
 * @par 描述:
 * 注册回调函数。
 *
 * @param[in]  pfunc   通道去枚举后将被调用的回调
 *
 * @retval <0，表示函数执行失败。
 * @retval >=0，表示函数执行成功，其值表示逻辑通道号所对应的物理通道号。
 *
 * @attention
 * <ul><li>请在初始化阶段完成注册。</li></ul>
 *
 * @par 依赖:
 * <ul><li>mdrv_pcdev.h：该接口声明所在的头文件。</li></ul>
 *
 * @see pcdev_disable_cb_t
 */
unsigned int mdrv_pcdev_reg_disablecb(pcdev_enum_done_cb_t pfunc);

#ifdef _cplusplus
}
#endif
#endif

