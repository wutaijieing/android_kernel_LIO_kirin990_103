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
#include "OmPortSwitch.h"
#include <securec.h>
#include <osl_spinlock.h>
#include <osl_types.h>
#include <osl_sem.h>
#include <bsp_icc.h>
#include <nv_stru_drv.h>
#include <bsp_slice.h>
#include "diag_port_manager.h"
#include "diag_system_debug.h"
#include "scm_common.h"


/*
 * 2 ȫ�ֱ�������
 */
ppm_port_switch_nv_info_s ppm_port_switch_info;
/* �˿��л���Ϣ�����ݽṹ�� */
ppm_port_cfg_info_s g_stPpmPortSwitchInfo;

extern DIAG_CHANNLE_PORT_CFG_STRU g_diag_port_cfg;
extern spinlock_t g_ppm_port_switch_spinlock;

/*
 * �� �� ��: ppm_ap_nv_icc_read_cb
 * ��������: ����ע��ICC�Ļص�����
 * �������: id: iccͨ����
 *                       len: ���ݳ���
 *                       context: ��������
 * �������: ��
 * �� �� ֵ: BSP_OK/BSP_ERROR
 */
static s32 ppm_ap_nv_icc_read_cb(u32 id, u32 len, void *context)
{
    ppm_port_switch_nv_info_s ppm_port_switch_back = {};
    u32 msg_len = sizeof(ppm_port_switch_back);
    s32 ret = 0;

    if (len != msg_len) {
        diag_error("readcb_len err, len=%d, msg_len=%d\n", len, msg_len);
        return ERR_MSP_INALID_LEN_ERROR;
    }
    ret = bsp_icc_read(id, (u8 *)(&ppm_port_switch_back), len);
    if (msg_len != (u32)ret) {
        diag_error("icc_read err(0x%x), msg_len=%d\n", ret, len);
        return ERR_MSP_INALID_LEN_ERROR;
    }
    if (ppm_port_switch_back.msg_id != PPM_MSGID_PORT_SWITCH_NV_C2A) {
        diag_error("msg_id err, msg_id=%d\n", ppm_port_switch_back.msg_id);
    }
    ppm_port_switch_info.ret = ppm_port_switch_back.ret;
    if (ppm_port_switch_back.ret) {
        diag_error("NV_Write fail,sn=%d\n", ppm_port_switch_back.sn);
    }

    return BSP_OK;
}


u32 ppm_set_ind_mode(void)
{
    u32 compress_state;
    u32 ret;

    /* Ϊ�˹��USB���ʱ��������ʱд���޷����ӹ���,�л���USB���ʱ��Ҫ��������SOCP�ĳ�ʱ�жϵ�Ĭ��ֵ */
    if (CPM_OM_PORT_TYPE_USB == g_diag_port_cfg.enPortNum) {
        compress_state = mdrv_socp_compress_status();
        if (COMPRESS_ENABLE_STATE == compress_state) {
            ret = mdrv_socp_compress_disable(SOCP_CODER_DST_OM_IND);
            if (BSP_OK != ret) {
                diag_error("deflate disable failed(0x%x)\n", ret);
                return (u32)ret;
            }
        }
        ret = bsp_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);
        if (BSP_OK != ret) {
            diag_error("set socp ind mode failed(0x%x)\n", ret);
            return (u32)ret;
        }
    }
    return ERR_MSP_SUCCESS;
}

