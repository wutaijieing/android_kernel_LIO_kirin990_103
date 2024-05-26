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

/*
 * 1 头文件包含
 */
#include "OmCommonPpm.h"
#include <linux/module.h>
#include <product_config.h>
#include <securec.h>
#include <bsp_socp.h>
#include <bsp_nvim.h>
#include <bsp_slice.h>
#include <nv_stru_drv.h>
#include <acore_nv_stru_drv.h>
#include <securec.h>
#include <bsp_icc.h>
#include <bsp_slice.h>
#include "diag_port_manager.h"
#include "diag_system_debug.h"
#include "../../adrv/adrv.h"
#include "scm_ind_src.h"
#include "scm_ind_dst.h"
#include "scm_cnf_src.h"
#include "scm_cnf_dst.h"
#include "scm_common.h"
#include "diag_port_manager.h"
#include "OmUsbPpm.h"
#include "OmVcomPpm.h"
#include "OmPortSwitch.h"

/*
 * 2 全局变量定义
 */

/* 用于ACPU上USB设备的UDI句柄 */
void *g_ppm_port_udi_handle[OM_PORT_HANDLE_BUTT];

/* USB承载的OM IND端口中，伪造为同步接口使用的数据结构体 */
ppm_pseudo_sync_s g_usb_ind_pseudo_sync;

/* USB承载的OM CNF端口中，伪造为同步接口使用的数据结构体 */
ppm_pseudo_sync_s g_usb_cfg_pseudo_sync;

/* 自旋锁，用来作AT命令端口切换的临界资源保护 */
spinlock_t g_ppm_port_switch_spinlock;

mdrv_ppm_chan_debug_info_s g_ppm_chan_info;

u32 g_ppm_debug_flag = false;

/*
 * 3 外部引用声明
 */
extern spinlock_t g_stScmSoftDecodeDataRcvSpinLock;
extern spinlock_t g_stCbtScmDataRcvSpinLock;
ppm_disconnect_port_cb g_ppm_disconnect_cb = NULL;
/*
 * 4 函数实现
 */
extern u32 ppm_socket_port_init(void);
void ppm_disconnect_cb_reg(ppm_disconnect_port_cb cb)
{
    g_ppm_disconnect_cb = cb;
}

void ppm_disconnect_port(unsigned int chan)
{
    if (g_ppm_disconnect_cb) {
        if (g_ppm_disconnect_cb()) {
            diag_error("disconnect error\n");
        }
    } else {
        //diag_error("disconnect func is null\n");
    }
    return;
}
/*
 * 函 数 名: ppm_get_send_data_len
 * 功能描述: PPM对数据发送长度进行处理
 * 输入参数: data_len: SOCP通道的数据长度
 *           enChanID:  SOCP channel ID
 * 输出参数: pulSendDataLen:实际发送出去的数据长度
 *           penPhyport:指向物理端口号
 * 返 回 值: void
 */
void ppm_get_send_data_len(SOCP_CODER_DST_ENUM_U32 chan, u32 data_len, u32 *send_len,
                        unsigned int *phy_port)
{
    unsigned int port;

    if (SOCP_CODER_DST_OM_CNF == chan) {
        port = cpm_query_phy_port(CPM_OM_CFG_COMM);
    } else {
        port = cpm_query_phy_port(CPM_OM_IND_COMM);
    }

    /* 当发送是通过USB并且发送长度大于60k的时候，需要限制发送长度 */
    if (((CPM_IND_PORT == port) || (CPM_CFG_PORT == port)) && (data_len > USB_MAX_DATA_LEN)) {
        *send_len = USB_MAX_DATA_LEN;
    } else {
        *send_len = data_len; /* 其他情况下不需要调整当前的大小，包括sd、wifi */
    }

    *phy_port = port;
    return;
}

/*
 * 函 数 名: ppm_port_status
 * 功能描述: 用于ACPU上面处理物理端口断开后的OM链接断开
 * 输入参数: enHandle:   UDI设备句柄
 *           phy_port:  物理端口
 *           enPortState:端口状态
 * 输出参数: 无
 * 返 回 值: 无
 */
