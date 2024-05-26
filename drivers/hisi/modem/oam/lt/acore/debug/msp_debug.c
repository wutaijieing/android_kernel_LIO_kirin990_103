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





/*****************************************************************************
  1 Include HeadFile
*****************************************************************************/
#include <securec.h>
#include "msp_debug.h"
#include <mdrv.h>
#include <mdrv_diag_system.h>
#include <msp_errno.h>
#include <nv_stru_drv.h>
#include <acore_nv_stru_drv.h>
#include <nv_stru_pam.h>
#include "diag_common.h"
#include "diag_debug.h"
#include "diag_cfg.h"


/*****************************************************************************
  2 Declare the Global Variable
*****************************************************************************/

#define    THIS_FILE_ID        MSP_FILE_ID_MSP_DEBUG_C

#define    DIAG_LOG_PATH       MODEM_LOG_ROOT"/drv/DIAG/"

/*****************************************************************************
  3 Function
*****************************************************************************/

/*****************************************************************************
 Function Name   : PTR : Process Trace Record (流程跟踪记录)
 Description     : 跟踪整个处理流程
*****************************************************************************/
extern mdrv_ppm_chan_debug_info_s g_ppm_chan_info;

extern DIAG_CHANNLE_PORT_CFG_STRU g_diag_port_cfg;

extern mdrv_ppm_vcom_debug_info_s g_ppm_vcom_debug_info[3];

extern VOS_UINT32 g_ulDiagCfgInfo;


VOS_VOID DIAG_DebugCommon(VOS_VOID)
{
    void *pFile = VOS_NULL;
    VOS_UINT32 ret;
    VOS_CHAR *DirPath = DIAG_LOG_PATH;
    VOS_CHAR *FilePath = DIAG_LOG_PATH"DIAG_DebugCommon.bin";
    VOS_UINT32 ulValue;
    VOS_UINT8 *pData = VOS_NULL;
    VOS_UINT32 ullen,offset;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];
    errno_t err = EOK;

    /* 前两个U32保存A/C核PID个数 */
    ullen =   sizeof(g_ulDiagCfgInfo)
            + sizeof(g_diag_port_cfg)
            + sizeof(VOS_UINT32) + sizeof(g_ppm_chan_info);
     ullen += sizeof(VOS_UINT32) + sizeof(g_ppm_vcom_debug_info);
    pData = (VOS_UINT8 *)VOS_MemAlloc(DIAG_AGENT_PID, DYNAMIC_MEM_PT, ullen);
    if(VOS_NULL == pData)
    {
        return;
    }

    /* 如果DIAG目录不存在则先创建目录 */
    if (VOS_OK != mdrv_file_access(DirPath, 0))
    {
        if (VOS_OK != mdrv_file_mkdir(DirPath))
        {
            diag_error("make file %s failed.\n", DIAG_LOG_PATH);
            VOS_MemFree(DIAG_AGENT_PID, pData);
            return ;
        }
    }

    pFile = mdrv_file_open(FilePath, "wb+");
    if(pFile == 0)
    {
        diag_error("open file %s failed.\n", FilePath);

        VOS_MemFree(DIAG_AGENT_PID, pData);

        return ;
    }

    ret = mdrv_diag_debug_file_header(pFile);
    if(VOS_OK != ret)
    {
        diag_error("add debug file header faile\n");
        (VOS_VOID)mdrv_file_close(pFile);
        VOS_MemFree(DIAG_AGENT_PID, pData);
        return ;
    }

    err = memset_s(aucInfo, sizeof(aucInfo), 0, sizeof(aucInfo));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    err = memcpy_s(aucInfo, sizeof(aucInfo), "DIAG common info", VOS_StrNLen("DIAG common info", sizeof(aucInfo)-1));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    /* 通用信息 */
    ret = mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        diag_error(" mdrv_file_write DIAG number info failed.\n");
    }

    offset  = 0;

    /* 当前DIAG的连接状态 */
    err = memcpy_s((pData + offset), (ullen - offset), &g_ulDiagCfgInfo, sizeof(g_ulDiagCfgInfo));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_ulDiagCfgInfo);

    /* CPM记录的当前连接的通道 */
    err = memcpy_s((pData + offset), (ullen - offset), &g_diag_port_cfg, sizeof(g_diag_port_cfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_diag_port_cfg);

    /* USB端口的相关可维可测信息 */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ppm_chan_info);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_ppm_chan_info, sizeof(g_ppm_chan_info));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_ppm_chan_info);

    /* netlink端口的相关可维可测信息 */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ppm_vcom_debug_info);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_ppm_vcom_debug_info, sizeof(g_ppm_vcom_debug_info));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_ppm_vcom_debug_info);

    ret = mdrv_file_write(pData, 1, offset, pFile);
    if(ret != offset)
    {
        diag_error(" mdrv_file_write pData failed.\n");
    }

    mdrv_diag_debug_file_tail(pFile, FilePath);

    (VOS_VOID)mdrv_file_close(pFile);

    VOS_MemFree(DIAG_AGENT_PID, pData);

    return ;
}


extern VOS_UINT8 g_EventModuleCfg[DIAG_CFG_PID_NUM];

extern DIAG_CBT_INFO_TBL_STRU g_astCBTInfoTbl[EN_DIAG_DEBUG_INFO_MAX];



