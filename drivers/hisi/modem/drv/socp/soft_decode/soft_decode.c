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
#include <linux/module.h>
#include <securec.h>
#include <mdrv.h>
#include <eagleye.h>
#include <osl_sem.h>
#include <osl_thread.h>
#include <osl_malloc.h>
#include <securec.h>
#include <bsp_rfile.h>
#include <bsp_dump.h>
#include <bsp_slice.h>
#include "hdlc.h"
#include "soft_cfg.h"
#include "soft_decode.h"


#define THIS_MODU mod_soft_dec
/*
 * 2 全局变量定义
 */
/* 自旋锁，用来作OM数据接收的临界资源保护 */
spinlock_t g_scm_soft_decode_data_rcv_spinlock;

/* HDLC控制结构 */
om_hdlc_s g_scm_hdlc_soft_decode_entity;

/* SCM数据接收数据缓冲区 */
s8 g_scm_data_rcv_buffer[SCM_DATA_RCV_PKT_SIZE];

/* SCM数据接收任务控制结构 */
scm_data_rcv_ctrl_s g_scm_data_rcv_task_ctrl_info;

scm_soft_decode_info_s g_scm_soft_decode_info;

/*
 * 3 外部引用声明
 */
extern void diag_save_soft_decode_info(void);
extern void cpm_logic_rcv_reg(unsigned int logic_port, cpm_rcv_cb recv_func);
extern void diag_ptr(diag_ptr_id_e type, u32 para_mark, u32 para0, u32 para1);

/*
 * 4 函数实现
 */

/*
 * 函 数 名: SCM_SoftDecodeCfgDataRcv
 * 功能描述: OM配置信息接收函数
 * 输入参数: p_buffer:数据内容
 *           len:数据长度
 * 输出参数: 无
 * 返 回 值: 无
 */
u32 SCM_SoftDecodeCfgDataRcv(u8 *p_buffer, u32 len)
{
    u32 ulRstl;
    unsigned long lock_level;

    spin_lock_irqsave(&g_scm_soft_decode_data_rcv_spinlock, lock_level);

    ulRstl = SCM_SoftDecodeDataRcv(p_buffer, len);

    spin_unlock_irqrestore(&g_scm_soft_decode_data_rcv_spinlock, lock_level);

    return ulRstl;
}

/*
 * 函 数 名: SCM_SoftDecodeDataRcv
 * 功能描述: SCM软解码数据接收函数
 * 输入参数: p_buffer:数据内容
 *           len:数据长度
 *           ulTaskId:SCM软解码任务ID
 * 输出参数: 无
 * 返 回 值: BSP_OK/ERR_MSP_FAILURE
 */
u32 SCM_SoftDecodeDataRcv(u8 *p_buffer, u32 len)
{
    rw_buffer_s rw_buffer;
    s32 ret;

    diag_ptr(EN_DIAG_PTR_SCM_SOFTDECODE, 0, 0, 0);

    diag_get_idle_buffer(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id, &rw_buffer);
    if ((rw_buffer.size + rw_buffer.rb_size) < len) {
        g_scm_soft_decode_info.rb_info.buff_no_enough++;
        diag_ptr(EN_DIAG_PTR_SCM_ERR1, 0, 0, 0);
        return ERR_MSP_FAILURE;
    }

    ret = diag_ring_buffer_put(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id,
        rw_buffer, p_buffer, (s32)len);
    COVERITY_TAINTED_SET((void *)&(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id.buf));
    if (ret) {
        g_scm_soft_decode_info.rb_info.rbuff_put_err++;
        diag_ptr(EN_DIAG_PTR_SCM_ERR2, 0, 0, 0);
        return ret;
    }

    osl_sem_up(&(g_scm_data_rcv_task_ctrl_info.sem_id));
    return BSP_OK;
}