void ppm_port_status(unsigned int handle, unsigned int phy_port, ACM_EVT_E port_state)
{
    unsigned int  logic_port;
    unsigned long lock;
    bool flag = false;
    u32 slice;

    if (ACM_EVT_DEV_SUSPEND == port_state) {
        slice = bsp_get_slice_value();

        g_ppm_chan_info.port_info[handle].usb_out_num++;

        g_ppm_chan_info.port_info[handle].usb_out_time = slice;

        diag_crit("Receive USB disconnect (chan %d) callback at slice 0x%x!\n", handle, slice);

        spin_lock_irqsave(&g_ppm_port_switch_spinlock, lock);

        flag = false;
        logic_port = OM_LOGIC_CHANNEL_BUTT;

        /* CFG端口处理GU和TL的端口断开，发消息到GU和TL去处理，但不断开CPM的关联 */
        if (OM_USB_CFG_PORT_HANDLE == handle) {
            if (phy_port == cpm_query_phy_port(CPM_OM_CFG_COMM)) {
                flag = true;
                logic_port = OM_LOGIC_CHANNEL_CNF;
            }
        }
        /* IND端口断开时发消息到GU和TL去处理 */
        else if (OM_USB_IND_PORT_HANDLE == handle) {
            if (phy_port == cpm_query_phy_port(CPM_OM_IND_COMM)) {
                flag = true;
                logic_port = OM_LOGIC_CHANNEL_IND;
            }
        } else {
        }

        spin_unlock_irqrestore(&g_ppm_port_switch_spinlock, lock);

        if (true == flag) {
            ppm_disconnect_port(logic_port);
        }
    } else if (ACM_EVT_DEV_READY == port_state) {
        g_ppm_chan_info.port_info[handle].usb_in_num++;

        g_ppm_chan_info.port_info[handle].usb_in_time = bsp_get_slice_value();
    } else {
        diag_error("The USB Port %d State %d is Unknow", (s32)phy_port, (s32)port_state);

        g_ppm_chan_info.port_info[handle].usb_state_err_num++;

        g_ppm_chan_info.port_info[handle].usb_state_err_time = bsp_get_slice_value();
    }

    return;
}

/*
 * 函 数 名: ppm_port_close_proc
 * 功能描述: OM端口已经消失，需要关闭USB端口
 * 输入参数: enHandle: 端口句柄枚举定义，关闭后端口设置为VOS_ERROR
 *           phy_port:物理端口
 * 输出参数: 无
 * 返 回 值: 无
 */
void ppm_port_close_proc(unsigned int handle, unsigned int phy_port)
{
    unsigned long lock;
    unsigned int  logic_port;
    bool flag = false;

    g_ppm_chan_info.port_info[handle].usb_close_num++;
    g_ppm_chan_info.port_info[handle].usb_close_slice = bsp_get_slice_value();

    if (!g_ppm_port_udi_handle[handle]) {
        return;
    }
    bsp_acm_close(g_ppm_port_udi_handle[handle]);
    g_ppm_port_udi_handle[handle] = NULL;

    g_ppm_chan_info.port_info[handle].usb_close_ok_num++;
    g_ppm_chan_info.port_info[handle].usb_close_ok_slice = bsp_get_slice_value();

    spin_lock_irqsave(&g_ppm_port_switch_spinlock, lock);

    flag = false;
    logic_port = OM_LOGIC_CHANNEL_BUTT;

    /* CFG端口处理GU和TL的端口断开，发消息到GU和TL去处理，但不断开CPM的关联 */
    if (OM_USB_CFG_PORT_HANDLE == handle) {
        if (phy_port == cpm_query_phy_port(CPM_OM_CFG_COMM)) {
            flag = true;
            logic_port = OM_LOGIC_CHANNEL_CNF;
        }
    }
    /* IND端口断开时发消息到GU和TL去处理，但不断开CPM的关联 */
    else if (OM_USB_IND_PORT_HANDLE == handle) {
        if (phy_port == cpm_query_phy_port(CPM_OM_IND_COMM)) {
            flag = true;
            logic_port = OM_LOGIC_CHANNEL_IND;
        }
    } else {
    }

    spin_unlock_irqrestore(&g_ppm_port_switch_spinlock, lock);

    if (true == flag) {
        ppm_disconnect_port(logic_port);
    }

    return;
}

