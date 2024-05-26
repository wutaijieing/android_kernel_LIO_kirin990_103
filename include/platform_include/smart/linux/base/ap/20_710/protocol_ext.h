/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: protocol header file
 * Create: 2021/12/05
 */
#ifndef __PROTOCOL_EXT_H__
#define __PROTOCOL_EXT_H__

#define SUBCMD_LEN                         4
#define MAX_PKT_LENGTH                     128
#define MAX_PKT_LENGTH_AP                  2560
#define MAX_LOG_LEN                        100
#define MAX_PATTERN_SIZE                   16
#define MAX_ACCEL_PARAMET_LENGTH           100
#define MAX_MAG_PARAMET_LENGTH             100
#define MAX_GYRO_PARAMET_LENGTH            100
#define MAX_ALS_PARAMET_LENGTH             100
#define MAX_PS_PARAMET_LENGTH              100
#define MAX_I2C_DATA_LENGTH                50
#define MAX_SENSOR_CALIBRATE_DATA_LENGTH   60
#define MAX_MAG_CALIBRATE_DATA_LENGTH        12
#define MAX_GYRO_CALIBRATE_DATA_LENGTH       72
#define MAX_PS_CALIBRATE_DATA_LENGTH         12
#define MAX_GYRO_TEMP_OFFSET_LENGTH          56
#define MAX_CAP_PROX_CALIBRATE_DATA_LENGTH   16
#define MAX_MAG_FOLDER_CALIBRATE_DATA_LENGTH 24
#define MAX_MAG_AKM_CALIBRATE_DATA_LENGTH    28
#define DATA_FLAG_FLUSH_OFFSET             0
#define DATA_FLAG_VALID_TIMESTAMP_OFFSET   1
#define FLUSH_END                  (1 << DATA_FLAG_FLUSH_OFFSET)
#define DATA_FLAG_VALID_TIMESTAMP  (1 << DATA_FLAG_VALID_TIMESTAMP_OFFSET)
#define ACC_CALIBRATE_DATA_LENGTH          15
#define GYRO_CALIBRATE_DATA_LENGTH         18
#define PS_CALIBRATE_DATA_LENGTH           3
#define ALS_CALIBRATE_DATA_LENGTH          6
#define ACC1_CALIBRATE_DATA_LENGTH         60
#define ACC1_OFFSET_DATA_LENGTH            15
#define GYRO1_CALIBRATE_DATA_LENGTH        18

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
	MOTION_TYPE_EXT_LOG,
	MOTION_TYPE_HEAD_DOWN,
	MOTION_TYPE_PUT_DOWN,
	MOTION_TYPE_REMOVE,
	MOTION_TYPE_FALL,
	MOTION_TYPE_SIDEGRIP,
	MOTION_TYPE_MOVE,
	/* NOTE:add string in motion_type_str when add type */
	MOTION_TYPE_END,
} motion_type_t;

typedef enum {
	FINGERPRINT_TYPE_START = 0x0,
	FINGERPRINT_TYPE_HUB,
	FINGERPRINT_TYPE_END,
} fingerprint_type_t;

typedef enum {
	CA_TYPE_START,
	CA_TYPE_PICKUP,
	CA_TYPE_PUTDOWN,
	CA_TYPE_ACTIVITY,
	CA_TYPE_HOLDING,
	CA_TYPE_MOTION,
	CA_TYPE_PLACEMENT,
	/* NOTE:add string in ca_type_str when add type */
	CA_TYPE_END,
} ca_type_t;

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
	ST_SCREENON,
	ST_SCREENOFF,
	ST_SLEEP,
	ST_WAKEUP,
	ST_POWEROFF,
	ST_RECOVERY_BEGIN,
	ST_RECOVERY_FINISH,
	ST_END
} sys_status_t;

typedef enum  {
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
} pkt_common_data_t;

typedef struct {
	pkt_common_data_t data_hd;
	struct sensor_data_xyz xyz[]; /* x,y,z,acc,time */
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

typedef struct {
	struct pkt_header hd;
	signed int type;
	signed short serial;
	signed short end;  /* 0: more additional info to be send  1:this pkt is last one */
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
	drop_info_t   data;
} pkt_drop_data_req_t;

typedef struct interval_param {
	unsigned int  period;
	unsigned int batch_count;
	unsigned char mode;
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
		unsigned int gpio_cs   : 16; /* bit0~8 is gpio NO., bit9~11 is gpio iomg set */
		unsigned int baudrate  : 5;  /* unit: MHz; 0 means default 5MHz */
		unsigned int mode      : 2;  /* low-bit: clk phase , high-bit: clk polarity convert, normally select:0 */
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
	char calibrate_data[MAX_PS_CALIBRATE_DATA_LENGTH];
} pkt_ps_calibrate_data_req_t;

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
	unsigned char buf[7]; /* byte alignment */
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
	/* Basic information */
	AINFO_UNTRACKED_DELAY =  0x10000,
	AINFO_INTERNAL_TEMPERATURE,
	AINFO_VEC3_CALIBRATION,
	AINFO_SENSOR_PLACEMENT,
	AINFO_SAMPLING,
	/* Sampling channel modeling information */
	AINFO_CHANNEL_NOISE = 0x20000,
	AINFO_CHANNEL_SAMPLER,
	AINFO_CHANNEL_FILTER,
	AINFO_CHANNEL_LINEAR_TRANSFORM,
	AINFO_CHANNEL_NONLINEAR_MAP,
	AINFO_CHANNEL_RESAMPLER,
	/* Custom information */
	AINFO_CUSTOM_START =    0x10000000,
	/* Debugging */
	AINFO_DEBUGGING_START = 0x40000000,
} additional_info_type_t;

