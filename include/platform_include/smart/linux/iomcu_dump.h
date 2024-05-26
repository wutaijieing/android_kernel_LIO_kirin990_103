/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: Contexthub share memory driver
 * Create: 2016-04-01
 */
#ifndef __IOMCU_DUMP_H__
#define __IOMCU_DUMP_H__
#include <linux/types.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <mntn_public_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_VOLTAGE_3_2V  3200000
#define SENSOR_VOLTAGE_3_1V  3100000
#define SENSOR_VOLTAGE_3V    3000000
#define SENSOR_VOLTAGE_1V8   1800000
#define IOM3_RECOVERY_UNINIT 0
#define IOM3_RECOVERY_IDLE (IOM3_RECOVERY_UNINIT + 1)
#define IOM3_RECOVERY_START (IOM3_RECOVERY_IDLE + 1)
#define IOM3_RECOVERY_MINISYS (IOM3_RECOVERY_START + 1)
#define IOM3_RECOVERY_DOING (IOM3_RECOVERY_MINISYS + 1)
#define IOM3_RECOVERY_3RD_DOING (IOM3_RECOVERY_DOING + 1)
#define IOM3_RECOVERY_FAILED (IOM3_RECOVERY_3RD_DOING + 1)

#define SENSORHUB_MODID DFX_BB_MOD_IOM_START
#define SENSORHUB_USER_MODID (DFX_BB_MOD_IOM_START + 1)
#define SENSORHUB_FDUL_MODID (DFX_BB_MOD_IOM_START + 2)
#define SENSORHUB_TINY_MODID (DFX_BB_MOD_IOM_START + 3)
#ifdef CONFIG_CONTEXTHUB_DFX_UNI_NOC
#define SENSORHUB_IOMCU_DMA_MODID (DFX_BB_MOD_IOM_START + 4)
#endif
#define PATH_MAXLEN 128
#define HISTORY_LOG_SIZE 256
#define HISTORY_LOG_MAX 0x80000 /* 512k */
#define ROOT_UID 0
#define SYSTEM_GID 1000
#define DIR_LIMIT 0770
#define FILE_LIMIT 0660
#ifdef CONFIG_CONTEXTHUB_LOG_PATH_VAR
#define SH_DMP_DIR "/var/log/hisi/sensorhub-log/"
#define SH_DMP_FS "/var/log"
#else
#ifdef CONFIG_INPUTHUB_AS
#define SH_DMP_DIR  "/data/log/sensorhub-log/"
#else
#define SH_DMP_DIR  "/data/log/sensorhub-log/"
#endif
#define SH_DMP_FS  "/data/lost+found"
#endif
#define SH_DMP_HISTORY_FILE "history.log"

#define DATATIME_MAXLEN 24 /* 14+8 +2, 2: '-'+'\0' */

enum exp_source {
	SH_FAULT_HARDFAULT = 0,
	SH_FAULT_BUSFAULT,
	SH_FAULT_USAGEFAULT,
	SH_FAULT_MEMFAULT,
	SH_FAULT_NMIFAULT,
	SH_FAULT_ASSERT,
	SH_FAULT_INTERNELFAULT = 16,
	SH_FAULT_IPC_RX_TIMEOUT,
	SH_FAULT_IPC_TX_TIMEOUT,
	SH_FAULT_RESET,
	SH_FAULT_USER_DUMP,
	SH_FAULT_RESUME,
	SH_FAULT_REDETECT,
	SH_FAULT_PANIC,
	SH_FAULT_NOC,
	SH_FAULT_REACT,
	SH_FAULT_EXP_BOTTOM,
};

extern char *rdr_get_timestamp(void);
#ifdef CONFIG_DFX_BB
extern u64 rdr_get_tick(void);
#endif
int iomcu_dump_init(void);
void iomcu_dump_exit(void);
extern int g_sensorhub_wdt_irq;
extern struct regulator *sensorhub_vddio;
extern void rdr_system_error(u32 modid, u32 arg1, u32 arg2);
extern void emg_flush_logbuff(void);
extern void reset_logbuff(void);
extern int iom3_need_recovery(int modid, enum exp_source f);
extern int recovery_init(void);
extern int register_iom3_recovery_notifier(struct notifier_block *nb);
extern int unregister_iom3_recovery_notifier(struct notifier_block *nb);
extern int iom3_rec_sys_callback(const struct pkt_header *head);
int get_iom3_power_state(void);
int get_iom3_state(void);
void notify_recovery_done(void);
int iomcu_check_dump_status(void);
void iomcu_minready_done(void);

#ifdef __cplusplus
}
#endif
#endif /* __IOMCU_DUMP_H__ */
