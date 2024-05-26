/*
 * Copyright (C) Huawei Tech. Co. Ltd. 2017-2019. All rights reserved.
 * Description: dev drvier to communicate with sensorhub swing app
 * Create: 2017.12.05
 */

#ifndef __LINUX_CONTEXTHUB_SWING_DBG_H__
#define __LINUX_CONTEXTHUB_SWING_DBG_H__

/* ioctl cmd define */
#define SWING_DBG_IO                         0xB2

#define SWING_DBG_IOCTL_SET_BKPT           _IOW(SWING_DBG_IO, 0xC0, short)
#define SWING_DBG_IOCTL_RM_BKPT            _IOW(SWING_DBG_IO, 0xC1, short)
#define SWING_DBG_IOCTL_SINGLE_STEP        _IOW(SWING_DBG_IO, 0xC2, short)
#define SWING_DBG_IOCTL_RESUME             _IOW(SWING_DBG_IO, 0xC3, short)
#define SWING_DBG_IOCTL_SUSPEND            _IOW(SWING_DBG_IO, 0xC4, short)
#define SWING_DBG_IOCTL_READ               _IOW(SWING_DBG_IO, 0xC5, short)
#define SWING_DBG_IOCTL_WRITE              _IOW(SWING_DBG_IO, 0xC6, short)
#define SWING_DBG_IOCTL_SHOW_BKPTS         _IOW(SWING_DBG_IO, 0xC7, short)
#define SWING_DBG_IOCTL_DUMP               _IOW(SWING_DBG_IO, 0xC8, short)
#define SWING_DBG_IOCTL_DBG_MODEL          _IOW(SWING_DBG_IO, 0xC9, short)
#define SWING_DBG_IOCTL_LIST_TASKS         _IOW(SWING_DBG_IO, 0xCA, short)
#define SWING_DBG_IOCTL_DBG_CALLBACK       _IOW(SWING_DBG_IO, 0xCB, short)
#define SWING_DBG_IOCTL_SWING_DBG_OPEN     _IOW(SWING_DBG_IO, 0xCC, short)
#define SWING_DBG_IOCTL_SWING_DBG_CLOSE    _IOW(SWING_DBG_IO, 0xCD, short)
#define SWING_DBG_IOCTL_RUN_BLAS           _IOW(SWING_DBG_IO, 0xCE, short)
#define SWING_DBG_IOCTL_PROFILE_CONFIG     _IOW(SWING_DBG_IO, 0xCF, short)
#define SWING_DBG_IOCTL_LOAD_MODEL         _IOW(SWING_DBG_IO, 0xD3, short)
#define SWING_DBG_IOCTL_UNLOAD_MODEL       _IOW(SWING_DBG_IO, 0xD4, short)
#define SWING_DBG_IOCTL_RUN_MODEL          _IOW(SWING_DBG_IO, 0xD5, short)
#define SWING_DBG_IOCTL_GET_INFO           _IOW(SWING_DBG_IO, 0xD6, short)
#define SWING_DBG_IOCTL_LIST_MODELS        _IOW(SWING_DBG_IO, 0xD7, short)
#define SWING_DBG_IOCTL_GET_PHYS           _IOW(SWING_DBG_IO, 0xD8, short)
#define SWING_DBG_IOCTL_FAKE_SUSPEND       _IOW(SWING_DBG_IO, 0xD9, short)
#define SWING_DBG_IOCTL_FAKE_RESUME        _IOW(SWING_DBG_IO, 0xDA, short)
#define SWING_DBG_IOCTL_IGS_SENSOR_TEST    _IOW(SWING_DBG_IO, 0xDB, short)

#ifdef CONFIG_CONTEXTHUB_IGS_20
#define MAX_SWING_DBG_MODEL_BLKS         1
#else
#define MAX_SWING_DBG_MODEL_BLKS         8
#endif

#define MAX_SWING_DBG_INPUT_NUM          (3)
#define MAX_SWING_DBG_OUTPUT_NUM         (3)
#define MAX_SWING_DBG_ADDR_DESC_NUM      (16)
#define MAX_SWING_DBG_HW_BKPT            (16)
#define MAX_SWING_DBG_SW_BKPT            (128)
#define MAX_SWING_DBG_MODEL_CNT          (64)
#define MAX_SWING_DBG_RW_LEN             (32)
#define MAX_SWING_DBG_TASK_CNT           (128)

#define SWING_DBG_BKPT_MAX_NUM           (16)
#define PMU_CNT_COUNT                    (8)

enum swing_dbg_read_data_e {
	SWING_DBG_READ_DBG_ISR = 0,
	SWING_DBG_READ_PROF,
};

struct swing_dbg_comm_resp_t {
	u32 ret_code;
};

struct swing_dbg_bkpt_t {
	u32 bkpt;
};

