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
#include "diag_port_manager.h"
#include <linux/module.h>
#include <mdrv.h>
#include <mdrv_diag_system.h>
#include <bsp_nvim.h>
#include <bsp_socp.h>
#include <nv_stru_drv.h>
#include <acore_nv_stru_drv.h>
#include "diag_system_debug.h"
#include "bsp_version.h"


/*
 * 2 全局变量定义
 */

cpm_phy_port_cfg_s g_cpm_phy_port_cfg[CPM_PORT_BUTT - CPM_IND_PORT];
cpm_logic_port_cfg_s g_cpm_logic_port_cfg[CPM_COMM_BUTT];

/* 端口配置全局变量 */
DIAG_CHANNLE_PORT_CFG_STRU g_diag_port_cfg;
unsigned int g_original_port;
/* 逻辑端口发送错误统计 */
cpm_com_port_send_err_info_s g_cpm_send_error_info = {0};

/* 物理端口发送错误统计 */
cpm_com_port_rcv_err_info_s g_cpm_rcv_error_info = {0};

/*
 * 3 函数体申明
 */

/*
 * 4 函数体定义
 */

/*
 * 函 数 名: cpm_phy_send_reg
 * 功能描述: 提供给外部的注册函数，用来物理通道接收到数据的处理
 * 输入参数: enPhyPort：  注册的物理通道号
 *           pRecvFunc：  数据接收函数
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_phy_send_reg(unsigned int phy_port, cpm_send_func send_func)
{
    if (CPM_PORT_BUTT > phy_port) {
        CPM_PHY_SEND_FUNC(phy_port - CPM_IND_PORT) = send_func;
    }

    return;
}

/*
 * 函 数 名: cpm_logic_rcv_reg
 * 功能描述: 给逻辑通道注册接收函数
 * 输入参数: enLogicPort： 注册的逻辑通道号
 *           pRecvFunc：   数据接收函数
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_logic_rcv_reg(unsigned int logic_port, cpm_rcv_cb rcv_func)
{
    if (CPM_COMM_BUTT > logic_port) {
        CPM_LOGIC_RCV_FUNC(logic_port) = rcv_func;
    }

    return;
}

/*
 * 函 数 名: cpm_query_phy_port
 * 功能描述: 查询当前逻辑通道使用的物理端口
 * 输入参数: enLogicPort：  逻辑通道号
 * 输出参数: 无
 * 返 回 值: 物理通道号
 */
unsigned int cpm_query_phy_port(unsigned int logic_port)
{
    return CPM_LOGIC_PHY_PORT(logic_port);
}

/*
 * 函 数 名: cpm_connect_ports
 * 功能描述: 将物理通道和逻辑通道连接上
 * 输入参数: enPhyPort：    物理通道号
 *           enLogicPort：  逻辑通道号
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_connect_ports(unsigned int phy_port, unsigned int logic_port)
{
    if ((CPM_PORT_BUTT <= phy_port) || (CPM_COMM_BUTT <= logic_port)) {
        return;
    }

    /* 连接发送通道 */
    CPM_LOGIC_SEND_FUNC(logic_port) = CPM_PHY_SEND_FUNC(phy_port - CPM_IND_PORT);

    /* 连接接收通道 */
    CPM_PHY_RCV_FUNC(phy_port - CPM_IND_PORT) = CPM_LOGIC_RCV_FUNC(logic_port);

    /* 将物理发送函数注册给逻辑通道 */
    CPM_LOGIC_PHY_PORT(logic_port) = phy_port;

    return;
}

/*
 * 函 数 名: cpm_disconnect_ports
 * 功能描述: 断开物理通道和逻辑通道连接
 * 输入参数: enPhyPort：    物理通道号
 *           enLogicPort：  逻辑通道号
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_disconnect_ports(unsigned int phy_port, unsigned int logic_port)
{
    if ((CPM_PORT_BUTT <= phy_port) || (CPM_COMM_BUTT <= logic_port)) {
        return;
    }

    /* 假如当前逻辑通道并没有使用此物理通道，则不用处理 */
    if (phy_port != CPM_LOGIC_PHY_PORT(logic_port)) {
        return;
    }

    /* 断开接收通道 */
    CPM_PHY_RCV_FUNC(phy_port - CPM_IND_PORT) = NULL;

    /* 断开发送通道 */
    CPM_LOGIC_SEND_FUNC(logic_port) = NULL;
    CPM_LOGIC_PHY_PORT(logic_port) = CPM_PORT_BUTT;

    return;
}

void cpm_set_original_port(unsigned int port)
{
    g_original_port = port;
    return;
}

unsigned int cpm_get_original_port(void)
{
    return g_original_port;
}
EXPORT_SYMBOL(cpm_get_original_port);

int cpm_port_associate_init(void)
{
    int ret;
    const bsp_version_info_s *version_info = bsp_get_version_info();

    if (version_info == NULL) {
        diag_error("get version info is null\n");
        return BSP_ERROR;
    }
    if ((version_info->plat_type == PLAT_ASIC) ||
        (version_info->plat_type == PLAT_EMU)  ||
        (version_info->plat_type == PLAT_FPGA)) {
        ret = cpm_asic_port_associate_init();
    } else {
        ret = cpm_hybrid_port_associate_init();
    }

    return ret;
}

