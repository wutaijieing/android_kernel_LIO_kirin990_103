

#ifndef __OAL_LITEOS_SDIO_IF_H__
#define __OAL_LITEOS_SDIO_IF_H__

#ifdef CONFIG_MMC
#include <mmc/host.h>
#include <mmc/sdio_func.h>
#include <mmc/sdio.h>
#include <mmc/card.h>
#include <gpio.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#else

#include <mmc_sdio/host.h>
#include <mmc_sdio/sdio_func.h>
#include <mmc_sdio/sdio.h>
#include <mmc_sdio/card.h>
#include <mmc_sdio/core.h>
#include <gpio.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#endif


#ifdef CONFIG_MMC

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define SDIO_GET_MAX_BLOCK_COUNT(func)     (func->card->host->max_blk_num)
#define SDIO_GET_MAX_REQ_SIZE(func)        (func->card->host->max_request_size)
#define SDIO_GET_MAX_BLK_SIZE(func)        (func->card->host->max_blk_size)
#define SDIO_EN_TIMEOUT(func)        (func->en_timeout_ms)

#define SDIO_FUNC_NUM(func)             (func->func_num)

/* Hi1131 sdio modify begin */
#define SDIO_ANY_ID (~0)

#define SDIO_DEVICE(vend, dev) \
    .class = SDIO_ANY_ID, \
    .vendor = (vend), .device = (dev)

struct sdio_device_id {
    unsigned char    class;                  /* Standard interface or SDIO_ANY_ID */
    unsigned short int   vendor;                 /* Vendor or SDIO_ANY_ID */
    unsigned short int   device;                 /* Device ID or SDIO_ANY_ID */
};
/* Hi1131 sdio modify end */
/* Hi1131 sdio modify begin */
#define sdio_get_drvdata(func)                (func->data)
#define sdio_set_drvdata(func, priv)        (func->data = (void *)priv)


/*
 * SDIO function device driver
 */
struct sdio_driver {
    char *name;
    const struct sdio_device_id *id_table;

    int (*probe)(struct sdio_func *, const struct sdio_device_id *);
    void (*remove)(struct sdio_func *);
};

/* Hi1131 sdio modify end */
#define sdio_enable_func(func)      sdio_en_func(func)
#define sdio_disable_func(func)      sdio_dis_func(func)
#define sdio_set_block_size(func, blksz)      sdio_set_cur_blk_size(func, blksz)
#define sdio_readb(func, addr, err)      sdio_read_byte(func, addr, err)
#define sdio_writeb(func, byte, addr, err)      sdio_write_byte(func, byte, addr, err)
#define sdio_memcpy_fromio(func, dst, addr, size)   sdio_read_incr_block(func, dst, addr, size)


OAL_STATIC OAL_INLINE int32_t sdio_register_driver(struct sdio_driver *driver)
{
    return OAL_SUCC;
}

extern struct sdio_func *g_sdio_func;
OAL_STATIC OAL_INLINE void sdio_unregister_driver(struct sdio_driver *driver)
{
    return;
}

OAL_STATIC OAL_INLINE void sdio_claim_host(struct sdio_func *func)
{
    mmc_acquire_card(func->card);
    return;
}

OAL_STATIC OAL_INLINE void sdio_release_host(struct sdio_func *func)
{
    mmc_release_card(func->card);
    return;
}

/**
 *  oal_sdio_writesb - write to a FIFO of a SDIO function
 *  @func: SDIO function to access
 *  @addr: address of (single byte) FIFO
 *  @src: buffer that contains the data to write
 *  @count: number of bytes to write
 *
 *  Writes to the specified FIFO of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  int32_t oal_sdio_writesb(struct sdio_func *func, uint32_t addr, void *src,
                                                int count)
{
    int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
    time_start = oal_ktime_get();
#endif

    ret = sdio_write_fifo_block(func, addr, src, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if (oal_unlikely(ret)) {
        /* If sdio transfer failed, dump the sdio info */
        uint64_t  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]oal_sdio_writesb fail=%d, time cost:%llu us,[src:%p,addr:%u,count:%d]\n",
               ret, trans_us, src, addr, count);
    }
#endif
    return ret;
}


/**
 *  oal_sdio_readsb - read from a FIFO on a SDIO function
 *  @func: SDIO function to access
 *  @dst: buffer to store the data
 *  @addr: address of (single byte) FIFO
 *  @count: number of bytes to read
 *
 *  Reads from the specified FIFO of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  int32_t oal_sdio_readsb(struct sdio_func *func, void *dst, uint32_t addr,
                                               int32_t count)
{
    int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
    time_start = oal_ktime_get();
#endif

    ret = sdio_read_fifo_block(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if (oal_unlikely(ret)) {
        /* If sdio transfer failed, dump the sdio info */
        uint64_t  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_readsb fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
               ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

