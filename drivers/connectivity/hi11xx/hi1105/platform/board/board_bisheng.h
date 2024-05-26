

#ifndef __BOARD_BISHENG_H__
#define __BOARD_BISHENG_H__

/* 其他头文件包含 */
#include "plat_type.h"
#include "hw_bfg_ps.h"

/* 函数声明 */
int32_t bisheng_wifi_subsys_reset(void);
void bisheng_bfgx_subsys_reset(void);
void board_info_init_bisheng(void);

#endif
