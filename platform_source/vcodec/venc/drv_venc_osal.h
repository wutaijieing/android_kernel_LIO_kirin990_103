/*
 * drv_venc_osal.h
 *
 * This is for venc drv.
 *
 * Copyright (c) 2009-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DRV_VENC_OSAL_H__
#define __DRV_VENC_OSAL_H__

#include <linux/rtc.h>
#include <linux/kfifo.h>
#include <linux/timer.h>
#include "vcodec_type.h"

typedef struct vcodec_kernel_event {
	wait_queue_head_t   queue_head;
	bool              is_done;
} kernel_event_t;

typedef kernel_event_t  vedu_osal_event_t;
typedef unsigned long vedu_lock_flag_t;
typedef struct timer_list venc_timer_t;

#define time_period(begin, end) (((end) >= (begin)) ? ((end) - (begin)) : (~0LL - (begin) + (end)))
#define DEFAULT_PRINT_ENABLE 0xf

typedef enum {
	VENC_FATAL = 0,
	VENC_ERR,
	VENC_WARN,
	VENC_INFO,
	VENC_DBG,
	VENC_ALW,
	VENC_LOG_BUTT,
} venc_print_t;

static const char *psz_msg[(uint8_t)VENC_LOG_BUTT] = {
	"VENC_FATAL",
	"VENC_ERR",
	"VENC_WARN",
	"VENC_IFO",
	"VENC_DBG",
	"VENC_ALW"
}; /*lint !e785*/

