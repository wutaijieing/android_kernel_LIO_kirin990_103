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

/*lint -save -e537*/
#include "mem_balong.h"
#include <securec.h>

/*lint -restore*/

/*lint -save -e413 -e19*/
/*
 * �궨��
 */
#ifndef __BSP_MEM_DEBUG__
#define __BSP_MEM_DEBUG__
#endif /*__BSP_MEM_DEBUG__*/
/* ÿ���ڴ�ڵ�����ֵ���� */
#define MEM_NODE_MGR_SIZE           32
/* MAGIC������ */
#define MEM_MAGIC_NUM               0x11223344
/* �ڴ�ߴ����� */
u32 g_alloc_list_size[] = { 32, 128, 512, 1024, 2048, 4096, 8192, 0x4000, 0x8000, 0x10000, 0x20000 };
u32 g_alloc_min_num[] = { 512, 256, 100, 10, 1, 2, 10, 10, 1, 1, 1 };
u32 g_alloc_max_num[] = { 1024, 1024, 1024, 1024, 2048, 100, 80, 40, 10, 4, 1 };

#define MEM_ALLOC_LIST_NUM          (sizeof(g_alloc_list_size) / sizeof(u32))

/*
 * ���Ͷ���
 */
/* �ڴ�ڵ�״̬���� */
typedef enum {
    MEM_FREE = 0,
    MEM_ALLOC = 1
} mem_status_e;

/* �ڴ�ع�����Ϣ */
typedef struct {
    u32 bsae_addr;            /* �ڴ�ػ���ַ */
    u32 size;                /* �ڴ���ܴ�С */
    u32 cur_pos_addr;          /* �ڴ�ص�ǰ���䵽��λ�� */
    u32 left;                /* �ڴ��ʣ���С */
    u32 mgr_size;             /* ����ṹ��С */
} mem_pool_info_s;

/* ÿ���ڴ�ڵ�Ĺ�����Ϣ(ע��,���Ҫ���� 32bytes) */
typedef struct {
    u32 magic_number;         /* ���ڼ�鵱ǰ�ڴ���Ƿ���Ч������Debugģʽ��д��ã� */
    u32 next;                /* ����ָ�򱾽ڵ����һ���ڴ� */
    u32 size;                /* �������ڴ��Ĵ�С */
    u32 flags;               /* �������ڴ������ԣ�˫��ʱ��Ҫ����������AXI����DDR�� */
#ifdef __BSP_MEM_DEBUG__
    u32 filename;            /* ʹ�ø��ڴ��� .c �ļ�������Debugģʽ��д��ã� */
    u32 line;                /* ʹ�ø��ڴ���� .c �ļ��е����� */
    u32 status;              /* ��¼��ǰ�ڴ���ʹ��״̬����Malloc״̬����Free״̬ */
#endif
} mem_mgr_info_s;

/* ÿ���ڴ�ڵ��ʹ�ü��� */
typedef struct {
    u32 cur_num;              /* ��ǰʹ�ø������� */
    u32 max_num;              /* ʹ�ø�����ֵ */
    u32 total_malloc_num;      /* �ۻ�malloc���� */
    u32 total_free_num;        /* �ۻ�free���� */
} mem_used_info_s;

/* �ڴ���������Ϣ */
typedef struct {
    u32 alloc_list[MEM_ALLOC_LIST_NUM];  /* �������յ���Ӧ�ڴ�ڵ�Ĵ�С */
    u32 alloc_num[MEM_ALLOC_LIST_NUM];   /* �Ѿ������������Ӧ�ڴ�ڵ������ */
    mem_used_info_s allocUsedInfoList[MEM_ALLOC_LIST_NUM];        /* ����ʹ�� */
    mem_pool_info_s memPoolInfo;  /* �ڴ����Ϣ */
    u32 most_used_item;           /* ��Ƶ��ʹ�õĳߴ�ı�־ */
    u32 alloc_fail_cnt;        /* ����ʧ�ܵĴ��� */
} mem_alloc_info_s;

/*
 * ȫ�ֱ���
 */
LOCAL mem_alloc_info_s g_stlocal_alloc_info[1];

#define AXI_MEM_ADDR        (SRAM_BASE_ADDR)
#define AXI_MEM_SIZE        (HI_SRAM_MEM_SIZE)

#define MEM_CTX_RESERVED    4

