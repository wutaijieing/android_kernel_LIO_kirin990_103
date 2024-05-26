/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: protocol header file
 * Create: 2021/12/05
 */
#ifndef __PROTOCOL_EXT_H__
#define __PROTOCOL_EXT_H__

#include "protocol_base.h"

#define CMD_ALL 0xff
#define SUBCMD_LEN 4
#define MAX_PKT_LENGTH                       128
#define MAX_PKT_LENGTH_AP                    2560
#define MAX_LOG_LEN                          100
#define MAX_I2C_DATA_LENGTH                  50
#define MAX_SENSOR_CALIBRATE_DATA_LENGTH     60
#define MAX_VIB_CALIBRATE_DATA_LENGTH        12
#define MAX_MAG_CALIBRATE_DATA_LENGTH        12
#define MAX_GYRO_CALIBRATE_DATA_LENGTH       72
#define MAX_GYRO_TEMP_OFFSET_LENGTH          56
#define MAX_CAP_PROX_CALIBRATE_DATA_LENGTH   16
#define MAX_MAG_FOLDER_CALIBRATE_DATA_LENGTH 24
#define MAX_MAG_AKM_CALIBRATE_DATA_LENGTH    28
#define MAX_TOF_CALIBRATE_DATA_LENGTH        47
#define MAX_PS_CALIBRATE_DATA_LENGTH         52
#define MAX_ALS_CALIBRATE_DATA_LENGTH        24

/* data flag consts */
#define DATA_FLAG_FLUSH_OFFSET           0
#define DATA_FLAG_VALID_TIMESTAMP_OFFSET 1
#define FLUSH_END                        (1 << DATA_FLAG_FLUSH_OFFSET)
#define DATA_FLAG_VALID_TIMESTAMP        (1 << DATA_FLAG_VALID_TIMESTAMP_OFFSET)
#define ACC_CALIBRATE_DATA_LENGTH        15
#define GYRO_CALIBRATE_DATA_LENGTH       18
#define PS_CALIBRATE_DATA_LENGTH         13
#define ALS_CALIBRATE_DATA_LENGTH        6
#define ACC1_CALIBRATE_DATA_LENGTH       60
#define ACC1_OFFSET_DATA_LENGTH          15
#define GYRO1_CALIBRATE_DATA_LENGTH      18
#define MAX_GYRO1_CALIBRATE_DATA_LENGTH  72
#define MAX_THP_SYNC_INFO_LEN            (2 * 1024)

typedef enum {
	FINGERPRINT_TYPE_START = 0x0,
	FINGERPRINT_TYPE_HUB,
	FINGERPRINT_TYPE_END,
} fingerprint_type_t;

typedef enum {
	AUTO_MODE = 0,
	FIFO_MODE,
	INTEGRATE_MODE,
	REALTIME_MODE,
	MODE_END
} obj_report_mode_t;

typedef enum {
	/* system status */
	ST_NULL = 0,
	ST_BEGIN,
	ST_POWERON = ST_BEGIN,
	ST_MINSYSREADY,
	ST_DYNLOAD,
	ST_MCUREADY,
	ST_TIMEOUTSCREENOFF,
	ST_SCREENON, /* 6 */
	ST_SCREENOFF, /* 7 */
	ST_SLEEP, /* 8 */
	ST_WAKEUP, /* 9 */
	ST_POWEROFF,
	ST_RECOVERY_BEGIN, /* for ar notify modem when iom3 recovery */
	ST_RECOVERY_FINISH, /* for ar notify modem when iom3 recovery */
	ST_END
} sys_status_t;

typedef enum {
	DUBAI_EVENT_NULL = 0,
	DUBAI_EVENT_AOD_PICKUP = 3,
	DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN = 4,
	DUBAI_EVENT_AOD_TIME_STATISTICS = 6,
	DUBAI_EVENT_FINGERPRINT_ICON_COUNT = 7,
	DUBAI_EVENT_ALL_SENSOR_STATISTICS = 8,
	DUBAI_EVENT_ALL_SENSOR_TIME = 9,
	DUBAI_EVENT_SWING = 10,
	DUBAI_EVENT_TP = 11,
	DUBAI_EVENT_LCD_STATUS_TIMES = 14,
	DUBAI_EVENT_END
} dubai_event_type_t;

