#ifndef __FREQDUMP_PLAT_H__
#define __FREQDUMP_PLAT_H__ 
#include <tsensor_interface.h>
enum MODULE_CLK {
 LITTLE_CLUSTER_CLK = 0,
 MID_CLUSTER_CLK,
 BIG_CLUSTER_CLK,
 GPU_CLK,
 DDR_CLKA,
 DDR_CLKB,
 DMSS_CLK,
 L3_CLK,
 MAX_MODULE_CLK,
};
enum MODULE_TEMP {
 LITTLE_CLUSTER_TEMP = 0,
 BIG_CLUSTER_TEMP,
 GPU_TEMP,
 PERI_TEMP,
 MIDDLE_CLUSTER_TEMP,
 MAX_MODULE_TEMP,
};
const char *sensor_name[] = {
 "local_sensor_m0",
 "big_sensor_0",
 "big_sensor_1",
 "middle_sensor_0",
 "middle_sensor_1",
 "venc0_sensor",
 "pdf0_sensor",
 "local_sensor_m1",
 "res_sensor_0",
 "res_sensor_1",
 "res_sensor_2",
 "res_sensor_3",
 "res_sensor_4",
 "res_sensor_5",
 "local_sensor_m2",
 "gpu_sensor_0",
 "gpu_sensor_1",
 "ivp_sensor",
 "gpu_sensor_3",
 "dss_sensor",
 "fcm0_sensor",
 "local_sensor_m3",
 "isp0_sensor",
 "system_cache_sensor",
 "npu0_sensor",
 "npu1_sensor",
 "fcm1_sensor",
};
enum MODULE_POWER {
 LITTLE_CLUSTER_POWER = 0,
 MID_CLUSTER_POWER,
 BIG_CLUSTER_POWER,
 GPU_POWER,
 DDR_POWER,
 PERI_POWER,
 MAX_MODULE_POWER,
};
#define MAX_MODULE_BUCK 4
enum MODULE_ADC_BUCK {
 BUCK_0_I = 0,
 BUCK_1_I,
 BUCK_2_I,
 BUCK_3_I,
 MAX_MODULE_ADC_BUCK,
};
enum MODULE_DDR_DIVCFG {
 DDR_DIVCFGA = 0,
 DDR_DIVCFGB,
 DMSS_DIVCFG,
 MAX_DDR_DIVCFG,
};
#define GPU_DIVCFG (DDR_DIVCFGA)
#define MAX_NPU_MODULE_SHOW 6
struct freqdump_data {
 unsigned int apll[6][2];
 unsigned int ppll[5][2];
 unsigned int clksel[MAX_MODULE_CLK];
 unsigned int ddrclkdiv[MAX_DDR_DIVCFG];
 unsigned int clkdiv2;
 unsigned int clkdiv9;
 unsigned int ddr_dumreg_pllsel;
 unsigned int dmss_asi_status;
 unsigned int dmc_curr_stat[4];
 unsigned int dmc_flux_rd[4];
 unsigned int dmc_flux_wr[4];
 unsigned int dmc0_ddrc_cfg_wm;
 unsigned int dmc0_ddrc_intsts;
 unsigned int dmc0_tmon_aref;
 unsigned int qosbuf_cmd[2];
 unsigned int temp[MAX_MODULE_TEMP];
 unsigned int buck[MAX_MODULE_BUCK];
 unsigned int voltage[MAX_MODULE_POWER];
 unsigned int mem_voltage[MAX_MODULE_POWER];
 unsigned int peri_usage[4];
 unsigned int end_reserved;
 unsigned int adc_buck_i[MAX_MODULE_ADC_BUCK];
 unsigned int adc_vol[2];
 unsigned int clkdiv0;
 unsigned int clkdiv23;
 unsigned int clkdiv25;
 unsigned int cluster_pwrstat;
 unsigned int peri_vote[3];
 unsigned int flags;
 unsigned int npu[MAX_NPU_MODULE_SHOW][2];
 char npu_name[MAX_NPU_MODULE_SHOW][10];
 unsigned int tsensor_temp[SENSOR_UNKNOWN_MAX];
} freqdump_data_value;
#endif