u32 SCM_SoftDecodeAcpuRcvData(om_hdlc_s *p_hdlc_ctrl, u8 *p_data, u32 len)
{
    u32 i;
    u32 result;
    u8 gutl_type;
    u8 tmp_char;

    result = ERR_MSP_FAILURE;

    for (i = 0; i < len; i++) {
        tmp_char = (u8)p_data[i];

        result = diag_hdlc_decap(p_hdlc_ctrl, tmp_char);

        if (HDLC_SUCC == result) {
            g_scm_soft_decode_info.hdlc_decap_data.data_len += p_hdlc_ctrl->info_len;
            g_scm_soft_decode_info.hdlc_decap_data.num++;

            gutl_type = p_hdlc_ctrl->p_decap_buff[0];

            diag_ptr(EN_DIAG_PTR_SCM_RCVDATA_SUCCESS, 0, 0, 0);

            scm_rcv_data_dispatch(p_hdlc_ctrl, gutl_type);
        } else if (HDLC_NOT_HDLC_FRAME == result) {
            /* 不是完整分帧,继续HDLC解封装 */
        } else {
            g_scm_soft_decode_info.frame_decap_data++;
        }
    }

    return BSP_OK;
}


u32 SCM_SoftDecodeCfgHdlcInit(om_hdlc_s *p_hdlc)
{
    /* 申请用于HDLC解封装的缓存 */
    p_hdlc->p_decap_buff = (u8 *)osl_malloc(SCM_DATA_RCV_PKT_SIZE);

    if (NULL == p_hdlc->p_decap_buff) {
        soft_decode_error("Alloc Decapsulate buffer fail!\n");

        return ERR_MSP_FAILURE;
    }

    /* HDLC解封装缓存长度赋值 */
    p_hdlc->decap_buff_size = SCM_DATA_RCV_PKT_SIZE;

    /* 初始化HDLC解封装控制上下文 */
    diag_hdlc_init(p_hdlc);

    return BSP_OK;
}

/*
 * 函 数 名: SCM_SoftDecodeCfgRcvSelfTask
 * 功能描述: SCM软解码OM配置数据接收任务
 * 输入参数: ulPara1:参数1
 *           ulPara2:参数2
 *           ulPara3:参数3
 *           ulPara4:参数4
 * 输出参数: 无
 * 返 回 值: 无
 */
int SCM_SoftDecodeCfgRcvSelfTask(void *para)
{
    s32 ret;
    s32 len;
    rw_buffer_s rw_buffer;
    unsigned long lock_level;

    for (;;) {
        osl_sem_down(&(g_scm_data_rcv_task_ctrl_info.sem_id));

        diag_ptr(EN_DIAG_PTR_SCM_SELFTASK, 0, 0, 0);

        COVERITY_TAINTED_SET((VOID *)&(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id.buf));

        diag_get_data_buffer(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id, &rw_buffer);
        len = rw_buffer.size + rw_buffer.rb_size;
        if (len == 0) {
            continue;
        }

        ret = diag_ring_buffer_get(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id, rw_buffer,
            g_scm_data_rcv_task_ctrl_info.p_buffer, len);
        if (ret) {
            spin_lock_irqsave(&g_scm_soft_decode_data_rcv_spinlock, lock_level);
            diag_ring_buffer_flush(g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id);
            spin_unlock_irqrestore(&g_scm_soft_decode_data_rcv_spinlock, lock_level);
            g_scm_soft_decode_info.rb_info.rbuff_flush++;
            diag_ptr(EN_DIAG_PTR_SCM_ERR3, 0, 0, 0);
            continue;
        }

        g_scm_soft_decode_info.get_info.data_len += len;
        diag_ptr(EN_DIAG_PTR_SCM_RCVDATA, 0, 0, 0);

        /* 调用HDLC解封装函数 */
        if (BSP_OK != SCM_SoftDecodeAcpuRcvData(&g_scm_hdlc_soft_decode_entity,
                                                (u8 *)g_scm_data_rcv_task_ctrl_info.p_buffer,
                                                (u32)len)) {
            soft_decode_error("SCM_SoftDecodeAcpuRcvData Fail\n");
        }
    }

    return 0;
}

/*
 * 函 数 名: SCM_SoftDecodeCfgRcvTaskInit
 * 功能描述: SCM软解码OM配置数据接收函数初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: BSP_OK/ERR_MSP_FAILURE
 */