/**
 *  oal_sdio_readb - read a single byte from a SDIO function
 *  @func: SDIO function to access
 *  @addr: address to read
 *  @err_ret: optional status value from transfer
 *
 *  Reads a single byte from the address space of a given SDIO
 *  function. If there is a problem reading the address, 0xff
 *  is returned and @err_ret will contain the error code.
 */
OAL_STATIC OAL_INLINE uint8_t oal_sdio_readb(struct sdio_func *func, uint32_t addr, int32_t *err_ret)
{
    return sdio_read_byte(func, addr, err_ret);
}

/**
 *  oal_sdio_writeb - write a single byte to a SDIO function
 *  @func: SDIO function to access
 *  @b: byte to write
 *  @addr: address to write to
 *  @err_ret: optional status value from transfer
 *
 *  Writes a single byte to the address space of a given SDIO
 *  function. @err_ret will contain the status of the actual
 *  transfer.
 */
OAL_STATIC OAL_INLINE void oal_sdio_writeb(struct sdio_func *func, uint8_t b, uint32_t addr, int32_t *err_ret)
{
    sdio_write_byte(func, b, addr, err_ret);
}

/**
 *  oal_sdio_readl - read a 32 bit integer from a SDIO function
 *  @func: SDIO function to access
 *  @addr: address to read
 *  @err_ret: optional status value from transfer
 *
 *  Reads a 32 bit integer from the address space of a given SDIO
 *  function. If there is a problem reading the address,
 *  0xffffffff is returned and @err_ret will contain the error
 *  code.
 */
OAL_STATIC OAL_INLINE uint32_t oal_sdio_readl(struct sdio_func *func, uint32_t addr, int32_t *err_ret)
{
    uint32_t val;
    int32_t ret;

    ret = sdio_read_incr_block(func, &val, addr, 4);
    *err_ret = ret;

    if (ret)
        return 0xFF;
    else
        return val;
}

/**
 *  oal_sdio_writel - write a 32 bit integer to a SDIO function
 *  @func: SDIO function to access
 *  @b: integer to write
 *  @addr: address to write to
 *  @err_ret: optional status value from transfer
 *
 *  Writes a 32 bit integer to the address space of a given SDIO
 *  function. @err_ret will contain the status of the actual
 *  transfer.
 */
OAL_STATIC OAL_INLINE void oal_sdio_writel(struct sdio_func *func, uint32_t b,
                                           uint32_t addr, int32_t *err_ret)
{
    int32_t ret;
    uint32_t val = b;
    ret = sdio_write_incr_block(func, addr, &val, 4);
    if (err_ret != NULL)
        *err_ret = ret;
}

/**
 *  sdio_memcpy_fromio - read a chunk of memory from a SDIO function
 *  @func: SDIO function to access
 *  @dst: buffer to store the data
 *  @addr: address to begin reading from
 *  @count: number of bytes to read
 *
 *  Reads from the address space of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE int32_t oal_sdio_memcpy_fromio(struct sdio_func *func, void *dst,
                                                     uint32_t addr, int32_t count)
{
    int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
    time_start = oal_ktime_get();
#endif
    ret = sdio_memcpy_fromio(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if (oal_unlikely(ret)) {
        /* If sdio transfer failed, dump the sdio info */
        uint64_t  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_memcpy_fromio fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
               ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

#else

#define SDIO_GET_MAX_BLOCK_COUNT(func)     ((func)->card->host->max_blk_count)
#define SDIO_GET_MAX_REQ_SIZE(func)        ((func)->card->host->max_req_size)
#define SDIO_GET_MAX_BLK_SIZE(func)        ((fun)->card->host->max_blk_size)
#define SDIO_EN_TIMEOUT(func)        ((func)->enable_timeout)

#define SDIO_FUNC_NUM(func)             ((func)->num)

/**
 *  oal_sdio_writesb - write to a FIFO of a SDIO function
 *  @func: SDIO function to access
 *  @addr: address of (single byte) FIFO
 *  @src: buffer that contains the data to write
 *  @count: number of bytes to write
 *
 *  Writes to the specified FIFO of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  int32_t oal_sdio_writesb(struct sdio_func *func, uint32_t addr, void *src,
                                                int count)
{
int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
oal_time_t_stru time_start;
time_start = oal_ktime_get();
#endif

ret = sdio_writesb(func, addr, src, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
if (oal_unlikely(ret)) {
    /* If sdio transfer failed, dump the sdio info */
    uint64_t  trans_us;
    oal_time_t_stru time_stop = oal_ktime_get();
    trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
    printk(KERN_WARNING"[E]oal_sdio_writesb fail=%d, time cost:%llu us,[src:%p,addr:%u,count:%d]\n",
           ret, trans_us, src, addr, count);
}
#endif
return ret;
}


