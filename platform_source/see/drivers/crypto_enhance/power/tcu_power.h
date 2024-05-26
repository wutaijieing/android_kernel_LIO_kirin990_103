/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description:
 * Create: 2020/05/18
 */

#ifndef TCU_POWER_H
#define TCU_POWER_H

#include <linux/of_platform.h>
#include <linux/types.h>

#define HIEPS_SUSPEND_DELAY_TIME   50

typedef enum {
	HIEPS_POWER_SUCCESS = 0x5A,
	HIEPS_POWER_FAILED  = 0xA5,
} hieps_power_result;

typedef enum {
	HIEPS_POWER_STATUS_ON  = 0x37,
	HIEPS_POWER_STATUS_OFF = 0xC8,
} hieps_power_status;

int32_t hieps_power_cmd(const uint32_t profile, uint32_t cmd);

int32_t hieps_change_voltage(uint32_t data);

int32_t hieps_suspend(struct platform_device *pdev, struct pm_message state);

int32_t hieps_resume(struct platform_device *pdev);

#endif /* TCU_POWER_H */