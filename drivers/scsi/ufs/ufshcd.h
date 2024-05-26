/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Universal Flash Storage Host controller driver
 * Copyright (C) 2011-2013 Samsung India Software Operations
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
 *
 * Authors:
 *	Santosh Yaraganavi <santosh.sy@samsung.com>
 *	Vinayak Holikatti <h.vinayak@samsung.com>
 */

#ifndef _UFSHCD_H
#define _UFSHCD_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rwsem.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/regulator/consumer.h>
#include <linux/bitfield.h>
#include <linux/devfreq.h>
#include <linux/keyslot-manager.h>
#include "unipro.h"
#ifdef CONFIG_HUFS_MANUAL_BKOPS
#include <linux/mas_bkops_core.h>
#endif

#include <asm/irq.h>
#include <asm/byteorder.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_eh.h>
#include <linux/android_kabi.h>

#ifdef CONFIG_HUAWEI_DSM_IOMT_UFS_HOST
#include <linux/iomt_host/dsm_iomt_ufs_host.h>
#endif

#include "ufs.h"
#include "ufs_quirks.h"
#include "ufshci.h"
#include "ufs-fault-inject.h"
#ifdef CONFIG_HUFS_HC
#include "hufs_hcd.h"
#endif

#define UFSHCD "ufshcd"
#define UFSHCD_DRIVER_VERSION "0.2"

#define UFSHCD_REQ_SENSE_SIZE 96

/* Auto Hibern8 Timer, default: 5ms */
#define UFS_AHIT_AUTOH8_TIMER 0x0C05

/* Auto Hibern8 Timer, RPMB: 5s */
#define UFS_AHIT_AUTOH8_RPMB_TIMER 0x11F4

/* maximum number of init retries before giving up */
#define MAX_HOST_INIT_RETRIES 7
#define HPB_TEST

struct ufs_hba;
#ifdef CONFIG_HUFS_MANUAL_BKOPS
struct ufs_dev_bkops_ops;
#endif
#ifdef CONFIG_SCSI_HUFS
#define ufs_dev_desc ufs_dev_info
void ufshcd_check_errors(struct ufs_hba *hba, bool lock_flag);
#endif
int ufshcd_hba_enable(struct ufs_hba *hba);
int __ufshcd_wait_for_doorbell_clr(struct ufs_hba *hba);
bool ufshcd_is_hufs_hc(struct ufs_hba *hba);
void __ufshcd_disable_dev_tmt_cnt(struct ufs_hba *hba);
void ufshcd_idle_handler(struct ufs_hba *hba);
void __ufshcd_disable_pwm_cnt(struct ufs_hba *hba);
bool ufshcd_support_inline_encrypt(struct ufs_hba *hba);
int hpb_reset_process(struct ufs_hba *hba, bool *hpb_after_reset);
int ufstt_do_scsi_cmd(struct ufs_hba *hba, struct scsi_cmnd *cmd,
			     unsigned int timeout, ktime_t start_time);

/* UFSHCD states */
enum {
	UFSHCD_STATE_RESET,
	UFSHCD_STATE_ERROR,
	UFSHCD_STATE_OPERATIONAL,
	UFSHCD_STATE_EH_SCHEDULED,
	UFSHCD_STATE_FFU,
	UFSHCD_STATE_EH_SCHEDULED_FATAL,
	UFSHCD_STATE_EH_SCHEDULED_NON_FATAL,
};

/* Auto Hibern8 Timer, RPMB: 5s */
#define UFS_AHIT_AUTOH8_RPMB_TIMER 0x11F4
enum dev_cmd_type {
	DEV_CMD_TYPE_NOP		= 0x0,
	DEV_CMD_TYPE_QUERY		= 0x1,
};

enum ufs_event_type {
	/* uic specific errors */
	UFS_EVT_PA_ERR = 0,
	UFS_EVT_DL_ERR,
	UFS_EVT_NL_ERR,
	UFS_EVT_TL_ERR,
	UFS_EVT_DME_ERR,

	/* fatal errors */
	UFS_EVT_AUTO_HIBERN8_ERR,
	UFS_EVT_FATAL_ERR,
	UFS_EVT_LINK_STARTUP_FAIL,
	UFS_EVT_RESUME_ERR,
	UFS_EVT_SUSPEND_ERR,

	/* abnormal events */
	UFS_EVT_DEV_RESET,
	UFS_EVT_HOST_RESET,
	UFS_EVT_ABORT,

	UFS_EVT_CNT,
};

/**
 * struct uic_command - UIC command structure
 * @command: UIC command
 * @argument1: UIC command argument 1
 * @argument2: UIC command argument 2
 * @argument3: UIC command argument 3
 * @cmd_active: Indicate if UIC command is outstanding
 * @done: UIC command completion
 */
struct uic_command {
	u32 command;
	u32 argument1;
	u32 argument2;
	u32 argument3;
	int cmd_active;
	struct completion done;
};

/* Used to differentiate the power management options */
enum ufs_pm_op {
	UFS_RUNTIME_PM,
	UFS_SYSTEM_PM,
	UFS_SHUTDOWN_PM,
};

#define ufshcd_is_runtime_pm(op) ((op) == UFS_RUNTIME_PM)
#define ufshcd_is_system_pm(op) ((op) == UFS_SYSTEM_PM)
#define ufshcd_is_shutdown_pm(op) ((op) == UFS_SHUTDOWN_PM)

/* Host <-> Device UniPro Link state */
enum uic_link_state {
	UIC_LINK_OFF_STATE	= 0, /* Link powered down or disabled */
	UIC_LINK_ACTIVE_STATE	= 1, /* Link is in Fast/Slow/Sleep state */
	UIC_LINK_HIBERN8_STATE	= 2, /* Link is in Hibernate state */
	UIC_LINK_BROKEN_STATE	= 3, /* Link is in broken state */
};

#define ufshcd_is_link_off(hba) ((hba)->uic_link_state == UIC_LINK_OFF_STATE)
#define ufshcd_is_link_active(hba) ((hba)->uic_link_state == \
				    UIC_LINK_ACTIVE_STATE)
#define ufshcd_is_link_hibern8(hba) ((hba)->uic_link_state == \
				    UIC_LINK_HIBERN8_STATE)
#define ufshcd_is_link_broken(hba) ((hba)->uic_link_state == \
				   UIC_LINK_BROKEN_STATE)
#define ufshcd_set_link_off(hba) ((hba)->uic_link_state = UIC_LINK_OFF_STATE)
#define ufshcd_set_link_active(hba) ((hba)->uic_link_state = \
				    UIC_LINK_ACTIVE_STATE)
#define ufshcd_set_link_hibern8(hba) ((hba)->uic_link_state = \
				    UIC_LINK_HIBERN8_STATE)
#define ufshcd_set_link_broken(hba) ((hba)->uic_link_state = \
				    UIC_LINK_BROKEN_STATE)

#define ufshcd_set_ufs_dev_active(h) \
	((h)->curr_dev_pwr_mode = UFS_ACTIVE_PWR_MODE)
#define ufshcd_set_ufs_dev_sleep(h) \
	((h)->curr_dev_pwr_mode = UFS_SLEEP_PWR_MODE)
#define ufshcd_set_ufs_dev_poweroff(h) \
	((h)->curr_dev_pwr_mode = UFS_POWERDOWN_PWR_MODE)
#define ufshcd_is_ufs_dev_active(h) \
	((h)->curr_dev_pwr_mode == UFS_ACTIVE_PWR_MODE)
#define ufshcd_is_ufs_dev_sleep(h) \
	((h)->curr_dev_pwr_mode == UFS_SLEEP_PWR_MODE)
#define ufshcd_is_ufs_dev_poweroff(h) \
	((h)->curr_dev_pwr_mode == UFS_POWERDOWN_PWR_MODE)

#define WB_MODE_MASK 0x1F
#define DOWNLOAD_MODE 0xE

enum {
	/* errors which require the host controller reset for recovery */
	UFS_ERR_HIBERN8_EXIT,
	UFS_ERR_VOPS_SUSPEND,
	UFS_ERR_EH,
	UFS_ERR_CLEAR_PEND_XFER_TM,
	UFS_ERR_INT_FATAL_ERRORS,
	UFS_ERR_INT_UIC_ERROR,

	/* other errors */
	UFS_ERR_HIBERN8_ENTER,
	UFS_ERR_RESUME,
	UFS_ERR_SUSPEND,
	UFS_ERR_LINKSTARTUP,
	UFS_ERR_POWER_MODE_CHANGE,
	UFS_ERR_TASK_ABORT,
	UFS_ERR_MAX,
};

/* UFSHCD error handling flags */
enum {
	UFSHCD_EH_IN_PROGRESS = (1 << 0),
};

#define ufshcd_set_eh_in_progress(h) \
	((h)->eh_flags |= UFSHCD_EH_IN_PROGRESS)
#define ufshcd_eh_in_progress(h) \
	((h)->eh_flags & UFSHCD_EH_IN_PROGRESS)
#define ufshcd_clear_eh_in_progress(h) \
	((h)->eh_flags &= ~UFSHCD_EH_IN_PROGRESS)

/*
 * UFS Power management levels.
 * Each level is in increasing order of power savings.
 */
enum ufs_pm_level {
	UFS_PM_LVL_0, /* UFS_ACTIVE_PWR_MODE, UIC_LINK_ACTIVE_STATE */
	UFS_PM_LVL_1, /* UFS_ACTIVE_PWR_MODE, UIC_LINK_HIBERN8_STATE */
	UFS_PM_LVL_2, /* UFS_SLEEP_PWR_MODE, UIC_LINK_ACTIVE_STATE */
	UFS_PM_LVL_3, /* UFS_SLEEP_PWR_MODE, UIC_LINK_HIBERN8_STATE */
	UFS_PM_LVL_4, /* UFS_POWERDOWN_PWR_MODE, UIC_LINK_HIBERN8_STATE */
	UFS_PM_LVL_5, /* UFS_POWERDOWN_PWR_MODE, UIC_LINK_OFF_STATE */
	UFS_PM_LVL_MAX
};

struct ufs_pm_lvl_states {
	enum ufs_dev_pwr_mode dev_state;
	enum uic_link_state link_state;
};