VOS_VOID diag_numberinfo(const void *pFile)
{
    VOS_UINT32 ret;
    VOS_UINT32 ulValue;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];
    errno_t err = EOK;

    err = memset_s(aucInfo, sizeof(aucInfo),0, DIAG_DEBUG_INFO_LEN);
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    err = memcpy_s(aucInfo, sizeof(aucInfo), "DIAG number info", VOS_StrNLen("DIAG number info",sizeof(aucInfo)-1));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    /* 上报次数信息 */
    ret = (VOS_UINT32)mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        diag_error(" mdrv_file_write DIAG number info failed.\n");
    }

    /* 当前的slice */
    ulValue = VOS_GetSlice();
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        diag_error(" mdrv_file_write ulTime failed.\n");
    }

    /* 变量的size */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_astCBTInfoTbl);
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        diag_error(" mdrv_file_write ulTime failed.\n");
    }

    /* 各上报次数统计量 */
    ret = (VOS_UINT32)mdrv_file_write(&g_astCBTInfoTbl[0], 1, sizeof(g_astCBTInfoTbl), pFile);
    if(ret != sizeof(g_astCBTInfoTbl))
    {
        diag_error(" mdrv_file_write g_astCBTInfoTbl failed.\n");
    }
}


VOS_VOID DIAG_DebugNoIndLog(VOS_VOID)
{
    void *pFile = VOS_NULL;
    VOS_UINT32 ret;
    VOS_CHAR *DirPath = DIAG_LOG_PATH;
    VOS_CHAR *FilePath = DIAG_LOG_PATH"DIAG_AcoreNoIndLog.bin";
    VOS_UINT32 ulValue;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];
    VOS_UINT8 *pData;
    VOS_UINT32 ullen,offset;
    errno_t err = EOK;

    /* 前两个U32保存A/C核PID个数 */
    ullen = (2 * sizeof(VOS_UINT32))
            + sizeof(VOS_UINT32) + sizeof(g_ALayerSrcModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_CLayerSrcModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_ALayerDstModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_CLayerDstModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_stMsgCfg)
            + sizeof(VOS_UINT32) + sizeof(g_EventModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_PrintTotalCfg)
            + sizeof(VOS_UINT32) + sizeof(g_PrintModuleCfg);

    pData = VOS_MemAlloc(DIAG_AGENT_PID, DYNAMIC_MEM_PT, ullen);
    if(VOS_NULL == pData)
    {
        return;
    }

    /* 如果DIAG目录不存在则先创建目录 */
    if (VOS_OK != mdrv_file_access(DirPath, 0))
    {
        if (VOS_OK != mdrv_file_mkdir(DirPath))
        {
            diag_error(" mkdir %s failed.\n", DIAG_LOG_PATH);
            VOS_MemFree(DIAG_AGENT_PID, pData);
            return ;
        }
    }

    pFile = mdrv_file_open(FilePath, "wb+");
    if(pFile == 0)
    {
        diag_error("open %s failed.\n", FilePath);

        VOS_MemFree(DIAG_AGENT_PID, pData);

        return ;
    }

    ret = mdrv_diag_debug_file_header(pFile);
    if(VOS_OK != ret)
    {
        diag_error("add file header failed .\n");
        VOS_MemFree(DIAG_AGENT_PID, pData);
        (VOS_VOID)mdrv_file_close(pFile);
        return ;
    }

    err = memset_s(aucInfo, sizeof(aucInfo), 0, sizeof(aucInfo));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    err = memcpy_s(aucInfo, sizeof(aucInfo), "DIAG config info", VOS_StrNLen("DIAG config info", sizeof(aucInfo)-1));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    /* 配置开关信息 */
    ret = (VOS_UINT32)mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        diag_error(" mdrv_file_write DIAG config info failed.\n");
    }

    offset  = 0;

    /* A核PID个数 */
    ulValue = VOS_CPU_ID_1_PID_BUTT - VOS_PID_CPU_ID_1_DOPRAEND;
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    /* C核PID个数 */
    ulValue = VOS_CPU_ID_0_PID_BUTT - VOS_PID_CPU_ID_0_DOPRAEND;
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ALayerSrcModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_ALayerSrcModuleCfg[0], sizeof(g_ALayerSrcModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_ALayerSrcModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_CLayerSrcModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_CLayerSrcModuleCfg[0], sizeof(g_CLayerSrcModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_CLayerSrcModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ALayerDstModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_ALayerDstModuleCfg[0], sizeof(g_ALayerDstModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_ALayerDstModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_CLayerDstModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_CLayerDstModuleCfg[0], sizeof(g_CLayerDstModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_CLayerDstModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_stMsgCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_stMsgCfg, sizeof(g_stMsgCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_stMsgCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_EventModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_EventModuleCfg[0], sizeof(g_EventModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_EventModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_PrintTotalCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_PrintTotalCfg, sizeof(g_PrintTotalCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_PrintTotalCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_PrintModuleCfg);
    err = memcpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(ulValue);

    err = memcpy_s((pData + offset), (ullen - offset), &g_PrintModuleCfg[0], sizeof(g_PrintModuleCfg));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }
    offset += sizeof(g_PrintModuleCfg);

    ret = (VOS_UINT32)mdrv_file_write(pData, 1, offset, pFile);
    if(ret != offset)
    {
        diag_error(" mdrv_file_write pData failed.\n");
    }

    diag_numberinfo(pFile);

    /* 延迟5秒后再统计一次 */
    (VOS_VOID)VOS_TaskDelay(5000);

    diag_numberinfo(pFile);

    mdrv_diag_debug_file_tail(pFile, FilePath);

    (VOS_VOID)mdrv_file_close(pFile);

    VOS_MemFree(DIAG_AGENT_PID, pData);

    return ;
}



