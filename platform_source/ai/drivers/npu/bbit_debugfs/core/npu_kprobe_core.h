/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */
#ifndef __NPU_KPROBE_CORE_H
#define __NPU_KPROBE_CORE_H

#define NPU_CTRL_CORE_FUNC "npu_ctrl_core"

int npu_kprobe_core_ablility(kprobe_cmd_t cmd, char *func_name, uint8_t index);

#endif