struct swing_dbg_bkpt_param_t {
	struct swing_dbg_bkpt_t bkpt;
	struct swing_dbg_comm_resp_t bkpt_resp;
};

struct swing_dbg_read_t {
	u32 module;
	u32 addr;
	u32 len;
	u32 reserved;
	u8 data[MAX_SWING_DBG_RW_LEN];
};

struct swing_dbg_read_resp_t {
	u32 ret_code;
	u32 module;
	u32 addr;
	u32 len;
	u64 data[4];
};

struct swing_dbg_read_param_t {
	struct swing_dbg_read_t read;
	struct swing_dbg_read_resp_t read_resp;
};

/* write param & resp. */
struct swing_dbg_write_t {
	u32 module;
	u32 addr;
	u32 len;
	u8 data[MAX_SWING_DBG_RW_LEN];
};

struct swing_dbg_write_param_t {
	struct swing_dbg_write_t write;
	struct swing_dbg_comm_resp_t write_resp;
};

struct swing_dbg_dump_t {
	u32 module;
	u32 addr;
	u32 len;
	u32 paddr;
};

struct swing_dbg_dump_param_t {
	struct swing_dbg_dump_t dump;
	struct swing_dbg_comm_resp_t dump_resp;
};

struct swing_dbg_show_bkpts_resp_t {
	u32 ret_code;
	u32 bkpts_num;
	u64 bkpts[MAX_SWING_DBG_HW_BKPT + MAX_SWING_DBG_SW_BKPT];
};

struct swing_dbg_show_bkpts_param_t {
	struct swing_dbg_show_bkpts_resp_t show_bkpts_resp;
};

struct swing_dbg_model_info_short_t {
	u32 uid;
	u32 is_standby : 1; /* standby model or normal model */
	u32 is_sc_model : 1; /* just for standby model, is system cache support. */
	/*
	 * just for normal model
	 * True: dynamic model, won't be parsed to ddr.
	 * False: static model, will be parsed to ddr.
	 */
	u32 is_dync : 1;
	u32 reserved :29;
};

struct swing_dbg_get_list_resp_t {
	u32 ret_code;
	u32 model_cnt;
	struct swing_dbg_model_info_short_t models[MAX_SWING_DBG_MODEL_CNT];
};

struct swing_dbg_get_list_param_t {
	struct swing_dbg_get_list_resp_t get_list_resp;
};

struct swing_dbg_dbg_model_t {
	u32 uid;
	u16 task_id;
	u16 is_enable;
};

struct swing_dbg_dbg_model_param_t {
	struct swing_dbg_dbg_model_t dbg_model;
	struct swing_dbg_comm_resp_t dbg_model_resp;
};

struct swing_dbg_list_tasks_t {
	u32 uid;
	u32 start;
};

struct swing_dbg_task_short_t {
	u32 task_entry;
	u32 flowtable_addr;
};

struct swing_dbg_tasks_list_resp_t {
	u32 ret_code;
	u32 reserved;
	u32 uid;
	u32 start;
	u32 task_cnt;
	u32 total_task_cnt;
	struct swing_dbg_task_short_t tasks[MAX_SWING_DBG_TASK_CNT];
};

struct swing_dbg_list_tasks_param_t {
	struct swing_dbg_list_tasks_t list_tasks;
	struct swing_dbg_tasks_list_resp_t list_tasks_resp;
};

struct swing_dbg_get_info_t {
	u32 uid;
};

struct swing_dbg_addr_desc_t {
	u64 vaddr;
	u64 paddr;
	u32 type;
	u32 size;
};

struct swing_dbg_get_info_resp_t {
	u32 ret_code;
	u32 reserved;
	u32 uid;
	u32 ops;
	u32 run_counter;
	u32 run_time;
	u32 input_num;
	u32 output_num;
	u32 in_size[MAX_SWING_DBG_INPUT_NUM]; /* input size */
	u32 out_size[MAX_SWING_DBG_OUTPUT_NUM]; /* output size */
	struct swing_dbg_addr_desc_t addr_desc[MAX_SWING_DBG_ADDR_DESC_NUM];
};

struct swing_dbg_get_info_param_t {
	struct swing_dbg_get_info_t get_info;
	struct swing_dbg_get_info_resp_t get_info_resp;
};

struct swing_dbg_run_blas_t {
	u32 test_mode; /* 0: blas-test  1: low-power test */
	u32 clk_mode; /* 0: 184M[FLL] 1:324M[PLL] */
	u32 out_loops; /* out-loops */
	u32 inner_loops; /* for low-power test only */
	u32 context;
	u32 flowtable_addr;
	u32 flowtable_size;
	u32 instr_addr;
	u32 instr_size;
	u32 input_addr;
	u32 input_size;
	u32 output_addr;
	u32 output_size;
	u32 input_addr_lb; /* ap view */
	u32 output_addr_lb; /* ap view */
};

