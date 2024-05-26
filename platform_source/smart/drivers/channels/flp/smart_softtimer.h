/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: Contexthub softtimer driver.
 * Create: 2016-03-15
 */

#ifndef __SMART_CONTEXTHUB_SOFTTIMER_H__
#define __SMART_CONTEXTHUB_SOFTTIMER_H__

#include <linux/list.h>

#ifdef CONFIG_SMART_TIMER_V500
#define timer_load(n)                       (((n) << 10) + 0x04)
#define timer_value(n)                      (((n) << 10) + 0x14)
#define timer_ctrl(n)                       (((n) << 10) + 0x08)
#define time_intclr(n)                      (((n) << 10) + 0x10)
#define time_ris(n)                         (((n) << 10) + 0x0C)
#define time_mis(n)                         (((n) << 10) + 0x0C)
#define time_bgload(n)                      (((n) << 10) + 0x04)
#else
#define timer_load(n)                       ((n) << 5)
#define timer_value(n)                      (((n) << 5) + 0x04)
#define timer_ctrl(n)                       (((n) << 5) + 0x08)
#define time_intclr(n)                      (((n) << 5) + 0x0C)
#define time_ris(n)                         (((n) << 5) + 0x10)
#define time_mis(n)                         (((n) << 5) + 0x14)
#define time_bgload(n)                      (((n) << 5) + 0x18)

#define TIMER_EN_ACK                        (1<<4)
#define TIMER_INT_MASK                      (1<<2)
#define TIMER_MODE_PERIOD                   (1<<1)
#define HARD_TIMER_ENABLE                   1
#define HARD_TIMER_DISABLE                  0
#define TIMER_ONCE_COUNT                    0
#define TIMER_PERIOD_COUNT                  1
#endif

typedef void (*softtimer_func)(unsigned long);

struct softtimer_list {
	softtimer_func func;
	unsigned long para;
	unsigned int timeout;
	unsigned int timer_type;
	struct list_head entry;
	unsigned int count_num;
	unsigned int is_base_timer;
	unsigned int init_flags;
};
/*
 * Add soft timer
 */
extern void smart_softtimer_add(struct softtimer_list *timer);
/*
 * Delete soft timer
 */
extern int smart_softtimer_delete(struct softtimer_list *timer);
/*
 * Create a soft timer
 */
extern int smart_softtimer_create(struct softtimer_list *timer, softtimer_func func,
		unsigned long para, unsigned int timeout);
/*
 * Modify soft timer
 */
extern int smart_softtimer_modify(struct softtimer_list *timer, unsigned int new_expire_time);
#endif
