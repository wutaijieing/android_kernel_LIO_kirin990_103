/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description:secs_power_ctrl.h.
 * Create: 2017-09-19.
 */

#ifndef __SECS_POWER_CTRL_H__
#define __SECS_POWER_CTRL_H__

int hisi_secs_power_on(void);
int hisi_secs_power_down(void);
unsigned long get_secs_suspend_status(void);

#endif /* end of secs_power_ctrl.h */
