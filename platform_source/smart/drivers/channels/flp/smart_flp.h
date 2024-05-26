/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Contexthub flp driver.
 * Create: 2017-02-18
 */

#ifndef __SMART_CONTEXTHUB_FLP_H__
#define __SMART_CONTEXTHUB_FLP_H__
#include <linux/types.h>

/* ioctrl cmd type */
#define FLP_TAG_FLP         0
#define FLP_TAG_GPS         1

#define FLP_GPS_BATCHING_MAX_SIZE   50
#define FLP_GEOFENCE_MAX_NUM        2000
#define FLP_MAX_WIFIFENCE_SIZE      116

#define FLP_IOCTL_CMD_MASK              0xFFFF00FF

#define FLP_IOCTL_TAG_MASK              0xFFFFFF00
#define FLP_IOCTL_TAG_FLP               0x464C0000
#define FLP_IOCTL_TAG_GPS               0x464C0100
#define FLP_IOCTL_TAG_AR                0x464C0200
#define FLP_IOCTL_TAG_COMMON            0x464CFF00

#define FLP_IOCTL_TYPE_MASK             0xFFFF00F0
#define FLP_IOCTL_TYPE_PDR              0x464C0000
#define FLP_IOCTL_TYPE_GEOFENCE         0x464C0020
#define FLP_IOCTL_TYPE_BATCHING         0x464C0030
#define FLP_IOCTL_TYPE_CELLFENCE        0x464C0040
#define FLP_IOCTL_TYPE_WIFIFENCE        0x464C0050
#define FLP_IOCTL_TYPE_DIAG             0x464C0060

#define FLP_IOCTL_TYPE_COMMON           0x464C00F0

#define flp_ioctl_pdr_start(x)          (0x464C0000 + ((x) * 0x100) + 1)
#define flp_ioctl_pdr_stop(x)           (0x464C0000 + ((x) * 0x100) + 2)
#define flp_ioctl_pdr_read(x)           (0x464C0000 + ((x) * 0x100) + 3)
#define flp_ioctl_pdr_write(x)          (0x464C0000 + ((x) * 0x100) + 4)
#define flp_ioctl_pdr_config(x)         (0x464C0000 + ((x) * 0x100) + 5)
#define flp_ioctl_pdr_update(x)         (0x464C0000 + ((x) * 0x100) + 6)
#define flp_ioctl_pdr_flush(x)          (0x464C0000 + ((x) * 0x100) + 7)
#define flp_ioctl_pdr_step_cfg(x)       (0x464C0000 + ((x) * 0x100) + 8)


#define FLP_IOCTL_GEOFENCE_ADD                  (0x464C0000 + 0x21)
#define FLP_IOCTL_GEOFENCE_REMOVE               (0x464C0000 + 0x22)
#define FLP_IOCTL_GEOFENCE_MODIFY               (0x464C0000 + 0x23)
#define FLP_IOCTL_GEOFENCE_STATUS               (0x464C0000 + 0x24)
#define FD_IOCTL_GEOFENCE_GET_LOCATION          (0x464C0000 + 0x25)
#define FD_IOCTL_GEOFENCE_GET_SIZE              (0x464C0000 + 0x26)

#define FLP_IOCTL_BATCHING_START                (0x464C0000 + 0x31)
#define FLP_IOCTL_BATCHING_STOP                 (0x464C0000 + 0x32)
#define FLP_IOCTL_BATCHING_UPDATE               (0x464C0000 + 0x33)
#define FLP_IOCTL_BATCHING_CLEANUP              (0x464C0000 + 0x34)
#define FLP_IOCTL_BATCHING_LASTLOCATION         (0x464C0000 + 0x35)
#define FLP_IOCTL_BATCHING_FLUSH                (0x464C0000 + 0x36)
#define FLP_IOCTL_BATCHING_INJECT               (0x464C0000 + 0x37)
#define FLP_IOCTL_BATCHING_GET_SIZE             (0x464C0000 + 0x38)
#define FLP_IOCTL_BATCHING_SEND_GNSS_STATUS     (0x464C0000 + 0x39)
#define FLP_IOCTL_BATCHING_MULT_LASTLOCATION    (0x464C0000 + 0x3A)
#define FLP_IOCTL_BATCHING_MULT_FLUSH           (0x464C0000 + 0x3B)