/**
 * struct ufshcd_lrb - local reference block
 * @utr_descriptor_ptr: UTRD address of the command
 * @ucd_req_ptr: UCD address of the command
 * @ucd_rsp_ptr: Response UPIU address for this command
 * @ucd_prdt_ptr: PRDT address of the command
 * @utrd_dma_addr: UTRD dma address for debug
 * @ucd_prdt_dma_addr: PRDT dma address for debug
 * @ucd_rsp_dma_addr: UPIU response dma address for debug
 * @ucd_req_dma_addr: UPIU request dma address for debug
 * @cmd: pointer to SCSI command
 * @sense_buffer: pointer to sense buffer address of the SCSI command
 * @sense_bufflen: Length of the sense buffer
 * @scsi_status: SCSI status of the command
 * @command_type: SCSI, UFS, Query.
 * @task_tag: Task tag of the command
 * @lun: LUN of the command
 * @intr_cmd: Interrupt command (doesn't participate in interrupt aggregation)
 * @issue_time_stamp: time stamp for debug purposes
 * @compl_time_stamp: time stamp for statistics
 * @crypto_key_slot: the key slot to use for inline crypto (-1 if none)
 * @data_unit_num: the data unit number for the first block for inline crypto
 * @req_abort_skip: skip request abort task flag
 */
struct ufshcd_lrb {
	struct utp_transfer_req_desc *utr_descriptor_ptr;
	struct hufs_utp_transfer_req_desc *hufs_utr_descriptor_ptr;
	struct utp_upiu_req *ucd_req_ptr;
	struct utp_upiu_rsp *ucd_rsp_ptr;
	struct ufshcd_sg_entry *ucd_prdt_ptr;

	dma_addr_t utrd_dma_addr;
	dma_addr_t ucd_req_dma_addr;
	dma_addr_t ucd_rsp_dma_addr;
	dma_addr_t ucd_prdt_dma_addr;

	struct scsi_cmnd *cmd;
	u8 *sense_buffer;
	unsigned int sense_bufflen;
	unsigned int saved_sense_len;
	int scsi_status;

	int command_type;
	int task_tag;
	u8 lun; /* UPIU LUN id field is only 8-bit wide */
	bool intr_cmd;
	ktime_t issue_time_stamp;
	ktime_t compl_time_stamp;
	u8 opcode;
	int req_tag;
	bool is_hufs_utrd; /* use hufs host controller UTRD structure */
	unsigned int operation_lba;
	u8 hpb_flag;
	u64 regionid;
	u64 subregionid;
	void *ufstt_private;
#ifdef	HPB_HUFS_DEBUG_PRINT
	ktime_t end_time;
	ktime_t start_time;
	u64 find_time;
#endif
#ifdef CONFIG_SCSI_UFS_CRYPTO
	int crypto_key_slot;
	u64 data_unit_num;
#endif

	bool req_abort_skip;
	bool req_send;

	ANDROID_KABI_RESERVE(1);
};

/**
 * struct ufs_query - holds relevant data structures for query request
 * @request: request upiu and function
 * @descriptor: buffer for sending/receiving descriptor
 * @response: response upiu and response
 */
struct ufs_query {
	struct ufs_query_req request;
	u8 *descriptor;
	struct ufs_query_res response;
};

/**
 * struct ufs_dev_cmd - all assosiated fields with device management commands
 * @type: device management command type - Query, NOP OUT
 * @lock: lock to allow one command at a time
 * @complete: internal commands completion
 */
struct ufs_dev_cmd {
	enum dev_cmd_type type;
	struct mutex lock;
	struct completion *complete;
	wait_queue_head_t tag_wq;
	struct ufs_query query;
};

#define UIC_ERR_REG_HIST_LENGTH 8
/**
 * struct ufs_uic_err_reg_hist - keeps history of uic errors
 * @pos: index to indicate cyclic buffer position
 * @reg: cyclic buffer for registers value
 * @tstamp: cyclic buffer for time stamp
 */
struct ufs_uic_err_reg_hist {
	int pos;
	u32 reg[UIC_ERR_REG_HIST_LENGTH];
	ktime_t tstamp[UIC_ERR_REG_HIST_LENGTH];
};

#ifdef CONFIG_DEBUG_FS
struct debugfs_files {
	struct dentry *debugfs_root;
	struct dentry *tag_stats;
	struct dentry *err_stats;
	struct dentry *show_hba;
	struct dentry *host_regs;
	struct dentry *dump_dev_desc;
	struct dentry *power_mode;
	struct dentry *dme_local_read;
	struct dentry *dme_peer_read;
	struct dentry *req_stats;
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *idle_intr_verify;
	struct dentry *idle_timeout_val;
	struct dentry *idle_intr_check_timer_threshold;
#endif
	u32 dme_local_attr_id;
	u32 dme_peer_attr_id;
};
/* tag stats statistics types */
enum ts_types {
	TS_NOT_SUPPORTED	= -1,
	TS_TAG			= 0,
	TS_READ			= 1,
	TS_WRITE		= 2,
	TS_URGENT_READ		= 3,
	TS_URGENT_WRITE		= 4,
	TS_FLUSH		= 5,
	TS_NUM_STATS		= 6,
};
/**
 * struct ufshcd_req_stat - statistics for request handling times (in usec)
 * @min: shortest time measured
 * @max: longest time measured
 * @sum: sum of all the handling times measured (used for average calculation)
 * @count: number of measurements taken
 */
struct ufshcd_req_stat {
	u64 min;
	u64 max;
	u64 sum;
	u64 count;
};
#endif

/**
 * struct ufs_stats - keeps usage/err statistics
 * @enabled: enable tag stats for debugfs
 * @tag_stats: pointer to tag statistic counters
 * @q_depth: current amount of busy slots
 * @err_stats: counters to keep track of various errors
 * @req_stat: request handling time statistics per request type
 * @hibern8_exit_cnt: Counter to keep track of number of exits,
 *		reset this after link-startup.
 * @last_hibern8_exit_tstamp: Set time after the hibern8 exit.
 *		Clear after the first successful command completion.
 * @pa_err: tracks pa-uic errors
 * @dl_err: tracks dl-uic errors
 * @nl_err: tracks nl-uic errors
 * @tl_err: tracks tl-uic errors
 * @dme_err: tracks dme errors
 */


/**
 * struct ufs_clk_info - UFS clock related info
 * @list: list headed by hba->clk_list_head
 * @clk: clock node
 * @name: clock name
 * @max_freq: maximum frequency supported by the clock
 * @min_freq: min frequency that can be used for clock scaling
 * @curr_freq: indicates the current frequency that it is set to
 * @keep_link_active: indicates that the clk should not be disabled if
		      link is active
 * @enabled: variable to check against multiple enable/disable
 */
struct ufs_clk_info {
	struct list_head list;
	struct clk *clk;
	const char *name;
	u32 max_freq;
	u32 min_freq;
	u32 curr_freq;
	bool keep_link_active;
	bool enabled;
};

enum ufs_notify_change_status {
	PRE_CHANGE,
	POST_CHANGE,
};

struct ufs_pa_layer_attr {
	u32 gear_rx;
	u32 gear_tx;
	u32 lane_rx;
	u32 lane_tx;
	u32 pwr_rx;
	u32 pwr_tx;
	u32 hs_rate;
};

struct ufs_pwr_mode_info {
	bool is_valid;
	struct ufs_pa_layer_attr info;
};

/* vendor specific pre-defined parameters */
#define SLOW 1
#define FAST 2

#define DOORBELL_TIMEOUT_MS (10 * 1000)

/**
 * struct ufs_hba_variant_ops - variant specific callbacks
 * @name: variant name
 * @init: called when the driver is initialized
 * @exit: called to cleanup everything done in init
 * @get_ufs_hci_version: called to get UFS HCI version
 * @clk_scale_notify: notifies that clks are scaled up/down
 * @setup_clocks: called before touching any of the controller registers
 * @setup_regulators: called before accessing the host controller
 * @hce_enable_notify: called before and after HCE enable bit is set to allow
 *                     variant specific Uni-Pro initialization.
 * @link_startup_notify: called before and after Link startup is carried out
 *                       to allow variant specific Uni-Pro initialization.
 * @pwr_change_notify: called before and after a power mode change
 *			is carried out to allow vendor spesific capabilities
 *			to be set.
 * @setup_xfer_req: called before any transfer request is issued
 *                  to set some things
 * @setup_task_mgmt: called before any task management request is issued
 *                  to set some things
 * @hibern8_notify: called around hibern8 enter/exit
 * @apply_dev_quirks: called to apply device specific quirks
 * @suspend: called during host controller PM callback
 * @resume: called during host controller PM callback
 * @dbg_register_dump: used to dump controller debug information
 * @phy_initialization: used to initialize phys
 * @device_reset: called to issue a reset pulse on the UFS device
 * @program_key: program or evict an inline encryption key
 * @event_notify: called to notify important events
 */
