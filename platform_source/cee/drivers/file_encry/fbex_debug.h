/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: fbex ca header files
 * Create : 2020/01/02
 */

#ifndef __FBEX_DEBUG_H_
#define __FBEX_DEBUG_H_

#include <linux/types.h>
#include <platform_include/see/fbe_ctrl.h>

#define FBEX_MAGIC01  0xA5
#define FBEX_MAGIC02  0x5A
#define TEST_TIMEOUT  4000

#ifdef CONFIG_FILE_BASED_ENCRYPTO_DBG

int fbex_init_debugfs(void);
void fbex_cleanup_debugfs(void);
#else
static inline int fbex_init_debugfs(void) {return 0; }
static inline void fbex_cleanup_debugfs(void) {}
#endif
#endif /* __FBEX_DEBUG_H_ */
