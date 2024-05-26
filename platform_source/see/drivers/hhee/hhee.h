/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: hhee header files
 * Create : 2016/12/6
 */

#ifndef __HHEE_H__
#define __HHEE_H__

#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <linux/arm-smccc.h>
#include <linux/interrupt.h>
#include <platform_include/see/hkip_hhee.h>

/* struct for logbuf */
struct circular_buffer {
	unsigned long size; /* Indicates the total size of the buffer */
	unsigned long start; /* Starting point of valid data in buffer */
	unsigned long end; /* First character which is empty (can be written to) */
	unsigned long overflow; /* Indicator whether buffer has overwritten itself */
	unsigned long virt_log_addr; /* Indicator the virtual addr of buffer */
	unsigned long virt_log_size; /* Indicator the max size of buffer */
	unsigned int inited; /* Indicator the status of buffer */
	unsigned int logtype; /* Indicator the type of buffer */
	char *buf;
};

/* enum for logtype */
enum ltype {
	NORMAL_LOG,
	CRASH_LOG,
	MONITOR_LOG,
};

#define XOM_DEFAULT_READ_COUNT 20

irqreturn_t hhee_irq_handle(int irq, void *data);
int hhee_logger_init(void);
void reset_hkip_irq_counters(void);

#ifdef CONFIG_HHEE_DEBUG
int hhee_init_debugfs(void);
void hhee_cleanup_debugfs(void);
ssize_t hhee_copy_logs(char __user *buf, size_t count, loff_t *offp,
		       int logtpye);
#endif

#ifdef CONFIG_HKIP_MODULE_ALLOC
void hhee_module_init(void);
#else
static inline void hhee_module_init(void){};
#endif

#endif /* __HHEE_H_ */