#define FD_IOCTL_CELLFENCE_ADD                  (FLP_IOCTL_TAG_FLP + 0x41)
#define FD_IOCTL_CELLFENCE_OPERATE              (FLP_IOCTL_TAG_FLP + 0x42)
#define FD_IOCTL_TRAJECTORY_CONFIG              (FLP_IOCTL_TAG_FLP + 0x43)
#define FD_IOCTL_TRAJECTORY_REQUEST             (FLP_IOCTL_TAG_FLP + 0x44)
#define FD_IOCTL_CELLFENCE_INJECT_RESULT        (FLP_IOCTL_TAG_FLP + 0x45)
#define FD_IOCTL_CELLFENCE_ADD_V2               (FLP_IOCTL_TAG_FLP + 0x46)
#define FD_IOCTL_CELLBATCHING_CONFIG            (FLP_IOCTL_TAG_FLP + 0x47)
#define FD_IOCTL_CELLBATCHING_REQUEST           (FLP_IOCTL_TAG_FLP + 0x48)
#define FD_IOCTL_CELLFENCE_GET_SIZE             (FLP_IOCTL_TAG_FLP + 0x49)

#define FD_IOCTL_WIFIFENCE_ADD                  (0x464C0000 + 0x51)
#define FD_IOCTL_WIFIFENCE_REMOVE               (0x464C0000 + 0x52)
#define FD_IOCTL_WIFIFENCE_PAUSE                (0x464C0000 + 0x53)
#define FD_IOCTL_WIFIFENCE_RESUME               (0x464C0000 + 0x54)
#define FD_IOCTL_WIFIFENCE_GET_STATUS           (0x464C0000 + 0x55)
#define FD_IOCTL_WIFIFENCE_GET_SIZE             (0x464C0000 + 0x56)

#define FD_IOCTL_DIAG_SEND_CMD                  (FLP_IOCTL_TAG_FLP + 0x61)

/* common ioctl */
#define FLP_IOCTL_COMMON_GET_UTC                (0x464C0000 + 0xFFF1)
#define FLP_IOCTL_COMMON_SLEEP                  (0x464C0000 + 0xFFF2)
#define FLP_IOCTL_COMMON_AWAKE_RET              (0x464C0000 + 0xFFF3)
#define FLP_IOCTL_COMMON_SETPID                 (0x464C0000 + 0xFFF4)
#define FLP_IOCTL_COMMON_CLOSE_SERVICE          (0x464C0000 + 0xFFF5)
#define FLP_IOCTL_COMMON_HOLD_WAKELOCK          (0x464C0000 + 0xFFF6)
#define FLP_IOCTL_COMMON_RELEASE_WAKELOCK       (0x464C0000 + 0xFFF7)
#define FLP_IOCTL_COMMON_STOP_SERVICE_TYPE      (0x464C0000 + 0xFFF8)
#define FLP_IOCTL_COMMON_WIFI_CFG               (0x464C0000 + 0xFFF9)
#define FLP_IOCTL_COMMON_INJECT                 (0x464C0000 + 0xFFFA)
#define FLP_IOCTL_COMMON_LOCATION_STATUS        (0x464C0000 + 0xFFFB)
#define FLP_IOCTL_COMMON_DEBUG_CONFIG           (0x464C0000 + 0xFFFC)
#define FLP_IOCTL_COMMON_INJECT_DATA            (0x464C0000 + 0xFFFD)


enum {
	FLP_SERVICE_RESET,
	FLP_GNSS_RESET,
	FLP_GNSS_RESUME,
	FLP_IOMCU_RESET,
};

struct compensate_data_t {
	unsigned int        compensate_step;
	int                 compensate_position_x;
	int                 compensate_position_y;
	unsigned int        compensate_distance;
};

struct step_report_t {
	int data[12];
};

struct buf_info_t {
	char            *buf;
	unsigned long   len;
};

/* define Batching and Geofence struct */
struct iomcu_location {
	u16      flags;
	double   latitude;
	double   longitude;
	double   altitude;
	float    speed;
	float    bearing;
	float    accuracy;
	s64      timestamp;
	u32      sources_used;
} __packed;

struct gps_batching_report_t {
	s32 num_locations;
	struct iomcu_location location[FLP_GPS_BATCHING_MAX_SIZE];
} __packed;

/* modify Geofence */
struct geofencing_option_info_t {
	unsigned int  virtual_id;
	unsigned char last_transition;
	unsigned char monitor_transitions;
	unsigned short int accuracy;
	unsigned int unknown_timer_ms;
	unsigned int sources_to_use;
	unsigned int floor;
	unsigned int parent_fence_id;
	unsigned int report_type;
} __packed;

struct geofencing_circle_t {
	double latitude;
	double longitude;
	double radius_m;
} __packed;

struct geofencing_point_t {
	double latitude;
	double longitude;
} __packed;

#define POLYGON_POINT_NUM_MAX 8
struct geofencing_polygon_t {
	int polynum;
	struct geofencing_point_t point[POLYGON_POINT_NUM_MAX];
} __packed;

struct geofencing_data_t {
	unsigned int geofence_type;
	union {
		struct geofencing_circle_t circle;
		struct geofencing_polygon_t polygon;
	} geofence;
} __packed;

struct geofencing_useful_data_t {
	struct geofencing_option_info_t  opt;
	struct geofencing_data_t info;
} __packed;

