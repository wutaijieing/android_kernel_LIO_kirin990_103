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
#include "OmVcomPpm.h"
#include <product_config.h>
#include <mdrv_diag_system.h>
#include <securec.h>
#include <bsp_slice.h>
#include "diag_system_debug.h"
#include "diag_port_manager.h"
#include "OmPortSwitch.h"


/*
 * 2 ȫ�ֱ�������
 */

/* ���ڼ�¼ VCOM ͨ�����͵�ͳ����Ϣ */
mdrv_ppm_vcom_debug_info_s g_ppm_vcom_debug_info[3] = {};

/*
 * �� �� ��: ppm_vcom_cfg_send_data
 * ��������: ��VCOM�˿ڷ�����������
 * �������: pucVirAddr:   �������ַ
 *           pucPhyAddr:   ����ʵ��ַ
 *           data_len: ���ݳ���
 * �������: ��
 * �� �� ֵ: CPM_SEND_ERR/CPM_SEND_OK
 */
u32 ppm_vcom_cfg_send_data(u8 *virt_addr, u8 *phy_addr, u32 len)
{
    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_num++;
    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_len += len;

    if (0 != PPM_VCOM_SEND_DATA(PPM_VCOM_CHAN_CTRL, virt_addr, len, PPM_VCOM_DATA_MODE_TRANSPARENT)) {
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_num++;
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_len += len;

        diag_error("vcom cnf cmd failed, cnf sum len 0x%x, cnf err len 0x%x.\n",
                   g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_len,
                   g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_len);

        return CPM_SEND_ERR;
    }

    /* ���ֻ��������ʱ��������ʱ�ϱ����Ҵ�ӡ�������У����������ӡ */
    diag_crit("vcom cnf cmd success, cnf sum len 0x%x, cnf err len 0x%x.\n",
              g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_len,
              g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_len);

    return CPM_SEND_OK;
}

/*
 * �� �� ��: PPM_VComCfgEvtCB
 * ��������: ����VCOMͨ���򿪹رջص�
 * �������: chan_id :ͨ����
 *           ulEvent:   �򿪻��߹ر�
 * �������: ��
 * �� �� ֵ: CPM_SEND_ERR/CPM_SEND_OK
 */
void ppm_vcom_event_cb(u32 chan, u32 event)
{
    unsigned int  logic_port;
    bool send_flag = false;

    diag_crit("ppm_vcom_event_cb Chan:%s Event:%s.\n", (chan == PPM_VCOM_CHAN_CTRL) ? "cnf" : "ind",
              (event == PPM_VCOM_EVT_CHAN_OPEN) ? "open" : "close");

    if (chan == PPM_VCOM_CHAN_CTRL) {
        logic_port = OM_LOGIC_CHANNEL_CNF;
    } else if (chan == PPM_VCOM_CHAN_DATA) {
        logic_port = OM_LOGIC_CHANNEL_IND;
    } else {
        diag_error("Error channel NO %d\n", chan);
        return;
    }

    /* �򿪲���ֱ�ӷ��� */
    if (event == PPM_VCOM_EVT_CHAN_OPEN) {
        diag_crit("ppm_vcom_event_cb open, do nothing.\n");
        return;
    } else if (event == PPM_VCOM_EVT_CHAN_CLOSE) {
        send_flag = false;

        if ((CPM_VCOM_CFG_PORT == cpm_query_phy_port(CPM_OM_CFG_COMM)) &&
            (CPM_VCOM_IND_PORT == cpm_query_phy_port(CPM_OM_IND_COMM))) {
            send_flag = true;
        }
        if (send_flag == true) {
            diag_crit("ppm_vcom_event_cb close, disconnect all ports.\n");
            ppm_disconnect_port(logic_port);
        }
    } else {
        diag_error("Error Event State %d\n", event);
    }

    return;
}

/*
 * �� �� ��: ppm_vcom_cfg_read_data
 * ��������: NAS���յ����ݵ���OM �ӿڷ���
 * �������:  ucDevIndex: ����˿�
 *            data    : �յ�����
 *            uslength : ���ݳ���
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 */
u32 ppm_vcom_cfg_read_data(u32 index, u8 *data, u32 len)
{
    u32 ret = 0xFFFFFFF;

    if (index != PPM_VCOM_CHAN_CTRL) {
        diag_error("PhyPort port is error: %d\n", index);

        return ERR_MSP_FAILURE;
    }

    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_num++;
    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_len += len;

    if ((NULL == data) || (0 == len)) {
        diag_error("Send data is NULL\n");

        return ERR_MSP_FAILURE;
    }

    /* ���ֻ��������ʱ���·��������ޣ��Ҵ�ӡ�������У����������ӡ */
    diag_crit("vcom receive cmd, length : 0x%x, sum length : 0x%x.\n", len,
              g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_len);

    ret = cpm_com_receive(CPM_VCOM_CFG_PORT, data, len);
    if (BSP_OK != ret) {
        diag_error("cpm_com_receive error(0x%x)\n", ret);
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_err_num++;
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_err_len += len;
    }

    return BSP_OK;
}