/*
 * 函 数 名: GU_OamUSBReadData
 * 功能描述: 用于ACPU上面底软把数据通过ICC发送给OM模块
 * 输入参数: phy_port: 物理端口
 *           UdiHandle:设备句柄
 *           enHandle: UDI设备句柄数组偏移
 * 输出参数: 无
 * 返 回 值: BSP_ERROR/BSP_OK
 */
u32 ppm_read_port_data(unsigned int phy_port, void *udi_handle, unsigned int ppm_handle)
{
    struct acm_wr_async_info acm_info = {NULL,NULL,0,NULL};
    u32 ret = 0xFFFFFFFF;

    diag_ptr(EN_DIAG_PTR_PPM_READDATA, 0, 0, 0);

    if (NULL == udi_handle) {
        diag_error("Input HANDLE is err\n");

        g_ppm_chan_info.port_info[ppm_handle].usb_udi_handle_err++;

        diag_ptr(EN_DIAG_PTR_PPM_ERR1, 0, 0, 0);

        return (u32)BSP_ERROR;
    }

    (void)memset_s(&acm_info, sizeof(acm_info), 0, sizeof(acm_info));

    /* 获取USB的IO CTRL口的读缓存 */

    if (BSP_OK != bsp_acm_ioctl(udi_handle, ACM_IOCTL_GET_RD_BUFF, &acm_info)) {
        diag_error("Call ioctl Failed\n");
        g_ppm_chan_info.port_info[ppm_handle].usb_get_rd_data_err++;
        diag_ptr(EN_DIAG_PTR_PPM_ERR2, 0, 0, 0);
        return (u32)BSP_ERROR;
    }

    OM_ACPU_DEBUG_CHANNEL_TRACE(phy_port, (u8 *)acm_info.virt_addr, acm_info.size, OM_ACPU_USB_CB, OM_ACPU_DATA);

    ret = cpm_com_receive(phy_port, (u8 *)acm_info.virt_addr, acm_info.size);
    if (BSP_OK != ret) {
        diag_error("cpm_com_receive fail(0x%x), PhyPort is %d\n", ret, (s32)phy_port);
        g_ppm_chan_info.port_info[ppm_handle].usb_udi_rcv_null_err++;
    }

    g_ppm_chan_info.port_info[ppm_handle].usb_rcv_pkt_num++;
    g_ppm_chan_info.port_info[ppm_handle].usb_rcv_pkt_len += acm_info.size;

    if (BSP_OK != bsp_acm_ioctl(udi_handle, ACM_IOCTL_RETURN_BUFF, &acm_info)) {
        diag_error("ioctl Fail\n");
        g_ppm_chan_info.port_info[ppm_handle].usb_udi_read_buf_free_err++;
    }

    return BSP_OK;
}


u32 PPM_UdiRegCallBackFun(void *handle, u32 cmd_type, void *func)
{
    if (NULL == func) {
        return BSP_OK;
    }

    if (BSP_OK != bsp_acm_ioctl(handle, cmd_type, func)) {
        diag_error("mdrv_udi_ioctl Failed\n");
        return (u32)BSP_ERROR;
    }

    return BSP_OK;
}

#define OM_SOCP_CNF_BUFFER_SIZE (8 * 1024)
#define OM_SOCP_CNF_BUFFER_NUM (8)

#define OM_SOCP_IND_BUFFER_SIZE (2 * 1024)
#define OM_SOCP_IND_BUFFER_NUM (2)

