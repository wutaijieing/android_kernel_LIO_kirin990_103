
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

#ifndef __DUMP_M3_AGENT_H__
#define __DUMP_M3_AGENT_H__

#include <product_config.h>
#include <bsp_print.h>
#include <bsp_dump_mem.h>
#include "dump_area.h"

#define DUMP_MDM_M3_BASE_INFO 0x0d000000
#define DUMP_AREA_MAGICNUM 0x4e656464
#define DUMP_IPMSG_MAGIC_NUMBER 0x45454646

struct dump_lpm3_agent_ctrl_info_s {
    bool ulInitstate;
    bool is_lpm3exc;
    struct dump_area_s *virt_area_addr;
    void *phy_area_addr;
    u32 length;
};

#ifdef ENABLE_BUILD_DUMP_MDM_LPM3

s32 dump_lpm3_agent_init(void);
s32 dump_mdm_lpm3_callback(u32 modid, u32 etype, u64 coreid, char *logpath, pfn_cb_dump_done fndone);
s32 dump_wait_mdm_lpm3_done(bool block);
s32 dump_match_mdm_lpm3_rdr_id(dump_exception_info_s *dump_exception_info);

#else

static inline s32 dump_lpm3_agent_init(void)
{
    return BSP_OK;
}
static inline s32 dump_mdm_lpm3_callback(u32 modid, u32 etype, u64 coreid, char *logpath, pfn_cb_dump_done fndone)
{
    return BSP_OK;
}

static inline s32 dump_wait_mdm_lpm3_done(bool block)
{
    return BSP_OK;
}
static inline s32 dump_match_mdm_lpm3_rdr_id(dump_exception_info_s *dump_exception_info)
{
    return BSP_OK;
}
#endif

#endif