/*
 * �� �� ��: ppm_vcom_ind_send_data
 * ��������: Vcom�ڳ��ص�OM IND�˿��յ����ݣ���NAS����
 * �������: pucVirAddr:   �������ַ
 *           pucPhyAddr:   ����ʵ��ַ
 *           data_len:    ���ݳ���
 * �������: ��
 * �� �� ֵ: BSP_ERROR/BSP_OK
 * ���ú���:
 * ��������:
 */
u32 ppm_vcom_ind_send_data(u8 *virt_addr, u8 *phy_addr, u32 len)
{
    u32 in_slice;
    u32 out_slice;
    u32 write_slice;
    int ret;
    u8 mode;

    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_num++;
    g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_len += len;

    mode = (COMPRESS_ENABLE_STATE == mdrv_socp_compress_status()) ? PPM_VCOM_DATA_MODE_COMPRESSED
                                                                    : PPM_VCOM_DATA_MODE_TRANSPARENT;

    in_slice = bsp_get_slice_value();

    ret = PPM_VCOM_SEND_DATA(PPM_VCOM_CHAN_DATA, virt_addr, len, mode);

    diag_system_debug_vcom_len(len);
    diag_system_debug_send_data_end();

    out_slice = bsp_get_slice_value();

    write_slice = (in_slice > out_slice) ? (0xffffffff - in_slice + out_slice) : (out_slice - in_slice);

    if (write_slice > g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].max_time_len) {
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].max_time_len = write_slice;
    }

    if (0 != ret) {
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_err_num++;
        g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_err_len += len;

        diag_system_debug_vcom_fail_len(len);

        return CPM_SEND_ERR;
    }
    diag_system_debug_vcom_sucess_len(len);

    return CPM_SEND_OK;
}

mdrv_ppm_vcom_debug_info_s *ppm_vcom_get_ind_info(void)
{
    return &g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND];
}

/*
 * �� �� ��: ppm_vcom_cfg_port_init
 * ��������: ���� Vcom ��OM CFGͨ���ĳ�ʼ��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_vcom_cfg_port_init(void)
{
    struct diag_vcom_cb_ops ops;

    ops.event_cb = ppm_vcom_event_cb;
    ops.data_rx_cb = ppm_vcom_cfg_read_data;
    diag_vcom_register_ops(PPM_VCOM_CHAN_CTRL, &ops);

    cpm_phy_send_reg(CPM_VCOM_CFG_PORT, ppm_vcom_cfg_send_data);

    return;
}

/*
 * �� �� ��: ppm_vcom_ind_port_init
 * ��������: ���� Vcom ��OM INDͨ���ĳ�ʼ��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void ppm_vcom_ind_port_init(void)
{
    struct diag_vcom_cb_ops ops;

    ops.event_cb = ppm_vcom_event_cb;
    ops.data_rx_cb = NULL;
    diag_vcom_register_ops(PPM_VCOM_CHAN_DATA, &ops);

    cpm_phy_send_reg(CPM_VCOM_IND_PORT, ppm_vcom_ind_send_data);

    return;
}


void ppm_vcom_port_init(void)
{
    (void)memset_s(g_ppm_vcom_debug_info, sizeof(g_ppm_vcom_debug_info), 0, sizeof(g_ppm_vcom_debug_info));

    /* Vcom ��OM INDͨ���ĳ�ʼ�� */
    ppm_vcom_ind_port_init();

    /* Vcom ��OM CNFͨ���ĳ�ʼ�� */
    ppm_vcom_cfg_port_init();

    return;
}


void ppm_vcom_info_show(void)
{
    diag_crit(" VCom30 Send num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_send_num);
    diag_crit(" VCom30 Send Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_send_len);

    diag_crit(" VCom30 Send Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_send_err_num);
    diag_crit(" VCom30 Send Error Len is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_send_err_len);

    diag_crit(" VCom30 receive num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_rcv_num);
    diag_crit(" VCom30 receive Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_rcv_len);

    diag_crit(" VCom30 receive Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_rcv_err_num);
    diag_crit(" VCom30 receive Error Len is         %d.\n\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CBT].vcom_rcv_err_len);

    diag_crit(" VCom28 Send num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_num);
    diag_crit(" VCom28 Send Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_len);

    diag_crit(" VCom28 Send Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_num);
    diag_crit(" VCom28 Send Error Len is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_send_err_len);

    diag_crit(" VCom28 receive num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_num);
    diag_crit(" VCom28 receive Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_len);

    diag_crit(" VCom28 receive Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_err_num);
    diag_crit(" VCom28 receive Error Len is         %d.\n\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_CNF].vcom_rcv_err_len);

    diag_crit(" VCom31 Send num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_num);
    diag_crit(" VCom31 Send Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_len);

    diag_crit(" VCom31 Send Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_err_num);
    diag_crit(" VCom31 Send Error Len is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_send_err_len);

    diag_crit(" VCom31 receive num is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_rcv_num);
    diag_crit(" VCom31 receive Len is           %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_rcv_len);

    diag_crit(" VCom31 receive Error num is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_rcv_err_num);
    diag_crit(" VCom31 receive Error Len is         %d.\n", g_ppm_vcom_debug_info[OM_LOGIC_CHANNEL_IND].vcom_rcv_err_len);

    return;
}
EXPORT_SYMBOL(ppm_vcom_info_show);