/*
 * 函 数 名: cpm_asic_port_associate_init
 * 功能描述: hybrid平台根据端口类型关联物理端口和逻辑端口
 			 hybrid平台直接将逻辑端口与SOCKET端口对接
 * 输入参数: void
 * 输出参数: 无
 * 返 回 值: BSP_OK:成功，其他为失败
 */
int cpm_hybrid_port_associate_init(void)
{
    cpm_connect_ports(CPM_WIFI_AT_PORT, CPM_AT_COMM);
    cpm_connect_ports(CPM_WIFI_OM_IND_PORT, CPM_OM_IND_COMM);
    cpm_connect_ports(CPM_WIFI_OM_CFG_PORT, CPM_OM_CFG_COMM);

	bsp_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);

	diag_crit("diag port manager (hybrid) init ok\n");
    return BSP_OK;
}
/*
 * 函 数 名: cpm_asic_port_associate_init
 * 功能描述: asic平台根据端口类型关联物理端口和逻辑端口
 * 输入参数: void
 * 输出参数: 无
 * 返 回 值: BSP_OK:成功，其他为失败
 */
int cpm_asic_port_associate_init(void)
{
    u32 i;

    for (i = 0; i < CPM_COMM_BUTT; i++) {
        g_cpm_logic_port_cfg[i].phy_port = CPM_PORT_BUTT;
    }


    /* 读取OM的物理输出通道 */
    if (NV_OK != bsp_nvm_read(NV_ID_DRV_DIAG_PORT, (u8 *)&g_diag_port_cfg, sizeof(DIAG_CHANNLE_PORT_CFG_STRU))) {
        return BSP_ERROR;
    }

    /* 检测参数 */
    if (CPM_OM_PORT_TYPE_USB == g_diag_port_cfg.enPortNum) {
        cpm_connect_ports(CPM_CFG_PORT, CPM_OM_CFG_COMM);
        cpm_connect_ports(CPM_IND_PORT, CPM_OM_IND_COMM);
    } else if (CPM_OM_PORT_TYPE_VCOM == g_diag_port_cfg.enPortNum) {
        cpm_connect_ports(CPM_VCOM_CFG_PORT, CPM_OM_CFG_COMM);
        cpm_connect_ports(CPM_VCOM_IND_PORT, CPM_OM_IND_COMM);
    } else if (CPM_OM_PORT_TYPE_WIFI == g_diag_port_cfg.enPortNum) {
        cpm_connect_ports(CPM_WIFI_AT_PORT, CPM_AT_COMM);
        cpm_connect_ports(CPM_WIFI_OM_IND_PORT, CPM_OM_IND_COMM);
        cpm_connect_ports(CPM_WIFI_OM_CFG_PORT, CPM_OM_CFG_COMM);
    }
    /* NV项不正确时按USB输出处理 */
    else {
        cpm_connect_ports(CPM_CFG_PORT, CPM_OM_CFG_COMM);
        cpm_connect_ports(CPM_IND_PORT, CPM_OM_IND_COMM);

        g_diag_port_cfg.enPortNum = CPM_OM_PORT_TYPE_USB;
    }

    cpm_set_original_port(g_diag_port_cfg.enPortNum);
    diag_crit("port_num = 0x%x\n", g_diag_port_cfg.enPortNum);

    /* 如果当前连接为USB输出，需要设置SOCP默认超时 */
    if (g_diag_port_cfg.enPortNum == CPM_OM_PORT_TYPE_USB) {
        bsp_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);
    }

    diag_crit("diag port manager (asic) init ok\n");
    return BSP_OK;
}
/*
 * 函 数 名: cpm_com_send
 * 功能描述: 发送数据函数，提供给逻辑通道使用
 * 输入参数: enLogicPort：逻辑通道号
 *           pucVirData:  数据虚拟地址
 *           pucPHYData:  数据物理地址
 *           pucData：    发送数据的指针
 *           length:       发送数据的长度
 * 输出参数: 无
 * 返 回 值: BSP_OK:成功，其他为失败
 */
u32 cpm_com_send(unsigned int logic_port, u8 *virt_addr, u8 *phy_addr, u32 len)
{
    cpm_send_func func = NULL;
    
    if (CPM_OM_CFG_COMM == logic_port) {
        diag_ptr(EN_DIAG_PTR_CPM_COMSEND, 0, 0, 0);
    }

    /* 参数检测 */
    if (CPM_COMM_BUTT <= logic_port) {
        g_cpm_send_error_info.port_err++;

        return CPM_SEND_PARA_ERR;
    }

    if ((NULL == virt_addr) || (0 == len)) {
        g_cpm_send_error_info.cpm_send_err_info[logic_port].para_err++;

        return CPM_SEND_PARA_ERR;
    }

    func = CPM_LOGIC_SEND_FUNC(logic_port);
    if (func == NULL) {
        g_cpm_send_error_info.cpm_send_err_info[logic_port].null_err++;

        return CPM_SEND_FUNC_NULL;
    }

    return func(virt_addr, phy_addr, len);
}