/**
 *  oal_sdio_readsb - read from a FIFO on a SDIO function
 *  @func: SDIO function to access
 *  @dst: buffer to store the data
 *  @addr: address of (single byte) FIFO
 *  @count: number of bytes to read
 *
 *  Reads from the specified FIFO of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  int32_t oal_sdio_readsb(struct sdio_func *func, void *dst, uint32_t addr,
                                               int32_t count)
{
    int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
    time_start = oal_ktime_get();
#endif

    ret = sdio_readsb(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if (oal_unlikely(ret)) {
        /* If sdio transfer failed, dump the sdio info */
        uint64_t  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_readsb fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
               ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

/**
 *  oal_sdio_readb - read a single byte from a SDIO function
 *  @func: SDIO function to access
 *  @addr: address to read
 *  @err_ret: optional status value from transfer
 *
 *  Reads a single byte from the address space of a given SDIO
 *  function. If there is a problem reading the address, 0xff
 *  is returned and @err_ret will contain the error code.
 */
OAL_STATIC OAL_INLINE uint8_t oal_sdio_readb(struct sdio_func *func, uint32_t addr, int32_t *err_ret)
{
    return sdio_readb(func, addr, err_ret);
}

/**
 *  oal_sdio_writeb - write a single byte to a SDIO function
 *  @func: SDIO function to access
 *  @b: byte to write
 *  @addr: address to write to
 *  @err_ret: optional status value from transfer
 *
 *  Writes a single byte to the address space of a given SDIO
 *  function. @err_ret will contain the status of the actual
 *  transfer.
 */
OAL_STATIC OAL_INLINE void oal_sdio_writeb(struct sdio_func *func, uint8_t b, uint32_t addr, int32_t *err_ret)
{
    sdio_writeb(func, b, addr, err_ret);
}

/**
 *  oal_sdio_readl - read a 32 bit integer from a SDIO function
 *  @func: SDIO function to access
 *  @addr: address to read
 *  @err_ret: optional status value from transfer
 *
 *  Reads a 32 bit integer from the address space of a given SDIO
 *  function. If there is a problem reading the address,
 *  0xffffffff is returned and @err_ret will contain the error
 *  code.
 */
OAL_STATIC OAL_INLINE uint32_t oal_sdio_readl(struct sdio_func *func, uint32_t addr, int32_t *err_ret)
{
    return sdio_readl(func, addr, err_ret);
}

/**
 *  oal_sdio_writel - write a 32 bit integer to a SDIO function
 *  @func: SDIO function to access
 *  @b: integer to write
 *  @addr: address to write to
 *  @err_ret: optional status value from transfer
 *
 *  Writes a 32 bit integer to the address space of a given SDIO
 *  function. @err_ret will contain the status of the actual
 *  transfer.
 */
OAL_STATIC OAL_INLINE void oal_sdio_writel(struct sdio_func *func, uint32_t b,
                                           uint32_t addr, int32_t *err_ret)
{
    sdio_writel(func, b, addr, err_ret);
}

/**
 *  sdio_memcpy_fromio - read a chunk of memory from a SDIO function
 *  @func: SDIO function to access
 *  @dst: buffer to store the data
 *  @addr: address to begin reading from
 *  @count: number of bytes to read
 *
 *  Reads from the address space of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE int32_t oal_sdio_memcpy_fromio(struct sdio_func *func, void *dst,
                                                     uint32_t addr, int32_t count)
{
    int32_t ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
    time_start = oal_ktime_get();
#endif
    ret = sdio_memcpy_fromio(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if (oal_unlikely(ret)) {
        /* If sdio transfer failed, dump the sdio info */
        uint64_t  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (uint64_t)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_memcpy_fromio fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
               ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

OAL_STATIC OAL_INLINE int32_t sdio_register_driver(struct sdio_driver *driver)
{
    return OAL_SUCC;
}

extern struct sdio_func *g_sdio_func;
OAL_STATIC OAL_INLINE void sdio_unregister_driver(struct sdio_driver *driver)
{
    driver->remove(g_sdio_func);
    return;
}

OAL_STATIC OAL_INLINE void sdio_claim_host(struct sdio_func *func)
{
    return;
}

OAL_STATIC OAL_INLINE void sdio_release_host(struct sdio_func *func)
{
    return;
}

OAL_STATIC OAL_INLINE int sdio_require_irq(struct sdio_func *func, sdio_irq_handler_t *handler)
{
    return sdio_claim_irq(func, handler);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
#endif /* end of oal_sdio_if.h */