int SCM_SoftDecodeCfgRcvTaskInit(void)
{
    u32 result;
    OSL_TASK_ID task_id;

    g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id = diag_ring_buffer_create(SCM_DATA_RCV_BUFFER_SIZE);

    if (NULL == g_scm_data_rcv_task_ctrl_info.rng_om_rbuff_id) {
        soft_decode_error("Creat OMCFG ringBuffer Fail.\n");

        g_scm_soft_decode_info.rb_info.ringbuff_create_err++;

        (void)diag_save_soft_decode_info();

        return BSP_ERROR;
    }

    osl_sem_init(0, &(g_scm_data_rcv_task_ctrl_info.sem_id));

    /* 注册OM配置数据接收自处理任务 */
    result = (u32)osl_task_init("soft_dec", 76, 8096, (OSL_TASK_FUNC)SCM_SoftDecodeCfgRcvSelfTask, NULL, &task_id);
    if (result) {
        soft_decode_error("RegisterSelfTaskPrio Fail.\n");
        return BSP_ERROR;
    }

    (void)memset_s(&g_scm_soft_decode_info, sizeof(g_scm_soft_decode_info), 0, sizeof(g_scm_soft_decode_info));

    if (BSP_OK != SCM_SoftDecodeCfgHdlcInit(&g_scm_hdlc_soft_decode_entity)) {
        soft_decode_error("HDLC Init Fail.\n");

        g_scm_soft_decode_info.hdlc_init_err++;

        return BSP_ERROR;
    }

    g_scm_data_rcv_task_ctrl_info.p_buffer = &g_scm_data_rcv_buffer[0];

    spin_lock_init(&g_scm_soft_decode_data_rcv_spinlock);

    scm_soft_decode_init();

    soft_decode_crit("[init]soft decode init ok\n");

    return BSP_OK;
}
#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
module_init(SCM_SoftDecodeCfgRcvTaskInit);
#endif
void scm_soft_decode_init(void)
{
    cpm_logic_rcv_reg(CPM_OM_CFG_COMM, SCM_SoftDecodeCfgDataRcv);

    g_scm_soft_decode_info.cpm_reg_logic_rcv_suc++;
}

void scm_soft_decode_info_show(void)
{
    soft_decode_crit("\r\nSCM_SoftDecodeInfoShow:\r\n");

    soft_decode_crit("\r\nSem Creat Error %d:\r\n", g_scm_soft_decode_info.rb_info.sem_create_err);
    soft_decode_crit("\r\nSem Give Error %d:\r\n", g_scm_soft_decode_info.rb_info.sem_give_err);
    soft_decode_crit("\r\nRing Buffer Creat Error %d:\r\n", g_scm_soft_decode_info.rb_info.ringbuff_create_err);
    soft_decode_crit("\r\nTask Id Error %d:\r\n", g_scm_soft_decode_info.rb_info.task_id_err);
    soft_decode_crit("\r\nRing Buffer not Enough %d:\r\n", g_scm_soft_decode_info.rb_info.buff_no_enough);
    soft_decode_crit("\r\nRing Buffer Flush %d:\r\n", g_scm_soft_decode_info.rb_info.rbuff_flush);
    soft_decode_crit("\r\nRing Buffer Put Error %d:\r\n", g_scm_soft_decode_info.rb_info.rbuff_put_err);

    soft_decode_crit("\r\nRing Buffer Put Success Times %d:\r\n", g_scm_soft_decode_info.put_info.num);
    soft_decode_crit("\r\nRing Buffer Put Success Bytes %d:\r\n", g_scm_soft_decode_info.put_info.data_len);

    soft_decode_crit("\r\nRing Buffer Get Success Times %d:\r\n", g_scm_soft_decode_info.get_info.num);
    soft_decode_crit("\r\nRing Buffer Get Success Bytes %d:\r\n", g_scm_soft_decode_info.get_info.data_len);

    soft_decode_crit("\r\nHDLC Decode Success Times %d:\r\n", g_scm_soft_decode_info.hdlc_decap_data.num);
    soft_decode_crit("\r\nHDLC Decode Success Bytes %d:\r\n", g_scm_soft_decode_info.hdlc_decap_data.data_len);

    soft_decode_crit("\r\nHDLC Decode Error Times %d:\r\n", g_scm_soft_decode_info.frame_decap_data);

    soft_decode_crit("\r\nHDLC Init Error Times %d:\r\n", g_scm_soft_decode_info.hdlc_init_err);

    soft_decode_crit("\r\nHDLC Decode Data Type Error Times %d:\r\n", g_scm_soft_decode_info.data_type_err);

    soft_decode_crit("\r\nCPM Reg Logic Rcv Func Success Times %d:\r\n", g_scm_soft_decode_info.cpm_reg_logic_rcv_suc);
}
EXPORT_SYMBOL(scm_soft_decode_info_show);

