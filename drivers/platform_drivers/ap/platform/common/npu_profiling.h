#ifndef __NPU_PROFILING_H__
#define __NPU_PROFILING_H__ 
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define AICPU_MAX_PMU_NUM 8
#define AI_CORE_MAX_PMU_NUM 8
#define PROF_SETTING_INFO_SIZE 256
#define PROF_HEAD_MANAGER_SIZE 0x1000
#define NPU_MAILBOX_RESERVED_SIZE 256
enum profiling_mode {
 PROF_NORMAL_MODE,
 PROF_USER_MODE,
 PROF_MODE_CNT,
};
enum profiling_type {
 PROF_TYPE_TSCPU = 0,
 PROF_TYPE_HWTS_LOG,
 PROF_TYPE_HWTS_PROFILING,
 PROF_TYPE_CNT,
};
enum aicore_profiling_enable_switch {
 AICORE_PROF_ENABLE_TASK_DISABLE = 0,
 AICORE_PROF_ENABLE_TASK_PMU = 1 << 0,
 AICORE_PROF_ENABLE_TASK_LOG = 1 << 1,
 AICORE_PROF_ENABLE_BLOCK_PMU = 1 << 2,
 AICORE_PROF_ENABLE_BLOCK_LOG = 1 << 3,
};
enum profiling_channel {
 PROF_CHANNEL_TSCPU = 0,
 PROF_CHANNEL_AICPU = 1,
 PROF_CHANNEL_HWTS_LOG = 1,
 PROF_CHANNEL_AICORE = 2,
 PROF_CHANNEL_HWTS_PROFILING = 2,
 PROF_CHANNEL_MAX,
};
struct prof_ts_cpu_config {
 unsigned int en;
 unsigned int task_state_switch;
 unsigned int task_type_switch;
};
struct prof_ai_cpu_config {
 unsigned int en;
 unsigned int event_num;
 unsigned int event[AICPU_MAX_PMU_NUM];
};
struct prof_ai_core_config {
 unsigned int en;
 unsigned int event_num;
 unsigned int event[AI_CORE_MAX_PMU_NUM];
 unsigned int mode;
};
struct prof_setting_info {
 struct prof_ts_cpu_config tscpu;
 struct prof_ai_cpu_config aicpu;
 struct prof_ai_core_config aicore;
};
struct profiling_ring_buff_manager {
 volatile unsigned int read;
 volatile unsigned int length;
 volatile unsigned int base_addr_l;
 volatile unsigned int base_addr_h;
 unsigned int reserved0[12];
 volatile unsigned int write;
 volatile unsigned int iova_addr;
 unsigned int reserved1[14];
};
struct profiling_buff_manager {
 union {
  struct prof_setting_info info;
  char data[PROF_SETTING_INFO_SIZE];
 } cfg;
 struct profiling_ring_buff_manager ring_buff[PROF_TYPE_CNT];
 unsigned int start_flag;
};
struct npu_prof_info {
 union {
  struct profiling_buff_manager manager;
  char data[PROF_HEAD_MANAGER_SIZE];
 } head;
};
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