/*
 * 函 数 名: cpm_com_receive
 * 功能描述: 接收数据函数，提供给物理通道使用
 * 输入参数: enPhyPort：  物理通道号
 *           pucData：    接收数据的指针
 *           length:       接收数据的长度
 * 输出参数: 无
 * 返 回 值: BSP_OK:成功，其他为失败
 */
u32 cpm_com_receive(unsigned int phy_port, u8 *addr, u32 len)
{
    cpm_rcv_cb func = NULL;
    /* 参数检测 */
    if (CPM_PORT_BUTT <= phy_port) {
        g_cpm_rcv_error_info.port_err++;
        diag_ptr(EN_DIAG_PTR_CPM_ERR1, 0, 0, 0);
        return (u32)ERR_MSP_INVALID_PARAMETER;
    }

    if ((NULL == addr) || (0 == len)) {
		diag_error("data error, len=0x%x\n", len);
        g_cpm_rcv_error_info.cpm_rcv_err_info[phy_port - CPM_IND_PORT].para_err++;
        diag_ptr(EN_DIAG_PTR_CPM_ERR2, 0, 0, 0);
        return (u32)ERR_MSP_PARA_NULL;
    }

    func = CPM_PHY_RCV_FUNC(phy_port - CPM_IND_PORT);
    if (func == NULL) {
        diag_error("cpm_com_receive The Phy Port %d Rec Func is NULL\n", (s32)phy_port);
        g_cpm_rcv_error_info.cpm_rcv_err_info[phy_port - CPM_IND_PORT].null_err++;
        diag_ptr(EN_DIAG_PTR_CPM_ERR3, 0, 0, 0);

        return (u32)ERR_MSP_DIAG_CB_NULL_ERR;
    }

    diag_ptr(EN_DIAG_PTR_CPM_COMRCV, len, 0, 0);

    return func(addr, len);
}

/*
 * 函 数 名: cpm_show
 * 功能描述: 显示当前的逻辑和物理端口对应关系
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_show(void)
{
    unsigned int phy_port;
    unsigned int logic_port;

    (void)diag_crit("cpm_show The Logic and Phy Relation is :");

    for (logic_port = CPM_AT_COMM; logic_port < CPM_COMM_BUTT; logic_port++) {
        phy_port = cpm_query_phy_port(logic_port);

        if (CPM_OM_IND_COMM == logic_port) {
            diag_crit("The Logic Port %d OM IND is connnect PHY ", logic_port);
        } else if (CPM_OM_CFG_COMM == logic_port) {
            diag_crit("The Logic Port %d OM CFG is connnect PHY ", logic_port);
        } else {
            diag_crit("The Logic Port %d        is connnect PHY ", logic_port);
        }

        if ((CPM_IND_PORT == phy_port) || (CPM_CFG_PORT == phy_port)) {
            diag_crit("Port %d(USB Port).", phy_port);
        } else if ((CPM_WIFI_OM_IND_PORT == phy_port) || (CPM_WIFI_OM_CFG_PORT == phy_port)) {
            diag_crit("Port %d(socket).", phy_port);
        } else if ((CPM_VCOM_IND_PORT == phy_port) || (CPM_VCOM_CFG_PORT == phy_port)) {
            diag_crit("Port %d(netlink).", phy_port);
        } else {
            diag_crit("Port %d.", phy_port);
        }
    }

    (void)diag_crit("cpm_show The Phy Info is :\n");
    return;
}
EXPORT_SYMBOL(cpm_show);

/*
 * 函 数 名: cpm_com_err_show
 * 功能描述: 显示当前的逻辑和物理端口错误显示
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void cpm_com_err_show(void)
{
    unsigned int logic_port;
    unsigned int phy_port;

    (void)diag_crit("cpm_com_err_show:");

    (void)diag_crit("Logic Port Err Times: %d", g_cpm_send_error_info.port_err);

    for (logic_port = 0; logic_port < CPM_COMM_BUTT; logic_port++) {
        diag_crit("Logic %d Port Para Err Times: %d", logic_port,
                  g_cpm_send_error_info.cpm_send_err_info[logic_port].para_err);
        diag_crit("Logic %d Port Null Ptr Times: %d", logic_port,
                  g_cpm_send_error_info.cpm_send_err_info[logic_port].null_err);
    }

    diag_crit("Phy Port Err Times: %d", g_cpm_rcv_error_info.port_err);

    for (phy_port = 0; phy_port < (CPM_PORT_BUTT - CPM_IND_PORT); phy_port++) {
        diag_crit("Phy %d Port Para Err Times: %d", phy_port, g_cpm_rcv_error_info.cpm_rcv_err_info[phy_port].para_err);
        diag_crit("Phy %d Port Null Ptr Times: %d", phy_port, g_cpm_rcv_error_info.cpm_rcv_err_info[phy_port].null_err);
    }

    return;
}
EXPORT_SYMBOL(cpm_com_err_show);