/*
 * 函 数 名: ppm_read_port_data_init
 * 功能描述: 用于初始化OM使用的设备
 * 输入参数: phy_port: 物理端口号
 *           enHandle: 端口的句柄
 *           pReadCB: 该端口上面的读取回调函数
 *           pWriteCB: 该端口上面的异步写回调函数
 *           pStateCB: 该端口上面的状态回调函数
 * 输出参数: 无
 * 返 回 值: BSP_OK/BSP_ERROR
 */
void ppm_read_port_data_init(unsigned int phy_port, unsigned int ppm_handle, void *read_cb,
                          void *write_cb, void *state_cb)
{
    struct udi_open_param                    udi_para;
    struct acm_read_buff_info                  read_buff;

    /* 初始化当前使用的USB通道 */
    if (CPM_IND_PORT == phy_port) {
        read_buff.buff_size = OM_SOCP_IND_BUFFER_SIZE;
        read_buff.buff_num = OM_SOCP_IND_BUFFER_NUM;
        udi_para.devid = UDI_ACM_LTE_DIAG_ID;
    } else if (CPM_CFG_PORT == phy_port) {
        read_buff.buff_size = OM_SOCP_CNF_BUFFER_SIZE;
        read_buff.buff_num = OM_SOCP_CNF_BUFFER_NUM;
        udi_para.devid = UDI_ACM_GPS_ID;
    } else {
        diag_error("Open Wrong Port %d\n", (s32)phy_port);

        return;
    }

    g_ppm_chan_info.port_info[ppm_handle].usb_open_num++;
    g_ppm_chan_info.port_info[ppm_handle].usb_open_slice = mdrv_timer_get_normal_timestamp();

    if (NULL != g_ppm_port_udi_handle[ppm_handle]) {
        diag_crit("The UDI Handle is not Null\n");
        return;
    }

    /* 打开OM使用的USB通道 */
    g_ppm_port_udi_handle[ppm_handle] = bsp_acm_open(udi_para.devid);

    if (NULL == g_ppm_port_udi_handle[ppm_handle]) {
        diag_error("Open UDI ACM failed!");

        return;
    }

    g_ppm_chan_info.port_info[ppm_handle].usb_open_ok_num++;
    g_ppm_chan_info.port_info[ppm_handle].usb_open_ok_slice = mdrv_timer_get_normal_timestamp();

    /* 配置OM使用的USB通道缓存 */
    if (BSP_OK != bsp_acm_ioctl(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_RELLOC_READ_BUFF, &read_buff)) {
        diag_error("mdrv_udi_ioctl Failed\n");

        return;
    }

    /* 注册OM使用的USB读数据回调函数 */
    if (BSP_OK != PPM_UdiRegCallBackFun(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_SET_READ_CB, read_cb)) {
        diag_error("mdrv_udi_ioctl Failed\r\n");

        return;
    }

#ifdef BSP_CONFIG_PHONE_TYPE
    if (BSP_OK != PPM_UdiRegCallBackFun(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_SET_WRITE_CB, write_cb)) {
        diag_error("mdrv_udi_ioctl Failed\r\n");

        return;
    }
#else
    if (ppm_handle == OM_USB_IND_PORT_HANDLE) {
        if (BSP_OK != PPM_UdiRegCallBackFun(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_SET_WRITE_INFO_CB, write_cb)) {
            diag_error("mdrv_udi_ioctl Failed\r\n");
            return;
        }
    } else {
        if (BSP_OK != PPM_UdiRegCallBackFun(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_SET_WRITE_CB, write_cb)) {
            diag_error("mdrv_udi_ioctl Failed\r\n");

            return;
        }
    }
#endif

    if (BSP_OK != PPM_UdiRegCallBackFun(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_SET_EVT_CB, state_cb)) {
        diag_error("mdrv_udi_ioctl Failed\r\n");

        return;
    }

    if (BSP_OK != bsp_acm_ioctl(g_ppm_port_udi_handle[ppm_handle], ACM_IOCTL_WRITE_DO_COPY, NULL)) {
        diag_error("mdrv_udi_ioctl Failed\r\n");

        return;
    }

    g_ppm_chan_info.port_info[ppm_handle].usb_open_ok_num2++;
    g_ppm_chan_info.port_info[ppm_handle].usb_open_ok_slice2 = mdrv_timer_get_normal_timestamp();

    return;
}


