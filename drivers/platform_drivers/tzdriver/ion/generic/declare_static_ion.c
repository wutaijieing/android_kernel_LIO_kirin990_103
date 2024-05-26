/*
 * declare_static_ion.c
 *
 * get and set static mem info.
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "declare_static_ion.h"
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include "tc_ns_log.h"

static u64 g_ion_mem_addr;
static u64 g_ion_mem_size;

static int supersonic_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_ion_mem_addr = rmem->base;
		g_ion_mem_size = rmem->size;
	} else {
		tloge("rmem is NULL\n");
	}

	return 0;
}

RESERVEDMEM_OF_DECLARE(supersonic, "platform-supersonic",
	supersonic_reserve_tee_mem);

static u64 g_secfacedetect_mem_addr;
static u64 g_secfacedetect_mem_size;

static int secfacedetect_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_secfacedetect_mem_addr = rmem->base;
		g_secfacedetect_mem_size = rmem->size;
	} else {
		tloge("secfacedetect_reserve_tee_mem mem is NULL\n");
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(secfacedetect, "platform-secfacedetect",
	secfacedetect_reserve_tee_mem);

static u64 g_pt_addr = 0;
static u64 g_pt_size = 0;

static int reserve_pt_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_pt_addr = rmem->base;
		g_pt_size = rmem->size;
		tloge("reserve pp mem is not NULL\n");
	} else {
		tloge("reserve pt mem is NULL\n");
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(pagetable, "platform-ai-pagetable",
	reserve_pt_mem);

static u64 g_pp_addr = 0;
static u64 g_pp_size = 0;

static int reserve_pp_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_pp_addr = rmem->base;
		g_pp_size = rmem->size;
		tloge("reserve pp mem is not NULL\n");
	} else {
		tloge("reserve pp mem is NULL\n");
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(ai_running, "platform-ai-running",
	reserve_pp_mem);

static u64 g_voiceid_addr;
static u64 g_voiceid_size;

static int voiceid_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_voiceid_addr = rmem->base;
		g_voiceid_size = rmem->size;
	} else {
		tloge("voiceid_reserve_tee_mem  mem is NULL\n");
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(voiceid, "platform-voiceid",
	voiceid_reserve_tee_mem);

static u64 g_secos_ex_addr;
static u64 g_secos_ex_size;

static int secos_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_secos_ex_addr = rmem->base;
		g_secos_ex_size = rmem->size;
	} else {
		tloge("secos reserve tee mem is NULL\n");
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(secos_ex, "platform-secos-ex",
	secos_reserve_tee_mem);

static u64 g_ion_ex_mem_addr;
static u64 g_ion_ex_mem_size;

static int supersonic_ex_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem) {
		g_ion_ex_mem_addr = rmem->base;
		g_ion_ex_mem_size = rmem->size;
	} else {
		tloge("rmem is NULL\n");
	}

	return 0;
}

RESERVEDMEM_OF_DECLARE(supersonic_ex, "platform-supersonic-ex",
	supersonic_ex_reserve_tee_mem);

static void set_mem_tag(struct register_ion_mem_tag *memtag,
	u64 addr, u64 size, uint32_t tag, uint32_t *pos)
{
	memtag->memaddr[*pos] = addr;
	memtag->memsize[*pos] = size;
	memtag->memtag[*pos] = tag;
	(*pos)++;
}

void set_ion_mem_info(struct register_ion_mem_tag *memtag)
{
	uint32_t pos = 0;

	if (!memtag) {
		tloge("invalid memtag\n");
		return;
	}

	tloge("ion mem static reserved for tee face=%d,finger=%d,voiceid=%d,"
		"secos=%d,finger-ex=%d,pt_size=%d,pp_size=%d\n",
		(uint32_t)g_secfacedetect_mem_size, (uint32_t)g_ion_mem_size,
		(uint32_t)g_voiceid_size, (uint32_t)g_secos_ex_size,
		(uint32_t)g_ion_ex_mem_size, (uint32_t)g_pt_size,
		(uint32_t)g_pp_size);

	if (g_ion_mem_addr != (u64)0 && g_ion_mem_size  != (u64)0)
		set_mem_tag(memtag, g_ion_mem_addr, g_ion_mem_size, PP_MEM_TAG, &pos);
	if (g_secfacedetect_mem_addr != (u64)0 && g_secfacedetect_mem_size != (u64)0)
		set_mem_tag(memtag, g_secfacedetect_mem_addr, g_secfacedetect_mem_size, PP_MEM_TAG, &pos);
	if (g_voiceid_addr != (u64)0 && g_voiceid_size != (u64)0)
		set_mem_tag(memtag, g_voiceid_addr, g_voiceid_size, PP_MEM_TAG, &pos);
	if (g_secos_ex_addr != (u64)0 && g_secos_ex_size != (u64)0)
		set_mem_tag(memtag, g_secos_ex_addr, g_secos_ex_size, PP_MEM_TAG, &pos);
	if (g_pt_addr != (u64)0 && g_pt_size != (u64)0)
		set_mem_tag(memtag, g_pt_addr, g_pt_size, PT_MEM_TAG, &pos);
	if (g_pp_addr != (u64)0 && g_pp_size != (u64)0)
		set_mem_tag(memtag, g_pp_addr, g_pp_size, PRI_PP_MEM_TAG, &pos);
	if (g_ion_ex_mem_addr != (u64)0 && g_ion_ex_mem_size != (u64)0)
		set_mem_tag(memtag, g_ion_ex_mem_addr, g_ion_ex_mem_size, PP_MEM_TAG, &pos);
	/* here pos max is 7, memaddr[] has 10 positions, just 3 free */
	memtag->size = pos;
	return;
}