enum {
	FILE_BEGIN,
	FILE_BASIC_SENSOR_APP = FILE_BEGIN,
	FILE_FUSION,
	FILE_FUSION_GAME,
	FILE_FUSION_GEOMAGNETIC,
	FILE_MOTION,
	FILE_PEDOMETER,
	FILE_PDR,
	FILE_AR,
	FILE_GSENSOR_GATHER_FOR_GPS,
	FILE_PHONECALL,
	FILE_FINGERSENSE,
	FILE_SIX_FUSION,
	FILE_HANDPRESS,
	FILE_CA,
	FILE_OIS,
	FILE_FINGERPRINT,
	FILE_KEY,
	FILE_GSENSOR_GATHER_SINGLE_FOR_GPS,
	FILE_AOD,
	FILE_MODEM,
	FILE_CHARGING,
	FILE_MAGN_BRACKET,
	FILE_FLP,
	FILE_TILT_DETECTOR,
	FILE_RPC,
	FILE_FINGERPRINT_UD = 28,
	FILE_DROP,
	FILE_APP_ID_MAX = 31,
	FILE_AKM09911_DOE_MAG,
	FILE_BMI160_ACC,
	FILE_LSM6DS3_ACC,
	FILE_BMI160_GYRO,
	FILE_LSM6DS3_GYRO,
	FILE_AKM09911_MAG,
	FILE_BH1745_ALS,
	FILE_PA224_PS,
	FILE_ROHM_BM1383,
	FILE_APDS9251_ALS,
	FILE_LIS3DH_ACC,
	FILE_KIONIX_ACC,
	FILE_APDS993X_ALS,
	FILE_APDS993X_PS,
	FILE_TMD2620_PS,
	FILE_GPS_4774,
	FILE_ST_LPS22BH,
	FILE_APDS9110_PS,
	FILE_CYPRESS_HANDPRESS,
	FILE_LSM6DSM_ACC,
	FILE_LSM6DSM_GYRO,
	FILE_ICM20690_ACC,
	FILE_ICM20690_GYRO,
	FILE_LTR578_ALS,
	FILE_LTR578_PS,
	FILE_FPC1021_FP,
	FILE_CAP_PROX,
	FILE_CYPRESS_KEY,
	FILE_CYPRESS_SAR,
	FILE_GPS_4774_SINGLE,
	FILE_SX9323_CAP_PROX,
	FILE_BQ25892_CHARGER,
	FILE_FSA9685_SWITCH,
	FILE_SCHARGER_V300,
	FILE_YAS537_MAG,
	FILE_AKM09918_MAG,
	FILE_TMD2745_ALS,
	FILE_TMD2745_PS,
	FILE_YAS537_DOE_MAG = 73,
	FILE_FPC1268_FP,
	FILE_GOODIX8206_FP,
	FILE_FPC1075_FP,
	FILE_FPC1262_FP,
	FILE_GOODIX5296_FP,
	FILE_GOODIX3288_FP,
	FILE_SILEAD6185_FP,
	FILE_SILEAD6275_FP,
	FILE_BOSCH_BMP380,
	FILE_TMD3725_ALS,
	FILE_TMD3725_PS,
	FILE_DRV2605_DRV,
	FILE_LTR582_ALS,
	FILE_LTR582_PS,
	FILE_GOODIX_BAIKAL_FP,
	FILE_GOODIX5288_FP,
	FILE_QFP1500_FP,
	FILE_FPC1291_FP,
	FILE_FPC1028_FP,
	FILE_GOODIX3258_FP,
	FILE_SILEAD6152_FP,
	FILE_APDS9999_ALS,
	FILE_APDS9999_PS,
	FILE_TMD3702_ALS,
	FILE_TMD3702_PS,
	FILE_AMS8701_TOF,
	FILE_VCNL36658_ALS,
	FILE_VCNL36658_PS,
	FILE_SILEAD6165_FP,
	FILE_SYNA155A_FP,
	FILE_TSL2591_ALS = 105,
	FILE_BH1726_ALS = 106,
	FILE_GOODIX3658_FP = 107,
	FILE_LTR2568_PS = 108,
	FILE_STK3338_PS = 109,
	FILE_SILEAD6152B_FP = 110,
	FILE_VCNL36832_PS = 111,
	FILE_MMC5603_MAG,
	FILE_MMC5603_DOE_MAG,
	FILE_FPC1511_FP = 114,
	FILE_STK3338_ALS = 115,
	FILE_APDS9308_ALS = 116,
	FILE_LTR2568_ALS = 117,
	FILE_VCNL36832_ALS,
	FILE_TMD2702_ALS = 119,
	FILE_A96T3X6_CAP_PROX,
	FILE_LIS2DWL_ACC,
	FILE_BMA422_ACC,
	FILE_GOODIX3216_FP = 129,
	FILE_ET525_FP = 130,
	FILE_BMA2X2_ACC = 131,
	FILE_MC34XX_ACC = 132,
	FILE_SY3079_ALS = 133,
	FILE_STK3321_ALS = 134,
	FILE_STK3321_PS = 135,
	FILE_STK3235_ALS = 136,
	FILE_LTR578_039WA_PS = 137,
	FILE_MC3419_ACC = 138,
	FILE_DA718_ACC = 185,
	FILE_BU27006MUC_ALS = 186,
	FILE_VD6281_ALS = 187,
	FILE_AW9610XB_CAP_PROX = 188,
	FILE_TCS34083_ALS = 189,
	FILE_QMC6308_DOE_MAG = 190,
	FILE_CHT8305_HUMITURE = 191,
	FILE_VI5300_TOF = 192,
	FILE_VL53L3_TOF = 193,
	FILE_ID_MAX, /* MAX VALID FILE ID */
};

#endif /* end of include guard: __PROOCOL_EXT_H__ */