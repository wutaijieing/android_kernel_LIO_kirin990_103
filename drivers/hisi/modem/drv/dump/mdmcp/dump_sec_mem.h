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

#ifndef __DUMP_SEC_MEM_H__
#define __DUMP_SEC_MEM_H__

#define DUMP_SEC_TRANS_SIGNLE_MAX_SIZE (0xfff0)
#define DUMP_SEC_TRANS_FILE_MAGIC (0x5678fedc)
#define DUMP_SEC_TRANS_MAX_FILE_NUM (8)

#define MODEM_SEC_DUMP_ENABLE_LR_CHANNEL_CMD (0x56781234)
#define MODEM_SEC_DUMP_ENABLE_NR_CHANNEL_CMD (0x78563412)
#define MODEM_SEC_DUMP_STOP_LR_CHANNEL_CMD (0x12345678)
#define MODEM_SEC_DUMP_STOP_NR_CHANNEL_CMD (0x34127856)

typedef struct {
    u32 enInitState;   /* ͨ����ʼ��״̬����ʼ�����Զ��޸� */
    u32 enChannelID;   /* ����Ŀ��ͨ��ID���̶����� */
    size_t ulBufLen;   /* ����Ŀ��ͨ�����ݿռ��С */
    u32 ulThreshold;   /* ����Ŀ��ͨ����ֵ */
    u32 ulTimeoutMode; /* ����Ŀ��ͨ����ʱ���� */
    u8 *pucBuf;        /* ����Ŀ��ͨ�����ݿռ�ָ�� */
    u8 *pucBufPHY;     /**/
    void *tempucBuf;   /* ƹ��buff�������ݱ��� */
    bool dumpState;    /**/
} dump_sec_coder_dest_cfg_s;

struct dump_file_info_s {
    char name[32];
    void *addr;
    unsigned int size;
};

#define DUMP_SEC_CODER_DST_IND_SIZE (2 * 1024 * 1024)
#define DUMP_SEC_CODER_DST_THRESHOLD (0x600)
#define DUMP_SEC_DEBUG_SIZE (0x1000)
#define DUMP_SEC_FILE_SAVE_START (0xA5A5A5A5)
#define DUMP_SEC_FILE_SAVE_END   (0x5A5A5A5A)
#define DUMP_SEC_FILE_RESAVE_DONE   (0x6a6a6a6a)


int dump_sec_init(void);
void dump_sec_enable_trans(u32 channel_id);
void dump_sec_disable_trans(u32 channel_id);
void dump_sec_save_file(const char *path);
int dump_sec_dst_channel_free(void);
s32 dump_sec_free_src_channel(u32 src_channel);
void dump_sec_traverse_all_file(void);
int dump_check_lr_sec_dump(void);
int dump_check_nr_sec_dump(void);



#endif