struct swing_dbg_run_blas_param_t {
	struct swing_dbg_run_blas_t run_blas;
	struct swing_dbg_comm_resp_t run_blas_resp;
};

struct fdul_davinci_pmu_data {
	u64 pmu_task_cyc_cnt;
	u64 pmu_min_ov_cnt;
	u32 pmu_overflow;
	u32 reserved;
	u32 pmu_cnt[PMU_CNT_COUNT];
};

struct fdul_davinci_pmu_cfg {
	u64 pmu_start_cnt_cyc;
	u64 pmu_stop_cnt_cyc;
	u32 user_profile_mode;
	u32 sample_profile_mode;
	u8 pmu_cnt_idx[PMU_CNT_COUNT];
};

/* sensorhub->AP: report pmudata */
struct swing_dbg_pmu_data {
	u64 bkpt;
	struct fdul_davinci_pmu_data pmu_data;
};

/* AP->sensorhub: config profile para */
struct swing_dbg_profile_cfg {
	struct fdul_davinci_pmu_cfg pmu_cfg;
	u64 bkpt_num; /* 0: task report mode, 1 ~ SWING_DBG_BKPT_MAX_NUM: break point report mode */
	u64 bk_pt[SWING_DBG_BKPT_MAX_NUM];
};

#pragma pack(4)

struct swing_dbg_shmem_ap {
	u32 cmd;
	union {
		struct swing_dbg_profile_cfg pmu_cfg;
	};
};

struct swing_dbg_shmem_sh {
	struct pkt_header hd;
	union {
		struct swing_dbg_pmu_data pmu_data;
	};
};

#pragma pack()

struct swing_dbg_dbg_isr_t {
	u64 pc;
	u32 uid;
	u32 reserved;
};

#ifdef CONFIG_CONTEXTHUB_IGS_20
struct swing_dbg_data_t {
	u32 type         : 8; /* ION */
	u32 data_size    : 24;
	u32 data_addr;
};

struct swing_dbg_load_model_t {
	u32 uid            : 24;
	u32 blk_cnt        : 8;
	u32 load_context;
	struct swing_dbg_data_t model_blks[MAX_SWING_DBG_MODEL_BLKS];
};
#else
struct swing_dbg_data_t {
	u32 type; /* ION */
	u32 data_addr;
	u32 data_size;
};

struct swing_dbg_load_model_t {
	u32 uid;
	u32 blk_cnt;
	u32 context;
	u32 reserved;
	struct swing_dbg_data_t model_blks[MAX_SWING_DBG_MODEL_BLKS];
};
#endif

struct swing_dbg_load_resp_t {
	u32 ret_code;
	u32 uid;
	u32 load_context;
};

struct swing_dbg_load_model_param_t {
	struct swing_dbg_load_model_t load;
	struct swing_dbg_load_resp_t load_resp;
};

struct swing_dbg_unload_model_t {
	u32 uid;
	u32 context;
};

struct swing_dbg_unload_resp_t {
	u32 ret_code;
	u32 uid;
	u32 unload_context;
};

struct swing_dbg_unload_model_param_t {
	struct swing_dbg_unload_model_t unload;
	struct swing_dbg_load_resp_t unload_resp;
};

struct swing_dbg_run_model_t {
	u32 uid;
	u32 input_num;
	u32 output_num;
	u32 run_context;
	u32 is_high_prio;
	u32 reserved;
	struct swing_dbg_data_t input[MAX_SWING_DBG_INPUT_NUM];
	struct swing_dbg_data_t output[MAX_SWING_DBG_OUTPUT_NUM];
};

struct swing_dbg_run_resp_t {
	int ret_code;
	u32 uid;
	u32 run_context;
};

struct swing_dbg_run_model_param_t {
	struct swing_dbg_run_model_t run;
	struct swing_dbg_run_resp_t run_resp;
};

struct swing_addr_info_t {
	int fd;
	unsigned long long *pa;
};

struct swing_dbg_step_param_t {
	struct swing_dbg_comm_resp_t step_resp;
};

struct swing_dbg_resume_param_t {
	struct swing_dbg_comm_resp_t resume_resp;
};

struct swing_dbg_stall_param_t {
	struct swing_dbg_comm_resp_t stall_resp;
};

union igs_sensor_test_param_union {
	u32 data[16];
};

union igs_sensor_test_report_union {
	u32 data[4];
};

typedef struct igs_dbg_sensor_test_param {
	unsigned int target;
	unsigned int cmd;
	union igs_sensor_test_param_union param;
	union igs_sensor_test_report_union report;
}igs_dbg_sensor_test_t;

#endif