u32 *g_palloc_size_tbl = NULL;
mem_alloc_info_s *g_picc_alloc_size_tbl = NULL;
u32 *g_mem_init_mark = NULL;

/*
 * ��ʵ��
 */
#define MEM_GET_ALLOC_SIZE(i)       (*(g_palloc_size_tbl + i))
#define MEM_GET_ALLOC_INFO(type)    (((type) >= MEM_ICC_DDR_POOL) ? \
                                    (((mem_alloc_info_s*)(g_picc_alloc_size_tbl)) + ((u32)(type)-(u32)MEM_ICC_DDR_POOL)) : \
                                    (&g_stlocal_alloc_info[MEM_NORM_DDR_POOL]))
#define MEM_MGR_SIZE_FOR_CACHE      MEM_NODE_MGR_SIZE

#define MEM_GET_ALLOC_ADDR(x)       ((void*)((unsigned long)(x) - (unsigned long)MEM_NODE_MGR_SIZE))
#define MEM_OFFSET_OF(type, member) ((size_t) (&((type *)0)->member))
#define MEM_ITEM_NEXT(x)            (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, next))))
#define MEM_ITEM_SIZE(x)            (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, size))))
#define MEM_ITEM_FLAGS(x)           (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, flags))))
#define MEM_ITEM_MAGIC(x)           (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, magic_number))))
#define MEM_ITEM_FILE_NAME(x)       (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, filename))))
#define MEM_ITEM_LINE(x)            (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, line))))
#define MEM_ITEM_STATUS(x)          (*(u32*)(((x) - MEM_NODE_MGR_SIZE + MEM_OFFSET_OF(mem_mgr_info_s, status))))
#define MEM_IS_AXI_ADDR(ptr) \
((unsigned long)(ptr) >= (unsigned long)AXI_MEM_ADDR && (unsigned long)(ptr) < ((unsigned long)AXI_MEM_ADDR + AXI_MEM_SIZE))

spinlock_t g_mem_spinlock;
unsigned long g_mem_spinflag = 0;

#define MEM_LOCAL_LOCK() \
do {\
    spin_lock_irqsave(&g_mem_spinlock, g_mem_spinflag);\
} while (0)

#define MEM_LOCAL_UNLOCK() spin_unlock_irqrestore(&g_mem_spinlock, g_mem_spinflag)

#define MEM_SPIN_LOCK() \
do {\
    MEM_LOCAL_LOCK();\
    (void)bsp_ipc_spin_lock((u32)IPC_SEM_MEM);\
} while (0)

#define MEM_SPIN_UNLOCK() \
do {\
    (void)bsp_ipc_spin_unlock((u32)IPC_SEM_MEM);\
    MEM_LOCAL_UNLOCK();\
} while (0)

#define MEM_LOCK_BY_TYPE(type)  \
do {\
    if ((mem_pool_type_e)type >= MEM_ICC_DDR_POOL)\
    {\
        MEM_SPIN_LOCK();\
    }\
    else\
    {\
        MEM_LOCAL_LOCK();\
    }\
} while (0)

#define MEM_UNLOCK_BY_TYPE(type) \
do {\
    if ((mem_pool_type_e)type >= MEM_ICC_DDR_POOL)\
    {\
        MEM_SPIN_UNLOCK();\
    }\
    else\
    {\
        MEM_LOCAL_UNLOCK();\
    }\
} while (0)

#define MEM_FLUSH_CACHE(ptr, size)   mb()
#define MEM_INVALID_CACHE(ptr, size) (void)invalidate_kernel_vmap_range(ptr, size)
#define MEM_FLUSH_CACHE_BY_TYPE(ptr, size, type) \
do {\
    if ((mem_pool_type_e)type == MEM_ICC_DDR_POOL)\
    {\
        MEM_FLUSH_CACHE(ptr, size);\
    }\
} while (0)

#define MEM_INVALID_CACHE_BY_TYPE(ptr, size, type)\
do {\
    if ((mem_pool_type_e)type == MEM_ICC_DDR_POOL)\
    {\
        MEM_INVALID_CACHE(ptr, size);\
    }\
} while (0)

#define MEM_DEFINE_TIMES()
#define MEM_INC_TIMES()
#define MEM_PRINT_TIMES(size)

