/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub activity recognition driver.
 * Create: 2017-03-31
 */

#ifndef	SMART_AR_H
#define	SMART_AR_H

#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include "smart_softtimer.h"
#include "inputhub_api/inputhub_api.h"

#define FLP_TAG_AR          2
#define FLP_IOCTL_TYPE_AR               0x464C0010

#define FLP_IOCTL_CMD_MASK              0xFFFF00FF

#define FLP_IOCTL_TAG_MASK              0xFFFFFF00
#define FLP_IOCTL_TAG_FLP               0x464C0000
#define FLP_IOCTL_TAG_GPS               0x464C0100
#define FLP_IOCTL_TAG_AR                0x464C0200
#define FLP_IOCTL_TAG_COMMON            0x464CFF00

#define FLP_AR_DATA	(0x1<<1)
#define FLP_IOCTL_TYPE_COMMON           0x464C00F0
#define CONTEXT_PRIVATE_DATA_MAX (64)
#define DATABASE_DATALEN (64*1024)
/* common ioctrl */
#define FLP_IOCTL_COMMON_GET_UTC            (0x464C0000 + 0xFFF1)
#define FLP_IOCTL_COMMON_SLEEP              (0x464C0000 + 0xFFF2)
#define FLP_IOCTL_COMMON_AWAKE_RET          (0x464C0000 + 0xFFF3)
#define FLP_IOCTL_COMMON_SETPID             (0x464C0000 + 0xFFF4)
#define FLP_IOCTL_COMMON_CLOSE_SERVICE      (0x464C0000 + 0xFFF5)
#define FLP_IOCTL_COMMON_HOLD_WAKELOCK      (0x464C0000 + 0xFFF6)
#define FLP_IOCTL_COMMON_RELEASE_WAKELOCK   (0x464C0000 + 0xFFF7)

#define flp_ioctl_ar_start(x)           (0x464C0000 + ((x) * 0x100) + 0x11)
#define flp_ioctl_ar_stop(x)            (0x464C0000 + ((x) * 0x100) + 0x12)
#define flp_ioctl_ar_read(x)            (0x464C0000 + ((x) * 0x100) + 0x13)
#define flp_ioctl_ar_update(x)          (0x464C0000 + ((x) * 0x100) + 0x15)
#define flp_ioctl_ar_flush(x)           (0x464C0000 + ((x) * 0x100) + 0x16)
#define flp_ioctl_ar_state(x)           (0x464C0000 + ((x) * 0x100) + 0x17)
#define flp_ioctl_ar_config(x)           (0x464C0000 + ((x) * 0x100) + 0x18)
#define flp_ioctl_ar_state_v2(x)           (0x464C0000 + ((x) * 0x100) + 0x1D)

#define FLP_IOCTL_TYPE_MASK             0xFFFF00F0
#define FLP_IOCTL_TYPE_COMMON           0x464C00F0

#define MAX_CONFIG_SIZE           (32 * 1024)

enum dts_status {
	DTS_DISABLED = 0x00,
	EXT_SENSORHUB_DTS_ENABLED = 0x01,
	SENSORHUB_DTS_ENABLED,
};

enum {
	AR_ACTIVITY_VEHICLE         = 0x00,
	AR_ACTIVITY_RIDING          = 0x01,
	AR_ACTIVITY_WALK_SLOW       = 0x02,
	AR_ACTIVITY_RUN_FAST        = 0x03,
	AR_ACTIVITY_STATIONARY      = 0x04,
	AR_ACTIVITY_TILT            = 0x05,
	AR_ACTIVITY_END             = 0x10,
	AR_VE_BUS                   = 0x11, /* bus */
	AR_VE_CAR                   = 0x12, /* car */
	AR_VE_METRO                 = 0x13, /* subway */
	AR_VE_HIGH_SPEED_RAIL       = 0x14, /* high-speed rail */
	AR_VE_AUTO                  = 0x15, /* Road traffic */
	AR_VE_RAIL                  = 0x16, /* Railway traffic */
	AR_CLIMBING_MOUNT           = 0x17, /* Mountain climbing */
	AR_FAST_WALK                = 0x18, /* Go fast */
	AR_STOP_VEHICLE             = 0x19, /* STOP VEHICLE */
	AR_VEHICLE_UNKNOWN          = 0x1B, /* on vehicle, but not known which type */
	AR_ON_FOOT                  = 0x1C, /* ON FOOT */
	AR_OUTDOOR                  = 0x1F, /* outdoor:1, indoor:0 */
	AR_ELEVATOR                 = 0x20, /* elevator */
	AR_RELATIVE_STILL           = 0x21, /* relative still */
	AR_WALKING_HANDHOLD         = 0x22,
	AR_LYING_POSTURE            = 0x23,
	AR_SMART_FLIGHT             = 0x25,
	AR_PLANE                    = 0x26, /* airplane */
	AR_STAY                     = 0x27, /* stay */
	AR_UNKNOWN                  = 0x3F,
	AR_STATE_BUTT               = 0x40,
};

#define CONTEXT_TYPE_MAX (0x40) /* MAX(AR_STATE_BUTT, AR_ENVIRONMENT_END) */

enum {
	AR_STATE_ENTER = 1,
	AR_STATE_EXIT = 2,
	AR_STATE_ENTER_EXIT = 3,
	AR_STATE_MAX
};

struct ar_packet_header_t {
	unsigned char tag;
	unsigned char  cmd;
	unsigned char  resp : 1;
	unsigned char  rsv : 3;
	unsigned char  core : 4;   /* AP set to zero */
	unsigned char  partial_order;
	unsigned short tranid;
	unsigned short length;
};   /* compatable to struct pkt_header */

