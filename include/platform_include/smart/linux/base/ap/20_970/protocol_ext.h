/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: protocol header file
 * Create: 2021/12/05
 */
#ifndef __PROTOCOL_EXT_H__
#define __PROTOCOL_EXT_H__

#include "protocol_base.h"

#define SUBCMD_LEN                                  4
#define MAX_PKT_LENGTH                              128
#define MAX_PKT_LENGTH_AP                           2560
#define MAX_LOG_LEN                                 100
#define MAX_PATTERN_SIZE                            16
#define MAX_ACCEL_PARAMET_LENGTH                    100
#define MAX_MAG_PARAMET_LENGTH                      100
#define MAX_GYRO_PARAMET_LENGTH                     100
#define MAX_ALS_PARAMET_LENGTH                      100
#define MAX_PS_PARAMET_LENGTH                       100
#define MAX_I2C_DATA_LENGTH                         50
#define MAX_SENSOR_CALIBRATE_DATA_LENGTH            60
#define MAX_MAG_CALIBRATE_DATA_LENGTH               12
#define MAX_GYRO_CALIBRATE_DATA_LENGTH              72
#define MAX_GYRO_TEMP_OFFSET_LENGTH                 56
#define MAX_CAP_PROX_CALIBRATE_DATA_LENGTH          16
#define MAX_MAG_FOLDER_CALIBRATE_DATA_LENGTH        24
#define MAX_MAG_AKM_CALIBRATE_DATA_LENGTH           28
// data flag consts
#define DATA_FLAG_FLUSH_OFFSET                      (0)
#define DATA_FLAG_VALID_TIMESTAMP_OFFSET            (1)
#define FLUSH_END                                   (1<<DATA_FLAG_FLUSH_OFFSET)
#define DATA_FLAG_VALID_TIMESTAMP                   (1<<DATA_FLAG_VALID_TIMESTAMP_OFFSET)
#define ACC_CALIBRATE_DATA_LENGTH                   (15)
#define GYRO_CALIBRATE_DATA_LENGTH                  (18)
#define PS_CALIBRATE_DATA_LENGTH                    (3)
#define ALS_CALIBRATE_DATA_LENGTH                   (6)
#define ACC1_CALIBRATE_DATA_LENGTH                  60
#define ACC1_OFFSET_DATA_LENGTH                     15
#define GYRO1_CALIBRATE_DATA_LENGTH                 18

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
	MOTION_TYPE_EXT_LOG, //type 0xc
	MOTION_TYPE_HEAD_DOWN,
	MOTION_TYPE_PUT_DOWN,
	//
	MOTION_TYPE_SIDEGRIP, //sensorhub internal use, must at bottom;
	/*!!!NOTE:add string in motion_type_str when add type*/
	MOTION_TYPE_END,
} motion_type_t;

enum fp_type {
	FP_TYPE_START = 0x0,
	FP_TYPE_HUB,
	FP_TYPE_END,
};

typedef enum
{
	CA_TYPE_START,
	CA_TYPE_PICKUP,
	CA_TYPE_PUTDOWN,
	CA_TYPE_ACTIVITY,
	CA_TYPE_HOLDING,
	CA_TYPE_MOTION,
	CA_TYPE_PLACEMENT,
/*!!!NOTE:add string in ca_type_str when add type*/
	CA_TYPE_END,
}ca_type_t;

typedef enum
{
	AUTO_MODE = 0,
	FIFO_MODE,
	INTEGRATE_MODE,
	REALTIME_MODE,
	MODE_END
} obj_report_mode_t;

/* system status */
typedef enum {
	ST_NULL = 0,
	ST_BEGIN,
	ST_POWERON = ST_BEGIN,
	ST_MINSYSREADY,
	ST_DYNLOAD,
	ST_MCUREADY,
	ST_TIMEOUTSCREENOFF,
	ST_SCREENON,               /* 6 */
	ST_SCREENOFF,              /* 7 */
	ST_SLEEP,                  /* 8 */
	ST_WAKEUP,                 /* 9 */
	ST_POWEROFF,
	ST_RECOVERY_BEGIN,         // for ar notify modem when iom3 recovery
	ST_RECOVERY_FINISH,        // for ar notify modem when iom3 recovery
	ST_END
} sys_status_t;

typedef enum {
	DUBAI_EVENT_NULL = 0,
	DUBAI_EVENT_AOD_PICKUP = 3,
	DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN =4,
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
	BIG_DATA_EVENT_TOF_PHONECALL
} big_data_event_id_t;

typedef enum {
	TYPE_STANDARD,
	TYPE_EXTEND
} type_step_counter_t;

typedef struct {
	unsigned char tag;
	unsigned char partial_order;
} pkt_part_header_t;

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
	signed int bitmap_type_count;       // 2, dual clock
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

typedef struct
{
	struct pkt_header hd;
	unsigned short data_flag;
	unsigned short cnt;
	unsigned short len_element;
	unsigned short sample_rate;
	unsigned long long timestamp;
}  pkt_common_data_t;