struct ufs_hba_variant_ops {
	const char *name;
	int	(*init)(struct ufs_hba *);
	void    (*exit)(struct ufs_hba *);
	u32	(*get_ufs_hci_version)(struct ufs_hba *);
	int	(*clk_scale_notify)(struct ufs_hba *, bool,
				    enum ufs_notify_change_status);
	int	(*setup_clocks)(struct ufs_hba *, bool,
				enum ufs_notify_change_status);
	int     (*setup_regulators)(struct ufs_hba *, bool);
	int	(*hce_enable_notify)(struct ufs_hba *,
				     enum ufs_notify_change_status);
	int	(*link_startup_notify)(struct ufs_hba *,
				       enum ufs_notify_change_status);
	int	(*pwr_change_notify)(struct ufs_hba *,
					enum ufs_notify_change_status status,
					struct ufs_pa_layer_attr *,
					struct ufs_pa_layer_attr *);
	void	(*setup_xfer_req)(struct ufs_hba *, int, bool);
	void	(*setup_task_mgmt)(struct ufs_hba *, int, u8);
	void    (*hibern8_notify)(struct ufs_hba *, enum uic_cmd_dme,
					enum ufs_notify_change_status);
	int	(*apply_dev_quirks)(struct ufs_hba *hba);
	void	(*fixup_dev_quirks)(struct ufs_hba *hba);
	int (*hufs_dme_link_startup)(struct ufs_hba *);
	int (*hufs_uic_write_reg)(struct ufs_hba *, uint32_t, uint32_t);
	int (*hufs_uic_read_reg)(struct ufs_hba *, uint32_t, uint32_t *);
	int (*hufs_uic_peer_set)(struct ufs_hba *, uint32_t, uint32_t);
	int (*hufs_uic_peer_get)(struct ufs_hba *, uint32_t, uint32_t *);
	void (*ufshcd_hufs_enable_auto_hibern8)(struct ufs_hba *);
	int (*ufshcd_hufs_disable_auto_hibern8)(struct ufs_hba *);
	int (*ufshcd_hibern8_op_irq_safe)(struct ufs_hba *, bool);
	void (*ufshcd_hufs_host_memory_configure)(struct ufs_hba *);
	int (*ufshcd_hufs_uic_change_pwr_mode)(struct ufs_hba *, u8);
	uint32_t (*snps_to_hufs_addr)(uint32_t, uint32_t);
	void (*dbg_hufs_dme_dump)(struct ufs_hba *);
	int (*suspend_before_set_link_state)(struct ufs_hba *, enum ufs_pm_op);
	int     (*suspend)(struct ufs_hba *, enum ufs_pm_op);
	int     (*resume)(struct ufs_hba *, enum ufs_pm_op);
	int (*resume_after_set_link_state)(struct ufs_hba *, enum ufs_pm_op);
	void (*full_reset)(struct ufs_hba *);
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	int (*uie_config_init)(struct ufs_hba *);
	void (*uie_utrd_pre)(struct ufs_hba *, struct ufshcd_lrb *);
#endif
	void (*set_ref_clk)(struct ufs_hba *);
	/* hufs specific ops */
	void (*dbg_hci_dump)(struct ufs_hba *);
	void (*dbg_uic_dump)(struct ufs_hba *);
#ifdef CONFIG_DEBUG_FS
	void (*add_debugfs)(struct ufs_hba *hba, struct dentry *root);
	void (*remove_debugfs)(struct ufs_hba *hba);
#endif
#ifdef CONFIG_SCSI_HUFS_LINERESET_CHECK
	int (*background_thread)(void *d);
#endif
	void	(*dbg_register_dump)(struct ufs_hba *hba);
	int	(*phy_initialization)(struct ufs_hba *);
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	int (*get_pwr_by_debug_register)(struct ufs_hba *hba);
#endif
	void (*vcc_power_on_off)(struct ufs_hba *hba);
	int (*get_device_info)(struct ufs_hba *hba,
			       struct ufs_dev_info *dev_info, u8 *desc_buf);
	void (*fix_mphy_adapt)(struct ufs_hba *hba);
#ifdef CONFIG_DEBUG_FS
	void (*device_reset)(struct ufs_hba *hba);
#else
	int	(*device_reset)(struct ufs_hba *hba);
#endif
	void	(*config_scaling_param)(struct ufs_hba *hba,
					struct devfreq_dev_profile *profile,
					void *data);
	int	(*program_key)(struct ufs_hba *hba,
			       const union ufs_crypto_cfg_entry *cfg, int slot);
	void	(*event_notify)(struct ufs_hba *hba,
				enum ufs_event_type evt, void *data);

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
};

/* clock gating state  */
enum clk_gating_state {
	CLKS_OFF,
	CLKS_ON,
	REQ_CLKS_OFF,
	REQ_CLKS_ON,
};

/**
 * struct ufs_clk_gating - UFS clock gating related info
 * @gate_work: worker to turn off clocks after some delay as specified in
 * delay_ms
 * @ungate_work: worker to turn on clocks that will be used in case of
 * interrupt context
 * @state: the current clocks state
 * @delay_ms: gating delay in ms
 * @is_suspended: clk gating is suspended when set to 1 which can be used
 * during suspend/resume
 * @delay_attr: sysfs attribute to control delay_attr
 * @enable_attr: sysfs attribute to enable/disable clock gating
 * @is_enabled: Indicates the current status of clock gating
 * @is_initialized: Indicates whether clock gating is initialized or not
 * @active_reqs: number of requests that are pending and should be waited for
 * completion before gating clocks.
 */
struct ufs_clk_gating {
	struct delayed_work gate_work;
	struct work_struct ungate_work;
	enum clk_gating_state state;
	unsigned long delay_ms;
	bool is_suspended;
	struct device_attribute delay_attr;
	struct device_attribute enable_attr;
	bool is_enabled;
	bool is_initialized;
	int active_reqs;
	struct workqueue_struct *clk_gating_workq;

	ANDROID_KABI_RESERVE(1);
};
/* UFS Host Controller debug print bitmask */
#define UFSHCD_DBG_PRINT_CLK_FREQ_EN		UFS_BIT(0)
#define UFSHCD_DBG_PRINT_UIC_ERR_HIST_EN	UFS_BIT(1)
#define UFSHCD_DBG_PRINT_HOST_REGS_EN		UFS_BIT(2)
#define UFSHCD_DBG_PRINT_TRS_EN			UFS_BIT(3)
#define UFSHCD_DBG_PRINT_TMRS_EN		UFS_BIT(4)
#define UFSHCD_DBG_PRINT_PWR_EN			UFS_BIT(5)
#define UFSHCD_DBG_PRINT_HOST_STATE_EN		UFS_BIT(6)

#define UFSHCD_DBG_PRINT_ALL						   \
		(UFSHCD_DBG_PRINT_CLK_FREQ_EN | \
		 UFSHCD_DBG_PRINT_UIC_ERR_HIST_EN | \
		 UFSHCD_DBG_PRINT_HOST_REGS_EN | UFSHCD_DBG_PRINT_TRS_EN | \
		 UFSHCD_DBG_PRINT_TMRS_EN | UFSHCD_DBG_PRINT_PWR_EN |	   \
		 UFSHCD_DBG_PRINT_HOST_STATE_EN)

#ifdef CONFIG_SCSI_UFS_HI1861_VCMD
struct ufs_hi1861_fsr {
	struct device_attribute fsr_attr;
};
#endif

struct ufs_temp {
	struct device_attribute temp_attr;
	int temp;
};
struct ufs_inline_state {
	struct device_attribute inline_attr;
};

#if defined(CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO) && defined(CONFIG_DFX_DEBUG_FS)
enum ufs_inline_debug_state {
	DEBUG_LOG_OFF,
	DEBUG_LOG_ON,
	DEBUG_CRYPTO_OFF,
	DEBUG_CRYPTO_ON,
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	DEBUG_CRYPTO_V3_ON,
#endif
};


struct ufs_inline_debug {
	struct device_attribute inline_attr;
};
#endif

struct ufs_saved_pwr_info {
	struct ufs_pa_layer_attr info;
	bool is_valid;
};

/**
 * struct ufs_clk_scaling - UFS clock scaling related data
 * @active_reqs: number of requests that are pending. If this is zero when
 * devfreq ->target() function is called then schedule "suspend_work" to
 * suspend devfreq.
 * @tot_busy_t: Total busy time in current polling window
 * @window_start_t: Start time (in jiffies) of the current polling window
 * @busy_start_t: Start time of current busy period
 * @enable_attr: sysfs attribute to enable/disable clock scaling
 * @saved_pwr_info: UFS power mode may also be changed during scaling and this
 * one keeps track of previous power mode.
 * @workq: workqueue to schedule devfreq suspend/resume work
 * @suspend_work: worker to suspend devfreq
 * @resume_work: worker to resume devfreq
 * @min_gear: lowest HS gear to scale down to
 * @is_enabled: tracks if scaling is currently enabled or not, controlled by
		clkscale_enable sysfs node
 * @is_allowed: tracks if scaling is currently allowed or not, used to block
		clock scaling which is not invoked from devfreq governor
 * @is_initialized: Indicates whether clock scaling is initialized or not
 * @is_busy_started: tracks if busy period has started or not
 * @is_suspended: tracks if devfreq is suspended or not
 */
struct ufs_clk_scaling {
	int active_reqs;
	unsigned long tot_busy_t;
	ktime_t window_start_t;
	ktime_t busy_start_t;
	struct device_attribute enable_attr;
	struct ufs_saved_pwr_info saved_pwr_info;
	struct workqueue_struct *workq;
	struct work_struct suspend_work;
	struct work_struct resume_work;
	u32 min_gear;
	bool is_enabled;
	bool is_allowed;
	bool is_initialized;
	bool is_busy_started;
	bool is_suspended;

	ANDROID_KABI_RESERVE(1);
};

#define UFS_EVENT_HIST_LENGTH 8
/**
 * struct ufs_event_hist - keeps history of errors
 * @pos: index to indicate cyclic buffer position
 * @reg: cyclic buffer for registers value
 * @tstamp: cyclic buffer for time stamp
 * @cnt: error counter
 */
struct ufs_event_hist {
	int pos;
	u32 val[UFS_EVENT_HIST_LENGTH];
	ktime_t tstamp[UFS_EVENT_HIST_LENGTH];
	unsigned long long cnt;
};

/**
 * UNIQUE NUMBER definition
 */
struct ufs_unique_number {
	/* serial_number length 12 */
	uint8_t serial_number[12];
	uint16_t manufacturer_date;
	uint16_t manufacturer_id;
};

/**
 * struct ufs_stats - keeps usage/err statistics
 * @last_intr_status: record the last interrupt status.
 * @last_intr_ts: record the last interrupt timestamp.
 * @hibern8_exit_cnt: Counter to keep track of number of exits,
 *		reset this after link-startup.
 * @last_hibern8_exit_tstamp: Set time after the hibern8 exit.
 *		Clear after the first successful command completion.
 */
struct ufs_stats {
	u32 last_intr_status;
	ktime_t last_intr_ts;

	u32 hibern8_exit_cnt;
	ktime_t last_hibern8_exit_tstamp;
	struct ufs_event_hist event[UFS_EVT_CNT];
#ifdef CONFIG_DEBUG_FS
	bool enabled;
	u64 **tag_stats;
	int q_depth;
	int err_stats[UFS_ERR_MAX];
	struct ufshcd_req_stat req_stats[TS_NUM_STATS];
#endif
#ifdef CONFIG_SCSI_HUFS
	struct ufs_uic_err_reg_hist pa_err;
	struct ufs_uic_err_reg_hist dl_err;
	struct ufs_uic_err_reg_hist nl_err;
	struct ufs_uic_err_reg_hist tl_err;
	struct ufs_uic_err_reg_hist dme_err;
#endif
};