u32 PPM_PortSendCheckParm(unsigned int handle, u8 *virt_addr, u8 *phy_addr, u32 len)
{
    if ((OM_USB_CFG_PORT_HANDLE == handle) || (OM_HSIC_CFG_PORT_HANDLE == handle)) {
        diag_ptr(EN_DIAG_PTR_PPM_PORTSEND, 1, handle, len);
    }

    if ((NULL == virt_addr) || (NULL == phy_addr)) {
        diag_error("Vir or Phy Addr is Null \n");
        return CPM_SEND_PARA_ERR;
    }

    if (!g_ppm_port_udi_handle[handle]) {
        g_ppm_chan_info.invalid_chan++;
        return CPM_SEND_ERR;
    }

    return ERR_MSP_SUCCESS;
}
/*
 * 函 数 名: ppm_port_send
 * 功能描述: 将输入的数据通过异步发送给PC侧
 * 输入参数:  enHandle: 发送端口句柄的枚举定义
 *            pucVirAddr:   数据虚地址
 *            pucPhyAddr:   数据实地址
 *            data_len: 数据长度
 * 输出参数: 无
 * 返 回 值: BSP_ERROR/BSP_OK
 */
u32 ppm_port_send(unsigned int handle, u8 *virt_addr, u8 *phy_addr, u32 data_len)
{
    s32 ret;
    struct acm_wr_async_info acm_info;
    u32 in_slice;
    u32 out_slice;
    u32 write_slice;

    ret = (s32)PPM_PortSendCheckParm(handle, virt_addr, phy_addr, data_len);
    if (ret) {
        return (u32)ret;
    }

    acm_info.virt_addr = (char *)virt_addr;
    acm_info.phy_addr = (char *)phy_addr;
    acm_info.size = data_len;
    acm_info.priv = NULL;

    g_ppm_chan_info.port_info[handle].usb_write_num1++;

    if (OM_USB_IND_PORT_HANDLE == handle) {
        g_ppm_chan_info.ind_debug_info.usb_send_len += data_len;
        g_usb_ind_pseudo_sync.last_ind_send.length = data_len;
        g_usb_ind_pseudo_sync.last_ind_send.virt_addr = virt_addr;
        g_usb_ind_pseudo_sync.last_ind_send.phy_addr = phy_addr;
        diag_system_debug_usb_len(data_len);
        diag_system_debug_send_data_end();
        diag_system_debug_send_usb_start();
    } else {
        g_ppm_chan_info.cnf_debug_info.usb_send_len += data_len;
    }

    in_slice = bsp_get_slice_value();

    /* 返回写入数据长度代表写操作成功 */
    ret = (s32)bsp_acm_ioctl(g_ppm_port_udi_handle[handle], ACM_IOCTL_WRITE_ASYNC, &acm_info);

    g_ppm_chan_info.port_info[handle].usb_write_num2++;

    out_slice = bsp_get_slice_value();

    if (in_slice > out_slice) {
        write_slice = (0xffffffff - in_slice) + out_slice;
    } else {
        write_slice = out_slice - in_slice;
    }

    if (write_slice > g_ppm_chan_info.port_info[handle].usb_write_max_time) {
        g_ppm_chan_info.port_info[handle].usb_write_max_time = write_slice;
    }

    if (MDRV_OK == ret) /* 当前发送成功 */
    {
        if (OM_USB_IND_PORT_HANDLE == handle) {
            diag_system_debug_usb_ok_len(data_len);
        }
        return CPM_SEND_AYNC;

    } else if (MDRV_OK > ret) /* 临时错误 */
    {
        if (OM_USB_IND_PORT_HANDLE == handle) {
            diag_system_debug_usb_fail_len(data_len);
        }
        g_ppm_chan_info.port_info[handle].usb_write_err_num++;
        g_ppm_chan_info.port_info[handle].usb_write_err_len += data_len;
        g_ppm_chan_info.port_info[handle].usb_write_err_val = (u32)ret;
        g_ppm_chan_info.port_info[handle].usb_write_err_time = bsp_get_slice_value();

        return CPM_SEND_FUNC_NULL; /* 对于临时错误，需要返回NULL丢弃数据 */
    } else                         /* 其他错误需要复位单板 */
    {
        if (OM_USB_IND_PORT_HANDLE == handle) {
            diag_system_debug_usb_fail_len(data_len);
        }

        return CPM_SEND_ERR;
    }
}