#define DIAG_LOG_PATH MODEM_LOG_ROOT "/drv/DIAG/"

/* 软解码************************************************************************ */
void diag_save_soft_decode_info(void)
{
    u32 p_file;
    u32 ret;
    char *dir_path = DIAG_LOG_PATH;
    char *file_path = DIAG_LOG_PATH "DIAG_SoftDecode.bin";
    char info[32];

    /* 如果DIAG目录不存在则先创建目录 */
    if (BSP_OK != bsp_access(dir_path, 0)) {
        if (BSP_OK != bsp_mkdir(dir_path, 0755)) {
            soft_decode_error("mkdir %s failed.\n", DIAG_LOG_PATH);
            return;
        }
    }

    p_file = bsp_open(file_path, RFILE_RDWR | RFILE_CREAT, 0664);
    if (p_file == 0) {
        soft_decode_error(" file open failed.\n");
        return;
    }

    ret = diag_fill_soft_header(p_file);
    if (BSP_OK != ret) {
        soft_decode_error(" diag_debug_file_header failed\n");
        (void)bsp_close(p_file);

        return;
    }

    ret = (u32)memset_s(info, sizeof(info), 0, sizeof(info));
    if (ret) {
        soft_decode_error("memset_s fail\n");
    }

    ret = (u32)memcpy_s(info, sizeof(info), "DIAG SoftDecode info",
                        strnlen("DIAG SoftDecode info", sizeof(info) - 1));
    if (ret) {
        soft_decode_error("memcpy_s fail\n");
    }

    ret = bsp_write(p_file, (s8 *)info, sizeof(info));
    if (ret != sizeof(info)) {
        soft_decode_error(" write DIAG number info failed.\n");
    }

    ret = bsp_write(p_file, (s8 *)&g_scm_soft_decode_info, sizeof(g_scm_soft_decode_info));
    if (ret != sizeof(g_scm_soft_decode_info)) {
        soft_decode_error("write pData failed.\n");
    }

    diag_fill_soft_tail(p_file, file_path);

    (void)bsp_close(p_file);

    return;
}

/*
 * Function Name: diag_debug_file_header
 * Description: 给debug文件写上文件头
 */
u32 diag_fill_soft_header(u32 p_file)
{
    u32 ret;
    u32 value;

    ret = (u32)bsp_lseek(p_file, 0, DRV_SEEK_SET);
    if (BSP_OK != ret) {
        soft_decode_error("file seek failed .\n");
        return (u32)BSP_ERROR;
    }

    value = 0xaaaa5555;

    /* file start flag */
    ret = (u32)bsp_write(p_file, (s8 *)&value, sizeof(value));
    if (ret != sizeof(value)) {
        soft_decode_error("write start flag failed.\n");
        return (u32)BSP_ERROR;
    }

    value = 0x00010000;

    /* debug version */
    ret = (u32)bsp_write(p_file, (s8 *)&value, sizeof(value));
    if (ret != sizeof(value)) {
        soft_decode_error("write debug version failed.\n");
        return (u32)BSP_ERROR;
    }

    value = 0;

    /* file size */
    ret = (u32)bsp_write(p_file, (s8 *)&value, sizeof(value));
    if (ret != sizeof(value)) {
        soft_decode_error("write file size failed.\n");
        return (u32)BSP_ERROR;
    }

    value = bsp_get_slice_value();

    /* 当前的slice */
    ret = (u32)bsp_write(p_file, (s8 *)&value, sizeof(value));
    if (ret != sizeof(value)) {
        soft_decode_error("write ulTime failed.\n");
        return (u32)BSP_ERROR;
    }

    return BSP_OK;
}

/*
 * Function Name: diag_fill_soft_tail
 * Description: 给debug文件写上文件尾
 * History:
 *    1.    2015-06-21 Draft Enact
 */
void diag_fill_soft_tail(u32 p_file, s8 *file_path)
{
    u32 ret;
    u32 value;

    /* file end flag */
    value = 0x5555aaaa;
    ret = (u32)bsp_write(p_file, (s8 *)&value, sizeof(value));
    if (ret != sizeof(value)) {
        soft_decode_error("write start flag failed.\n");
    }
}

EXPORT_SYMBOL(diag_save_soft_decode_info);