enum ufshcd_quirks {
	/* Interrupt aggregation support is broken */
	UFSHCD_QUIRK_BROKEN_INTR_AGGR			= 1 << 0,

	/*
	 * delay before each dme command is required as the unipro
	 * layer has shown instabilities
	 */
	UFSHCD_QUIRK_DELAY_BEFORE_DME_CMDS		= 1 << 1,

	/*
	 * If UFS host controller is having issue in processing LCC (Line
	 * Control Command) coming from device then enable this quirk.
	 * When this quirk is enabled, host controller driver should disable
	 * the LCC transmission on UFS device (by clearing TX_LCC_ENABLE
	 * attribute of device to 0).
	 */
	UFSHCD_QUIRK_BROKEN_LCC				= 1 << 2,

	/*
	 * The attribute PA_RXHSUNTERMCAP specifies whether or not the
	 * inbound Link supports unterminated line in HS mode. Setting this
	 * attribute to 1 fixes moving to HS gear.
	 */
	UFSHCD_QUIRK_BROKEN_PA_RXHSUNTERMCAP		= 1 << 3,

	/*
	 * This quirk needs to be enabled if the host controller only allows
	 * accessing the peer dme attributes in AUTO mode (FAST AUTO or
	 * SLOW AUTO).
	 */
	UFSHCD_QUIRK_DME_PEER_ACCESS_AUTO_MODE		= 1 << 4,

	/*
	 * This quirk needs to be enabled if the host controller doesn't
	 * advertise the correct version in UFS_VER register. If this quirk
	 * is enabled, standard UFS host driver will call the vendor specific
	 * ops (get_ufs_hci_version) to get the correct version.
	 */
	UFSHCD_QUIRK_BROKEN_UFS_HCI_VERSION		= 1 << 5,

	/*
	 * Clear handling for transfer/task request list is just opposite.
	 */
	UFSHCI_QUIRK_BROKEN_REQ_LIST_CLR		= 1 << 6,

	/*
	 * This quirk needs to be enabled if host controller doesn't allow
	 * that the interrupt aggregation timer and counter are reset by s/w.
	 */
	UFSHCI_QUIRK_SKIP_RESET_INTR_AGGR		= 1 << 7,

	/*
	 * This quirks needs to be enabled if host controller cannot be
	 * enabled via HCE register.
	 */
	UFSHCI_QUIRK_BROKEN_HCE				= 1 << 8,

	/*
	 * This quirk needs to be enabled if the host controller regards
	 * resolution of the values of PRDTO and PRDTL in UTRD as byte.
	 */
	UFSHCD_QUIRK_PRDT_BYTE_GRAN			= 1 << 9,

	/*
	 * This quirk needs to be enabled if the host controller reports
	 * OCS FATAL ERROR with device error through sense data
	 */
	UFSHCD_QUIRK_BROKEN_OCS_FATAL_ERROR		= 1 << 10,

	/*
	 * This quirk needs to be enabled if the host controller has
	 * auto-hibernate capability but it doesn't work.
	 */
	UFSHCD_QUIRK_BROKEN_AUTO_HIBERN8		= 1 << 11,

	/*
	 * This quirk needs to disable manual flush for write booster
	 */
	UFSHCI_QUIRK_SKIP_MANUAL_WB_FLUSH_CTRL		= 1 << 12,

	/*
	 * This quirk needs to disable unipro timeout values
	 * before power mode change
	 */
	UFSHCD_QUIRK_SKIP_DEF_UNIPRO_TIMEOUT_SETTING = 1 << 13,

	/*
	 * This quirk allows only sg entries aligned with page size.
	 */
	UFSHCD_QUIRK_ALIGN_SG_WITH_PAGE_SIZE		= 1 << 14,

	/*
	 * This quirk needs to be enabled if the host controller does not
	 * support UIC command
	 */
	UFSHCD_QUIRK_BROKEN_UIC_CMD			= 1 << 15,

	/*
	 * This quirk needs to be enabled if the host controller cannot
	 * support interface configuration.
	 */
	UFSHCD_QUIRK_SKIP_INTERFACE_CONFIGURATION	= 1 << 16,

	/*
	 * This quirk needs to be enabled if the host controller supports inline
	 * encryption, but it needs to initialize the crypto capabilities in a
	 * nonstandard way and/or it needs to override blk_ksm_ll_ops.  If
	 * enabled, the standard code won't initialize the blk_keyslot_manager;
	 * ufs_hba_variant_ops::init() must do it instead.
	 */
	UFSHCD_QUIRK_CUSTOM_KEYSLOT_MANAGER		= 1 << 20,

	/*
	 * This quirk needs to be enabled if the host controller supports inline
	 * encryption, but the CRYPTO_GENERAL_ENABLE bit is not implemented and
	 * breaks the HCE sequence if used.
	 */
	UFSHCD_QUIRK_BROKEN_CRYPTO_ENABLE		= 1 << 21,

	/*
	 * This quirk needs to be enabled if the host controller requires that
	 * the PRDT be cleared after each encrypted request because encryption
	 * keys were stored in it.
	 */
	UFSHCD_QUIRK_KEYS_IN_PRDT			= 1 << 22,

	/*
	 * This quirk needs to be enabled if the host controller doesn't want
	 * to use HCD(Host Controller Disable) to reset host. If is enabled,
	 * UFS error handler directly hard reset host instead of a soft one.
	 */
	UFSHCD_QUIRK_BROKEN_HCE                         = 1 << 28,
	UFSHCD_QUIRK_DEVICE_CONFIG                      = 1 << 29,
	UFSHCD_QUIRK_UNIPRO_TERMINATION                 = 1 << 30,
	UFSHCD_QUIRK_UNIPRO_SCRAMBLING                  = 1 << 31,
};

enum ufshcd_caps {
	/* Allow dynamic clk gating */
	UFSHCD_CAP_CLK_GATING				= 1 << 0,

	/* Allow hiberb8 with clk gating */
	UFSHCD_CAP_HIBERN8_WITH_CLK_GATING		= 1 << 1,

	/* Allow dynamic clk scaling */
	UFSHCD_CAP_CLK_SCALING				= 1 << 2,

	/* Allow auto bkops to enabled during runtime suspend */
	UFSHCD_CAP_AUTO_BKOPS_SUSPEND			= 1 << 3,

	/*
	 * This capability allows host controller driver to use the UFS HCI's
	 * interrupt aggregation capability.
	 * CAUTION: Enabling this might reduce overall UFS throughput.
	 */
	UFSHCD_CAP_INTR_AGGR				= 1 << 4,

	/*
	 * This capability allows the device auto-bkops to be always enabled
	 * except during suspend (both runtime and suspend).
	 * Enabling this capability means that device will always be allowed
	 * to do background operation when it's active but it might degrade
	 * the performance of ongoing read/write operations.
	 */
	UFSHCD_CAP_KEEP_AUTO_BKOPS_ENABLED_EXCEPT_SUSPEND = 1 << 5,

	/*
	 * This capability allows host controller driver to automatically
	 * enable runtime power management by itself instead of waiting
	 * for userspace to control the power management.
	 */
	UFSHCD_CAP_RPM_AUTOSUSPEND			= 1 << 6,

	/*
	 * This capability allows the host controller driver to turn-on
	 * WriteBooster, if the underlying device supports it and is
	 * provisioned to be used. This would increase the write performance.
	 */
	UFSHCD_CAP_WB_EN				= 1 << 7,

	/*
	 * This capability allows the host controller driver to use the
	 * inline crypto engine, if it is present
	 */
	UFSHCD_CAP_CRYPTO				= 1 << 8,

	/*
	 * This capability allows the controller regulators to be put into
	 * lpm mode aggressively during clock gating.
	 * This would increase power savings.
	 */
	UFSHCD_CAP_AGGR_POWER_COLLAPSE			= 1 << 9,
};

struct ufs_hba_variant_params {
	struct devfreq_dev_profile devfreq_profile;
	struct devfreq_simple_ondemand_data ondemand_data;
	u16 hba_enable_delay_us;
	u32 wb_flush_threshold;
};

#ifdef CONFIG_SCSI_UFS_HPB
/**
 * struct ufshpb_dev_info - UFSHPB device related info
 * @num_lu: the number of user logical unit to check whether all lu finished
 *          initialization
 * @rgn_size: device reported HPB region size
 * @srgn_size: device reported HPB sub-region size
 * @slave_conf_cnt: counter to check all lu finished initialization
 * @hpb_disabled: flag to check if HPB is disabled
 * @max_hpb_single_cmd: device reported bMAX_DATA_SIZE_FOR_SINGLE_CMD value
 * @is_legacy: flag to check HPB 1.0
 * @control_mode: either host or device
 */
struct ufshpb_dev_info {
	int num_lu;
	int rgn_size;
	int srgn_size;
	atomic_t slave_conf_cnt;
	bool hpb_disabled;
	u8 max_hpb_single_cmd;
	bool is_legacy;
	u8 control_mode;
};
#endif

struct ufs_hba_monitor {
	unsigned long chunk_size;

	unsigned long nr_sec_rw[2];
	ktime_t total_busy[2];

	unsigned long nr_req[2];
	/* latencies*/
	ktime_t lat_sum[2];
	ktime_t lat_max[2];
	ktime_t lat_min[2];

	u32 nr_queued[2];
	ktime_t busy_start_ts[2];

	ktime_t enabled_ts;
	bool enabled;
};

#ifdef CONFIG_HUFS_HC_CORE_UTR
struct hufs_qos_ctrl {
	bool hufs_qos_inited;
	bool hufs_qos_en;
#define NUM_OF_QOS_LVL	8
	unsigned char core_qos_outstd_num[NUM_OF_QOS_LVL];
	/* num of promotable QOS Level, range from 0 to 6 */
#define NUM_OF_PROMOTE_QOS_LVL	7
	unsigned char core_qos_prmt_outstd_num[NUM_OF_PROMOTE_QOS_LVL];
	unsigned char core_qos_incrs_outstd_num[NUM_OF_PROMOTE_QOS_LVL];
	unsigned char core_qos_outstand_arb_num;
};
#endif

struct samsung_v5_pnm {
	char *ufs_pnm;
	struct list_head p;
};

