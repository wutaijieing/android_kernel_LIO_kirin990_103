/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: iomcu_pm.h.
 * Create: 2019/11/05
 */
#ifndef __IOMCU_PM_H
#define __IOMCU_PM_H
#include <platform_include/smart/linux/base/ap/protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ipc_debug {
	int event_cnt[TAG_END];
	int pack_error_cnt;
};

enum sub_power_status {
	SUB_POWER_ON,
	SUB_POWER_OFF
};

struct iomcu_power_status {
	uint8_t power_status;
	uint8_t app_status[TAG_END];
	uint32_t idle_time;
	uint64_t active_app_during_suspend;
};

#define RESUME_INIT 0
#define RESUME_MINI 1
#define RESUME_SKIP 2

#define SENSOR_VBUS "sensor-io"
#define SENSOR_VBUS_LDO12 "lsensor-io"

#define SENSOR_POWER_DO_RESET                           (0)
#define SENSOR_POWER_NO_RESET                           (1)

#define SENSOR_REBOOT_REASON_MAX_LEN 32

/*
 * Function    : sensorhub_io_driver_init
 * Description : driver init for pm
 * Input       : none
 * Output      : none
 * Return      : none
 */
int sensorhub_io_driver_init(void);

/*
 * Function    : sensor_power_check
 * Description : when boot, call this to check sensor power
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise error, always return succ currently
 */
int sensor_power_check(void);

/*
 * Function    : tell_ap_status_to_mcu
 * Description : when suspend or resume, tell ap status to mcu
 * Input       : [ap_st] ap status
 * Output      : none
 * Return      : none
 */
int tell_ap_status_to_mcu(int ap_st);

/*
* Description : get current iom3 sr status
* Output      : none
* Return      : none
*/
sys_status_t get_iom3_sr_status(void);

/*
* Description : enter contexthub s4 status
* Output      : none
* Return      : 0 is ok, otherwise error
*/
int sensorhub_pm_s4_entry(void);

void inputhub_pm_callback(struct pkt_header *head);

int get_iomcu_power_state(void);

void atomic_get_iomcu_power_status(struct iomcu_power_status *p);

#ifdef __cplusplus
}
#endif
#endif /* __IOMCU_PM_H */