u32 ppm_phy_port_switch(u32 phy_port)
{
    unsigned int cfg_port;
    unsigned int ind_port;
    unsigned long lock;
    bool send_flag;
    unsigned int original_port;

    cfg_port = cpm_query_phy_port(CPM_OM_CFG_COMM);
    ind_port = cpm_query_phy_port(CPM_OM_IND_COMM);

    send_flag = false;

    original_port = cpm_get_original_port();

    spin_lock_irqsave(&g_ppm_port_switch_spinlock, lock);

    /* �л���VCOM��� */
    if (CPM_OM_PORT_TYPE_VCOM == phy_port) {
        /* ��ǰ��USB��� */
        if ((CPM_CFG_PORT == cfg_port) && (CPM_IND_PORT == ind_port)) {
            send_flag = true;

            if ((CPM_CFG_PORT == cfg_port) && (CPM_IND_PORT == ind_port)) {
                /* ��Ҫ�Ͽ����� */
                cpm_disconnect_ports(CPM_CFG_PORT, CPM_OM_CFG_COMM);
                cpm_disconnect_ports(CPM_IND_PORT, CPM_OM_IND_COMM);
            } else if ((CPM_WIFI_OM_CFG_PORT == cfg_port) && (CPM_WIFI_OM_IND_PORT == ind_port)) {
                cpm_disconnect_ports(CPM_WIFI_OM_CFG_PORT, CPM_OM_CFG_COMM);
                cpm_disconnect_ports(CPM_WIFI_OM_IND_PORT, CPM_OM_IND_COMM);
            } else {
                diag_error("exceptional cfg port = 0x%x, ind port=0x%x\n", cfg_port, ind_port);
            }
        }

        /* ��ǰOM��VCOM�ϱ� */
        cpm_connect_ports(CPM_VCOM_CFG_PORT, CPM_OM_CFG_COMM);
        cpm_connect_ports(CPM_VCOM_IND_PORT, CPM_OM_IND_COMM);

        g_diag_port_cfg.enPortNum = CPM_OM_PORT_TYPE_VCOM;
    }
    /* �л���USB || SOCKET��� */
    else {
        /* ��ǰ��VCOM��� */
        if ((CPM_VCOM_CFG_PORT == cfg_port) && (CPM_VCOM_IND_PORT == ind_port)) {
            /* �Ͽ����� */
            send_flag = true;

            cpm_disconnect_ports(CPM_VCOM_CFG_PORT, CPM_OM_CFG_COMM);
            cpm_disconnect_ports(CPM_VCOM_IND_PORT, CPM_OM_IND_COMM);
        }

        if ((original_port == CPM_OM_PORT_TYPE_USB) || (original_port == CPM_OM_PORT_TYPE_VCOM)) {
            /* OM��USB�ϱ� */
            cpm_connect_ports(CPM_CFG_PORT, CPM_OM_CFG_COMM);
            cpm_connect_ports(CPM_IND_PORT, CPM_OM_IND_COMM);

            g_diag_port_cfg.enPortNum = CPM_OM_PORT_TYPE_USB;
            diag_crit("change port to 0x%x\n", g_diag_port_cfg.enPortNum);
        } else if (original_port == CPM_OM_PORT_TYPE_WIFI) {
            /* OM��Socket�ϱ� */
            cpm_connect_ports(CPM_WIFI_OM_CFG_PORT, CPM_OM_CFG_COMM);
            cpm_connect_ports(CPM_WIFI_OM_IND_PORT, CPM_OM_IND_COMM);
            
            g_diag_port_cfg.enPortNum = CPM_OM_PORT_TYPE_WIFI;
            diag_crit("change port to 0x%x\n", g_diag_port_cfg.enPortNum); 
        } else {
            diag_error("exceptional port = 0x%x\n", g_diag_port_cfg.enPortNum);
        }

    }

    spin_unlock_irqrestore(&g_ppm_port_switch_spinlock, lock);

    return send_flag;
}
u32 ppm_write_port_nv(void)
{
    u32 ret;

    ppm_port_switch_info.msg_id = PPM_MSGID_PORT_SWITCH_NV_A2C;
    ppm_port_switch_info.sn++;
    /* Ĭ�ϴ��󣬸��ݷ���ֵ�鿴�Ƿ�дNV�ɹ� */
    ppm_port_switch_info.ret = (u32)BSP_ERROR;
    ppm_port_switch_info.len = sizeof(ppm_port_switch_info.data);
    memcpy_s((void *)(&ppm_port_switch_info.data), sizeof(ppm_port_switch_info.data), (void *)(&g_diag_port_cfg),
             sizeof(g_diag_port_cfg));
    ret = (u32)bsp_icc_send(ICC_CPU_MODEM, (ICC_CHN_IFC << 16 | IFC_RECV_FUNC_PPM_NV), (u8 *)(&ppm_port_switch_info),
                            sizeof(ppm_port_switch_info));
    if (ret != sizeof(ppm_port_switch_info)) {
        diag_error("bsp_icc_send fail(0x%x)\n", ret);
        return (u32)ERR_MSP_INALID_LEN_ERROR;
    }

    return ERR_MSP_SUCCESS;
}
/*
 * �� �� ��: ppm_port_switch
 * ��������: �ṩ��NAS���ж˿��л�
 * �������: phy_port: ���л�����˿�ö��ֵ
 *           ulEffect:�Ƿ�������Ч
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 * �޸���ʷ:
 */
