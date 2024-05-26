/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: aicpufw_api.h
 * Create: 2019-12-21
 */

#ifndef AICPUFW_API_H
#define AICPUFW_API_H

#include <sys/types.h>

#include "drv_base.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct aicpufwConfig {
    int task_start;
} aicpufwConfig_t;
typedef enum tagAicpufwPlat {
    AICPUFW_ONLINE_PLAT = 0,
    AICPUFW_OFFLINE_PLAT,
    AICPUFW_MAX_PLAT,
} aicpufwPlat_t;

int drvSetAicpuConfig(aicpufwConfig_t *config);
int drvGetAicpuConfig(aicpufwConfig_t *config);
drvError_t drvSetAicpuChipID(uint32_t chip_id);
/*
 * mode = ONLINE_PLAT, online, pid is host pid
 * mode = OFFLINE_PLAT, offline, pid is invalid
 */
drvError_t drvCreateAicpuWorkTasks(pid_t pid, int32_t mode);

/*
 * pid: device pid
 */
drvError_t aicpufw_monitor_recycle_so(pid_t pid);

#ifdef __cplusplus
}
#endif
#endif