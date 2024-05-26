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

/*****************************************************************************/
/*                                                                           */
/*                Copyright 1999 - 2008, Huawei Tech. Co., Ltd.              */
/*                           ALL RIGHTS RESERVED                             */
/*                                                                           */
/* FileName: OMRingBuffer.c                                                  */
/*                                                                           */
/* Author: Windriver                                                         */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date: 2008-06                                                             */
/*                                                                           */
/* Description: implement ring buffer subroutine                             */
/*                                                                           */
/* Others:                                                                   */
/*                                                                           */
/* History:                                                                  */
/* 1. Date:                                                                  */
/*    Author:                                                                */
/*    Modification: Adapt this file                                          */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
#include <linux/module.h>
#include <securec.h>
#include <bsp_dump.h>
#include <osl_spinlock.h>
#include <osl_malloc.h>
#include "ring_buffer.h"
#include "soft_decode.h"


#define THIS_MODU mod_soft_dec

#define OM_MIN(x, y)             (((x) < (y)) ? (x) : (y))


om_ring_id diag_ring_buffer_create(int nbytes)
{
    char *buffer = NULL;
    om_ring_id ring_id = NULL;

    buffer = (char *)osl_malloc((u32)nbytes);
    if (buffer == NULL) {
        return NULL;
    }

    ring_id = (om_ring_id)osl_malloc(sizeof(om_ring_s));
    if (ring_id == NULL) {
        osl_free(buffer);
        return NULL;
    }

    ring_id->buf_size = nbytes;
    ring_id->buf = buffer;

    diag_ring_buffer_flush(ring_id);
    
    return (ring_id);
}

void diag_ring_buffer_flush(om_ring_id ring_id)
{
    ring_id->p_tobuff = 0;
    ring_id->p_frombuff = 0;
}


void diag_get_data_buffer(om_ring_id ring_buffer, rw_buffer_s *rw_buff)
{
    if (ring_buffer->p_frombuff <= ring_buffer->p_tobuff) {
        /* 写指针大于读指针，直接计算 */
        rw_buff->buffer = ring_buffer->buf + ring_buffer->p_frombuff;
        rw_buff->size = ring_buffer->p_tobuff - ring_buffer->p_frombuff;
        rw_buff->rb_buffer = NULL;
        rw_buff->rb_size = 0;
    } else {
        /* 读指针大于写指针，需要考虑回卷 */
        rw_buff->buffer = ring_buffer->buf + ring_buffer->p_frombuff;
        rw_buff->size = ring_buffer->buf_size - ring_buffer->p_frombuff ;
        rw_buff->rb_buffer = ring_buffer->buf;
        rw_buff->rb_size = ring_buffer->p_tobuff;
    }
}


int diag_ring_buffer_get(om_ring_id ring_id, rw_buffer_s rw_buffer, u8 *buffer, int data_len)
{
    s32 ret;

    if (data_len == 0) {
        return 0;
    }

    ret = memcpy_s(buffer, data_len, rw_buffer.buffer, rw_buffer.size);
    if (ret) {
        soft_decode_error("memcpy_s fail, ret=0x%x\n", ret);
        return ret;
    }

    if (rw_buffer.rb_size != 0) {
        ret = memcpy_s(buffer + rw_buffer.size, data_len - rw_buffer.size,
            rw_buffer.rb_buffer, data_len - rw_buffer.size);
        if (ret) {
            soft_decode_error("memcpy_s fail, ret=0x%x\n", ret);
            return ret;
        }
        ring_id->p_frombuff = data_len - rw_buffer.size;
    } else {
        ring_id->p_frombuff += data_len;
    }
    
    return 0;
}


void diag_get_idle_buffer(om_ring_id ring_buffer, rw_buffer_s *rw_buff)
{
    if (ring_buffer->p_tobuff < ring_buffer->p_frombuff) {
        /* 读指针大于写指针，直接计算 */
        rw_buff->buffer = ring_buffer->buf + ring_buffer->p_tobuff;
        rw_buff->size = (ring_buffer->p_frombuff - ring_buffer->p_tobuff - 1);
        rw_buff->rb_buffer = NULL;
        rw_buff->rb_size = 0;
    } else {
        /* 写指针大于读指针，需要考虑回卷 */
        if (ring_buffer->p_frombuff != 0) {
            rw_buff->buffer = ring_buffer->buf + ring_buffer->p_tobuff;
            rw_buff->size = ring_buffer->buf_size   - ring_buffer->p_tobuff ;
            rw_buff->rb_buffer = ring_buffer->buf;
            rw_buff->rb_size = ring_buffer->p_frombuff - 1;
        } else {
            rw_buff->buffer = ring_buffer->buf + ring_buffer->p_tobuff;
            rw_buff->size = ring_buffer->buf_size - ring_buffer->p_tobuff - 1;
            rw_buff->rb_buffer = NULL;
            rw_buff->rb_size = 0;
        }
    }
}


int diag_ring_buffer_put(om_ring_id ring_id, rw_buffer_s rw_buffer, const u8 *buffer, int data_len)
{
    u32 size;
    u32 rb_size;
    s32 ret;

    if (data_len == 0) {
        return 0;
    }

    if (rw_buffer.size > data_len) {
        ret = memcpy_s((u8 *)rw_buffer.buffer, rw_buffer.size, buffer, data_len);
        if (ret) {
            soft_decode_error("memcpy_s fail, ret=0x%x\n", ret);
            return ret;
        }

        ring_id->p_tobuff += data_len;
    } else {
        if ((rw_buffer.buffer) && (rw_buffer.size)) {
            size = rw_buffer.size;
            ret = memcpy_s((u8 *)rw_buffer.buffer, rw_buffer.size, buffer, size);
            if (ret) {
                soft_decode_error("memcpy_s fail, ret=0x%x\n", ret);
                return ret;
            }
        } else {
            size = 0;
        }

        rb_size = data_len - rw_buffer.size;
        if (rb_size && (rw_buffer.rb_buffer != NULL)) {
            ret = memcpy_s((u8 *)rw_buffer.rb_buffer, rw_buffer.rb_size, ((u8 *)buffer + size), rb_size);
            if (ret) {
                soft_decode_error("memcpy_s fail, ret=0x%x\n", ret);
                return ret;
            }
        }

        ring_id->p_tobuff = rb_size;
    }

    return 0;
}

int diag_ring_buffer_init(void)
{
    return BSP_OK;
}

