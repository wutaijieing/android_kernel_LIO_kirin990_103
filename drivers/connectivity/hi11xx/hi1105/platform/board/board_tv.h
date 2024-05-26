

#ifndef __BOARD_TV_H__
#define __BOARD_TV_H__

/* 其他头文件包含 */
#include "plat_type.h"
#include "hw_bfg_ps.h"

#ifdef _PRE_HI_DRV_GPIO
int PdmGetHwVer(void); /* Get board type. */
int32_t hitv_get_board_power_gpio(struct platform_device *pdev);
int32_t hitv_board_wakeup_gpio_init(struct platform_device *pdev);
#endif

#ifdef _PRE_SUSPORT_OEMINFO
int32_t is_hitv_miniproduct(void);
#endif

#endif
