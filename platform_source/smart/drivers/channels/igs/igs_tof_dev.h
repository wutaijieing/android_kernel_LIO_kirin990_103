

#ifndef _SWING_TOF_DEV_H
#define _SWING_TOF_DEV_H
#include <linux/types.h>

#define NAME_LEN 32
#define BUS_TYPE_NAME_LEN 8
#define PARA_MAX_LEN 32

struct query_swing_tof_msg {
	u32 i2c_index;
	char name[NAME_LEN];
	char bus_type[BUS_TYPE_NAME_LEN];
};

struct ack_query_swing_tof_msg {
	char name[NAME_LEN];
	u32 status;
};

struct acquire_swing_tof_msg {
	char sensor_name[NAME_LEN];
	char i2c_bus_type[BUS_TYPE_NAME_LEN];
	u32 i2c_index;
	u32 phy_id;
	u32 phy_work_mode;
	u32 phy_mode;
	u32 tof_calib;
};

struct ack_acquire_swing_tof_msg {
	u32 status;
};

enum set_swing_tof_power_status_result_msg {
	TOF_POWER_ON_OK = 0,
	TOF_POWER_ON_FAIL,
	TOF_POWER_DOWN_OK,
	TOF_POWER_DOWN_FAIL,
	TOF_POWER_STATUS_MAX
};

struct set_swing_tof_status_result_msg {
	u32 status;
};

struct ack_set_swing_tof_power_status_result_msg {
	u32 status;
};

enum tof_event_info {
	EVENT_TOF_POWER_ON,
	EVENT_TOF_POWER_OFF,
};

struct swing_tof_event {
	u32 event_id;
	char event_params[PARA_MAX_LEN];
};

enum swing_tof_upload_type {
	MATCH_ID = 0,
	ACUIQRE,
	REQ_POWER_ON_OFF,
	SET_POWER_ON_OFF_RESULT,
	SWING_TOF_RECOVERY = 0xFF,
	UPLOAD_TYPE_MAX
};

struct swing_tof_msg {
	u32 upload_type;
	union {
		/* request items */
		struct query_swing_tof_msg query_swing_tof;
		struct acquire_swing_tof_msg acquire_swing_tof;
		struct set_swing_tof_status_result_msg
			set_swing_tof_status_result;

		/* ack items */
		struct ack_query_swing_tof_msg ack_query_swing_tof;
		struct ack_acquire_swing_tof_msg ack_acquire_swing_tof;
		struct ack_set_swing_tof_power_status_result_msg
			ack_set_swing_tof_status_result;

		/* sent to ap */
		struct swing_tof_event event;
	} u;
};

#define SWING_TOF_IO 0xD1
#define SWING_TOF_MATCH_ID _IOWR(SWING_TOF_IO, 0xD1, struct swing_tof_msg)
#define SWING_TOF_ACQUIRE _IOWR(SWING_TOF_IO, 0xD2, struct swing_tof_msg)
#define SWING_TOF_SET_POWER_ON_OFF_RESULT _IOWR(SWING_TOF_IO, 0xD3, struct swing_tof_msg)

#endif /* _SWING_TOF_DEV_H */