/* ��size list �в��Һ��ʵ��ڴ�ڵ�,����Ҳ����򷵻� MEM_ALLOC_LIST_NUM */
#define MEM_FIND_RIGHT_ITEM(item, size, most_used) \
do {\
    MEM_DEFINE_TIMES();\
    if ((size) > MEM_GET_ALLOC_SIZE(most_used))\
    {\
        MEM_INC_TIMES();\
        for ((item) = (most_used+1); (item) < MEM_ALLOC_LIST_NUM && size > MEM_GET_ALLOC_SIZE(item); (item)++)\
        {\
            MEM_INC_TIMES();\
        }\
    }\
    else\
    {\
        MEM_INC_TIMES();\
        for ((item) = 0; (item) <= (most_used) && (size) > MEM_GET_ALLOC_SIZE(item); (item)++)\
        {\
            MEM_INC_TIMES();\
        }\
        /* �����Ч, ����Чֵ��Ϊ MEM_ALLOC_LIST_NUM */\
        if ((item) > (most_used))\
        {\
            (item) = MEM_ALLOC_LIST_NUM;\
        }\
    }\
    MEM_PRINT_TIMES(size);\
} while (0)

/*
 * �ڲ�����
 */
s32 bsp_init_poll(u32 pool_type)
{
    mem_alloc_info_s *alloc_info = MEM_GET_ALLOC_INFO(pool_type);

    /* �������ַ�ʹ�С */
    switch ((mem_pool_type_e) pool_type) {
        case MEM_NORM_DDR_POOL:
            {
            }
            break;
        case MEM_ICC_DDR_POOL:
            {
                alloc_info->memPoolInfo.cur_pos_addr =
                    alloc_info->memPoolInfo.bsae_addr = (u32)(unsigned long)SHD_DDR_V2P(MEM_ICC_DDR_POOL_BASE_ADDR);
                alloc_info->memPoolInfo.left = alloc_info->memPoolInfo.size = MEM_ICC_DDR_POOL_SIZE;
                alloc_info->memPoolInfo.mgr_size = MEM_MGR_SIZE_FOR_CACHE;
            }
            break;
        case MEM_ICC_AXI_POOL:
            {
            }
            break;
        default:
            mem_print_error("Invalid pool type:%d, line:%d\n", pool_type, __LINE__);
            return BSP_ERROR;
    }
    if (pool_type == MEM_ICC_DDR_POOL) {
        if (!alloc_info->memPoolInfo.bsae_addr) {
            mem_print_error("Invalid pool ptr, line:%d\n", __LINE__);
            return BSP_ERROR;
        }
        /* ��ʼ������ȫ�ֱ��� */
        alloc_info->most_used_item = 0;
    }
    return BSP_OK;
}

BSP_BOOL bsp_ptr_invalid(const void *pmem)
{
    u32 type;
    mem_pool_info_s *pool_info = NULL;
    u32 find_mem = 0;
    void *prel = NULL;

    if (NULL == pmem) {
        return FALSE;
    }
    prel = (void *)SHD_DDR_V2P(pmem);
    for (type = MEM_NORM_DDR_POOL; type < MEM_POOL_MAX; type++) {
        pool_info = &(MEM_GET_ALLOC_INFO(type)->memPoolInfo);
        if ((u32)(uintptr_t)prel >= pool_info->bsae_addr ||
            (u32)(uintptr_t)prel < pool_info->bsae_addr + pool_info->size) {
            find_mem = 1;
        }
    }
    if (!find_mem || MEM_MAGIC_NUM != MEM_ITEM_MAGIC(pmem) || MEM_ITEM_FLAGS(pmem) >= (u32)MEM_POOL_MAX) {
        return TRUE;
    }
    return FALSE;
}

void *bsp_pool_alloc(mem_alloc_info_s *alloc_info, u32 size)
{
    u32 ret_addr = 0;
    void *pv_ret_addr = NULL;

    if (alloc_info->memPoolInfo.left < size) {
        mem_print_error("alloc fail! left size = %x alloc size = %x\n", alloc_info->memPoolInfo.left, size);
        return NULL;
    }

    ret_addr = alloc_info->memPoolInfo.cur_pos_addr;

    pv_ret_addr = (void *)SHD_DDR_P2V((unsigned long)ret_addr);

    alloc_info->memPoolInfo.cur_pos_addr += size;
    alloc_info->memPoolInfo.left -= size;

    return (void *)((uintptr_t)pv_ret_addr + alloc_info->memPoolInfo.mgr_size);
}

