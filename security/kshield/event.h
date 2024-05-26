/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: the event.h provides upload API for kshield module
 * Create: 2019-11-09
 */

#ifndef _KSHIELD_EVENT_H_
#define _KSHIELD_EVENT_H_

void gen_exp_event(unsigned int hook_id);
void gen_cha_event(unsigned int hook_id, unsigned long data, unsigned long tick,
			const char *extend_info);

#endif
