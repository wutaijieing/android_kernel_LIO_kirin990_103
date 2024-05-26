/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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
#include "at_ltev_ttf_cmd_tbl.h"
#include "AtParse.h"

#include "at_cmd_proc.h"
#include "at_parse_core.h"
#include "at_test_para_cmd.h"

#include "at_ltev_ttf_set_cmd_proc.h"
#include "at_ltev_ttf_qry_cmd_proc.h"


/*
 * Э��ջ��ӡ��㷽ʽ�µ�.C�ļ��궨��
 */
#define THIS_FILE_ID PS_FILE_ID_AT_LTEV_TTF_CMD_TBL_C

#if (FEATURE_LTEV == FEATURE_ON)
static const AT_ParCmdElement g_atLtevTtfCmdTbl[] = {
    /*
     * [���]: Э��AT-LTE-V���
     * [����]: ����Ͳ�ѯV2X���պͷ��͵����ݰ����
     * [˵��]: ��������������Ͳ�ѯV2X���պͷ��͵����ݰ������
     * [�﷨]:
     *     [����]: ^PTRRPT=<op>
     *     [���]: ִ�гɹ�ʱ��
     *             <CR><LF>^PTRRPT: <tx_bytes>,<rx_bytes>,<tx_packets>,<rx_packets>
     *             <CR><LF>OK<CR><LF>
     *             ִ��ʧ��ʱ��
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     *     [����]: ^PTRRPT?
     *     [���]: ִ�гɹ�ʱ��
     *             <CR><LF>^PTRRPT: <tx_bytes>,<rx_bytes>,<tx_packets>,<rx_packets>
     *             <CR><LF>
     *             ִ��ʧ��ʱ��
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     * [����]:
     *     <tx_bytes>: PC5�ڷ��͵����ֽ���
     *     <rx_bytes>: PC5�ڽ��յ����ֽ���
     *     <tx_packets>: PC5�ڷ��͵��ܰ���
     *     <rx_packets>: PC5�ڽ��յ��ܰ���
     *     <op>: 1���շ����ݰ����㡣
     * [ʾ��]:
     *     �� �������
     *       AT^PTRRPT=1
     *       ^PHYR: 0,0,0,0
     *       OK
     *     �� ��ѯ����
     *       AT^PTRRPT?
     *       ^PHYR: 1024,2048,900,800
     *       OK
     */
    { AT_CMD_PTRRPT,
      AT_SetPtrRpt, AT_SET_PARA_TIME, AT_QryPtrRpt, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^PTRRPT", (VOS_UINT8 *)"(1)" },

    /*
     * [���]: Э��AT-LTE-V���
     * [����]: ��ѯLTE-V PC5��RSSI��ֵ����
     * [˵��]: ����������LTE-V PC5��RSSI��ֵ��ѯ��
     * [�﷨]:
     *     [����]: ^VRSSI?
     *     [���]: ִ�гɹ�ʱ��
     *             CR><LF>^VRSSI: <RSSI><CR>
     *             <CR><LF>OK<CR><LF>
     *             ִ��ʧ��ʱ��
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     * [����]:
     *     <RSSI>: PC5��RSSI��ֵ
     * [ʾ��]:
     *     �� ��ѯPC5 RSSI��ֵ��
     *       AT^VRSSI?
     *       ^VRSSI: -80
     *       OK
     */
    { AT_CMD_VRSSI,
      VOS_NULL_PTR, AT_NOT_SET_TIME, AT_QryVRssi, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
      (VOS_UINT8 *)"^VRSSI", VOS_NULL_PTR },

};

/* ע��TTF LTEV AT����� */
VOS_UINT32 AT_RegisterLtevTtfCmdTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_atLtevTtfCmdTbl, sizeof(g_atLtevTtfCmdTbl) / sizeof(g_atLtevTtfCmdTbl[0]));
}
#endif