typedef struct
{
	pkt_common_data_t data_hd;
	struct sensor_data_xyz xyz[];	/* x,y,z,acc,time */
} pkt_batch_data_req_t;

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

typedef struct
{
	struct pkt_header hd;
	signed int type;
	signed short serial;
	signed short end;        // 0: more additional info to be send  1:this pkt is last one
	union {
		signed int data_int32[14];
	};
}pkt_additional_info_req_t;

typedef struct
{
	pkt_common_data_t data_hd;
	signed int status;
} pkt_magn_bracket_data_req_t;

typedef struct
{
	unsigned int type;
	unsigned int initial_speed;
	unsigned int height;
	int angle_pitch;
	int angle_roll;
	unsigned int material;
} drop_info_t;

typedef struct
{
	pkt_common_data_t data_hd;
	drop_info_t   data;
} pkt_drop_data_req_t;

typedef struct interval_param{
	unsigned int  period;
	unsigned int batch_count;
	unsigned char mode;
	unsigned char reserved[3];
}__packed interval_param_t;

typedef struct {
	struct pkt_header hd;
	interval_param_t param;
}__packed pkt_cmn_interval_req_t;

typedef struct {
	struct pkt_header hd;
	char app_config[16];
}__packed pkt_cmn_motion_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	unsigned char para[128];
}__packed pkt_parameter_req_t;

union spi_ctrl {
	unsigned int data;
	struct {
		unsigned int gpio_cs   : 16;    /* bit0~8 is gpio NO., bit9~11 is gpio iomg set */
		unsigned int baudrate  : 5;     /* unit: MHz; 0 means default 5MHz */
		unsigned int mode      : 2;     /* low-bit: clk phase , high-bit: clk polarity convert, normally select:0 */
		unsigned int bits_per_word : 5; /* 0 means default: 8 */
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
	unsigned long long current_app_mask;
} pkt_power_log_report_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned int event_id;
} pkt_big_data_report_t;

typedef struct {
	struct pkt_header hd;
	unsigned char end;
	unsigned char file_count;
	unsigned short file_list[];
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

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
	char calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
} pkt_mag_calibrate_data_req_t;

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
	char calibrate_data[MAX_GYRO_TEMP_OFFSET_LENGTH];
} pkt_gyro_temp_offset_req_t;

typedef struct {
	pkt_common_data_t fhd;
	signed int data;
} fingerprint_upload_pkt_t;

typedef struct {
	unsigned int sub_cmd;
	unsigned char buf[7]; //byte alignment
	unsigned char len;
} fingerprint_req_t;

typedef struct {
	struct pkt_header hd;
	unsigned long long app_id;
	unsigned short msg_type;
	unsigned char res[2];
	unsigned char data[];
} chre_req_t;

typedef struct{
	unsigned long long sample_start_time;
	unsigned int sample_interval;
	unsigned int integ_time;
} als_run_stop_para_t;
typedef enum additional_info_type {
	AINFO_BEGIN = 0x0,
	AINFO_END   = 0x1,
	// Basic information
	AINFO_UNTRACKED_DELAY =  0x10000,
	AINFO_INTERNAL_TEMPERATURE,
	AINFO_VEC3_CALIBRATION,
	AINFO_SENSOR_PLACEMENT,
	AINFO_SAMPLING,
	AINFO_CHANNEL_NOISE = 0x20000,
	AINFO_CHANNEL_SAMPLER,
	AINFO_CHANNEL_FILTER,
	AINFO_CHANNEL_LINEAR_TRANSFORM,
	AINFO_CHANNEL_NONLINEAR_MAP,
	AINFO_CHANNEL_RESAMPLER,
	// Custom information
	AINFO_CUSTOM_START =    0x10000000,
	// Debugging
	AINFO_DEBUGGING_START = 0x40000000,
} additional_info_type_t;

enum {
	FILE_BEGIN,
	FILE_BASIC_SENSOR_APP = FILE_BEGIN,     /* 0 */
	FILE_FUSION,                            /* 1 */
	FILE_FUSION_GAME,                       /* 2 */
	FILE_FUSION_GEOMAGNETIC,                /* 3 */
	FILE_MOTION,                            /* 4 */
	FILE_PEDOMETER,                         /* 5 */
	FILE_PDR,                               /* 6 */
	FILE_AR,                                /* 7 */
	FILE_GSENSOR_GATHER_FOR_GPS,            /* 8 */
	FILE_PHONECALL,                         /* 9 */
	FILE_FINGERSENSE,                       /* 10 */
	FILE_SIX_FUSION,                        /* 11 */
	FILE_HANDPRESS,                         /* 12 */
	FILE_CA,                                /* 13 */
	FILE_OIS,                               /* 14 */
	FILE_FINGERPRINT,                       /*fingerprint_app*/
	FILE_KEY,			 	// 16
	FILE_GSENSOR_GATHER_SINGLE_FOR_GPS,     //17 Single line protocol for austin
	FILE_AOD,                               //18
	FILE_MODEM,                             //19
	FILE_CHARGING,                          /* 20 */
	FILE_MAGN_BRACKET,  			//21
	FILE_FLP,                               // 22
	FILE_TILT_DETECTOR,                     //23
	FILE_RPC,
	FILE_FINGERPRINT_UD = 28,
	FILE_DROP,                              //29
	FILE_APP_ID_MAX = 31,                   /* MAX VALID FILE ID FOR APPs */