/**
 * struct ufs_hba - per adapter private structure
 * @mmio_base: UFSHCI base register address
 * @ucdl_base_addr: UFS Command Descriptor base address
 * @utrdl_base_addr: UTP Transfer Request Descriptor base address
 * @utmrdl_base_addr: UTP Task Management Descriptor base address
 * @ucdl_dma_addr: UFS Command Descriptor DMA address
 * @utrdl_dma_addr: UTRDL DMA address
 * @utmrdl_dma_addr: UTMRDL DMA address
 * @host: Scsi_Host instance of the driver
 * @dev: device handle
 * @lrb: local reference block
 * @cmd_queue: Used to allocate command tags from hba->host->tag_set.
 * @outstanding_tasks: Bits representing outstanding task requests
 * @outstanding_reqs: Bits representing outstanding transfer requests
 * @capabilities: UFS Controller Capabilities
 * @nutrs: Transfer Request Queue depth supported by controller
 * @nutmrs: Task Management Queue depth supported by controller
 * @ufs_version: UFS Version to which controller complies
 * @vops: pointer to variant specific operations
 * @priv: pointer to variant specific private data
 * @sg_entry_size: size of struct ufshcd_sg_entry (may include variant fields)
 * @irq: Irq number of the controller
 * @active_uic_cmd: handle of active UIC command
 * @uic_cmd_mutex: mutex for uic command
 * @tmf_tag_set: TMF tag set.
 * @tmf_queue: Used to allocate TMF tags.
 * @pwr_done: completion for power mode change
 * @ufshcd_state: UFSHCD states
 * @eh_flags: Error handling flags
 * @intr_mask: Interrupt Mask Bits
 * @ee_ctrl_mask: Exception event control mask
 * @is_powered: flag to check if HBA is powered
 * @shutting_down: flag to check if shutdown has been invoked
 * @host_sem: semaphore used to serialize concurrent contexts
 * @eh_wq: Workqueue that eh_work works on
 * @eh_work: Worker to handle UFS errors that require s/w attention
 * @eeh_work: Worker to handle exception events
 * @errors: HBA errors
 * @uic_error: UFS interconnect layer error status
 * @saved_err: sticky error mask
 * @saved_uic_err: sticky UIC error mask
 * @force_reset: flag to force eh_work perform a full reset
 * @force_pmc: flag to force a power mode change
 * @silence_err_logs: flag to silence error logs
 * @dev_cmd: ufs device management command information
 * @last_dme_cmd_tstamp: time stamp of the last completed DME command
 * @auto_bkops_enabled: to track whether bkops is enabled in device
 * @vreg_info: UFS device voltage regulator information
 * @clk_list_head: UFS host controller clocks list node head
 * @pwr_info: holds current power mode
 * @max_pwr_info: keeps the device max valid pwm
 * @desc_size: descriptor sizes reported by device
 * @urgent_bkops_lvl: keeps track of urgent bkops level for device
 * @is_urgent_bkops_lvl_checked: keeps track if the urgent bkops level for
 *  device is known or not.
 * @scsi_block_reqs_cnt: reference counting for scsi block requests
 * @crypto_capabilities: Content of crypto capabilities register (0x100)
 * @crypto_cap_array: Array of crypto capabilities
 * @crypto_cfg_register: Start of the crypto cfg array
 * @ksm: the keyslot manager tied to this hba
 */
struct ufs_hba {
	void __iomem *mmio_base;
	void __iomem *ufs_unipro_base;
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	void __iomem *key_reg_base;
#endif
	unsigned char inline_encryto_ver;
	/* Virtual memory reference */
	struct utp_transfer_cmd_desc *ucdl_base_addr;
	struct utp_transfer_req_desc *utrdl_base_addr;
	struct hufs_utp_transfer_req_desc *hufs_utrdl_base_addr;
	struct utp_task_req_desc *utmrdl_base_addr;

	/* DMA memory reference */
	dma_addr_t ucdl_dma_addr;
	dma_addr_t utrdl_dma_addr;
	dma_addr_t utmrdl_dma_addr;

#ifdef CONFIG_HUAWEI_DSM_IOMT_UFS_HOST
	struct iomt_host_info iomt_host_info_entity;
#endif
	struct Scsi_Host *host;
	struct device *dev;
	struct request_queue *cmd_queue;
	/*
	 * This field is to keep a reference to "scsi_device" corresponding to
	 * "UFS device" W-LU.
	 */
	struct scsi_device *sdev_ufs_device;
	struct scsi_device *sdev_rpmb;

	enum ufs_dev_pwr_mode curr_dev_pwr_mode;
	enum uic_link_state uic_link_state;
	/* Desired UFS power management level during runtime PM */
	enum ufs_pm_level rpm_lvl;
	/* Desired UFS power management level during system PM */
	enum ufs_pm_level spm_lvl;
	struct device_attribute rpm_lvl_attr;
	struct device_attribute spm_lvl_attr;
	int pm_op_in_progress;
	int idle_intr_disabled;
	/* Auto-Hibernate Idle Timer register value */
	u32 ahit;

	struct ufshcd_lrb *lrb;
	unsigned long lrb_in_use;

	unsigned long outstanding_tasks;
	unsigned long outstanding_reqs;

	u32 capabilities;
	int nutrs;
	int nutmrs;
	u32 ufs_version;
	const struct ufs_hba_variant_ops *vops;
	struct ufs_hba_variant_params *vps;
	void *priv;
	size_t sg_entry_size;
	unsigned int irq;
	int volt_irq;
	bool is_irq_enabled;
	enum ufs_ref_clk_freq dev_ref_clk_freq;

/* hufs host controller */
	int use_hufs_hc;
#ifdef CONFIG_SCSI_UFS_INTR_HUB
	int ufs_intr_hub_irq;
#else
	int unipro_irq;
	int fatal_err_irq;
#endif
#ifdef CONFIG_HUFS_HC_CORE_UTR
#define CORE_IRQ_NAME_LEN	32
	char core_irq_name[8][CORE_IRQ_NAME_LEN + 1];
	int core_irq[8];
	struct hufs_qos_ctrl qos_ctrl;
	struct hlist_node node;
#endif
	struct mutex update_lock;
	bool in_suspend;
	bool in_shutdown;
	u64 ufshpb_4k_total;
	u64 ufshpb_hit_count;
	u64 ufshpb_read_buffer_count;
	u64 ufshpb_read_buffer_fail_count;
	u64 ufshpb_check_rsp_err;
	u64 ufshpb_scsi_timeout;
	int ufshpb_reset_count;
	u64 ufshpb_inactive_count;
	u64 ufshpb_add_node_count;
	u64 ufshpb_delete_node_count;
	u64 ufshpb_work_count;
	u64 ufstt_4k_total;
	u64 ufstt_hit_count;
	struct ufstt_lu_ctl *ufstt_lu_ctl[UFS_UPIU_MAX_GENERAL_LUN];
	struct delayed_work ufstt_init_work;
#ifdef CONFIG_SCSI_UFS_SYNCCACHE_RECORD
	struct delayed_work synccache_worker;
#endif
	struct work_struct ufstt_eh_work;
	int ufstt_state;
	bool issue_ioctl;
	u8 ufstt_l1_size;
	u64 ufstt_read_buffer_count;
	u16 ufstt_version;
	bool ufstt_table_syning;
	u32 ufstt_vendor_ufs_feature_support;
	u64 ufstt_capacity;
	bool ufstt_batch_mode;

	unsigned int quirks;	/* Deviations from standard UFSHCI spec. */

	/* Device deviations from standard UFS device spec. */
	unsigned int dev_quirks;

	wait_queue_head_t tm_wq;
	wait_queue_head_t tm_tag_wq;
	unsigned long tm_condition;
	unsigned long tm_slots_in_use;

	struct blk_mq_tag_set tmf_tag_set;
	struct request_queue *tmf_queue;

	struct uic_command *active_uic_cmd;
	struct mutex uic_cmd_mutex;
	struct completion *uic_async_done;

	/* for hufs host controller */
	struct completion uic_linkup_done;
	struct completion uic_pmc_done;
	struct completion uic_peer_get_done;

	u32 ufshcd_state;
	u32 eh_flags;
	u32 intr_mask;
	u16 ee_ctrl_mask;
	bool is_powered;
	bool shutting_down;
	struct semaphore host_sem;

	/* Work Queues */
	struct workqueue_struct *eh_wq;
	struct task_struct *eh_worker_task;
	struct kthread_worker eh_worker;
#ifdef CONFIG_SCSI_HUFS
	struct kthread_work eh_work;
#else
	struct work_struct eh_work;
#endif
	struct work_struct eeh_work;
	struct work_struct dsm_work;
#ifdef CONFIG_SCSI_UFS_HI1861_VCMD
	struct work_struct fsr_work;
#endif
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	struct work_struct recover_hs_work;
#endif
	struct work_struct ffu_pm_work;

	struct work_struct rpmb_pm_work;
	/* HBA Errors */
	u32 errors;
	u32 uic_error;
	u32 saved_err;
	u32 saved_uic_err;
	struct ufs_stats ufs_stats;
	bool force_reset;
	bool force_pmc;
	bool silence_err_logs;
	u32 error_handle_count;
	u32 io_timeout_cnt;

	/* Device management request data */
	struct ufs_dev_cmd dev_cmd;
	ktime_t last_dme_cmd_tstamp;

	/* Keeps information of the UFS device connected to this host */
	struct ufs_dev_info dev_info;
	bool auto_bkops_enabled;
	struct ufs_vreg_info vreg_info;
	struct list_head clk_list_head;

	bool wlun_dev_clr_ua;

	/* Number of requests aborts */
	int req_abort_count;

	/* Number of lanes available (1 or 2) for Rx/Tx */
	u32 lanes_per_direction;
	struct ufs_pa_layer_attr pwr_info;
	struct ufs_pwr_mode_info max_pwr_info;

	struct ufs_clk_gating clk_gating;
/* Control to enable/disable host capabilities */
#ifdef CONFIG_SCSI_UFS_HI1861_VCMD
	struct ufs_hi1861_fsr ufs_fsr;
	bool is_hi186x_query_vcmd;
#endif
	struct ufs_temp ufs_temp;
	struct ufs_inline_state inline_state;

#if defined(CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO) && defined(CONFIG_DFX_DEBUG_FS)
	struct ufs_inline_debug inline_debug_state;
	u32 inline_debug_flag;
	struct ufs_inline_debug inline_dun_cci_test;
#endif
	/* Control to enable/disable host capabilities */
	u32 caps;

