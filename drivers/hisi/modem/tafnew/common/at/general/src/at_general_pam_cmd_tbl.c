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

#include "at_general_pam_cmd_tbl.h"
#include "at_general_pam_set_cmd_proc.h"
#include "at_general_pam_qry_cmd_proc.h"
#include "AtParse.h"
#include "at_cmd_proc.h"
#include "at_test_para_cmd.h"


/*
 * Э��ջ��ӡ��㷽ʽ�µ�.C�ļ��궨��
 */
#define THIS_FILE_ID PS_FILE_ID_AT_GENERAL_PMM_CMD_TBL_C

static const AT_ParCmdElement g_atGeneralPamCmdTbl[] = {
    /* OAMӦ�ն�Ҫ������AT���� */
    /*
     * [���]: Э��AT-ͨ�ò���
     * [����]: �ȱ��������趨
     * [˵��]: �������������ѯ��ǰ���ȱ����¶�״̬���Ƿ�ʹ���¶ȱ������Լ���ǰ�����¶����ޡ�
     *         Ҫ��ر�PA���¶�����>�����¶�����>�ָ��¶����ޣ����򱨴���
     * [�﷨]:
     *     [����]: ^ADCTEMP=[<value1>,<value2>,<value3>,<value4>]
     *     [���]: <CR><LF>OK<CR><LF>
     *             ���������
     *             <CR><LF>+CME ERROR: <err><CR><LF>
     *     [����]: ^ADCTEMP?
     *     [���]: <CR><LF>^ADCTEMP: <value1>,<value2>,<value3>,<value4><CR><LF>
     *             <CR><LF>OK<CR><LF>
     *     [����]: ^ADCTEMP=?
     *     [���]: <CR><LF>^ADCTEMP: (list of supported <value>s) <CR><LF>
     *             <CR><LF>OK<CR><LF>
     * [����]:
     *     <value1>: ����ֵ���Ƿ�ʹ���¶ȱ�����
     *             0����ʹ�ܣ�
     *             1��ʹ�ܡ�
     *     <value2>: ����ֵ���ر�PA �������¶ȣ�Ϊ��150 ������ֵ��
     *     <value3>: ����ֵ�����������������¶ȣ�Ϊ<value2������ֵ��
     *     <value4>: ����ֵ���ӵ͹��Ļָ�����������ģʽ״̬�������¶ȣ�Ϊ<value3������ֵ��
     * [ʾ��]:
     *     �� �����ȱ�������^ADCTEMP
     *       AT^ADCTEMP=1,60,50,40
     *       OK
     */
    { AT_CMD_ADCTEMP,
      At_SetAdcTempPara, AT_SET_PARA_TIME, At_QryAdcTempPara, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
      VOS_NULL_PTR, AT_NOT_SET_TIME,
      AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE,
      (VOS_UINT8 *)"^ADCTEMP", (VOS_UINT8 *)"(0,1),(0-150),(0-150),(0-150)" },
};

/* ע��PAMͨ��AT����� */
VOS_UINT32 AT_RegisterGeneralPamCmdTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_atGeneralPamCmdTbl, sizeof(g_atGeneralPamCmdTbl) / sizeof(g_atGeneralPamCmdTbl[0]));
}