#define vcodec_print(type, fmt, arg...) \
do { \
	if (((type) == VENC_ALW) || ((DEFAULT_PRINT_ENABLE & (1LL << (type))) != 0)) \
		printk(KERN_ALERT "%s:<%d:%s>"fmt, psz_msg[type], (int)__LINE__, (char *)__func__, ##arg); \
} while (0)

#define VCODEC_FATAL_VENC(fmt, arg...) vcodec_print(VENC_FATAL, fmt, ##arg)
#define VCODEC_ERR_VENC(fmt, arg...)   vcodec_print(VENC_ERR, fmt, ##arg)
#define VCODEC_WARN_VENC(fmt, arg...)  vcodec_print(VENC_WARN, fmt, ##arg)
#define VCODEC_INFO_VENC(fmt, arg...)  vcodec_print(VENC_INFO, fmt, ##arg)
#define VCODEC_DBG_VENC(fmt, arg...)   vcodec_print(VENC_DBG, fmt, ##arg)


#define OSAL_WAIT_EVENT_TIMEOUT(event, condtion, timeout_in_ms) \
({ \
	int _ret = timeout_in_ms; \
	uint64_t start_time, cur_time; \
	start_time = osal_get_sys_time_in_ms(); \
	while (!(condtion) && (_ret != 0)) { \
		_ret = wait_event_interruptible_timeout(((event)->queue_head), (condtion), \
			(long)(msecs_to_jiffies(timeout_in_ms))); \
		if (_ret < 0) { \
			cur_time = osal_get_sys_time_in_ms(); \
			if (time_period(start_time, cur_time) > (uint64_t)(timeout_in_ms)) { \
				VCODEC_FATAL_VENC("waitevent time out, time : %lld", \
					time_period(start_time, cur_time)); \
				_ret = 0; \
				break; \
			} \
		} \
	} \
	if (_ret == 0) \
		VCODEC_WARN_VENC("waitevent timeout"); \
	if ((condtion)) { \
		_ret = 0; \
	} else { \
		_ret = VCODEC_FAILURE; \
	} \
	_ret; \
})

#define queue_is_empty(queue) kfifo_is_empty(&queue->fifo)
#define queue_is_null(queue) (queue->fifo.kfifo.data == NULL)

#define create_queue(type) \
({ \
	type *queue = NULL; \
	type *tmp = NULL; \
	tmp = (type *)kzalloc(sizeof(type), GFP_KERNEL); \
	if (!IS_ERR_OR_NULL(tmp)) { \
		if (venc_drv_osal_lock_create(&tmp->lock)) { \
			VCODEC_FATAL_VENC("alloc lock failed"); \
			kfree(tmp); \
		} else { \
			venc_drv_osal_init_event(&tmp->event); \
			queue = tmp; \
		} \
	} else { \
		VCODEC_FATAL_VENC("alloc queue failed"); \
	} \
	queue; \
})

#define alloc_queue(queue, size) \
({ \
	int32_t _ret = 0; \
	if (kfifo_alloc(&queue->fifo, size, GFP_KERNEL)) { \
		_ret = VCODEC_FAILURE; \
		VCODEC_FATAL_VENC("alloc kfifo failed"); \
	} \
	_ret; \
})

/* queue->fifo.kfifo.data is set to NULL in kfifo_free. */
#define free_queue(queue) \
({ \
	unsigned long _flag; \
	spin_lock_irqsave(queue->lock, _flag); \
	if (!queue_is_null(queue)) \
		kfifo_free(&queue->fifo); \
	spin_unlock_irqrestore(queue->lock, _flag); \
})

#define destory_queue(queue) \
({ \
	venc_drv_osal_lock_destroy(queue->lock); \
	kfree(queue); \
	queue = NULL; \
})

#define pop(queue, buf) \
({ \
	uint32_t _len; \
	int32_t _ret = VCODEC_FAILURE; \
	unsigned long _flag; \
	spin_lock_irqsave(queue->lock, _flag); \
	do { \
		if (queue_is_null(queue)) { \
			VCODEC_ERR_VENC("pop data is failed"); \
			break; \
		} \
		_len = kfifo_out(&queue->fifo, buf, 1); \
		_ret = (_len == 1) ? 0 : VCODEC_FAILURE; \
	} while (0); \
	spin_unlock_irqrestore(queue->lock, _flag); \
	_ret; \
})

#define push(queue, buf) \
({ \
	uint32_t _len; \
	int32_t _ret = VCODEC_FAILURE; \
	unsigned long _flag; \
	spin_lock_irqsave(queue->lock, _flag); \
	do { \
		if (queue_is_null(queue)) { \
			VCODEC_ERR_VENC("push data is failed"); \
			break; \
		} \
		_len = kfifo_in(&queue->fifo, buf, 1); \
		_ret = (_len == 1) ? 0 : VCODEC_FAILURE; \
	} while (0); \
	spin_unlock_irqrestore(queue->lock, _flag); \
	_ret; \
})

uint32_t      *vcodec_mmap(uint32_t addr, uint32_t range);
void      vcodec_munmap(uint32_t *mem_addr);
int32_t       vcodec_strncmp(const char *str_name, const char *dst_name, int32_t size);
void      vcodec_sleep_ms(uint32_t millisec);
void     *vcodec_mem_valloc(uint32_t mem_size);
void      vcodec_mem_vfree(const void *mem_addr);
void      vcodec_venc_init_sem(void *sem);
int32_t       vcodec_venc_down_interruptible(void *sem);
void      vcodec_venc_up_interruptible(void *sem);

int32_t venc_drv_osal_irq_init(uint32_t irq, irqreturn_t (*callback)(int32_t, void *));
void  venc_drv_osal_irq_free(uint32_t irq);
int32_t   venc_drv_osal_lock_create(spinlock_t **phlock);
void  venc_drv_osal_lock_destroy(spinlock_t *hlock);

int32_t venc_drv_osal_init_event(vedu_osal_event_t *event);
int32_t venc_drv_osal_give_event(vedu_osal_event_t *event);
int32_t venc_drv_osal_wait_event(vedu_osal_event_t *event, uint32_t ms_wait_time);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
void osal_init_timer(struct timer_list *timer, void (*function)(struct timer_list *));
#else
void osal_init_timer(struct timer_list *timer,
		void (*function)(unsigned long),
		unsigned long data);
#endif
void osal_add_timer(struct timer_list *timer, uint64_t time_in_ms);
int32_t osal_del_timer(struct timer_list *timer, bool is_sync);
uint64_t osal_get_sys_time_in_ms(void);
uint64_t osal_get_sys_time_in_us(void);
uint64_t osal_get_during_time(uint64_t start_time);
void *osal_fopen(const char *file_name, int32_t flags, int32_t mode);
void osal_fclose(void *fp);
int32_t osal_fread(void *fp, char *buf, int32_t len);
int32_t osal_fwrite(void *fp, const char *buf, int32_t len);
#endif