	/* Disable ufs runtime suspend resume */
#define DISABLE_UFS_PMRUNTIME (1 << 12)

	/* HPB property three way */
#define UFSHCD_CAP_HPB_HOST_CONTROL_HYNIX (1 << 22)
#define UFSHCD_CAP_HPB_HOST_CONTROL (1 << 23)
#define UFSHCD_CAP_HPB_DEVICE_CONTROL (1 << 24)

	/* UFS Turbo Talbe v1 */
#define UFSHCD_CAP_HPB_TURBO_TABLE (1 << 25)
	/*
	 * This capability allows the host to enable write booster.
	 * This just was a host capabilities, we need to detect if device'
	 * provision was configured with a valid buffer size.
	 */
#define UFSHCD_CAP_WRITE_BOOSTER (1 << 26)
	/*
	 * Hufs specific intr used to detect no packet transfed in phy layer.
	 */
#define UFSHCD_CAP_BROKEN_IDLE_INTR (1 << 27)
	/*
	 * PWM Daemon Intr: auto hibernate fail or line reset may cause ufs down
	 * to pwm mode without notifing software. This interrupt acts as a daemon
	 * thread watching UFS be in pwm or not.
	 */
#define UFSHCD_CAP_PWM_DAEMON_INTR (1 << 28)
	/*
	 * Device  Timeout Intr: Doorbell is not zero but nothing transfered in
	 * low level physical layer. Used for fast error handler.
	 */
#define UFSHCD_CAP_DEV_TMT_INTR (1 << 29)
	/*
	 * SSU needs to send directly to ufs to avoid accessing blk layer,
	 * blk layer can freeze blk queue during SR,
	 * which block SSU sending to UFS layer
	 */
#define UFSHCD_CAP_SSU_BY_SELF (1 << 30)
	/*
	 * Allow auto hiberb8, this is not just a switch of auto hibern8,
	 * it exists with a hufs specific implement of debuging and
	 * recovering solution.
	 */
#define UFSHCD_CAP_AUTO_HIBERN8 (1UL << 31)

	unsigned int autoh8_disable_depth;

	/* write booster */
	struct work_struct wb_work;
	struct delayed_work wb_flush_work;
	bool wb_work_sched;
	bool wb_exception_enabled;
	bool wb_flush_enabled;
	bool flush_is_open;
	enum wb_type wb_type;
	u32 wb_shared_alloc_units;
	u32 d_ext_ufs_feature_sup;
	bool wb_permanent_disabled;
	bool is_samsung_v5;
	struct list_head wb_flush_v5_whitelist;

	/* turbo table */
	bool ufstt_support;		/* device capability */
	bool ufstt_enabled;		/* turbo table workable */

	/*
	 * Some device has a attribute dSupportedVendorFeatureOptions which
	 * indicates that query command to some special field needs to force set
	 * selector to one. Operations to access dSupportedVendorFeatureOptions
	 * itself shall request with selector = 00h or 01h in Query Request.
	 */
#define WB_VENDOR_FEATURE_SUPPORT BIT(1)
	bool force_selector;

	unsigned int dev_tmt_disable_depth;

	bool is_hibernate;

#ifdef CONFIG_DEBUG_FS
	struct debugfs_files debugfs_files;
#endif
	struct devfreq *devfreq;
	struct ufs_clk_scaling clk_scaling;
	bool is_sys_suspended;

	enum bkops_status urgent_bkops_lvl;
	bool is_urgent_bkops_lvl_checked;
	int latency_hist_enabled;

	struct ufs_unique_number unique_number;
	struct rw_semaphore clk_scaling_lock;
	unsigned char desc_size[QUERY_DESC_IDN_MAX];
	atomic_t scsi_block_reqs_cnt;

	struct device		bsg_dev;
	struct request_queue	*bsg_queue;
	bool wb_buf_flush_enabled;
	bool wb_enabled;
	struct delayed_work rpm_dev_flush_recheck_work;

	struct ufs_hba_monitor	monitor;

#ifdef CONFIG_SCSI_UFS_CRYPTO
	union ufs_crypto_capabilities crypto_capabilities;
	union ufs_crypto_cap_entry *crypto_cap_array;
	u32 crypto_cfg_register;
	struct blk_keyslot_manager ksm;
#endif
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_root;
#endif

	uint16_t manufacturer_id;
	uint16_t manufacturer_date;
	char model[MAX_MODEL_LEN + 1];

	uint16_t ufs_device_spec_version;
	/* Bitmask for enabling debug prints */
	u32 ufshcd_dbg_print;

	struct dentry *hba_addr;

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	int hs_single_lane;
	int use_pwm_mode;
	struct wakeup_source *recover_wake_lock;
	int disable_suspend;
	int check_pwm_after_h8;
	int v_rx;
	int v_tx;
	int init_retry;
#endif
#ifdef CONFIG_SCSI_HUFS_LINERESET_CHECK
	bool bg_task_enable;
	struct task_struct *background_task;
	u32 reg_uecpa;
#endif

#ifdef CONFIG_DEVICE_HEALTH_INFO
	u8 device_health_info[DESC_HEALTH_INFO_SIZE];
#endif

#ifdef CONFIG_HUFS_MANUAL_BKOPS
	bool ufs_bkops_enabled;
	struct ufs_dev_bkops_ops *ufs_dev_bkops_ops;
	struct mas_bkops *ufs_bkops;
	struct list_head bkops_whitelist;
	struct list_head bkops_blacklist;
#endif
	int reset_retry_max;

	uint32_t last_intr;
	ktime_t last_intr_time_stamp;
	uint32_t last_vs_intr;
	ktime_t last_vs_intr_time_stamp;
	uint32_t last_unipro_intr;
	ktime_t last_unipro_intr_time_stamp;
	uint32_t last_fatal_intr;
	ktime_t last_fatal_intr_time_stamp;
	uint32_t last_core;
	uint32_t last_core_intr;
	ktime_t last_core_intr_time_stamp;
	int is_hs_gear4_dev;

	bool ufs_idle_intr_en;
#define UFSHCD_IDLE_TIMEOUT_DEFAULT_VAL	5000
	u32 idle_timeout_val; /* in us */
	int timer_irq;

	struct timer_list idle_intr_check_timer;
	struct timer_list sync_one_region_timer;
#define UFSHCD_IDLE_INTR_CHECK_INTERVAL	3600000
	unsigned int idle_intr_check_timer_threshold; /* in ms */
#ifdef CONFIG_DFX_DEBUG_FS
	bool ufs_idle_intr_verify;
#endif
	uint8_t tz_version;

	struct mutex eh_mutex;
	bool ufs_in_err_handle;
	int ufs_init_retries;
	int ufs_reset_retries;
	bool ufs_init_completed;
	bool invalid_attr_print;

	struct ufs_hpb_info *ufs_hpb;
	struct ufs_tt_info *ufs_tt;
	struct work_struct hpb_update_work;
#ifdef	HPB_HUFS_DEBUG_PRINT
	u64 hit_nd_cnt;
	u64 rd_cnt;
	u64 delete_node_cnt;
	u64 add_node_cnt;
	u64 read_io_time;
	u64 rd_find_node_time;
	u64 read_cnt;
	u64 write_io_time;
	u64 wr_find_node_time;
	u64 write_cnt;
#endif

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
};

/*
 * Compared to the upstream equivalent, @hpb_dev has been moved from struct
 * ufs_hba into struct ufs_hba_with_hpb to satisfy the Android ABI checks.
 */
struct ufs_hba_with_hpb {
	struct ufs_hba hba;
#ifdef CONFIG_SCSI_UFS_HPB
	struct ufshpb_dev_info hpb_dev;
#endif
};

#ifdef CONFIG_SCSI_UFS_SYNCCACHE_RECORD
enum ufs_dcmd_scenario {
	UFS_SC_POWERDOWN = 1,
	UFS_SC_EMERGENCY,
};
#endif

/* Returns true if clocks can be gated. Otherwise false */
static inline bool ufshcd_is_clkgating_allowed(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_CLK_GATING;
}
static inline bool ufshcd_can_hibern8_during_gating(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_HIBERN8_WITH_CLK_GATING;
}
static inline int ufshcd_is_clkscaling_supported(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_CLK_SCALING;
}

static inline bool ufshcd_can_autobkops_during_suspend(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_AUTO_BKOPS_SUSPEND;
}

static inline bool ufshcd_is_rpm_autosuspend_allowed(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_RPM_AUTOSUSPEND;
}

static inline bool ufshcd_is_intr_aggr_allowed(struct ufs_hba *hba)
{
/* DWC UFS Core has the Interrupt aggregation feature but is not detectable*/
#ifndef CONFIG_SCSI_UFS_DWC
	if ((hba->caps & UFSHCD_CAP_INTR_AGGR) &&
	    !(hba->quirks & UFSHCD_QUIRK_BROKEN_INTR_AGGR))
		return true;
	else
		return false;
#else
return true;
#endif
}

static inline bool ufshcd_can_aggressive_pc(struct ufs_hba *hba)
{
	return !!(ufshcd_is_link_hibern8(hba) &&
		  (hba->caps & UFSHCD_CAP_AGGR_POWER_COLLAPSE));
}

static inline bool ufshcd_is_auto_hibern8_supported(struct ufs_hba *hba)
{
	return (hba->capabilities & MASK_AUTO_HIBERN8_SUPPORT) &&
		!(hba->quirks & UFSHCD_QUIRK_BROKEN_AUTO_HIBERN8);
}

static inline bool ufshcd_is_auto_hibern8_enabled(struct ufs_hba *hba)
{
	return FIELD_GET(UFSHCI_AHIBERN8_TIMER_MASK, hba->ahit) ? true : false;
}

static inline bool ufshcd_is_auto_hibern8_allowed(struct ufs_hba *hba)
{
	if (hba->caps & UFSHCD_CAP_AUTO_HIBERN8)
		return true;
	else
		return false;
}

static inline bool ufshcd_is_wb_allowed(struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_WB_EN;
}

static inline bool ufshcd_is_user_access_allowed(struct ufs_hba *hba)
{
	return !hba->shutting_down;
}