/*
 * 函 数 名:GU_OamPortWriteAsyCB
 * 功能描述:OM异步发送回调函数
 * 输入参数:enHandle:UDI句柄
 *          pucData:需要发送的数据内容
 *          lLen:数据长度
 * 输出参数:无
 * 返 回 值:无
 */
void ppm_port_write_asy_cb(unsigned int ppm_handle, ppm_buff_info_s *ret_Addr)
{
    u32 rls_len;
    mdrv_ppm_port_debug_info_s *cnf_debug_info;
    mdrv_ppm_port_debug_info_s *ind_debug_info;

    cnf_debug_info = &g_ppm_chan_info.cnf_debug_info;
    ind_debug_info = &g_ppm_chan_info.ind_debug_info;

    if (ret_Addr->length < 0) {
        rls_len = 0;
    } else {
        rls_len = (u32)ret_Addr->length;
    }

    /* 统计数据通道的吞吐率 */
    if (OM_USB_IND_PORT_HANDLE == ppm_handle) {
        diag_throughput_save(EN_DIAG_THRPUT_DATA_CHN_CB, rls_len);
        diag_system_debug_usb_free_len(rls_len);

        diag_system_debug_send_usb_end();
    }

    /* 伪同步接口，释放信号量 */
    if (OM_USB_IND_PORT_HANDLE == ppm_handle) {
        g_usb_ind_pseudo_sync.length = rls_len;
        g_usb_ind_pseudo_sync.pucAsyncCBData = ret_Addr->virt_addr;
        if (memcmp(ret_Addr, &g_usb_ind_pseudo_sync.last_ind_send, sizeof(ppm_buff_info_s))) {
            ret_Addr->length = 0;
            diag_error("last ind send len;%d, vir:%lx phy:%lx\n", g_usb_ind_pseudo_sync.last_ind_send.length,
                      (uintptr_t)g_usb_ind_pseudo_sync.last_ind_send.virt_addr, (uintptr_t)g_usb_ind_pseudo_sync.last_ind_send.phy_addr);
            diag_error("recv usb ind len;%d, vir:%lx phy:%lx\n", ret_Addr->length,
                      (uintptr_t)ret_Addr->virt_addr, (uintptr_t)ret_Addr->phy_addr);
        }

        ind_debug_info->usb_send_real_len += rls_len;
        scm_rls_ind_dst_buff(rls_len);
    } else if (OM_USB_CFG_PORT_HANDLE == ppm_handle) {
        g_usb_cfg_pseudo_sync.length = rls_len;
        g_usb_cfg_pseudo_sync.pucAsyncCBData = ret_Addr->virt_addr;

        cnf_debug_info->usb_send_real_len += rls_len;
        scm_rls_cnf_dst_buff(rls_len);
    }

    g_ppm_chan_info.port_info[ppm_handle].usb_write_cb_num++;

    return;
}

ppm_pseudo_sync_s *ppm_get_pseudo_sync_info(void)
{
    return &g_usb_ind_pseudo_sync;
}

mdrv_ppm_chan_debug_info_s * ppm_get_chan_debug_info(void)
{
    return &g_ppm_chan_info;
}

/*
 * 函 数 名: ppm_init_phy_port
 * 功能描述: 初始化物理通道
 * 输入参数: void
 * 输出参数: 无
 * 返 回 值: BSP_OK:成功，其他为失败
 * 修改历史:
 */