typedef enum {
	BIG_DATA_EVENT_MOTION_TYPE = 936005001,
	BIG_DATA_EVENT_DDR_INFO,
	BIG_DATA_EVENT_TOF_PHONECALL,
	BIG_DATA_EVENT_PHONECALL_SCREEN_STATUS,
	BIG_DATA_EVENT_PS_SOUND_INFO = 936005006,
	BIG_DATA_EVENT_VIB_RESP_TIME = 907400028,
	BIG_DATA_FOLD_TEMP = 936004016,
	BIG_DATA_EVENT_SYSLOAD_PERIOD = 936005007,
	BIG_DATA_EVENT_SYSLOAD_TRIGGER = 936005008,
	BIG_DATA_EVENT_AOD_INFO = 936005009,
	BIG_DATA_EVENT_BLPWM_USED_INFO = 936005011,
} big_data_event_id_t;

typedef enum {
	TYPE_STANDARD,
	TYPE_EXTEND
} type_step_counter_t;

typedef enum {
	THP_EVENT_NULL = 0,
	THP_EVENT_LOG,
	THP_EVENT_ALGO_SYNC_INFO,
	THP_EVENT_KEY,
	THP_EVENT_VOLUMN,
	THP_EVENT_END
} thp_shb_event_type_t;

typedef struct {
	unsigned char tag;
	unsigned char partial_order;
} pkt_part_header_t;

struct modem_pkt_header_t {
	unsigned char tag;
	unsigned char cmd;
	unsigned char core : 4;
	unsigned char resp : 1;
	unsigned char partial_order : 3;
	unsigned char length;
};

typedef struct {
	struct pkt_header hd;
	unsigned char wr;
	unsigned int fault_addr;
} __packed pkt_fault_addr_req_t;

typedef struct aod_display_pos {
	unsigned short x_start;
	unsigned short y_start;
} aod_display_pos_t;

typedef struct aod_start_config {
	aod_display_pos_t aod_pos;
	signed int intelli_switching;
} aod_start_config_t;

typedef struct aod_time_config {
	unsigned long long curr_time;
	signed int time_zone;
	signed int sec_time_zone;
	signed int time_format;
} aod_time_config_t;

typedef struct aod_display_space {
	unsigned short x_start;
	unsigned short y_start;
	unsigned short x_size;
	unsigned short y_size;
} aod_display_space_t;

typedef struct aod_display_spaces {
	signed int dual_clocks;
	signed int display_type;
	signed int display_space_count;
	aod_display_space_t display_spaces[5];
} aod_display_spaces_t;

typedef struct aod_screen_info {
	unsigned short xres;
	unsigned short yres;
	unsigned short pixel_format;
} aod_screen_info_t;

typedef struct aod_bitmap_size {
	unsigned short xres;
	unsigned short yres;
} aod_bitmap_size_t;

typedef struct aod_bitmaps_size {
	signed int bitmap_type_count; /* 2, dual clock */
	aod_bitmap_size_t bitmap_size[2];
} aod_bitmaps_size_t;

typedef struct aod_config_info {
	unsigned int aod_fb;
	unsigned int aod_digits_addr;
	aod_screen_info_t screen_info;
	aod_bitmaps_size_t bitmap_size;
} aod_config_info_t;

typedef struct {
	struct pkt_header hd;
	unsigned int sub_cmd;
	union {
		aod_start_config_t start_param;
		aod_time_config_t time_param;
		aod_display_spaces_t display_param;
		aod_config_info_t config_param;
		aod_display_pos_t display_pos;
	};
} aod_req_t;

#define THERM_PARA_LEN 3
typedef struct {
	struct pkt_header hd;
	unsigned int sub_cmd;
	unsigned int para[THERM_PARA_LEN];
} therm_req_t;

typedef struct {
	struct pkt_header hd;
	signed int x;
	signed int y;
	signed int z;
	unsigned int accracy;
} pkt_xyz_data_req_t;

struct sensor_data_xyz {
	signed int x;
	signed int y;
	signed int z;
	unsigned int accracy;
};

typedef struct {
	struct pkt_header hd;
	unsigned short data_flag;
	unsigned short cnt;
	unsigned short len_element;
	unsigned short sample_rate;
	unsigned long long timestamp;
}  pkt_common_data_t;