void *bsp_get_item(mem_alloc_info_s *alloc_info, u32 cnt, u32 pool_type, u32 size)
{
    void *pitem = NULL;
    void **pphead = (void **)(&(alloc_info->alloc_list[cnt]));

    /* ���������û�нڵ�,����ڴ���з��� */
    if (!*pphead) {
        /* �ж��Ƿ�ﵽ������ */
        if ((pool_type != MEM_ICC_DDR_POOL) || (alloc_info->alloc_num[cnt] < g_alloc_max_num[cnt])) {
            /* ע����ڴ���з���ĳߴ�Ҫ������� MGR �Ĳ��� */
            pitem = bsp_pool_alloc(alloc_info, size + alloc_info->memPoolInfo.mgr_size);
            if (NULL == pitem) {
                alloc_info->alloc_fail_cnt++;
                return NULL;
            }
            MEM_ITEM_MAGIC(pitem) = (u32)MEM_MAGIC_NUM;
            MEM_ITEM_SIZE(pitem) = size;
            MEM_ITEM_FLAGS(pitem) = pool_type;
            if (MEM_ICC_DDR_POOL == pool_type) {
                alloc_info->alloc_num[cnt]++;
            }
#ifdef __BSP_MEM_DEBUG__
            MEM_ITEM_FILE_NAME(pitem) = 0;
            MEM_ITEM_LINE(pitem) = 0;
#endif
        } else {
            alloc_info->alloc_fail_cnt++;
            return NULL;
        }
    }
    /* ��������ȡ���ڵ� */
    else {
        pitem = (void *)SHD_DDR_P2V(*pphead);

        /* Invalid Cache */
        MEM_INVALID_CACHE_BY_TYPE(MEM_GET_ALLOC_ADDR((uintptr_t)pitem), MEM_MGR_SIZE_FOR_CACHE, pool_type);
        *pphead = (void *)((uintptr_t)MEM_ITEM_NEXT((uintptr_t)pitem));
    }
    return pitem;
}

u8 *bsp_memory_alloc(u32 pool_type, u32 size)
{
    u32 cnt;
    void *pitem = NULL;
    mem_alloc_info_s *alloc_info = MEM_GET_ALLOC_INFO(pool_type);
    u32 most_used_item = alloc_info->most_used_item;

    /* �Ȳ���AllocList���Ƿ��п��õ��ڴ�ڵ� */
    MEM_FIND_RIGHT_ITEM(cnt, size, most_used_item);

    /* ���û���ҵ���ֱ�ӷ���ʧ�� */
    if (cnt >= MEM_ALLOC_LIST_NUM) {
        mem_print_error("Invalid malloc size:%d, line:%d\n", size, __LINE__);
        return NULL;
    }

    /* ����sizeΪ�б��е�size */
    size = MEM_GET_ALLOC_SIZE(cnt);
    /*lint -save -e718 -e746*/
    MEM_LOCK_BY_TYPE(pool_type);
    /*lint -restore*/
    pitem = bsp_get_item(alloc_info, cnt, pool_type, size);
    if (NULL != pitem) {
#ifdef __BSP_MEM_DEBUG__
        alloc_info->allocUsedInfoList[cnt].cur_num++;
        alloc_info->allocUsedInfoList[cnt].total_malloc_num++;

        if (alloc_info->allocUsedInfoList[cnt].cur_num > alloc_info->allocUsedInfoList[cnt].max_num) {
            alloc_info->allocUsedInfoList[cnt].max_num = alloc_info->allocUsedInfoList[cnt].cur_num;
        }
        MEM_ITEM_STATUS(pitem) = MEM_ALLOC;
#endif
        /* ���Ҫ Flush Cache, ȷ��������Ϣд�� */
        MEM_FLUSH_CACHE_BY_TYPE(MEM_GET_ALLOC_ADDR(pitem), MEM_MGR_SIZE_FOR_CACHE, pool_type);
    }

    MEM_UNLOCK_BY_TYPE(pool_type);
    return pitem;
}