struct ar_activity_event_t {
	unsigned int        event_type;         /* 0:disable */
	unsigned int        activity;
	unsigned long       msec;
} __packed;

struct ar_activity_config_t {
	unsigned int        event_type;         /* 0:disable */
	unsigned int        activity;
} __packed;

struct ar_start_config_t {
	unsigned int            report_interval;
	struct ar_activity_config_t    activity_list[AR_STATE_BUTT];
};

struct ar_start_hal_config_t {
	unsigned int            report_interval;
	unsigned int            event_num;
	struct ar_activity_config_t    *pevent;
};

struct ar_start_mcu_config_t {
	unsigned int            event_num;
	unsigned int            report_interval;
	struct ar_activity_config_t    activity_list[AR_STATE_BUTT];
};

/* common */
struct ar_context_cfg_header_t {
	unsigned int	event_type;
	unsigned int	context;
#ifndef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	unsigned int	report_interval;
#endif
	unsigned int	len;
} __packed;


/* HAL--->KERNEL */
struct context_config_t {
	struct ar_context_cfg_header_t head;
	char buf[CONTEXT_PRIVATE_DATA_MAX];
} __packed;


struct context_hal_config_t {
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	unsigned int report_interval;
#endif
	unsigned int	context_num;
	struct context_config_t *context_addr;
} __packed;


/* KERNEL-->HUB */
struct context_iomcu_cfg_t {
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	unsigned int report_interval;
#endif
	unsigned int context_num;
	struct context_config_t context_list[CONTEXT_TYPE_MAX];
};


#define STATE_KERNEL_BUF_MAX	1024
struct context_dev_info_t {
	unsigned int	usr_cnt;
	unsigned int	cfg_sz;
	struct context_iomcu_cfg_t   cfg;
	struct completion state_cplt;
	unsigned int state_buf_len;
	char state_buf[STATE_KERNEL_BUF_MAX];
};

/* HUB-->KERNEL */
#define GET_STATE_NUM_MAX 64
struct context_event_t {
	unsigned int event_type;
	unsigned int context;
	unsigned long long msec; /* long long :keep some with iomcu */
	int confident;
	unsigned int buf_len;
	char buf[0];
} __packed;

struct ar_data_req_t {
	struct pkt_header pkt;
	unsigned int context_num;
	struct context_event_t context_event[0];
} __packed;

struct ar_state_t {
	unsigned int context_num;
	struct context_event_t context_event[0];
} __packed;

struct ar_state_shmen_t {
	unsigned int user_len;
	struct context_event_t context_event[CONTEXT_TYPE_MAX];
} __packed;

/* define flp netlink */
#define AR_GENL_NAME                   "TASKAR"
#define TASKAR_GENL_VERSION            0x01

enum {
	AR_GENL_ATTR_UNSPEC,
	AR_GENL_ATTR_EVENT,
	__AR_GENL_ATTR_MAX,
};
#define AR_GENL_ATTR_MAX (__AR_GENL_ATTR_MAX - 1)

enum {
	AR_GENL_CMD_UNSPEC,
	AR_GENL_CMD_PDR_DATA,
	AR_GENL_CMD_AR_DATA,
	AR_GENL_CMD_PDR_UNRELIABLE,
	AR_GENL_CMD_NOTIFY_TIMEROUT,
	AR_GENL_CMD_AWAKE_RET,
	AR_GENL_CMD_GEOFENCE_TRANSITION,
	AR_GENL_CMD_GEOFENCE_MONITOR,
	AR_GENL_CMD_GNSS_LOCATION,
	AR_GENL_CMD_IOMCU_RESET,
	__AR_GENL_CMD_MAX,
};
#define AR_GENL_CMD_MAX (__AR_GENL_CMD_MAX - 1)

#define HISMART_AR_DBG KERN_INFO
#define HISMART_AR_DBG_DUMP KERN_DEBUG
/*lint +e64*/
/*lint +e785*/
struct ar_data_buf_t {
	char  *data_buf;
	unsigned int buf_size;
	unsigned int read_index;
	unsigned int write_index;
	unsigned int data_count;
	unsigned int  data_len;
};

struct ar_hdr_t {
	unsigned char   core;
	unsigned char   rsv1;
	unsigned char   rsv2;
	unsigned char   sub_cmd;
};

struct ar_ioctl_cmd_t {
	struct pkt_header    hd;
	struct ar_hdr_t sub_cmd;
};

extern int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length,
					  struct read_info *rd);

struct ar_port_t {
	struct work_struct work;
	struct wakeup_source *wlock;
	struct softtimer_list   sleep_timer;
	struct list_head list;
	unsigned int channel_type;
	struct ar_data_buf_t ar_buf;
	struct context_iomcu_cfg_t  ar_config;
	unsigned int portid;
	unsigned int flushing;
	unsigned int need_awake;
	unsigned int need_hold_wlock;
	unsigned int work_para;
};

/* ENV -E */
struct ar_device_t {
	struct list_head list;
	unsigned int service_type;
	struct mutex lock;
	struct mutex state_lock;
	unsigned int denial_sevice;
	struct completion get_complete;
	struct ar_state_shmen_t activity_shemem;
	struct context_dev_info_t ar_devinfo;

	struct workqueue_struct *wq;
};

int get_ar_data_from_mcu(const struct pkt_header *head);
int ar_state_shmem(const struct pkt_header *head);

#endif
