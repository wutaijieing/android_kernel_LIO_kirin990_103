

#ifndef _OAL_LITEOS_SCATTERLIST_H
#define _OAL_LITEOS_SCATTERLIST_H

#include <linux/scatterlist.h>
#include "oal_skbuff.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */
/* Hi1131 sdio modify begin */
struct sg_table {
    struct scatterlist *sgl;    /* the list */
    unsigned int nents;        /* number of mapped entries */
    unsigned int orig_nents;    /* original size of list */
};
/* Hi1131 sdio modify end */
/* Hi1131 sdio modify begin */
#define sg_is_chain(sg)         ((sg)->page_link & 0x01)
#define sg_is_last(sg)          ((sg)->page_link & 0x02)
#define sg_chain_ptr(sg)        \
    ((struct scatterlist *) ((sg)->page_link & ~0x03))

static struct scatterlist *sg_next(struct scatterlist *sg)
{
#ifdef CONFIG_DEBUG_SG
    BUG_ON(sg->sg_magic != SG_MAGIC);
#endif
    if (sg_is_last(sg))
        return NULL;

    sg++;
    if (unlikely(sg_is_chain(sg)))
        sg = sg_chain_ptr(sg);

    return sg;
}

/*
 * Loop over each sg element, following the pointer to a new list if necessary
 */
#define for_each_sg(sglist, sg, nr, __i)    \
    for ((__i) = 0, (sg) = (sglist); (__i) < (nr); (__i)++, (sg) = sg_next(sg))

#define SG_MAX_SINGLE_ALLOC             (PAGE_SIZE / sizeof(struct scatterlist))
typedef struct scatterlist *(sg_alloc_fn)(unsigned int, gfp_t);

typedef void (sg_free_fn)(struct scatterlist *, unsigned int);


inline void *sg_virt(struct scatterlist *sg)
{
    return ((void *)(sg->dma_address + sg->offset));
}

static void sg_kfree(struct scatterlist *sg, unsigned int nents)
{
    kfree(sg);
}

void __sg_free_table(struct sg_table *table, unsigned int max_ents,
                     sg_free_fn *free_fn)
{
    struct scatterlist *sgl = NULL, *next = NULL;

    if (unlikely(!table->sgl)) {return;}
    sgl = table->sgl;
    while (table->orig_nents) {
        unsigned int alloc_size = table->orig_nents;
        unsigned int sg_size;

        /*
         * If we have more than max_ents segments left,
         * then assign 'next' to the sg table after the current one.
         * sg_size is then one less than alloc size, since the last
         * element is the chain pointer.
         */
        if (unlikely(sgl == NULL)) {
            break;
        }
        if (alloc_size > max_ents) {
            next = sg_chain_ptr(&sgl[max_ents - 1]);
            alloc_size = max_ents;
            sg_size = alloc_size - 1;
        } else {
            sg_size = alloc_size;
            next = NULL;
        }

        table->orig_nents -= sg_size;
        free_fn(sgl, alloc_size);
        sgl = next;
    }

    table->sgl = NULL;
}

void sg_free_table(struct sg_table *table)
{
    __sg_free_table(table, SG_MAX_SINGLE_ALLOC, sg_kfree);
}

static struct scatterlist *sg_kmalloc(unsigned int nents, gfp_t gfp_mask)
{
    return kmalloc(nents * sizeof(struct scatterlist), gfp_mask);
}
static inline void sg_chain(struct scatterlist *prv, unsigned int prv_nents,
                            struct scatterlist *sgl)
{
#ifndef ARCH_HAS_SG_CHAIN
    BUG();
#endif

    /*
     * offset and length are unused for chain entry.  Clear them.
     */
    prv[prv_nents - 1].offset = 0;
    prv[prv_nents - 1].length = 0;

    /*
     * Set lowest bit to indicate a link pointer, and make sure to clear
     * the termination bit if it happens to be set.
     */
    prv[prv_nents - 1].page_link = ((unsigned long)(uintptr_t)sgl | 0x01) & ~0x02;
}
int __sg_alloc_table(struct sg_table *table, unsigned int nents,
                     unsigned int max_ents, gfp_t gfp_mask,
                     sg_alloc_fn *alloc_fn)
{
    struct scatterlist *sg = NULL, *prv = NULL;
    unsigned int left;

    memset_s(table, sizeof(*table), 0, sizeof(*table));

    left = nents;
    prv = NULL;
    do {
        unsigned int sg_size;
        unsigned int alloc_size = left;

        if (alloc_size > max_ents) {
            alloc_size = max_ents;
            sg_size = alloc_size - 1;
        } else {
            sg_size = alloc_size;
        }
        left -= sg_size;
        sg = alloc_fn(alloc_size, gfp_mask);
        if (unlikely(sg == NULL)) {
            /*
             * Adjust entry count to reflect that the last
             * entry of the previous table won't be used for
             * linkage.  Without this, sg_kfree() may get
             * confused.
             */
            if (prv != NULL) {
                table->nents = ++table->orig_nents;
            }
            return -ENOMEM;
        }

        sg_init_table(sg, alloc_size);
        table->nents = table->orig_nents += sg_size;

        /*
         * If this is the first mapping, assign the sg table header.
         * If this is not the first mapping, chain previous part.
         */
        if (prv != NULL) {
            sg_chain(prv, max_ents, sg);
        } else {
            table->sgl = sg;
        }

        /*
         * If no more entries after this one, mark the end
         */
        if (!left) {
            sg_mark_end(&sg[sg_size - 1]);
        }
        prv = sg;
    } while (left);

    return 0;
}

int sg_alloc_table(struct sg_table *table, unsigned int nents, gfp_t gfp_mask)
{
    int ret;

    ret = __sg_alloc_table(table, nents, SG_MAX_SINGLE_ALLOC,
                           gfp_mask, sg_kmalloc);
    if (unlikely(ret))
        __sg_free_table(table, SG_MAX_SINGLE_ALLOC, sg_kfree);

    return ret;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
#endif /* _LINUX_SCATTERLIST_H */