void bsp_memory_free(u32 pool_type, void *pmem, u32 size)
{
    u32 cnt;
    u32 most_used_item;

    mem_alloc_info_s *alloc_info = MEM_GET_ALLOC_INFO(pool_type);

    most_used_item = alloc_info->most_used_item;
    /* �Ȳ���AllocList���Ƿ��п��õ��ڴ�ڵ� */
    MEM_FIND_RIGHT_ITEM(cnt, size, most_used_item);
#ifdef __BSP_MEM_DEBUG__
    /* �жϸýڵ��Ƿ���Ч */
    if (cnt >= MEM_ALLOC_LIST_NUM) {
        mem_print_error("bsp_pool_alloc Fail, size:%d, line:%d\n", size, __LINE__);
        return;
    }
#endif

    MEM_LOCK_BY_TYPE(pool_type);

    /* ��item�һص����� */
    if (MEM_ICC_AXI_POOL == pool_type) {
    } else if (MEM_ICC_DDR_POOL == pool_type) {
        MEM_ITEM_NEXT(pmem) = (u32)((unsigned long)(alloc_info->alloc_list[cnt]));
        alloc_info->alloc_list[cnt] = (u32)((unsigned long)SHD_DDR_V2P(pmem));
    }
#ifdef __BSP_MEM_DEBUG__
    alloc_info->allocUsedInfoList[cnt].cur_num--;
    alloc_info->allocUsedInfoList[cnt].total_free_num++;
    MEM_ITEM_STATUS(pmem) = MEM_FREE;
#endif
    /* Flush Cache */
    MEM_FLUSH_CACHE_BY_TYPE(MEM_GET_ALLOC_ADDR(pmem), MEM_MGR_SIZE_FOR_CACHE, pool_type);

    MEM_UNLOCK_BY_TYPE(pool_type);
    return;
}

/* ��ʼ���Զ������ */
int bsp_usr_init(void)
{
    return BSP_OK;
}

s32 bsp_mem_ccore_reset_cb(drv_reset_cb_moment_e en_param, int userdata)
{
    u32 pool_type = 0;
    u32 max_init_num = 0;

    if (MDRV_RESET_CB_BEFORE == en_param) {
        *g_mem_init_mark = 0;
        MEM_LOCK_BY_TYPE(MEM_ICC_DDR_POOL);
        if (memset_s((void *)g_picc_alloc_size_tbl, sizeof(*g_picc_alloc_size_tbl), 0, (sizeof(mem_alloc_info_s)))) {
            return BSP_ERROR;
        }
        max_init_num = MEM_POOL_MAX;
        for (pool_type = (u32)MEM_NORM_DDR_POOL; pool_type < (u32)max_init_num; pool_type++) {
            /* �����ڴ�� */
            (void)bsp_init_poll(pool_type);
            (void)bsp_set_most_used_size(512, pool_type);
        }
        *g_mem_init_mark = 1;
        MEM_UNLOCK_BY_TYPE(MEM_ICC_DDR_POOL);
    }
    return BSP_OK;
}

/*
 * �ӿ�ʵ��
 */
s32 bsp_mem_init(void)
{
    u32 pool_type = 0;
    u32 max_init_num = 0;

    spin_lock_init(&g_mem_spinlock);   /*lint !e123*/

    if (memset_s((void *)(SHM_BASE_ADDR + SHM_OFFSET_MEMMGR_FLAG), SHM_SIZE_MEMMGR_FLAG, 0, SHM_SIZE_MEMMGR_FLAG)) {
        return BSP_ERROR;
    }

    g_mem_init_mark = (u32 *)MEM_CTX_ADDR;
    g_palloc_size_tbl = (u32 *)(MEM_CTX_ADDR + MEM_CTX_RESERVED);
    g_picc_alloc_size_tbl = (mem_alloc_info_s *)(MEM_CTX_ADDR + sizeof(g_alloc_list_size) + MEM_CTX_RESERVED);

    if (memset_s((void *)g_picc_alloc_size_tbl, sizeof(*g_picc_alloc_size_tbl), 0, (sizeof(mem_alloc_info_s)))) {
        return BSP_ERROR;
    }
    if (memcpy_s(g_palloc_size_tbl, sizeof(g_alloc_list_size), g_alloc_list_size, sizeof(g_alloc_list_size))) {
        return BSP_ERROR;
    }
    mb();

    max_init_num = MEM_POOL_MAX;
    if (bsp_usr_init() != BSP_OK) {
        mem_print_error("bsp_usr_init call fail, line:%d\n", __LINE__);
    }

    for (pool_type = (u32)MEM_NORM_DDR_POOL; pool_type < (u32)max_init_num; pool_type++) {
        /* �����ڴ�� */
        if (BSP_OK != bsp_init_poll(pool_type)) {
            return BSP_ERROR;
        }

        (void)bsp_set_most_used_size(512, pool_type);
        mb();
    }
    mem_print_error("[init]OK!\n");
    /* ��ʼ���ɹ���ʾ */
    *g_mem_init_mark = 1;
    mb();
    return BSP_OK;
}

