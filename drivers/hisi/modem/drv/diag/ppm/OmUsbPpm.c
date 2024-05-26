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
 * 1 ͷ�ļ�����
 */
#include "OmUsbPpm.h"
#include <securec.h>
#include <product_config.h>
#include <mdrv_usb.h>
#include "scm_ind_src.h"
#include "scm_ind_dst.h"
#include "scm_cnf_src.h"
#include "scm_cnf_dst.h"
#include "diag_port_manager.h"
#include "diag_system_debug.h"
#include "OmCommonPpm.h"


/*
 * 2 ȫ�ֱ�������
 */

/*
 * 3 �ⲿ��������
 */

/*
 * 4 ����ʵ��
 */

/*
 * �� �� ��: ppm_usb_cfg_send_data
 * ��������: �����������ͨ��USB(CFG��)���͸�PC��
 * �������: pucVirAddr:   �������ַ
 *           pucPhyAddr:   ����ʵ��ַ
 *           data_len: ���ݳ���
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 */
u32 ppm_usb_cfg_send_data(u8 *virt_addr, u8 *phy_ddr, u32 len)
{
    return ppm_port_send(OM_USB_CFG_PORT_HANDLE, virt_addr, phy_ddr, len);
}

/*
 * �� �� ��: ppm_usb_cfg_port_close
 * ��������: USB���ص�OM CFG�˿��Ѿ���ʧ����Ҫ�ر�CFG�˿�
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_cfg_port_close(void)
{
    ppm_port_close_proc(OM_USB_CFG_PORT_HANDLE, CPM_CFG_PORT);

    return;
}

/*
 * �� �� ��: GU_OamUsbCfgStatusCB
 * ��������: ����ACPU���洦��USB����˿ڶϿ����OM���ӶϿ�
 * �������: enPortState:�˿�״̬
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_cfg_status_cb(ACM_EVT_E state)
{
    ppm_port_status(OM_USB_CFG_PORT_HANDLE, CPM_CFG_PORT, state);

    return;
}

/*
 * �� �� ��: GU_OamUsbCfgWriteDataCB
 * ��������: ���ڴ���USB���ص�OM CFG�˿ڵ��첽�������ݵĻص�
 * �������: pucData:   ��Ҫ���͵���������
 *           data_len: ���ݳ���
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_cfg_write_data_cb(char *virt_adddr, char *phy_addr, int len)
{
    ppm_buff_info_s buff;

    buff.virt_addr = virt_adddr;
    buff.phy_addr = phy_addr;
    buff.length = len;

    ppm_port_write_asy_cb(OM_USB_CFG_PORT_HANDLE, &buff);

    return;
}

/*
 * �� �� ��: GU_OamUsbCfgReadDataCB
 * ��������: ����ACPU��������USB���ص�OM CFG�˿�����ͨ��ICC���͸�OMģ��
 * �������: phy_port: ����˿�
 *           UdiHandle:�豸���
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 */
void ppm_usb_cfg_read_data_cb(void)
{
    ppm_read_port_data(CPM_CFG_PORT, g_ppm_port_udi_handle[OM_USB_CFG_PORT_HANDLE], OM_USB_CFG_PORT_HANDLE);
    return;
}

/*
 * �� �� ��: GU_OamUsbCfgPortOpen
 * ��������: ���ڳ�ʼ��USB���ص�OM CFG�˿�
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_cfg_port_open(void)
{
    ppm_read_port_data_init(CPM_CFG_PORT, OM_USB_CFG_PORT_HANDLE, ppm_usb_cfg_read_data_cb, ppm_usb_cfg_write_data_cb,
                         ppm_usb_cfg_status_cb);

    diag_crit("usb cfg port open\n");
    return;
}

/*
 * �� �� ��: ppm_usb_ind_status_cb
 * ��������: ����ACPU���洦��USB����˿ڶϿ����OM���ӶϿ�
 * �������: enPortState:   �˿�״̬
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_ind_status_cb(ACM_EVT_E state)
{
    ppm_port_status(OM_USB_IND_PORT_HANDLE, CPM_IND_PORT, state);

    return;
}

ppm_usb_debug_info_s g_ppm_usb_debug_info = {0};

static inline u32 Max_UsbProcTime(u32 oldtime, u32 newtime)
{
    return (oldtime >= newtime ? oldtime : newtime);
}
void ppm_query_usb_info(void *usb_info, u32 len)
{
    errno_t ret;
    ret = memcpy_s(usb_info, len, &g_ppm_usb_debug_info, sizeof(ppm_usb_debug_info_s));
    if (ret != EOK) {
        diag_error("memory copy error %x\n", ret);
    }
}
void ppm_clear_usb_debug_info(void)
{
    memset_s(&g_ppm_usb_debug_info, sizeof(ppm_usb_debug_info_s), 0, sizeof(ppm_usb_debug_info_s));
}
/*
 * �� �� ��  : GU_OamUsbIndWriteDataCB
 * ��������  : ���ڴ���USB OM�����ϱ��˿ڵ��첽�������ݵĻص�
 * �������  : pucData:   ��Ҫ���͵���������
 *             data_len: ���ݳ���
 * �������  : ��
 * �� �� ֵ  : ��
 */