/* start or update batching cmd */
struct flp_batch_options {
	double max_power_allocation_mw;
	u32 sources_to_use;
	u32 flags;
	s64 period_ns;
	float smallest_displacement_meters;
} __packed;

struct batching_config_t {
	s32  id;
	s32  batching_distance;
	struct flp_batch_options opt;
} __packed;

#define LOCATION_SOURCE_SUPPORT (2)
struct loc_source_status {
	unsigned int source;
	unsigned int status[LOCATION_SOURCE_SUPPORT];
};

struct loc_source_status_hal {
	unsigned int source;
	unsigned int status;
};

/* cellfence -s */
#define CELLFENCE_ADD_INFO_BUF_SIZE (500*1024)
struct cellfence_ioctrl_hdr_type {
	unsigned int len;
	char __user *buf;
} __packed;

enum {
	FLP_SHMEM_ADD_GEOFENCE = 0x1,
	FLP_SHMEM_ADD_CELLFENCE,
	FLP_SHMEM_ADD_WIFIFENCE,
	FLP_SHMEM_ADD_CELLFENCE_V2,
	FLP_SHMEM_REMOVE_GEOFENCE,
	FLP_SHMEM_CMD_INJECT_DATA,
};

#define FLP_IOMCU_SHMEM_TAG        (0xABAB)
#define SUB_CMD_FLP_SHMEM_REQ      (0xACAC)

struct flp_shmem_hdr {
	unsigned short int tag; /*0xABAB*/
	unsigned short int cmd; /*shmem cmd*/
	unsigned int transid;
	unsigned int data_len;
	char data[0];
} __packed;

struct cell_batching_start_config_t {
	unsigned int cmd;
	unsigned int interval;
	unsigned int reserved_1;
	unsigned int reserved_2;
};

struct cell_batching_data_t {
	unsigned int  timestamplow;
	unsigned int  timestamphigh;
	unsigned int  cid;
	unsigned short lac;
	short rssi;
	unsigned short mcc;
	unsigned short mnc;
};
/* cellfence -e */

/* wififence -b */
#define MAX_ADD_WIFI_NUM      (50)
#define WIFI_BSSID_INCLUDE_RSSI_NUM  (420)

struct hal_config_t {
	unsigned int  length;
	void *buf;
} __packed;

struct wififence_req_t {
	unsigned int id;
	unsigned int parentid;
	unsigned short int rssi;
	unsigned short int num;
	unsigned char bssid[WIFI_BSSID_INCLUDE_RSSI_NUM];
} __packed;

/* wififence -e */

/* define flp netlink */
#define FLP_GENL_NAME                   "TASKFLP"
#define TASKFLP_GENL_VERSION            0x01

enum {
	FLP_GENL_ATTR_UNSPEC,
	FLP_GENL_ATTR_EVENT,
	__FLP_GENL_ATTR_MAX,
};
#define FLP_GENL_ATTR_MAX (__FLP_GENL_ATTR_MAX - 1)

enum {
	FLP_GENL_CMD_UNSPEC,
	FLP_GENL_CMD_PDR_DATA,
	FLP_GENL_CMD_AR_DATA,
	FLP_GENL_CMD_PDR_UNRELIABLE,
	FLP_GENL_CMD_NOTIFY_TIMEROUT,
	FLP_GENL_CMD_AWAKE_RET,
	FLP_GENL_CMD_GEOFENCE_TRANSITION,
	FLP_GENL_CMD_GEOFENCE_MONITOR,
	FLP_GENL_CMD_GNSS_LOCATION,
	FLP_GENL_CMD_IOMCU_RESET,
	FLP_GENL_CMD_CELLTRANSITION,
	FLP_GENL_CMD_TRAJECTORY_REPORT,
	FLP_GENL_CMD_REQUEST_CELLDB,
	FLP_GENL_CMD_REQUEST_WIFIDB,
	FLP_GENL_CMD_WIFIFENCE_TRANSITION,
	FLP_GENL_CMD_WIFIFENCE_MONITOR,
	FLP_GENL_CMD_GEOFENCE_LOCATION,
	FLP_GENL_CMD_CELLBATCHING_REPORT,
	FLP_GENL_CMD_DIAG_DATA_REPORT,
	FLP_GENL_CMD_GEOFENCE_SIZE,
	FLP_GENL_CMD_CELLFENCE_SIZE,
	FLP_GENL_CMD_WIFIFENCE_SIZE,
	__FLP_GENL_CMD_MAX,
};
#define FLP_GENL_CMD_MAX (__FLP_GENL_CMD_MAX - 1)
extern unsigned int g_flp_debug_level;
/* register for Fused Location Provider function */
extern int flp_register(void);
/* unregister for Fused Location Provider function */
extern int flp_unregister(void);
/* resume for Fused Location Provider port function */
extern void flp_port_resume(void);

#endif