typedef struct {
	pkt_common_data_t data_hd;
	struct sensor_data_xyz xyz[]; /* x,y,z,acc,time */
} pkt_batch_data_req_t;

typedef struct {
	pkt_common_data_t hd;
	unsigned int status;
	unsigned int support;
} pkt_blpwm_sensor_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned char sub_cmd;
	unsigned char pwm_cycle;
	unsigned char usage;
	unsigned char usage_stop;
	unsigned int color;
} pkt_blpwm_req_t;

typedef struct {
	struct pkt_header hd;
	signed short zaxis_data[];
} pkt_fingersense_data_report_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned short data_flag;
	unsigned int step_count;
	unsigned int begin_time;
	unsigned short record_count;
	unsigned short capacity;
	unsigned int total_step_count;
	unsigned int total_floor_ascend;
	unsigned int total_calorie;
	unsigned int total_distance;
	unsigned short step_pace;
	unsigned short step_length;
	unsigned short speed;
	unsigned short touchdown_ratio;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned short action_record[];
} pkt_step_counter_data_req_t;

typedef struct {
	struct pkt_header hd;
	signed int type;
	signed short serial;
	/* 0: more additional info to be send  1:this pkt is last one */
	signed short end;
	/*
	 * for each frame, a single data type,
	 * either signed int or float, should be used
	 */
	union {
		signed int data_int32[14];
	};
} pkt_additional_info_req_t;

typedef struct {
	pkt_common_data_t data_hd;
	signed int status;
} pkt_magn_bracket_data_req_t;

typedef struct {
	unsigned int type;
	unsigned int initial_speed;
	unsigned int height;
	int angle_pitch;
	int angle_roll;
	unsigned int material;
} drop_info_t;

typedef struct {
	pkt_common_data_t data_hd;
	drop_info_t data;
} pkt_drop_data_req_t;

typedef struct interval_param {
	/* each group data of batch_count upload once, in & out */
	unsigned int period;
	/* input: expected value, out: the closest value actually supported */
	unsigned int batch_count;
	/*
	 * 0: auto mode, mcu reported according to the business characteristics
	 *    and system status
	 * 1: FIFO mode, maybe with multiple records
	 * 2: Integerate mode, update or accumulate the latest data, but do not
	 *    increase the record, and report it
	 * 3: real-time mode, real-time report no matter which status ap is
	 */
	unsigned char mode;
	/* reserved[0]: used by motion and pedometer now */
	unsigned char reserved[3];
} __packed interval_param_t;

typedef struct {
	struct pkt_header hd;
	interval_param_t param;
} __packed pkt_cmn_interval_req_t;

typedef struct {
	struct pkt_header hd;
	char app_config[16];
} __packed pkt_cmn_motion_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	unsigned char para[128];
} __packed pkt_parameter_req_t;

union spi_ctrl {
	unsigned int data;
	struct {
		/* bit0~8 is gpio NO., bit9~11 is gpio iomg set */
		unsigned int gpio_cs : 16;
		/* unit: MHz; 0 means default 5MHz */
		unsigned int baudrate : 5;
		/* low-bit: clk phase , high-bit: clk polarity convert, normally select:0 */
		unsigned int mode : 2;
		/* 0 means default: 8 */
		unsigned int bits_per_word : 5;
		unsigned int rsv_28_31 : 4;
	} b;
};

typedef struct {
	struct pkt_header hd;
	unsigned char busid;
	union {
		unsigned int i2c_address;
		union spi_ctrl ctrl;
	};
	unsigned short rx_len;
	unsigned short tx_len;
	unsigned char tx[];
} pkt_combo_bus_trans_req_t;

typedef struct {
	struct pkt_header_resp hd;
	unsigned char data[];
} pkt_combo_bus_trans_resp_t;

typedef struct {
	struct pkt_header hd;
	unsigned short status;
	unsigned short version;
} pkt_sys_statuschange_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int idle_time;
	unsigned int reserved;
	unsigned long long current_app_mask;
} pkt_power_log_report_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int event_id;
} pkt_big_data_report_t;

