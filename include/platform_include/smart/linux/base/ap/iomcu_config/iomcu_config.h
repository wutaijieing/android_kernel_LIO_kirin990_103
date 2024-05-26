/*
* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
* Description: IOMCU CONFIG DESC
* Create: 2021-12-13
*/

#ifndef __IOMCU_CONFIG_H__
#define __IOMCU_CONFIG_H__

enum dump_loc_t {
	DL_NONE = 0,
	DL_TCM,
	DL_EXT,
	DL_BOTTOM = DL_EXT,
};

enum dump_region {
	DE_TCM_CODE,
	DE_DDR_CODE,
	DE_DDR_DATA,
	DE_BOTTOM = 16,
};

#define VERSION_LENGTH 16

struct dump_region_config {
	unsigned char loc;
	unsigned char reserved[3];
};

struct dump_config {
	unsigned long long dump_addr;
	unsigned int dump_size;
	unsigned int reserved1;
	unsigned long long ext_dump_addr;
	unsigned int ext_dump_size;
	unsigned char enable;
	unsigned char finish;
	unsigned char reason;
	unsigned char reserved2;
	struct dump_region_config elements[DE_BOTTOM];
};

struct log_buff_backup {
	void *mutex;
	unsigned short index;
	unsigned short pingpang;
	unsigned char *buff;
	unsigned int ddr_log_buff_cnt;
	unsigned int ddr_log_buff_index;
	unsigned int ddr_log_buff_last_update_index;
};

struct wrong_wakeup_msg {
	unsigned int flag;
	unsigned int reserved1;
	unsigned long long time;
	unsigned int irq0;
	unsigned int irq1;
	unsigned int recvfromapmsg[4];
	unsigned int recvfromlpmsg[4];
	unsigned int sendtoapmsg[4];
	unsigned int sendtolpmsg[4];
	unsigned int recvfromapmsgmode;
	unsigned int recvfromlpmsgmode;
	unsigned int sendtoapmsgmode;
	unsigned int sendtolpmsgmode;
};

struct timestamp_kernel {
	unsigned long long syscnt;
	unsigned long long kernel_ns;
};

struct timestamp_base {
	unsigned long long syscnt;
	unsigned long long kernel_ns;
	unsigned int timer32k_cyc;
	unsigned int reserved;
};

struct coul_core_info {
	signed int c_offset_a;
	signed int c_offset_b;
	signed int r_coul_mohm;
	signed int reserved;
};

struct read_data_als_ud {
	float rdata;
	float gdata;
	float bdata;
	float irdata;
};

struct peri_bright_data {
	unsigned int mipi_data;
	unsigned int bright_data;
	unsigned long long time_stamp;
};

struct als_ud_config {
	unsigned char screen_status;
	unsigned char reserved[7];
	unsigned long long als_rgb_pa;
	struct peri_bright_data bright_data;
	struct read_data_als_ud read_data_history;
};

struct bbox_config {
	unsigned long long log_addr;
	unsigned int log_size;
	unsigned int rsvd;
};

struct ddr_config_t {
	struct dump_config dump_config;
	struct log_buff_backup logbuff_cb_backup;
	struct wrong_wakeup_msg wrong_wakeup_msg;
	unsigned int loglevel;
	float gyro_offset[3];
	unsigned char modem_open_app_state;
	unsigned char hifi_open_sound_msg;
	unsigned char igs_hardware_bypass;
#ifdef CONFIG_APP_PEDOMETER_CW
	unsigned char reserved1[4];
	unsigned char pedo_steps_valid;
	unsigned int pedo_total_steps;
	unsigned int reserved;
#else
	unsigned char igs_screen_status;
	unsigned char reserved1[4];
	unsigned long long reserved;
#endif
	struct timestamp_kernel timestamp_base_send;
	struct timestamp_base timestamp_base_iomcu;
	struct coul_core_info coul_info;
	struct bbox_config bbox_config;
	struct als_ud_config als_ud_config;
	unsigned int te_irq_tcs3701;
	unsigned int old_dc_flag;
	unsigned int lcd_freq;
	unsigned char phone_type_info[2];
	unsigned int sc_flag;
	unsigned int is_pll_on;
	unsigned int fall_down_support;
	char sensorhub_version[VERSION_LENGTH];
};

struct ddr_config_t *get_config_on_ddr(void);

#define PA_MARGIN_NUM     2 /* PA_HCI:0 PA_NBTI:1 */
#define NPU_AVS_DATA_NUN  14
#define NPU_SVFD_DATA_NUN 28
#define NPU_SVFD_PARA_NUN 2 /* actually used 1, increase to 2 for 8-bytes align */
#define PROFVOLT_NUM      7
#define PLAT_CFG_MAGIC_ON_DDR  0xABCD1243

struct npu_cfg_data {
	unsigned int pa_margin[PA_MARGIN_NUM];
	unsigned int avs_data[NPU_AVS_DATA_NUN];
	unsigned int svfd_data[NPU_SVFD_DATA_NUN];
	unsigned int svfd_para[NPU_SVFD_PARA_NUN];
	unsigned int prof_margin[PROFVOLT_NUM];
	unsigned int level2_chip;
};

struct plat_cfg_on_ddr {
	unsigned long long magic;
	struct timestamp_kernel timestamp_base_send;
	struct timestamp_base timestamp_base_iomcu;
	struct bbox_config bbox_config;
	struct dump_config dump_config;
	struct npu_cfg_data npu_data;
};

struct plat_cfg_on_ddr *get_plat_cfg_on_ddr(void);

#ifdef IPC_FEATURE_V3
#define get_plat_config_on_ddr()  get_plat_cfg_on_ddr()
#else
#define get_plat_config_on_ddr()  get_config_on_ddr()
#endif
#endif