#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
module_init(bsp_mem_init);
#endif
s32 bsp_set_most_used_size(u32 size, u32 pool_type)
{
    u32 item;

    if (pool_type != MEM_ICC_DDR_POOL) {
        return BSP_ERROR;
    }

    for ((item) = 0; (item) < MEM_ALLOC_LIST_NUM && size > MEM_GET_ALLOC_SIZE(item); (item)++)
        ;

    if (item >= MEM_ALLOC_LIST_NUM) {
        mem_print_error("invalid size:%d, line:%d\n", size, __LINE__);
        return BSP_ERROR;
    }

    /* ����ʱҪ���� MostItem - 1, ������� */
    MEM_GET_ALLOC_INFO(pool_type)->most_used_item = (item == 0) ? (0) : (item - 1);

    return BSP_OK;
}

/*
 * �� �� ��  : bsp_malloc
 * ��������  : BSP ��̬�ڴ����
 * �������  : size: ����Ĵ�С(byte)
 *             en_flags: �ڴ�����(�ݲ�ʹ��,Ԥ��)
 * �� �� ֵ  : ����������ڴ�ָ��
 */
void *bsp_malloc(u32 size, mem_pool_type_e en_flags)
{
    u8 *pitem = NULL;

    pitem = (u8 *)kmalloc(size, GFP_KERNEL);
    if (pitem == NULL) {
        return NULL;
    }
    if (memset_s((void *)pitem, size, 0, size)) {
        kfree(pitem);
        return NULL;
    }
    return (void *)pitem;
}
EXPORT_SYMBOL(bsp_malloc);

/*
 * �� �� ��  : bsp_malloc_dbg
 * ��������  : BSP ��̬�ڴ����(Debug�ӿ�)
 * �������  : size: ����Ĵ�С(byte)
 *             en_flags: �ڴ�����(�ݲ�ʹ��,Ԥ��)
 *             file_name: ʹ�õ�Դ�ļ�
 *             line:   �����ļ����к�
 * �� �� ֵ  : �ɹ�/ʧ��
 */
void *bsp_malloc_dbg(u32 size, mem_pool_type_e en_flags, u8 *file_name, u32 line)
{
    u8 *pitem = NULL;

    /* �����ڴ� */
    pitem = bsp_memory_alloc((u32)en_flags, (u32)size);

#ifdef __BSP_MEM_DEBUG__
    if (NULL != pitem) {
        /* DebugģʽҪ����MGR ��Ϣ */
        MEM_ITEM_LINE(pitem) = line;
        MEM_ITEM_FILE_NAME(pitem) = (u32)(uintptr_t)file_name;
    }
#endif
    return (void *)pitem;
}

/*
 * �� �� ��  : bsp_free
 * ��������  : BSP ��̬�ڴ��ͷ�
 * �������  : pmem: ��̬�ڴ�ָ��
 */
void bsp_free(void *pmem)
{
    if (pmem != NULL) {
        kfree(pmem);
        pmem = NULL;
    }
}
EXPORT_SYMBOL(bsp_free);

/*
 * �� �� ��  : BSP_Free
 * ��������  : BSP ��̬�ڴ��ͷ�(Debug�ӿ�)
 * �������  : pmem: ��̬�ڴ�ָ��
 *             file_name: ʹ�õ�Դ�ļ�
 *             line:   �����ļ����к�
 */
void bsp_free_dbg(void *pmem, u8 *file_name, u32 line)
{
#ifdef __BSP_MEM_DEBUG__
    /* ��鵱ǰ�ڴ��Ƿ���Ч */
    if (bsp_ptr_invalid(pmem)) {
        mem_print_error("invalid mem block, ptr:0x%lx, line:%d\n", (uintptr_t)pmem, __LINE__);
        return;
    }

    if (MEM_FREE == MEM_ITEM_STATUS(pmem) || MEM_NORM_DDR_POOL != MEM_ITEM_FLAGS(pmem)) {
        mem_print_error("error! ptr:0x%lx, may free twice, or wrong mem flags line:%d\n", (uintptr_t)pmem, __LINE__);
        return;
    }
#endif
    bsp_free(pmem);

    return;
}

