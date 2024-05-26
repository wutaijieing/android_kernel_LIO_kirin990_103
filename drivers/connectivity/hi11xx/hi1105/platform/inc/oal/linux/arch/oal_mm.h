

#ifndef __OAL_LINUX_MM_H__
#define __OAL_LINUX_MM_H__

/* ����ͷ�ļ����� */
/*lint -e322*/
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

/*lint +e322*/
/* �������� */
/*
 * �� �� ��  : oal_memalloc
 * ��������  : �������̬���ڴ�ռ䣬�����0������Linux����ϵͳ���ԣ�
 *             ��Ҫ�����ж������ĺ��ں������ĵĲ�ͬ���(GFP_KERNEL��GFP_ATOMIC)��
 */
OAL_STATIC OAL_INLINE void *oal_memalloc(uint32_t ul_size)
{
    int32_t l_flags = GFP_KERNEL;
    void *puc_mem_space = NULL;

    /* ��˯�߻����жϳ����б�־��ΪGFP_ATOMIC */
    if (in_interrupt() || irqs_disabled() || in_atomic()) {
        l_flags = GFP_ATOMIC;
    }

    if (unlikely(ul_size == 0)) {
        return NULL;
    }

    puc_mem_space = kmalloc(ul_size, l_flags);
    if (puc_mem_space == NULL) {
        return NULL;
    }

    return puc_mem_space;
}

/*
 * �� �� ��  : oal_mem_dma_blockalloc
 * ��������  : ����������DMA�ڴ棬�������룬ֱ������ɹ����������ó�ʱ
 */
OAL_STATIC OAL_INLINE void *oal_mem_dma_blockalloc(uint32_t size, unsigned long timeout)
{
    gfp_t flags = 0;
    void *puc_mem_space = NULL;
    unsigned long timeout2, timeout1;

    if (unlikely(size == 0)) {
        return NULL;
    }

    /* ��˯�߻����жϳ����б�־��ΪGFP_ATOMIC */
    if (in_interrupt() || irqs_disabled() || in_atomic()) {
        flags |= GFP_ATOMIC;

        return kmalloc(size, flags);
    } else {
        flags |= GFP_KERNEL;
    }

    flags |= __GFP_NOWARN;

    timeout2 = jiffies + msecs_to_jiffies(timeout);     /* ms */
    timeout1 = jiffies + msecs_to_jiffies(timeout / 2); /* 2��֮һ */

    do {
        puc_mem_space = kmalloc(size, flags);
        if (likely(puc_mem_space != NULL)) {
            /* sucuess */
            return puc_mem_space;
        }

        if (!time_after(jiffies, timeout1)) {
            cpu_relax();
            continue; /* δ��ʱ������ */
        }

        if (!time_after(jiffies, timeout2)) {
            msleep(1);
            continue; /* ��ʱ��δ���뵽����ʼ�ó����� */
        } else {
            if (flags & __GFP_NOWARN) {
                /* ��ʱ������������α�ǣ��������һ�� */
                flags &= ~__GFP_NOWARN;
                continue;
            } else {
                /* ��ʱ����ʧ�� */
                break;
            }
        }
    } while (1);

    return NULL;
}

/*
 * �� �� ��  : oal_mem_dma_blockfree
 * ��������  : �ͷ�DMA�ڴ�
 */
OAL_STATIC OAL_INLINE void oal_mem_dma_blockfree(void *puc_mem_space)
{
    if (likely(puc_mem_space != NULL)) {
        kfree(puc_mem_space);
    }
}

/*
 * �� �� ��  : oal_memtry_alloc
 * ��������  : �����������ڴ棬ָ�����������������С�ڴ棬
 *             ����ʵ��������ڴ棬����ʧ�ܷ���NULL
 */
OAL_STATIC OAL_INLINE void *oal_memtry_alloc(uint32_t request_maxsize, uint32_t request_minsize,
                                             uint32_t *actual_size, uint32_t align_size)
{
    gfp_t flags = 0;
    uint32_t is_atomic = 0;
    void *puc_mem_space = NULL;
    uint32_t request_size, alloc_size;

    if (WARN_ON(actual_size == NULL)) {
        return NULL;
    }

    *actual_size = 0;

    /* ��˯�߻����жϳ����б�־��ΪGFP_ATOMIC */
    if (in_interrupt() || in_atomic() || irqs_disabled()) {
        is_atomic = 1;
    }

    flags |= __GFP_NOWARN | GFP_ATOMIC;

    request_size = ((request_maxsize >= request_minsize) ? request_maxsize : request_minsize);

    do {
        if (request_size <= request_minsize) {
            if (is_atomic) {
                flags &= ~__GFP_NOWARN;
            } else {
                flags = GFP_KERNEL;
            }
        }
        if (align_size > 0) {
            alloc_size = ALIGN(request_size, align_size);
        } else {
            alloc_size = request_size;
        }

        puc_mem_space = kmalloc(alloc_size, flags);
        if (puc_mem_space != NULL) {
            *actual_size = alloc_size;
            return puc_mem_space;
        }

        if (request_size <= request_minsize) {
            /* ����СSIZE������Ȼʧ�ܷ���NULL */
            break;
        }

        /* ����ʧ��, �۰��������� */
        request_size = request_size >> 1;
        request_size = ((request_size >= request_minsize) ? request_size : request_minsize);
    } while (1);

    return NULL;
}

/*
 * �� �� ��  : oal_free
 * ��������  : �ͷź���̬���ڴ�ռ䡣
 * �������  : pbuf:���ͷŵ��ڴ��ַ
 */
OAL_STATIC OAL_INLINE void oal_free(void *p_buf)
{
    kfree(p_buf);
}

#endif /* end of oal_mm.h */