int ppm_init_phy_port(void)
{
    if (BSP_OK != ppm_port_init()) {
        diag_error("ppm_port_init failed\n");
        return BSP_ERROR;
    }

#if ((defined(FEATURE_HISOCKET)) && (FEATURE_HISOCKET == FEATURE_ON)) || (defined(FEATURE_SVLSOCKET))
    if (BSP_OK != ppm_socket_port_init()) {
        diag_crit("ppm socket not init\n");
    }
#endif

    if (BSP_OK != ppm_port_switch_init()) {
        diag_error("ppm_port_switch_init fail\n");
        return BSP_ERROR;
    }

    diag_crit("diag ppm init ok\n");
    return BSP_OK;
}

u32 ppm_port_init(void)
{
    (void)memset_s(&g_ppm_chan_info, sizeof(g_ppm_chan_info), 0, sizeof(g_ppm_chan_info));

    (void)memset_s(g_ppm_port_udi_handle, sizeof(g_ppm_port_udi_handle), 0, sizeof(g_ppm_port_udi_handle));
    spin_lock_init(&g_ppm_port_switch_spinlock);

    /* USB承载的虚拟端口通道的初始化 */
    ppm_usb_port_init();

    /* Vcom承载的虚拟端口通道的初始化 */
    ppm_vcom_port_init();


    return BSP_OK;
}

/*
 * 函 数 名: OmOpenLog
 * 功能描述: 打印当前OM通道的状态
 * 输入参数:
 * 输出参数:
 * 返 回 值:
 * 调用函数:
 * 被调函数:
 * 修改历史:
 */
void OmOpenLog(u32 ulFlag)
{
    g_ppm_debug_flag = ulFlag;

    return;
}