#define ufshcd_writel(hba, val, reg)	\
	writel((val), (hba)->mmio_base + (reg))
#define ufshcd_readl(hba, reg)	\
	(u32)readl((hba)->mmio_base + (reg))

/**
 * ufshcd_rmwl - read modify write into a register
 * @hba - per adapter instance
 * @mask - mask to apply on read value
 * @val - actual value to write
 * @reg - register address
 */
static inline void ufshcd_rmwl(struct ufs_hba *hba, u32 mask, u32 val, u32 reg)
{
	u32 tmp;

	tmp = ufshcd_readl(hba, reg);
	tmp &= ~mask;
	tmp |= (val & mask);
	ufshcd_writel(hba, tmp, reg);
}

int ufshcd_alloc_host(struct device *, struct ufs_hba **);
void ufshcd_dealloc_host(struct ufs_hba *);
int ufshcd_hba_enable(struct ufs_hba *hba);
#ifdef CONFIG_SCSI_HUFS
int ufshcd_init(struct ufs_hba *hba, void __iomem *mmio_base, unsigned int irq, int timer_irq);
#else
int ufshcd_init(struct ufs_hba *hba, void __iomem *mmio_base, unsigned int irq);
#endif
int ufshcd_link_recovery(struct ufs_hba *hba);
int ufshcd_make_hba_operational(struct ufs_hba *hba);
void ufshcd_remove(struct ufs_hba *);
void ufshcd_delay_us(unsigned long us, unsigned long tolerance);
int ufshcd_wait_for_register(struct ufs_hba *hba, u32 reg, u32 mask,
				u32 val, unsigned long interval_us,
				unsigned long timeout_ms);
void ufshcd_parse_dev_ref_clk_freq(struct ufs_hba *hba, struct clk *refclk);
void ufshcd_update_evt_hist(struct ufs_hba *hba, u32 id, u32 val);
void ufshcd_hba_stop(struct ufs_hba *hba);

static inline void check_upiu_size(void)
{
	BUILD_BUG_ON(ALIGNED_UPIU_SIZE <
		GENERAL_UPIU_REQUEST_SIZE + QUERY_DESC_MAX_SIZE);
}

/**
 * ufshcd_set_variant - set variant specific data to the hba
 * @hba - per adapter instance
 * @variant - pointer to variant specific data
 */
static inline void ufshcd_set_variant(struct ufs_hba *hba, void *variant)
{
	BUG_ON(!hba);
	hba->priv = variant;
}

/**
 * ufshcd_get_variant - get variant specific data from the hba
 * @hba - per adapter instance
 */
static inline void *ufshcd_get_variant(struct ufs_hba *hba)
{
	BUG_ON(!hba);
	return hba->priv;
}
static inline bool ufshcd_keep_autobkops_enabled_except_suspend(
							struct ufs_hba *hba)
{
	return hba->caps & UFSHCD_CAP_KEEP_AUTO_BKOPS_ENABLED_EXCEPT_SUSPEND;
}

static inline u8 ufshcd_wb_get_query_index(struct ufs_hba *hba)
{
	if (hba->dev_info.b_wb_buffer_type == WB_BUF_MODE_LU_DEDICATED)
		return hba->dev_info.wb_dedicated_lu;
	return 0;
}

extern int ufshcd_runtime_suspend(struct ufs_hba *hba);
extern int ufshcd_runtime_resume(struct ufs_hba *hba);
extern int ufshcd_runtime_idle(struct ufs_hba *hba);
extern int ufshcd_system_suspend(struct ufs_hba *hba);
extern int ufshcd_system_resume(struct ufs_hba *hba);
extern int ufshcd_shutdown(struct ufs_hba *hba);
extern int ufshcd_dme_set_attr(struct ufs_hba *hba, u32 attr_sel,
			       u8 attr_set, u32 mib_val, u8 peer);
extern int ufshcd_dme_get_attr(struct ufs_hba *hba, u32 attr_sel,
			       u32 *mib_val, u8 peer);
extern int ufshcd_exec_dev_cmd(
	struct ufs_hba *hba, enum dev_cmd_type cmd_type, int timeout);
extern void ufshcd_init_query(struct ufs_hba *hba,
	struct ufs_query_req **request, struct ufs_query_res **response,
	enum query_opcode opcode, u8 idn, u8 index, u8 selector);
extern void __ufshcd_transfer_req_compl(struct ufs_hba *hba,
					unsigned long _completed_reqs);
extern int ufshcd_config_pwr_mode(struct ufs_hba *hba,
			struct ufs_pa_layer_attr *desired_pwr_mode);

/* UIC command interfaces for DME primitives */
#define DME_LOCAL	0
#define DME_PEER	1
#define ATTR_SET_NOR	0	/* NORMAL */
#define ATTR_SET_ST	1	/* STATIC */