typedef struct {
	struct pkt_header hd;
	/* 1:aux sensorlist 0:filelist 2: loading */
	unsigned char file_flg;
	/*
	 * num must less than
	 * (MAX_PKT_LENGTH-sizeof(PKT_HEADER)-sizeof(End))/sizeof(UINT16)
	 */
	unsigned char file_count; /* num of file or aux sensor */
	unsigned short file_list[]; /* fileid or aux sensor tag */
} pkt_sys_dynload_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned char level;
	unsigned char dmd_case;
	unsigned char resv1;
	unsigned char resv2;
	unsigned int dmd_id;
	unsigned int info[5];
} pkt_dmd_log_report_req_t;

typedef struct{
	unsigned short rat;
	unsigned short band;
	signed short power;
	unsigned short rsv;
}eda_dmd_tx_info;

typedef struct{
	unsigned short modem_id;
	unsigned char validflag;
	unsigned char channel;
	unsigned char tas_staus;
	unsigned char mas_staus;
	unsigned short trx_tas_staus;
	eda_dmd_tx_info tx_info[4]; /* 4 modem info */
}eda_dmd_mode_info;


typedef struct {
	struct pkt_header hd;
	unsigned char level;
	unsigned char dmd_case;
	unsigned char resv1;
	unsigned char resv2;
	unsigned int dmd_id;
	eda_dmd_mode_info sensorhub_dmd_mode_info[3]; /* 3 modem */
} pkt_mode_dmd_log_report_req_t;

struct pkt_vibrator_calibrate_data_req_t {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_VIB_CALIBRATE_DATA_LENGTH];
};

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
} pkt_mag_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
} pkt_mag1_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_MAG_AKM_CALIBRATE_DATA_LENGTH];
} pkt_akm_mag_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_GYRO_CALIBRATE_DATA_LENGTH];
} pkt_gyro_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_GYRO1_CALIBRATE_DATA_LENGTH];
} pkt_gyro1_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_GYRO_TEMP_OFFSET_LENGTH];
} pkt_gyro_temp_offset_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_TOF_CALIBRATE_DATA_LENGTH];
} pkt_tof_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_PS_CALIBRATE_DATA_LENGTH];
} pkt_ps_calibrate_data_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_ALS_CALIBRATE_DATA_LENGTH];
} pkt_als_calibrate_data_req_t;

typedef struct {
	pkt_common_data_t fhd;
	signed int data;
} fingerprint_upload_pkt_t;

typedef struct {
	unsigned int sub_cmd;
	unsigned char buf[7]; /* byte alignment */
	unsigned char len;
} fingerprint_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned long long app_id;
	unsigned short msg_type;
	unsigned char res[6];
	unsigned char data[];
} chre_req_t;
typedef struct {
	struct pkt_header hd;
	unsigned char core;
	unsigned char cmd;
	unsigned char data[];
} swing_req_t;

typedef struct{
	unsigned long long sample_start_time;
	unsigned int sample_interval;
	unsigned int integ_time;
} als_run_stop_para_t;

typedef struct {
	struct pkt_header hd;
	unsigned char msg_type;
	unsigned char data[];
} touchpanel_report_pkt_t;

typedef struct {
	unsigned int size;
	char data[MAX_THP_SYNC_INFO_LEN];
} pkt_thp_sync_info_t; /* do not initialize in any function */