#ifdef BSP_CONFIG_PHONE_TYPE
void ppm_usb_ind_write_data_cb(char *virt_addr, char *phy_addr, int len)
{
    ppm_buff_info_s buff;

    buff.virt_addr = virt_addr;
    buff.phy_addr = phy_addr;
    buff.length = len;

    ppm_port_write_asy_cb(OM_USB_IND_PORT_HANDLE, &buff);
    return;
}
#else
void ppm_usb_ind_write_data_cb(struct acm_write_info *acm_write_info)
{
    u32 delta_send_time = 0;
    u32 delta_cb_time = 0;
    ppm_buff_info_s buff;

    g_ppm_usb_debug_info.usb_cb_cnt++;

    if (acm_write_info == NULL) {
        diag_error("Param error\n");
        return;
    }

    buff.virt_addr = acm_write_info->virt_addr;
    buff.phy_addr = acm_write_info->phy_addr;
    buff.length = acm_write_info->size;

    delta_send_time = acm_write_info->complete_time - acm_write_info->submit_time;
    g_ppm_usb_debug_info.usb_send_time += delta_send_time;
    g_ppm_usb_debug_info.usb_max_send_time = Max_UsbProcTime(g_ppm_usb_debug_info.usb_max_send_time, delta_send_time);
    delta_cb_time = acm_write_info->done_time - acm_write_info->complete_time;
    g_ppm_usb_debug_info.usb_cb_time += delta_cb_time;
    g_ppm_usb_debug_info.usb_max_cb_time = Max_UsbProcTime(g_ppm_usb_debug_info.usb_max_cb_time,
                                                              delta_cb_time);
    ppm_port_write_asy_cb(OM_USB_IND_PORT_HANDLE, &buff);

    return;
}
#endif
/*
 * �� �� ��: ppm_usb_ind_port_open
 * ��������: ���ڳ�ʼ��USB���ص�OM�����ϱ��˿�
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_ind_port_open(void)
{
    ppm_read_port_data_init(CPM_IND_PORT, OM_USB_IND_PORT_HANDLE, NULL, ppm_usb_ind_write_data_cb, ppm_usb_ind_status_cb);

    diag_crit("usb ind port open\n");
    return;
}

/*
 * �� �� ��: ppm_usb_ind_port_close
 * ��������: USB���ص�OM�����ϱ��˿��Ѿ���ʧ����Ҫ�ر�USB�˿�
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_usb_ind_port_close(void)
{
    ppm_port_close_proc(OM_USB_IND_PORT_HANDLE, CPM_IND_PORT);

    return;
}

/*
 * �� �� ��: ppm_usb_ind_send_data
 * ��������: �����������ͨ��USB�����ϱ��˿ڷ��͸�PC��
 * �������: pucVirAddr:   �������ַ
 *           pucPhyAddr:   ����ʵ��ַ
 *           data_len: ���ݳ���
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 */
u32 ppm_usb_ind_send_data(u8 *virt_addr, u8 *phy_addr, u32 len)
{
    return ppm_port_send(OM_USB_IND_PORT_HANDLE, virt_addr, phy_addr, len);
}

/*
 * �� �� ��: ppm_usb_cfg_port_init
 * ��������: ����USB��OM���ö˿�ͨ���ĳ�ʼ��
 * �������: ��
 * �������: ��
 * �� �� ֵ: BSP_ERROR -- ��ʼ��ʧ��
 *           BSP_OK  -- ��ʼ���ɹ�
 */
u32 ppm_usb_cfg_port_init(void)
{
    mdrv_usb_reg_enablecb((usb_udi_enable_cb)ppm_usb_cfg_port_open);

    mdrv_usb_reg_disablecb((usb_udi_disable_cb)ppm_usb_cfg_port_close);

    cpm_phy_send_reg(CPM_CFG_PORT, (cpm_send_func)ppm_usb_cfg_send_data);

    return BSP_OK;
}

/*
 * �� �� ��: ppm_usb_ind_port_init
 * ��������: ����USB ��OM�����ϱ��˿�ͨ���ĳ�ʼ��
 * �������: ��
 * �������: ��
 * �� �� ֵ: BSP_ERROR -- ��ʼ��ʧ��
 *           BSP_OK  -- ��ʼ���ɹ�
 */
u32 ppm_usb_ind_port_init(void)
{
    mdrv_usb_reg_enablecb((usb_udi_enable_cb)ppm_usb_ind_port_open);

    mdrv_usb_reg_disablecb((usb_udi_disable_cb)ppm_usb_ind_port_close);

    cpm_phy_send_reg(CPM_IND_PORT, (cpm_send_func)ppm_usb_ind_send_data);

    return BSP_OK;
}

/*
 * �� �� ��: ppm_usb_port_init
 * ��������: USB���ص�����˿�ͨ���ĳ�ʼ��:����OM IND��OM CNF�ȶ˿�
 * �������: ��
 * �������: ��
 * �� �� ֵ: BSP_OK   - ��ʼ���ɹ�
 *           BSP_ERROR  - ��ʼ��ʧ��
 */
u32 ppm_usb_port_init(void)
{
    /* USB ���ص�OM�����ϱ��˿ڵĳ�ʼ�� */
    if (BSP_OK != ppm_usb_ind_port_init()) {
        return (u32)BSP_ERROR;
    }

    /* USB ���ص�OM���ö˿ڵĳ�ʼ�� */
    if (BSP_OK != ppm_usb_cfg_port_init()) {
        return (u32)BSP_ERROR;
    }

    return BSP_OK;
}

