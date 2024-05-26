/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: define powerkey event notifier
 * Create: 2021-11-02
 */
#ifndef _POWERKEY_EVENT_H
#define _POWERKEY_EVENT_H
#include  <linux/notifier.h>

typedef  enum {
	PRESS_KEY_DOWN = 0,
	PRESS_KEY_UP,
	PRESS_KEY_1S,
	PRESS_KEY_6S,
	PRESS_KEY_8S,
	PRESS_KEY_10S,
	PRESS_KEY_MAX
} POWERKEY_EVENT_T;

#ifdef  CONFIG_POWERKEY_SPMI
int powerkey_register_notifier(struct notifier_block *nb);
int powerkey_unregister_notifier(struct notifier_block *nb);
int call_powerkey_notifiers(unsigned long val,void *v);
#else
static inline int powerkey_register_notifier(struct notifier_block *nb){return 0;};
static inline int powerkey_unregister_notifier(struct notifier_block *nb){return 0;};
static inline int call_powerkey_notifiers(unsigned long val, void *v){return 0;};
#endif

#endif