u32 ppm_port_switch(u32 phy_port, bool effect)
{
    u32 ret;

    if ((CPM_OM_PORT_TYPE_USB != phy_port) && (CPM_OM_PORT_TYPE_VCOM != phy_port)) {
        diag_error("phy_port error, port=%d\n", phy_port);

        g_stPpmPortSwitchInfo.port_type_err++;

        return (u32)ERR_MSP_INVALID_PARAMETER;
    }

    diag_crit("phy_port:%s ulEffect:%s.\n", (phy_port == CPM_OM_PORT_TYPE_USB) ? "USB" : "VCOM",
              (effect == true) ? "TRUE" : "FALSE");

    /* �л��Ķ˿��뵱ǰ�˿�һ�²��л� */
    if (phy_port == g_diag_port_cfg.enPortNum) {
        /* Ϊ�˹��USB���ʱ��������ʱд���޷����ӹ���,�л���USB���ʱ��Ҫ��������SOCP�ĳ�ʱ�жϵ�Ĭ��ֵ */
        if (CPM_OM_PORT_TYPE_USB == g_diag_port_cfg.enPortNum) {
            ret = bsp_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);
            if (BSP_OK != ret) {
                diag_error("set socp ind mode failed(0x%x)\n", ret);
                return (u32)ret;
            }
        }
        diag_crit("Set same port, don't need to do anything.\n");

        return BSP_OK;
    }

    g_stPpmPortSwitchInfo.start_slice = bsp_get_slice_value();

    /* port switch */
    if (ppm_phy_port_switch(phy_port)) {
        ppm_disconnect_port(OM_LOGIC_CHANNEL_CNF);
    }

    /* Ϊ�˹��USB���ʱ��������ʱд���޷����ӹ���,�л���USB���ʱ��Ҫ��������SOCP�ĳ�ʱ�жϵ�Ĭ��ֵ */
    ret = ppm_set_ind_mode();
    if (ret) {
        return ret;
    }

    g_stPpmPortSwitchInfo.switch_succ++;
    g_stPpmPortSwitchInfo.end_slice = bsp_get_slice_value();

    /* write Nv */
    if (true == effect) {
        ret = ppm_write_port_nv();
        if (ret) {
            return ret;
        }
    }
    diag_crit("PPM set port success!\n");

    return BSP_OK;
}
/*
 * �� �� ��: ppm_query_port
 * ��������: �ṩ��NAS����Log�˿ڲ�ѯ
 * �������: ��
 * �������: pulLogPort��ǰLog����˿�
 * �� �� ֵ: BSP_ERROR/BSP_OK
 */
u32 ppm_query_port(u32 *phy_port)
{
    if (NULL == phy_port) {
        diag_error("para is NULL\n");
        return (u32)ERR_MSP_PARA_NULL;
    }
    *phy_port = g_diag_port_cfg.enPortNum;
    if ((CPM_OM_PORT_TYPE_USB != *phy_port) && (CPM_OM_PORT_TYPE_VCOM != *phy_port)) {
        return (u32)ERR_MSP_INVALID_PARAMETER;
    }

    return BSP_OK;
}

int ppm_port_switch_init(void)
{
    u32 ret = 0;
    (void)memset_s(&g_stPpmPortSwitchInfo, sizeof(g_stPpmPortSwitchInfo), 0, sizeof(g_stPpmPortSwitchInfo));

    ret = bsp_icc_event_register((ICC_CHN_IFC << 16 | IFC_RECV_FUNC_PPM_NV), (read_cb_func)ppm_ap_nv_icc_read_cb,
                                 (void *)NULL, (write_cb_func)NULL, (void *)NULL);
    if (ret) {
        diag_error("icc_event_register fail, channel id:0x%x ret:0x%x\n", (ICC_CHN_IFC << 16 | IFC_RECV_FUNC_PPM_NV),
                   ret);
        return ret;
    }
    ppm_port_switch_info.sn = 0;


    return BSP_OK;
}


void ppm_port_switch_info_show(void)
{
    diag_crit("Port Type Err num is %d\n", g_stPpmPortSwitchInfo.port_type_err);

    diag_crit("Port Switch Fail time is %d\n", g_stPpmPortSwitchInfo.switch_fail);

    diag_crit("Port Switch Success time is %d\n", g_stPpmPortSwitchInfo.switch_succ);

    diag_crit("Port Switch Start slice is 0x%x\n", g_stPpmPortSwitchInfo.start_slice);

    diag_crit("Port Switch End slice is 0x%x.\n", g_stPpmPortSwitchInfo.end_slice);

    return;
}
EXPORT_SYMBOL(ppm_port_switch_info_show);

EXPORT_SYMBOL(ppm_port_switch);

