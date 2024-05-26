

#ifndef __OAL_LINUX_MM_H__
#define __OAL_LINUX_MM_H__

/* 其他头文件包含 */
/*lint -e322*/
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

/*lint +e322*/
/* 函数声明 */
/*
 * 函 数 名  : oal_memalloc
 * 功能描述  : 申请核心态的内存空间，并填充0。对于Linux操作系统而言，
 *             需要考虑中断上下文和内核上下文的不同情况(GFP_KERNEL和GFP_ATOMIC)。
 */
OAL_STATIC OAL_INLINE void *oal_memalloc(uint32_t ul_size)
{
    int32_t l_flags = GFP_KERNEL;
    void *puc_mem_space = NULL;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
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
 * 函 数 名  : oal_mem_dma_blockalloc
 * 功能描述  : 尝试申请大块DMA内存，阻塞申请，直到申请成功，可以设置超时
 */
OAL_STATIC OAL_INLINE void *oal_mem_dma_blockalloc(uint32_t size, unsigned long timeout)
{
    gfp_t flags = 0;
    void *puc_mem_space = NULL;
    unsigned long timeout2, timeout1;

    if (unlikely(size == 0)) {
        return NULL;
    }

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled() || in_atomic()) {
        flags |= GFP_ATOMIC;

        return kmalloc(size, flags);
    } else {
        flags |= GFP_KERNEL;
    }

    flags |= __GFP_NOWARN;

    timeout2 = jiffies + msecs_to_jiffies(timeout);     /* ms */
    timeout1 = jiffies + msecs_to_jiffies(timeout / 2); /* 2分之一 */

    do {
        puc_mem_space = kmalloc(size, flags);
        if (likely(puc_mem_space != NULL)) {
            /* sucuess */
            return puc_mem_space;
        }

        if (!time_after(jiffies, timeout1)) {
            cpu_relax();
            continue; /* 未超时，继续 */
        }

        if (!time_after(jiffies, timeout2)) {
            msleep(1);
            continue; /* 长时间未申请到，开始让出调度 */
        } else {
            if (flags & __GFP_NOWARN) {
                /* 超时，清掉报警屏蔽标记，尝试最后一次 */
                flags &= ~__GFP_NOWARN;
                continue;
            } else {
                /* 超时返回失败 */
                break;
            }
        }
    } while (1);

    return NULL;
}

/*
 * 函 数 名  : oal_mem_dma_blockfree
 * 功能描述  : 释放DMA内存
 */
OAL_STATIC OAL_INLINE void oal_mem_dma_blockfree(void *puc_mem_space)
{
    if (likely(puc_mem_space != NULL)) {
        kfree(puc_mem_space);
    }
}

/*
 * 函 数 名  : oal_memtry_alloc
 * 功能描述  : 尝试申请大块内存，指定期望申请的最大和最小内存，
 *             返回实际申请的内存，申请失败返回NULL
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

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
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
            /* 以最小SIZE申请依然失败返回NULL */
            break;
        }

        /* 申请失败, 折半重新申请 */
        request_size = request_size >> 1;
        request_size = ((request_size >= request_minsize) ? request_size : request_minsize);
    } while (1);

    return NULL;
}

/*
 * 函 数 名  : oal_free
 * 功能描述  : 释放核心态的内存空间。
 * 输入参数  : pbuf:需释放的内存地址
 */
OAL_STATIC OAL_INLINE void oal_free(void *p_buf)
{
    kfree(p_buf);
}

#endif /* end of oal_mm.h */