static inline int ufshcd_dme_set(struct ufs_hba *hba, u32 attr_sel,
				 u32 mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_write_reg) {
		attr_sel =
			hba->vops->snps_to_hufs_addr(UIC_CMD_DME_SET, attr_sel);
		return hba->vops->hufs_uic_write_reg(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_set_attr(hba, attr_sel, ATTR_SET_NOR,
				   mib_val, DME_LOCAL);
}

static inline int ufshcd_dme_st_set(struct ufs_hba *hba, u32 attr_sel,
				    u32 mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_write_reg) {
		attr_sel =
			hba->vops->snps_to_hufs_addr(UIC_CMD_DME_SET, attr_sel);
		return hba->vops->hufs_uic_write_reg(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_set_attr(hba, attr_sel, ATTR_SET_ST,
				   mib_val, DME_LOCAL);
}

static inline int ufshcd_dme_peer_set(struct ufs_hba *hba, u32 attr_sel,
				      u32 mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_peer_set) {
		attr_sel = hba->vops->snps_to_hufs_addr(
			UIC_CMD_DME_PEER_SET, attr_sel);
		return hba->vops->hufs_uic_peer_set(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_set_attr(hba, attr_sel, ATTR_SET_NOR,
				   mib_val, DME_PEER);
}

static inline int ufshcd_dme_peer_st_set(struct ufs_hba *hba, u32 attr_sel,
					 u32 mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_peer_set) {
		attr_sel = hba->vops->snps_to_hufs_addr(
			UIC_CMD_DME_PEER_SET, attr_sel);
		return hba->vops->hufs_uic_peer_set(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_set_attr(hba, attr_sel, ATTR_SET_ST,
				   mib_val, DME_PEER);
}

static inline int ufshcd_dme_get(struct ufs_hba *hba,
				 u32 attr_sel, u32 *mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_read_reg) {
		attr_sel =
			hba->vops->snps_to_hufs_addr(UIC_CMD_DME_GET, attr_sel);
		return hba->vops->hufs_uic_read_reg(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_get_attr(hba, attr_sel, mib_val, DME_LOCAL);
}

static inline int ufshcd_dme_peer_get(struct ufs_hba *hba,
				      u32 attr_sel, u32 *mib_val)
{
	if (ufshcd_is_hufs_hc(hba) && hba->vops &&
		hba->vops->hufs_uic_peer_get) {
		attr_sel = hba->vops->snps_to_hufs_addr(
			UIC_CMD_DME_PEER_GET, attr_sel);
		return hba->vops->hufs_uic_peer_get(hba, attr_sel, mib_val);
	}
	return ufshcd_dme_get_attr(hba, attr_sel, mib_val, DME_PEER);
}

int ufshcd_read_device_desc(struct ufs_hba *hba, u8 *buf, u32 size);
int ufshcd_enable_ee(struct ufs_hba *hba, u16 mask);

static inline bool ufshcd_is_hs_mode(struct ufs_pa_layer_attr *pwr_info)
{
	return (pwr_info->pwr_rx == FAST_MODE ||
		pwr_info->pwr_rx == FASTAUTO_MODE) &&
		(pwr_info->pwr_tx == FAST_MODE ||
		pwr_info->pwr_tx == FASTAUTO_MODE);
}

#ifdef CONFIG_DEBUG_FS
static inline void ufshcd_init_req_stats(struct ufs_hba *hba)
{
	memset(hba->ufs_stats.req_stats, 0, sizeof(hba->ufs_stats.req_stats));
}
#endif

int ufshcd_read_device_desc(struct ufs_hba *hba, u8 *buf, u32 size);
int ufshcd_read_geometry_desc(struct ufs_hba *hba, u8 *buf, u32 size);
int ufshcd_read_device_health_desc(struct ufs_hba *hba,
					u8 *buf, u32 size);

#define ASCII_STD true
#define UTF16_STD false

int ufshcd_hold(struct ufs_hba *hba, bool async);
void ufshcd_release(struct ufs_hba *hba);

u32 ufshcd_get_local_unipro_ver(struct ufs_hba *hba);

static inline void ufshcd_vops_full_reset(struct ufs_hba *hba)
{
	if (hba && hba->vops && hba->vops->full_reset)
		hba->vops->full_reset(hba);
}

static inline int ufshcd_disable_host_tx_lcc(struct ufs_hba *hba)
{
	return ufshcd_dme_set(hba, UIC_ARG_MIB(PA_LOCAL_TX_LCC_ENABLE), 0);
}

/* Expose Query-Request API */
int ufshcd_query_descriptor_retry(struct ufs_hba *hba,
				  enum query_opcode opcode,
				  enum desc_idn idn, u8 index,
				  u8 selector,
				  u8 *desc_buf, int *buf_len);
int ufshcd_read_desc_param(struct ufs_hba *hba,
			   enum desc_idn desc_id,
			   int desc_index,
			   u32 param_offset,
			   u8 *param_read_buf,
			   u32 param_size);
int ufshcd_query_attr(struct ufs_hba *hba, enum query_opcode opcode,
		      enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);
int ufshcd_query_attr_safe(struct ufs_hba *hba, enum query_opcode opcode,
	enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);
int ufshcd_query_descriptor_retry(struct ufs_hba *hba,
			enum query_opcode opcode, enum desc_idn idn, u8 index,
			u8 selector, u8 *desc_buf, int *buf_len);

int ufshcd_change_power_mode(struct ufs_hba *hba,
			     struct ufs_pa_layer_attr *pwr_mode);
int ufshcd_wait_for_doorbell_clr(struct ufs_hba *hba, u64 wait_timeout_us);
void ufshcd_enable_intr(struct ufs_hba *hba, u32 intrs);
#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
int ufshcd_keyregs_remap_wc(struct ufs_hba *hba, resource_size_t hci_reg_base);
#endif
#endif
void ufshcd_disable_run_stop_reg(struct ufs_hba *hba);
void ufshcd_init_pwr_info(struct ufs_hba *hba);
int wait_for_ufs_all_complete(struct ufs_hba *hba, int timeout_ms);
irqreturn_t ufshcd_intr(int irq, void *__hba);

int ufshcd_query_attr_retry(struct ufs_hba *hba,
	enum query_opcode opcode, enum attr_idn idn, u8 index, u8 selector,
	u32 *attr_val);
int ufshcd_query_flag(struct ufs_hba *hba, enum query_opcode opcode,
	enum flag_idn idn, u8 index, bool *flag_res);
int ufshcd_query_flag_retry(struct ufs_hba *hba,
	enum query_opcode opcode, enum flag_idn idn, u8 index, bool *flag_res);
int ufshcd_bkops_ctrl(struct ufs_hba *hba, enum bkops_status status);

void ufshcd_auto_hibern8_enable(struct ufs_hba *hba);
void ufshcd_auto_hibern8_update(struct ufs_hba *hba, u32 ahit);
void ufshcd_fixup_dev_quirks(struct ufs_hba *hba, struct ufs_dev_fix *fixups);
#define SD_ASCII_STD true
#define SD_RAW false
int ufshcd_read_string_desc(struct ufs_hba *hba, u8 desc_index,
			    u8 **buf, bool ascii);

int ufshcd_hold(struct ufs_hba *hba, bool async);
void ufshcd_release(struct ufs_hba *hba);

void ufshcd_map_desc_id_to_length(struct ufs_hba *hba, enum desc_idn desc_id,
				  int *desc_length);

u32 ufshcd_get_local_unipro_ver(struct ufs_hba *hba);

int ufshcd_send_uic_cmd(struct ufs_hba *hba, struct uic_command *uic_cmd);

int ufshcd_exec_raw_upiu_cmd(struct ufs_hba *hba,
			     struct utp_upiu_req *req_upiu,
			     struct utp_upiu_req *rsp_upiu,
			     int msgcode,
			     u8 *desc_buff, int *buff_len,
			     enum query_opcode desc_op);

/* Wrapper functions for safely calling variant operations */
static inline const char *ufshcd_get_var_name(struct ufs_hba *hba)
{
	if (hba->vops)
		return hba->vops->name;
	return "";
}

static inline int ufshcd_vops_init(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->init)
		return hba->vops->init(hba);

	return 0;
}

static inline void ufshcd_vops_exit(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->exit)
		return hba->vops->exit(hba);
}

static inline u32 ufshcd_vops_get_ufs_hci_version(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->get_ufs_hci_version)
		return hba->vops->get_ufs_hci_version(hba);

	return ufshcd_readl(hba, REG_UFS_VERSION);
}

static inline bool ufshcd_has_utrlcnr(struct ufs_hba *hba)
{
	return (hba->ufs_version >= ufshci_version(3, 0));
}

static inline int ufshcd_vops_clk_scale_notify(struct ufs_hba *hba,
			bool up, enum ufs_notify_change_status status)
{
	if (hba->vops && hba->vops->clk_scale_notify)
		return hba->vops->clk_scale_notify(hba, up, status);
	return 0;
}

static inline void ufshcd_vops_event_notify(struct ufs_hba *hba,
					    enum ufs_event_type evt,
					    void *data)
{
	if (hba->vops && hba->vops->event_notify)
		hba->vops->event_notify(hba, evt, data);
}

static inline int ufshcd_vops_setup_clocks(struct ufs_hba *hba, bool on,
					enum ufs_notify_change_status status)
{
	if (hba->vops && hba->vops->setup_clocks)
		return hba->vops->setup_clocks(hba, on, status);
	return 0;
}

static inline int ufshcd_vops_setup_regulators(struct ufs_hba *hba, bool status)
{
	if (hba->vops && hba->vops->setup_regulators)
		return hba->vops->setup_regulators(hba, status);

	return 0;
}

static inline int ufshcd_vops_hce_enable_notify(struct ufs_hba *hba,
						bool status)
{
	if (hba->vops && hba->vops->hce_enable_notify)
		return hba->vops->hce_enable_notify(hba, status);

	return 0;
}
static inline int ufshcd_vops_link_startup_notify(struct ufs_hba *hba,
						bool status)
{
	if (hba->vops && hba->vops->link_startup_notify)
		return hba->vops->link_startup_notify(hba, status);

	return 0;
}

static inline int ufshcd_vops_pwr_change_notify(struct ufs_hba *hba,
				  bool status,
				  struct ufs_pa_layer_attr *dev_max_params,
				  struct ufs_pa_layer_attr *dev_req_params)
{
	if (hba->vops && hba->vops->pwr_change_notify)
		return hba->vops->pwr_change_notify(hba, status,
					dev_max_params, dev_req_params);

	return -ENOTSUPP;
}

static inline void ufshcd_vops_setup_xfer_req(struct ufs_hba *hba, int tag,
					bool is_scsi_cmd)
{
	if (hba->vops && hba->vops->setup_xfer_req) {
		unsigned long flags;

		spin_lock_irqsave(hba->host->host_lock, flags);
		hba->vops->setup_xfer_req(hba, tag, is_scsi_cmd);
		spin_unlock_irqrestore(hba->host->host_lock, flags);
	}
}

static inline void ufshcd_vops_setup_task_mgmt(struct ufs_hba *hba,
					int tag, u8 tm_function)
{
	if (hba->vops && hba->vops->setup_task_mgmt)
		return hba->vops->setup_task_mgmt(hba, tag, tm_function);
}

static inline void ufshcd_vops_hibern8_notify(struct ufs_hba *hba,
					enum uic_cmd_dme cmd,
					enum ufs_notify_change_status status)
{
	if (hba->vops && hba->vops->hibern8_notify)
		return hba->vops->hibern8_notify(hba, cmd, status);
}

static inline int ufshcd_vops_apply_dev_quirks(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->apply_dev_quirks)
		return hba->vops->apply_dev_quirks(hba);
	return 0;
}

static inline void ufshcd_vops_fixup_dev_quirks(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->fixup_dev_quirks)
		hba->vops->fixup_dev_quirks(hba);
}

static inline int ufshcd_vops_suspend(struct ufs_hba *hba, enum ufs_pm_op op)
{
	if (hba->vops && hba->vops->suspend)
		return hba->vops->suspend(hba, op);

	return 0;
}

static inline int ufshcd_vops_resume(struct ufs_hba *hba, enum ufs_pm_op op)
{
	if (hba->vops && hba->vops->resume)
		return hba->vops->resume(hba, op);

	return 0;
}


static inline void ufshcd_vops_set_device_refclk(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->set_ref_clk)
		hba->vops->set_ref_clk(hba);
}

static inline int ufshcd_vops_suspend_before_set_link_state(struct ufs_hba *hba,
	enum ufs_pm_op op)
{
	if (hba->vops && hba->vops->suspend_before_set_link_state)
		return hba->vops->suspend_before_set_link_state(hba, op);

	return 0;
}

static inline void ufshcd_fix_mphy_adapt(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->fix_mphy_adapt)
		hba->vops->fix_mphy_adapt(hba);
}

int ufshcd_uic_change_pwr_mode(struct ufs_hba *hba, u8 mode);
int ufshcd_read_unit_desc_param(struct ufs_hba *hba, int lun,
	enum unit_desc_param param_offset, u8 *param_read_buf, u32 param_size);

int ufshcd_send_vendor_scsi_cmd(struct ufs_hba *hba,
		struct scsi_device *sdp, unsigned char* cdb, void* buf);
void ufshcd_set_auto_hibern8_delay(struct ufs_hba *hba, unsigned int value);
int ufshcd_query_attr_retry(struct ufs_hba *hba, enum query_opcode opcode,
			    enum attr_idn idn, u8 index, u8 selector,
			    u32 *attr_val);

static inline void ufshcd_vops_vcc_power_on_off(struct ufs_hba *hba)
{
	if (hba && hba->vops && hba->vops->vcc_power_on_off)
		hba->vops->vcc_power_on_off(hba);
}

static inline void ufshcd_vops_dbg_register_dump(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->dbg_register_dump)
		hba->vops->dbg_register_dump(hba);
}

static inline void ufshcd_vops_device_reset(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->device_reset) {
#ifdef CONFIG_SCSI_HUFS
		hba->vops->device_reset(hba);
		hba->wb_enabled = false;
#else
		int err = hba->vops->device_reset(hba);

		if (!err) {
			ufshcd_set_ufs_dev_active(hba);
			if (ufshcd_is_wb_allowed(hba)) {
				hba->wb_enabled = false;
				hba->wb_buf_flush_enabled = false;
			}
		}
		if (err != -EOPNOTSUPP)
			ufshcd_update_evt_hist(hba, UFS_EVT_DEV_RESET, err);
#endif
	}
}

static inline void ufshcd_vops_config_scaling_param(struct ufs_hba *hba,
						    struct devfreq_dev_profile
						    *profile, void *data)
{
	if (hba->vops && hba->vops->config_scaling_param)
		hba->vops->config_scaling_param(hba, profile, data);
}

extern struct ufs_pm_lvl_states ufs_pm_lvl_states[];

/*
 * ufshcd_scsi_to_upiu_lun - maps scsi LUN to UPIU LUN
 * @scsi_lun: scsi LUN id
 *
 * Returns UPIU LUN id
 */
static inline u8 ufshcd_scsi_to_upiu_lun(unsigned int scsi_lun)
{
	if (scsi_is_wlun(scsi_lun))
		return (scsi_lun & UFS_UPIU_MAX_UNIT_NUM_ID)
			| UFS_UPIU_WLUN_ID;
	else
		return scsi_lun & UFS_UPIU_MAX_UNIT_NUM_ID;
}

int ufshcd_dump_regs(struct ufs_hba *hba, size_t offset, size_t len,
		     const char *prefix);
int ufshcd_uic_hibern8_enter(struct ufs_hba *hba);
int ufshcd_uic_hibern8_exit(struct ufs_hba *hba);

#endif /* End of Header */