/*
 * �� �� ��  : bsp_smalloc
 * ��������  : BSP ��̬�ڴ����(��spin lock����,��˳���ʹ��)
 * �������  : size: ����Ĵ�С(byte)
 *             en_flags: �ڴ�����(�ݲ�ʹ��,Ԥ��)
 * �� �� ֵ  : ����������ڴ�ָ��
 */
void *bsp_smalloc(u32 size, mem_pool_type_e en_flags)
{
    u8 *pitem = NULL;

    if (0 == *g_mem_init_mark) {
        return NULL;
    }
    if (en_flags >= MEM_POOL_MAX || en_flags < MEM_NORM_DDR_POOL) {
        return NULL;
    }
    /* �����ڴ� */
    pitem = bsp_memory_alloc((u32)en_flags, size);

    return (void *)pitem;
}

EXPORT_SYMBOL(bsp_smalloc);

/*
 * �� �� ��  : bsp_smalloc_dbg
 * ��������  : BSP ��̬�ڴ����(��spin lock����,��˳���ʹ��)(Debug�ӿ�)
 * �������  : size: ����Ĵ�С(byte)
 *             en_flags: �ڴ�����(�ݲ�ʹ��,Ԥ��)
 *             file_name: ʹ�õ�Դ�ļ�
 *             line:   �����ļ����к�
 * �� �� ֵ  : �ɹ�/ʧ��
 */
void *bsp_smalloc_dbg(u32 size, mem_pool_type_e en_flags, u8 *file_name, u32 line)
{
    u8 *pitem = NULL;

    if (0 == *g_mem_init_mark) {
        return NULL;
    }
#ifdef __BSP_MEM_DEBUG__
    if ((u32)en_flags >= MEM_POOL_MAX) {
        mem_print_error("invalid mem en_flags:%d, line:%d\n", (u32)en_flags, __LINE__);
        return NULL;
    }
#endif

    /* �����ڴ� */
    pitem = bsp_memory_alloc((u32)en_flags, size);

    return (void *)pitem;
}

/*
 * �� �� ��  : bsp_sfree
 * ��������  : BSP ��̬�ڴ��ͷ�(��spin lock����,��˳���ʹ��)
 * �������  : pmem: ��̬�ڴ�ָ��
 */
void bsp_sfree(void *pmem)
{
    u32 size;
    u32 flags;

    if (0 == *g_mem_init_mark) {
        return;
    }

    /* Invalid Cache */
    if (!MEM_IS_AXI_ADDR((uintptr_t)pmem)) {
        MEM_INVALID_CACHE(MEM_GET_ALLOC_ADDR((uintptr_t)pmem), MEM_MGR_SIZE_FOR_CACHE);
    }
#ifdef __BSP_MEM_DEBUG__
    /* ��鵱ǰ�ڴ��Ƿ���Ч */
    if (bsp_ptr_invalid(pmem) || MEM_FREE == MEM_ITEM_STATUS(pmem) || MEM_ITEM_FLAGS(pmem) == MEM_NORM_DDR_POOL) {
        mem_print_error("error! ptr:0x%lx, invalid mem block, or may free twice, or wrong mem flags line:%d\n",
                        (uintptr_t)pmem, __LINE__);
        return;
    }
#endif

    size = MEM_ITEM_SIZE(pmem);
    flags = MEM_ITEM_FLAGS(pmem);

    bsp_memory_free(flags, pmem, size);
    return;
}

EXPORT_SYMBOL(bsp_sfree);

/*
 * �� �� ��  : BSP_SFree
 * ��������  : BSP ��̬�ڴ��ͷ�(��spin lock����,��˳���ʹ��)(Debug�ӿ�)
 * �������  : pmem: ��̬�ڴ�ָ��
 *             file_name: ʹ�õ�Դ�ļ�
 *             line:   �����ļ����к�
 */
void bsp_sfree_dbg(void *pmem, u8 *file_name, u32 line)
{
    if (0 == *g_mem_init_mark) {
        return;
    }
    bsp_sfree(pmem);
    return;
}

/*
 * ������Ϣʵ��
 */
EXPORT_SYMBOL(bsp_free_dbg);
EXPORT_SYMBOL(bsp_sfree_dbg);
EXPORT_SYMBOL(bsp_malloc_dbg);
EXPORT_SYMBOL(bsp_smalloc_dbg);