	FILE_AKM09911_DOE_MAG,                  /* 32 */
	FILE_BMI160_ACC,                        /* 33 */
	FILE_LSM6DS3_ACC,                       /* 34 */
	FILE_BMI160_GYRO,                       /* 35 */
	FILE_LSM6DS3_GYRO,                      /* 36 */
	FILE_AKM09911_MAG,                      /* 37 */
	FILE_BH1745_ALS,                        /* 38 */
	FILE_PA224_PS,                          /* 39 */
	FILE_ROHM_BM1383,                       /* 40 */
	FILE_APDS9251_ALS,                      /* 41 */
	FILE_LIS3DH_ACC,                        /* 42 */
	FILE_KIONIX_ACC,                        /* 43 */
	FILE_APDS993X_ALS,                      /* 44 */
	FILE_APDS993X_PS,                       /* 45 */
	FILE_TMD2620_PS,                        /* 46 */
	FILE_GPS_4774,                          /* 47 */
	FILE_ST_LPS22BH,                        /* 48 */
	FILE_APDS9110_PS,                       /* 49 */
	FILE_CYPRESS_HANDPRESS,                 //50
	FILE_LSM6DSM_ACC,                       //51
	FILE_LSM6DSM_GYRO,                      //52
	FILE_ICM20690_ACC,			//53
	FILE_ICM20690_GYRO,			//54
	FILE_LTR578_ALS,                        //55
	FILE_LTR578_PS,                         //56
	FILE_FPC1021_FP,                        //57
	FILE_CAP_PROX,                          //58
	FILE_CYPRESS_KEY,		        //59
	FILE_CYPRESS_SAR,			//60
	FILE_GPS_4774_SINGLE,		        //61
	FILE_SX9323_CAP_PROX,	                //62
	FILE_BQ25892_CHARGER,                   /* 63 */
	FILE_FSA9685_SWITCH,                    /* 64 */
	FILE_SCHARGER_V300,                     /* 65 */
	FILE_YAS537_MAG,                        /* 66 */
	FILE_AKM09918_MAG,                      /*67*/
	FILE_TMD2745_ALS,                       /* 68 */
	FILE_TMD2745_PS,                        /* 69 */
	FILE_YAS537_DOE_MAG = 73,               /* 73 */
	FILE_FPC1268_FP,                        /* 74 */
	FILE_GOODIX8206_FP,                     /* 75 */
	FILE_FPC1075_FP,                        /* 76 */
	FILE_FPC1262_FP,                        /* 77 */
	FILE_GOODIX5296_FP,                     /* 78 */
	FILE_GOODIX3288_FP,                     /* 79 */
	FILE_SILEAD6185_FP,                     /* 80 */
	FILE_SILEAD6275_FP,                     /* 81 */
	FILE_BOSCH_BMP380,			/* 82 */
	FILE_TMD3725_ALS,                       // 83
	FILE_TMD3725_PS,                        // 84
	FILE_DRV2605_DRV,                       // 85
	FILE_LTR582_ALS,                        // 86
	FILE_LTR582_PS,                         // 87
	FILE_GOODIX_BAIKAL_FP,                  // 88
	FILE_GOODIX5288_FP,
	FILE_QFP1500_FP,                        // 90
	FILE_FPC1291_FP,                        // 91
	FILE_FPC1028_FP,                        // 92
	FILE_GOODIX3258_FP,                     // 93
	FILE_SILEAD6152_FP,                     // 94
	FILE_APDS9999_ALS,                      // 95
	FILE_APDS9999_PS,                       // 96
	FILE_TMD3702_ALS,                       // 97
	FILE_TMD3702_PS,                        // 98
	FILE_AMS8701_TOF,                       // 99
	FILE_VCNL36658_ALS,                     // 100
	FILE_VCNL36658_PS,                      // 101
	FILE_SILEAD6165_FP,                     // 102
	FILE_SYNA155A_FP,                       // 103
	FILE_TSL2591_ALS,                       // 104
	FILE_BH1726_ALS,                        // 105
	FILE_GOODIX3658_FP,                     // 106
	FILE_FPC1511_FP,                        // 107
	FILE_PA224_PS_VER2,                     // 108
	FILE_ID_MAX,                            /* MAX VALID FILE ID */
};

#endif /* end of include guard: __PROOCOL_EXT_H__ */