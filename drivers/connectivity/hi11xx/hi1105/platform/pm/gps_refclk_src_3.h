

#ifndef __GPS_REFCLK_SRC_3_H__
#define __GPS_REFCLK_SRC_3_H__

#include "oal_util.h"
#include "hisi_oneimage.h"
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0))
#include <linux/huawei/hw_connectivity.h>
#include <linux/huawei/gps/huawei_gps.h>
#else
#include <huawei_platform/connectivity/hw_connectivity.h>
#include <huawei_platform/connectivity/huawei_gps.h>
#endif

typedef struct {
    bool enable;
    gps_modem_id_enum modem_id;
    gps_rat_mode_enum rat;
} gps_refclk_param;

/* EXTERN FUNCTION */
extern int hi_gps_plat_init(void);
extern void hi_gps_plat_exit(void);
extern int set_gps_ref_clk_enable_hi110x(bool enable, gps_modem_id_enum modem_id,
    gps_rat_mode_enum rat_mode);
#endif