void ppm_port_info_show(unsigned int handle)
{
    diag_crit("Invalidchanel num is             %d\n", g_ppm_chan_info.invalid_chan);

    diag_crit("The Port Write num 1 is          %d\n", g_ppm_chan_info.port_info[handle].usb_write_num1);
    diag_crit("The Port Write num 2 is          %d\n", g_ppm_chan_info.port_info[handle].usb_write_num2);
    diag_crit("The Port Write Max Time is       0x%x\n", g_ppm_chan_info.port_info[handle].usb_write_max_time);

    diag_crit("The Port Write CB Num is         %d\n", g_ppm_chan_info.port_info[handle].usb_write_cb_num);

    diag_crit("The Port Write Err Time is       %d\n", g_ppm_chan_info.port_info[handle].usb_write_err_time);
    diag_crit("The Port Write Err Num  is       %d\n", g_ppm_chan_info.port_info[handle].usb_write_err_num);
    diag_crit("The Port Write Err Value is      0x%x\n", g_ppm_chan_info.port_info[handle].usb_write_err_val);
    diag_crit("The Port Write Err Len is        0x%x\n", g_ppm_chan_info.port_info[handle].usb_write_err_len);

    diag_crit("The Port In CB Num is            %d\n", g_ppm_chan_info.port_info[handle].usb_in_num);
    diag_crit("The Port In CB Time is           0x%x\n", g_ppm_chan_info.port_info[handle].usb_in_time);
    diag_crit("The Port Out CB Num is           %d\n", g_ppm_chan_info.port_info[handle].usb_out_num);
    diag_crit("The Port Out CB Time is          0x%x\n", g_ppm_chan_info.port_info[handle].usb_out_time);
    diag_crit("The Port State CB Err Num is     %d\n", g_ppm_chan_info.port_info[handle].usb_state_err_num);
    diag_crit("The Port State CB Err Time is    0x%x\n", g_ppm_chan_info.port_info[handle].usb_state_err_time);

    diag_crit("The Port Open num is            %d\n", g_ppm_chan_info.port_info[handle].usb_open_num);
    diag_crit("The Port Open slice is          0x%x\n", g_ppm_chan_info.port_info[handle].usb_open_slice);

    diag_crit("The Port Open OK num is         %d\n", g_ppm_chan_info.port_info[handle].usb_open_ok_num);
    diag_crit("The Port Open OK slice is       0x%x\n", g_ppm_chan_info.port_info[handle].usb_open_ok_slice);

    diag_crit("The Port Open OK2 num is        %d\n", g_ppm_chan_info.port_info[handle].usb_open_ok_num2);
    diag_crit("The Port Open OK2 slice is      0x%x\n", g_ppm_chan_info.port_info[handle].usb_open_ok_slice2);

    diag_crit("The Port Close num is           %d\n", g_ppm_chan_info.port_info[handle].usb_close_num);
    diag_crit("The Port Close slice is         0x%x\n", g_ppm_chan_info.port_info[handle].usb_close_slice);

    diag_crit("The Port Close OK num is        %d\n", g_ppm_chan_info.port_info[handle].usb_close_ok_num);
    diag_crit("The Port Close OK slice is      0x%x", g_ppm_chan_info.port_info[handle].usb_close_ok_slice);

    diag_crit("USB IND Pseudo sync fail num is   %d\n",
              g_ppm_chan_info.port_info[handle].usb_ind_sync_fail_num);
    diag_crit("USB IND Pseudo sync fail slice is 0x%x\n",
              g_ppm_chan_info.port_info[handle].usb_ind_sync_fail_slice);
    diag_crit("USB CFG Pseudo sync fail num is   %d\n",
              g_ppm_chan_info.port_info[handle].usb_cnf_sync_fail_num);
    diag_crit("USB CFG Pseudo sync fail slice is 0x%x\n",
              g_ppm_chan_info.port_info[handle].usb_cnf_sync_fail_slice);

    diag_crit("The Port UDI Handle Err num is %d\n", g_ppm_chan_info.port_info[handle].usb_udi_handle_err);
    diag_crit("The Port UDI Handle Get Buffer Err num is %d\n",
              g_ppm_chan_info.port_info[handle].usb_get_rd_data_err);
    diag_crit("The Port UDI Handle Comm Rcv Null Ptr num is %d\n",
              g_ppm_chan_info.port_info[handle].usb_udi_rcv_null_err);
    diag_crit("The Port UDI Handle Read Buffer Free Err num is %d\n",
              g_ppm_chan_info.port_info[handle].usb_udi_read_buf_free_err);

    diag_crit("The Port UDI Handle Total Rcv Pkt num is %d\n", g_ppm_chan_info.port_info[handle].usb_rcv_pkt_num);
    diag_crit("The Port UDI Handle Total Rcv Byte is %d.\n", g_ppm_chan_info.port_info[handle].usb_rcv_pkt_len);

    return;
}
EXPORT_SYMBOL(ppm_port_info_show);


void ppm_debug_port_info_show(void)
{
    diag_crit("IND ppm_discard_num %d, len %d; usb_send_err_num %d, Len %d; usb_send_num %d, len %d, reallen %d.\n",
              g_ppm_chan_info.ind_debug_info.ppm_discard_num, g_ppm_chan_info.ind_debug_info.ppm_discard_len,
              g_ppm_chan_info.ind_debug_info.usb_send_err_num, g_ppm_chan_info.ind_debug_info.usb_send_err_len,
              g_ppm_chan_info.ind_debug_info.usb_send_num, g_ppm_chan_info.ind_debug_info.usb_send_len,
              g_ppm_chan_info.ind_debug_info.usb_send_real_len);

    diag_crit("CNF ppm_discard_num %d, len %d; usb_send_err_num %d, Len %d; usb_send_num %d, len %d, Reallen %d.\n",
              g_ppm_chan_info.cnf_debug_info.ppm_discard_num, g_ppm_chan_info.cnf_debug_info.ppm_discard_len,
              g_ppm_chan_info.cnf_debug_info.usb_send_err_num, g_ppm_chan_info.cnf_debug_info.usb_send_err_len,
              g_ppm_chan_info.cnf_debug_info.usb_send_num, g_ppm_chan_info.cnf_debug_info.usb_send_len,
              g_ppm_chan_info.cnf_debug_info.usb_send_real_len);
}
EXPORT_SYMBOL(ppm_debug_port_info_show);