typedef enum additional_info_type {
	/* Marks the beginning of additional information frames */
	AINFO_BEGIN = 0x0,
	/* Marks the end of additional information frames */
	AINFO_END   = 0x1,
	/* Basic information */
	/*
	 * Estimation of the delay that is not tracked by sensor
	 * timestamps. This includes delay introduced by
	 * sensor front-end filtering, data transport, etc.
	 * float[2]: delay in seconds
	 * standard deviation of estimated value
	 */
	AINFO_UNTRACKED_DELAY =  0x10000,
	AINFO_INTERNAL_TEMPERATURE, /* float: Celsius temperature */
	/*
	 * First three rows of a homogeneous matrix, which
	 * represents calibration to a three-element vector
	 * raw sensor reading.
	 * float[12]: 3x4 matrix in row major order
	 */
	AINFO_VEC3_CALIBRATION,
	/*
	 * Location and orientation of sensor element in the
	 * device frame: origin is the geometric center of the
	 * mobile device screen surface; the axis definition
	 * corresponds to Android sensor definitions.
	 * float[12]: 3x4 matrix in row major order
	 */
	AINFO_SENSOR_PLACEMENT,
	/*
	 * float[2]: raw sample period in seconds,
	 * standard deviation of sampling period
	 */
	AINFO_SAMPLING,
	/* Sampling channel modeling information */
	/*
	 * signed int: noise type
	 * float[n]: parameters
	 */
	AINFO_CHANNEL_NOISE = 0x20000,
	/*
	 * float[3]: sample period standard deviation of sample period,
	 * quantization unit
	 */
	AINFO_CHANNEL_SAMPLER,
	/*
	 * Represents a filter:
	 * \sum_j a_j y[n-j] == \sum_i b_i x[n-i]
	 * signed int[3]: number of feedforward coefficients, M,
	 * number of feedback coefficients, N, for FIR filter, N=1.
	 * bit mask that represents which element to which the filter is applied
	 * bit 0 == 1 means this filter applies to vector element 0.
	 * float[M+N]: filter coefficients (b0, b1, ..., BM-1),
	 * then (a0, a1, ..., aN-1), a0 is always 1.
	 * Multiple frames may be needed for higher number of taps.
	 */
	AINFO_CHANNEL_FILTER,
	/*
	 * signed int[2]: size in (row, column) ... 1st frame
	 * float[n]: matrix element values in row major order.
	 */
	AINFO_CHANNEL_LINEAR_TRANSFORM,
	/*
	 * signed int[2]: extrapolate method interpolate method
	 * float[n]: mapping key points in pairs, (in, out)...
	 * (may be used to model saturation)
	 */
	AINFO_CHANNEL_NONLINEAR_MAP,
	/*
	 * signed int: resample method (0-th order, 1st order...)
	 * float[1]: resample ratio (upsampling if < 1.0;
	 * downsampling if > 1.0).
	 */
	AINFO_CHANNEL_RESAMPLER,
	/* Custom information */
	AINFO_CUSTOM_START =    0x10000000,
	/* Debugging */
	AINFO_DEBUGGING_START = 0x40000000,
} additional_info_type_t;

#define MAX_THERMO_CALIBRATE_DATA_LENGTH     30
typedef enum {
	MOTION_TYPE_START,
	MOTION_TYPE_PICKUP,
	MOTION_TYPE_FLIP,
	MOTION_TYPE_PROXIMITY,
	MOTION_TYPE_SHAKE,
	MOTION_TYPE_TAP,
	MOTION_TYPE_TILT_LR,
	MOTION_TYPE_ROTATION,
	MOTION_TYPE_POCKET,
	MOTION_TYPE_ACTIVITY,
	MOTION_TYPE_TAKE_OFF,
	MOTION_TYPE_EXTEND_STEP_COUNTER,
	MOTION_TYPE_EXT_LOG, /* type 0xc */
	MOTION_TYPE_HEAD_DOWN,
	MOTION_TYPE_PUT_DOWN,
	MOTION_TYPE_REMOVE,
	MOTION_TYPE_FALL,
	MOTION_TYPE_SIDEGRIP,
	MOTION_TYPE_MOVE,
	MOTION_TYPE_FALL_DOWN,
	MOTION_TYPE_LF_END, /* 100hz sample type end */
	/* 500hz sample type start */
	MOTION_TYPE_TOUCH_LINK = 32,
	MOTION_TYPE_BACK_TAP,
	MOTION_TYPE_END, /* 500hz sample type end */
} motion_type_t;


typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	signed int return_data[MAX_THERMO_CALIBRATE_DATA_LENGTH];
} pkt_thermometer_data_req_t;

#define STARTUP_IOM3_CMD 0x00070001
#define RELOAD_IOM3_CMD 0x0007030D
#define IPC_SHM_MAGIC 0x1a2b3c4d
#define IPC_SHM_BUSY 0x67
#define IPC_SHM_FREE 0xab
#define MID_PKT_LEN (128 - sizeof(struct pkt_header))

struct ipc_shm_ctrl_hdr {
	signed int module_id;
	unsigned int buf_size;
	unsigned int offset;
	signed int msg_type;
	signed int checksum;
	unsigned int priv;
};

struct shmem_ipc_ctrl_package {
	struct pkt_header hd;
	struct ipc_shm_ctrl_hdr sh_hdr;
};

struct ipcshm_data_hdr {
	unsigned int magic_word;
	unsigned char data_free;
	unsigned char reserved[3]; /* reserved */
	struct pkt_header pkt;
};

#endif /* end of include guard: __PROOCOL_EXT_H__ */