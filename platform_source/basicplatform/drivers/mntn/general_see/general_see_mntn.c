/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: implement general see mntn function
 * Create: 2020-02-17
 */
#include "general_see_mntn.h"
#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <rpmb.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/kthread.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <securec.h>
#ifdef CONFIG_CRYPTO_CORE
#include <crypto_core_internal.h>
#include <crypto_core_fs.h>
#include <crypto_core_errno.h>
#include <crypto_core_dcs.h>
#include <crypto_core_upgrade.h>
#endif
#include "../mntn_filesys.h"
#ifdef CONFIG_GENERAL_SEE
#include <general_see.h>
#endif
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#ifdef CONFIG_GENERAL_SEE_SUPPORT_DCS
#include <general_see_dcs.h>
#endif
#include <mntn_subtype_exception.h>
#include <mntn_subtype_exception_general_see.h>
#include <linux/syscalls.h>

#define HISEE_MNTN_LPM3_STR    "lpm3"
#define HISEE_MNTN_ATF_STR     "atf"

#ifdef CONFIG_CRYPTO_CORE
#define HISEE_OK                        MSPC_OK
#define HISEE_ERROR                     MSPC_ERROR
#define HISEE_SECLIB_ERROR              MSPC_SECLIB_ERROR

/* below defines follow defines in rom */
#define HISEE_ROM_EXC_HEAD_VALUE        0xA0000000
#define HISEE_ROM_EXC_HEAD_MASK         0xF0000000
#define HISEE_EXC_TYPE_ID               1

enum {
	MSPC_EXCEPTION_RST = 0,
	MSPC_EXCEPTION_NORST = 1
};

struct mspc_diaginfo {
	u32 exception_type;
	u32 mod_id;
	u32 fi;
	u32 fv;
	u32 info0;
	u32 info1;
};

static int mspc_record_mspc_diaginfo(const struct mspc_diaginfo *diaginfo);
#endif

#define HISEE_MNTN_IPC_DATA0    0
#define HISEE_MNTN_IPC_DATA1    1
#define HISEE_MNTN_IPC_DATA2    2
#define HISEE_MNTN_IPC_DATA3    3
#define HISEE_MNTN_IPC_DATA4    4
#define HISEE_MNTN_IPC_DATA5    5
#define HISEE_MNTN_IPC_DATA6    6

static struct rdr_exception_info_s g_hisee_excetption_info[] = GENERAL_SEE_EXCEPTIONE_INFO_MAP;

enum hisee_int_id {
	HISEE_OTPC_INT_ID = 0,
	HISEE_TRNG_INT_ID,
	HISEE_AS2AP_INT_ID,
	HISEE_DS2AP_INT_ID,
	HISEE_SENC2AP_INT_ID,
	HISEE_SENC2AP0_INT_ID,
	HISEE_SENC2AP1_INT_ID,
	HISEE_TSENSOR0_INT_ID,
	HISEE_TSENSOR1_INT_ID,
	HISEE_LOCKUP_INT_ID,
	HISEE_RST_INT_ID,
	HISEE_EH2H_SLV_INT_ID,

	HISEE_INVALID_INT_ID = 0xFF,
};

/* for translation from original irq no to exception type that module id */
static struct hisee_exc_trans_s g_hisee_exc_trans[] = {
	{ HISEE_OTPC_INT_ID, MODID_GENERAL_SEE_EXC_ALARM0 },
	{ HISEE_TRNG_INT_ID, MODID_GENERAL_SEE_EXC_ALARM1 },
	{ HISEE_AS2AP_INT_ID, MODID_GENERAL_SEE_EXC_AS2AP_IRQ },
	{ HISEE_DS2AP_INT_ID, MODID_GENERAL_SEE_EXC_DS2AP_IRQ },
	{ HISEE_SENC2AP_INT_ID, MODID_GENERAL_SEE_EXC_SENC2AP_IRQ },
	{ HISEE_SENC2AP0_INT_ID, MODID_GENERAL_SEE_EXC_SENC2AP_IRQ0 },
	{ HISEE_SENC2AP1_INT_ID, MODID_GENERAL_SEE_EXC_SENC2AP_IRQ1 },
	{ HISEE_LOCKUP_INT_ID, MODID_GENERAL_SEE_EXC_LOCKUP },
	{ HISEE_EH2H_SLV_INT_ID, MODID_GENERAL_SEE_EXC_EH2H_SLV },
	{ HISEE_TSENSOR1_INT_ID, MODID_GENERAL_SEE_EXC_TSENSOR1 },
#if defined(CONFIG_GENERAL_SEE_MNTN_RESET_IRQ_SEPARATE) || defined(CONFIG_CRYPTO_CORE_MNTN_RESET_IRQ_SEPARATE)
	{ HISEE_RST_INT_ID, MODID_GENERAL_SEE_EXC_RST },
#endif
	/* Please add your new member above!!!! */
};

#ifdef CONFIG_CRYPTO_CORE
static struct mspc_ufs_flag_info g_curr_ver_mntn = {0};
#else
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
static struct hisee_partition_version_info g_curr_ver_mntn = {0};
#else
static struct cosimage_version_info g_curr_ver_mntn = {0};
#endif
static struct cosimage_version_info g_misc_version_mntn = {0};
#endif
static unsigned int g_hisee_lcs_mode_mntn;

static u32 g_hisee_exception_modid;
static u32 g_log_out_offset;
static u32 g_vote_val_lpm3;
static u32 g_vote_val_atf;
#ifdef CONFIG_CRYPTO_CORE
static u64 g_vote_val_tee;
#endif

/* flag that make sure only run this function after last running is over */
static int g_need_run_flag = HISEE_OK;
static int g_rpmb_status_flag = HISEE_OK; /* whether rpmb is ok: 0->ok; !0->ko */
static struct notifier_block g_hisee_ipc_block;
static struct rdr_register_module_result g_hisee_info;
static struct general_see_logbuf_info g_logbuf_info;
static void *g_hisee_mntn_addr;
static dma_addr_t g_hisee_log_phy;
static struct task_struct *g_hisee_mntn_thread;
static struct task_struct *g_hisee_mntn_print_verinfo;
static struct task_struct *g_hisee_mntn_save_print_log_task;
static DECLARE_COMPLETION(g_hisee_mntn_complete);
static DECLARE_COMPLETION(g_hisee_pwrdebug_complete);
static DECLARE_COMPLETION(g_hisee_mntn_save_print_log_complete);
static struct ipc_msg g_msg;
static enum hisee_mntn_state g_hisee_mntn_state = HISEE_STATE_INVALID;

#define GENERAL_SEE_LOGBUF_HEADER_LEN sizeof(struct general_see_logbuf_header)

#ifdef CONFIG_HUAWEI_DSM
static struct dsm_dev g_dsm_hisee = {
	.name = "dsm_hisee",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = HISEE_DMD_BUFF_SIZE,
};

static struct dsm_client *g_hisee_dclient;
#endif

/*
 * @brief      : Only for hisee, to call directly to record an exception
 * @param[in]  : data: data,data to save
 */
void rdr_hisee_call_to_record_exc(int data)
{
	g_hisee_exception_modid = (u32)MODID_SIMULATE_EXC_RPMB_KO;
	g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
	g_rpmb_status_flag = data;
	complete(&g_hisee_mntn_complete);
}

/*
 * @brief      : send msg to lpm3 by rproc
 * @param[in]  : p_data, the message buffer need to send
 * @param[in]  : len, the length of message
 * @return     : 0 on success, other value on failure
 */
static int hisee_mntn_send_msg_to_lpm3(const u32 *p_data, u32 len)
{
	int ret = -1;

	if (!p_data)
		return ret;

	ret = RPROC_ASYNC_SEND(IPC_ACPU_LPM3_MBX_5,
			       (mbox_msg_t *)p_data, (rproc_msg_len_t)len);
	if (ret != 0)
		pr_err("%s:RPROC_ASYNC_SEND failed! return 0x%x, msg:(%x)\n",
		       __func__, ret, *p_data);
	return ret;
}

/*
 * @brief      : translate the irq number to the exception type defined
 *               by kernel, so kernel can know what exception it is
 * @param[in]  : int_id, interrupt number id
 * @return     : exception type
 */
static unsigned int translate_exc_type(u32 int_id)
{
	u32 i;
	u32 module_id = (u32)MODID_GENERAL_SEE_EXC_UNKNOWN;

	for (i = 0; i < ARRAY_SIZE(g_hisee_exc_trans); i++)
		if (int_id == g_hisee_exc_trans[i].int_id_value) {
			module_id = g_hisee_exc_trans[i].module_value;
			break;
		}

	return module_id;
}

/*
 * @brief      : kenrel send msg to ATF
 */
noinline u64 atfd_hisi_service_hisee_mntn_smc(u64 _function_id,
	u64 _arg0,
	u64 _arg1,
	u64 _arg2)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct arm_smccc_res res = {0};
	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
		      0, 0, 0, 0, &res);
	return (u64)res.a0;
#else
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile(__asmeq("%0", "x0")
		     __asmeq("%1", "x1")
		     __asmeq("%2", "x2")
		     __asmeq("%3", "x3")
		     "smc    #0\n"
		     : "+r" (function_id)
		     : "r" (arg0), "r" (arg1), "r" (arg2));

	return (u64)function_id;
#endif
}

/*
 * @brief      : limit hiseelog size
 * @param[in]  : file path
 */
static void limit_hiseelog_size(const char *path)
{
#ifdef CONFIG_CRYPTO_CORE
	int ret;
	struct kstat hiseelog_stat;
	char path_bak[HISEE_MNTN_PATH_MAXLEN] = {0};
	mm_segment_t fs;

	ret = snprintf_s(path_bak, HISEE_MNTN_PATH_MAXLEN,
			 HISEE_MNTN_PATH_MAXLEN - 1,
			 "%s%s", PATH_ROOT, HISEE_PRINTLOG_FLIENAME_BAK);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s[%d] snprintf err.\n", __func__, __LINE__);
		return;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);/*lint !e501 */

	ret = vfs_stat(path, &hiseelog_stat);
	if (ret == 0) {
		if (hiseelog_stat.size > HISEELOG_LIMIT_SIZE) {
			if (vfs_stat(path_bak, &hiseelog_stat) == 0)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
				ksys_unlink(path_bak);
			ksys_rename(path, path_bak);
#else
				sys_unlink(path_bak);
			sys_rename(path, path_bak);
#endif
			ret = mntn_filesys_chown((const char *)path, ROOT_UID, SYSTEM_GID);
			if (ret)
				pr_err("chown %s uid [%u] gid [%u] failed err [%d]!\n",
				       path, ROOT_UID, SYSTEM_GID, ret);
		}
	} else {
		pr_err("%s ret:%d\n", __func__, ret);
	}

	set_fs(fs);
#endif
}

/*
 * @brief      : save hisee print log data in file
 */
void rdr_hisee_save_print_log(void)
{
	u32 tmpsize;
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	char *p_timestr = NULL;
	int ret;

	/* LOG_OUT smc has no valid return value */
	(void)atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID, (u64)HISEE_SMC_LOG_OUT,
					       (u64)g_log_out_offset, (u64)0x0);

	if (!g_logbuf_info.header || g_logbuf_info.header->real_size == 0)
		return;

	ret = snprintf_s(path, HISEE_MNTN_PATH_MAXLEN,
			 HISEE_MNTN_PATH_MAXLEN - 1, "%s", PATH_ROOT);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s(): snprintf1 err.\n", __func__);
		return;
	}
	/* return if there is no free space in hisi_logs */
	tmpsize = (u32)rdr_dir_size(path, HISEE_MNTN_PATH_MAXLEN, (bool)true);
	if (tmpsize > rdr_get_logsize() || tmpsize == 0) {
		pr_err("hisi_logs dir %d unavailable!\n", tmpsize);
		return;
	}

	ret = snprintf_s(path, HISEE_MNTN_PATH_MAXLEN,
			 HISEE_MNTN_PATH_MAXLEN - 1,
			 "%s%s", PATH_ROOT, HISEE_PRINTLOG_FLIENAME);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s(): snprintf2 err.\n", __func__);
		return;
	}

	limit_hiseelog_size(path);

	p_timestr = rdr_get_timestamp();
	/* save current time in hisee_printlog */
	if (p_timestr) {
		ret = mntn_filesys_write_log(path, (void *)(p_timestr),
					     (unsigned int)strlen(p_timestr),
					     HISEE_FILE_PERMISSION);
		if (ret <= 0)
			pr_err("save hisee print log time err, time is %s, ret = %d\n",
			       p_timestr, ret);
	}
	/* save hisee log to data/hisi_logs/time/hisee_printlog */
	ret = mntn_filesys_write_log(path,
				     (void *)((uintptr_t)g_logbuf_info.addr + GENERAL_SEE_LOGBUF_HEADER_LEN),
				     (unsigned int)(g_logbuf_info.header->real_size),
				     HISEE_FILE_PERMISSION);

	ret = mntn_filesys_chown((const char *)path, ROOT_UID, SYSTEM_GID);
	if (ret)
		pr_err("chown %s uid [%u] gid [%u] failed err [%d]!\n",
		       path, ROOT_UID, SYSTEM_GID, ret);
}

/*
 * @brief      : judge the current misc is new or not
 * @return     : ::int, HISEE_OK, new misc; other value: old misc
 */
int rdr_hisee_judge_new_misc(void)
{
	int ret = HISEE_OK;

#ifdef CONFIG_MNTN_CHECKMISC
	/* Only for the phones that has misc but low version(ver <= 0x40) */
	if (g_misc_version_mntn.magic == HISEE_SW_VERSION_MAGIC_VALUE) {
		if (g_misc_version_mntn.img_version_num[0] <= 0x40) {
			pr_err("Old Hisee misc, do nothing !!misc ver is %d\n",
			       g_misc_version_mntn.img_version_num[0]);
			ret = HISEE_ERROR;
		}
	}
#endif
	return ret;
}

/*
 * @brief      : hisee mntn thread
 * @param[in]  : void *arg
 * @return     : ::int, always is 0
 */
#ifdef CONFIG_CRYPTO_CORE
int rdr_hisee_thread(void *arg)
{
	struct mspc_diaginfo diaginfo = {
		.exception_type = (u32)MSPC_EXCEPTION_RST,
	};

	while (!kthread_should_stop()) {
		wait_for_completion(&g_hisee_mntn_complete);
		switch (g_hisee_mntn_state) {
		case HISEE_STATE_HISEE_EXC:
			diaginfo.mod_id = g_hisee_exception_modid;
			diaginfo.fi = g_msg.data[HISEE_MNTN_IPC_DATA2];
			diaginfo.fv = g_msg.data[HISEE_MNTN_IPC_DATA3];
			diaginfo.info0 = g_msg.data[HISEE_MNTN_IPC_DATA4];
			diaginfo.info1 = g_msg.data[HISEE_MNTN_IPC_DATA5];

			if (diaginfo.mod_id == (u32)MODID_GENERAL_SEE_EXC_BUS)
				mspc_report_rpmb();
			/*
			 * Donnot need to call mspc_exception_to_reset_rpmb
			 * here, because it has been called if exception modid=
			 * MODID_SIMULATE_EXC_RPMB_KO
			 */
			if (diaginfo.mod_id != (u32)MODID_SIMULATE_EXC_RPMB_KO)
				g_rpmb_status_flag = mspc_exception_to_reset_rpmb();

			if (diaginfo.mod_id == (u32)MODID_GENERAL_SEE_EXC_ROM)
				(void)mspc_load_image2ddr();
#ifdef CONFIG_CRYPTO_CORE_SUPPORT_DCS
			if (diaginfo.mod_id == (u32)MODID_GENERAL_SEE_EXC_DCS)
				mspc_dcs_data_load();
#endif
			hisee_mntn_print_cos_info();
			g_vote_val_atf = (u32)atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID,
				(u64)HISEE_SMC_GET_ATF_VOTE, (u64)0, (u64)0);
			g_vote_val_tee = atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID,
					 (u64)HISEE_SMC_GET_TEE_VOTE, (u64)0, (u64)0);
			/* 0/1/2/3/4/5/6 are ipc message data index */
			pr_err("fi[0x%x] fv[0x%x] ss/info0[0x%x] sc/info1[0x%x] rpmb %d\n",
			       diaginfo.fi, diaginfo.fv, diaginfo.info0, diaginfo.info1,
			       g_rpmb_status_flag);
			pr_err("vote lpm3[0x%x]vote atf[0x%x] vote TEE[0x%llx]\n",
			       g_msg.data[HISEE_MNTN_IPC_DATA6], g_vote_val_atf, g_vote_val_tee);
			pr_err("msg.data[0] = %u msg.data[1] = %u\n",
			       g_msg.data[HISEE_MNTN_IPC_DATA0], g_msg.data[HISEE_MNTN_IPC_DATA1]);
			/* not care record result */
			(void)mspc_record_mspc_diaginfo(&diaginfo);

			rdr_system_error(diaginfo.mod_id, 0, 0);
			break;
		default:
			break;
		}
		g_hisee_mntn_state = HISEE_STATE_READY;
	}
	return 0;
}
#else
int rdr_hisee_thread(void *arg)
{
	unsigned int msg[HISEE_MNTN_IPC_MSG_LEN] = {0};

	msg[HISEE_MNTN_IPC_MSG_0] = LPM3_HISEE_MNTN;
	msg[HISEE_MNTN_IPC_MSG_1] = HISEE_RESET;

	while (!kthread_should_stop()) {
		wait_for_completion(&g_hisee_mntn_complete);
		switch (g_hisee_mntn_state) {
		case HISEE_STATE_HISEE_EXC:
			/*
			 * Needn't call mspc_exception_to_reset_rpmb here,
			 * because it has been called if exception modid =
			 * MODID_SIMULATE_EXC_RPMB_KO
			 */
			if (g_hisee_exception_modid != HISEE_MNTN_IPC_MSG_0)
				g_rpmb_status_flag = mspc_exception_to_reset_rpmb();

#ifdef CONFIG_GENERAL_SEE_SUPPORT_DCS
			if (g_hisee_exception_modid == (u32)MODID_GENERAL_SEE_EXC_DCS)
				hisee_dcs_data_load();
#endif
			hisee_mntn_print_cos_info();
			/* new req from huanghuijin: record only for new misc!!! */
			if (rdr_hisee_judge_new_misc() == HISEE_OK) {
				g_vote_val_atf = (u32)atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID,
					(u64)HISEE_SMC_GET_ATF_VOTE, (u64)0, (u64)0);
				/* 0/1/2/3/4/5/6 are ipc message data index */
				pr_err("fi[0x%x] fv[0x%x] ss[0x%x] sc[0x%x] vote lpm3[0x%x] vote atf[0x%x] rpmb %d\n",
				       g_msg.data[HISEE_MNTN_IPC_DATA2], g_msg.data[HISEE_MNTN_IPC_DATA3],
				       g_msg.data[HISEE_MNTN_IPC_DATA4], g_msg.data[HISEE_MNTN_IPC_DATA5],
				       g_msg.data[HISEE_MNTN_IPC_DATA6], g_vote_val_atf,
				       g_rpmb_status_flag);
				pr_err("msg.data[0] = %u msg.data[1] = %u\n",
				       g_msg.data[HISEE_MNTN_IPC_DATA0], g_msg.data[HISEE_MNTN_IPC_DATA1]);
				rdr_system_error(g_hisee_exception_modid, 0, 0);
			} else {
				/* To tell lpm3 reset hisee, do nothing else */
				hisee_mntn_send_msg_to_lpm3(msg, HISEE_MNTN_IPC_MSG_LEN);
			}
			break;
		case HISEE_STATE_LOG_OUT:
			rdr_hisee_save_print_log();
			break;
		default:
			break;
		}
		g_hisee_mntn_state = HISEE_STATE_READY;
	}
	return 0;
}
#endif

#ifdef CONFIG_CRYPTO_CORE
/*
 * @brief      : mspc mntn body for printing cos ver info
 * @param[in]  : try_cnt: try count
 * @return     : ::bool, success is true, failure is false
 */
static bool __printverinfo_body(u32 try_cnt)
{
	struct mspc_access_info info;

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));
	info.access_type = MSPC_FLAG_INFO_READ;

	if (mspc_access_partition((char *)&g_curr_ver_mntn,
				  sizeof(g_curr_ver_mntn), &info) == MSPC_OK ||
	    try_cnt >= HISEE_MNTN_PRINT_COS_VER_MAXTRY) {
		hisee_mntn_print_cos_info();
		kthread_stop(g_hisee_mntn_print_verinfo);
		return true;
	}
	return false;
}
#else
/*
 * @brief      : hisee mntn body for printing cos ver info
 * @param[in]  : try_cnt: try count
 * @return     : ::bool, success is true, failure is false
 */
static bool __printverinfo_body(u32 try_cnt)
{
	if (access_hisee_image_partition((char *)&g_curr_ver_mntn, sizeof(g_curr_ver_mntn),
					 SW_VERSION_READ_TYPE) == HISEE_OK ||
	    try_cnt >= HISEE_MNTN_PRINT_COS_VER_MAXTRY) {
		if (access_hisee_image_partition(
			(char *)&g_misc_version_mntn,
			sizeof(g_misc_version_mntn),
			MISC_VERSION_READ_TYPE) != HISEE_OK)
			pr_err("%s:fail to get misc ver!\n", __func__);

		hisee_mntn_print_cos_info();
		kthread_stop(g_hisee_mntn_print_verinfo);
		return true;
	}
	return false;
}
#endif

/*
 * @brief      : hisee mntn thread for printing cos ver info
 * @param[in]  : arg: arguments
 * @return     : ::int, always is 0
 */
int hisee_mntn_printverinfo_thread(void *arg)
{
	u32 try_max = 0;
	bool stop_flg = false;

	while (!kthread_should_stop()) {
		if (g_hisee_mntn_state == HISEE_STATE_READY) {
			stop_flg = __printverinfo_body(try_max);
			if (stop_flg)
				break;

			pr_err("%s:msg is NULL!\n", __func__);
			msleep(HISEE_MNTN_PRINT_COS_VER_MS);
			try_max++;
		}
	}
	return 0;
}

int hisee_mntn_save_print_log_task(void *arg)
{
	while (!kthread_should_stop()) {
		wait_for_completion(&g_hisee_mntn_save_print_log_complete);
		rdr_hisee_save_print_log();
	}
	return 0;
}

#ifdef CONFIG_CRYPTO_CORE
enum DFT_IPC_DATA {
	DFT_TYPE_IPC_DATA = 0,
	DFT_ID_IPC_DATA,
	DFT_FAULT_ID_IPC_DATA,
	DFT_FAULT_VALUE_IPC_DATA,
	DFT_INFO0_IPC_DATA,
	DFT_INFO1_IPC_DATA,
};

union secflash_ic_info {
	u32 value;
	struct {
		u32 ic_name:8;
		u32 interface:8;
		u32 fw_version:16;
	} bits;
};

union secflash_retry_info {
	u32 value;
	struct {
		u32 stage:8;
		u32 operation:8;
		u32 retry_type:8;
		u32 retry_res:8;
	} bits;
};

/*
 * @brief      : record secflash type diaginfo
 * @param[in]  : err_id: record id
 * @param[in]  : mod_id: error mod id
 * @param[in]  : fv: fault value from mspc
 * @param[in]  : info0: extra info0
 * @param[in]  : info1: extra info1
 * @return     : ret, record result
 */
static int mspc_record_secflash_diaginfo(u32 err_id, u32 mod_id, u32 fv, u32 info0, u32 info1)
{
	int ret;
	union secflash_ic_info ic_info;
	union secflash_retry_info retry_info;

	ic_info.value = info0;
	retry_info.value = info1;

	ret = bbox_diaginfo_record(err_id, NULL,
				   "SecFlash error - func category %x; "
				   "fv %x; IC %x; interface %x; version %x; "
				   "stage %x; operation %x; retry %x; "
				   "res %x",
				   mod_id, fv, ic_info.bits.ic_name,
				   ic_info.bits.interface,
				   ic_info.bits.fw_version,
				   retry_info.bits.stage,
				   retry_info.bits.operation,
				   retry_info.bits.retry_type,
				   retry_info.bits.retry_res);
	return ret;
}

/*
 * @brief      : record mspc type diaginfo
 * @param[in]  : diaginfo: mspc_diaginfo
 * @return     : ret, record result
 */
static int mspc_record_mspc_diaginfo(const struct mspc_diaginfo *diaginfo)
{
	int ret;

	ret = bbox_diaginfo_record(MSPC_DIAG_INFO, NULL,
				   "MSPC error - exception %x; mod_id %x; "
				   "fi %x; fv %x; info0 %x; info1 %x",
				   diaginfo->exception_type, diaginfo->mod_id,
				   diaginfo->fi, diaginfo->fv,
				   diaginfo->info0, diaginfo->info1);
	return ret;
}

/*
 * @brief      : record mspc diaginfo
 * @param[in]  : msg: ipc msg from lpmcu
 */
static void mspc_record_diaginfo(struct ipc_msg *msg)
{
	u32 type, id, fv, info0, info1;
	struct mspc_diaginfo diaginfo = {
		.exception_type = (u32)MSPC_EXCEPTION_NORST,
	};
	int ret;

	type = msg->data[DFT_TYPE_IPC_DATA];
	id = msg->data[DFT_ID_IPC_DATA];
	fv = msg->data[DFT_FAULT_VALUE_IPC_DATA];
	info0 = msg->data[DFT_INFO0_IPC_DATA];
	info1 = msg->data[DFT_INFO1_IPC_DATA];

	switch (type) {
	case MSPC_SECFLASH_ERR:
		ret = mspc_record_secflash_diaginfo(MSPC_SECFLASH_ERR_INFO,
						    id, fv, info0, info1);
		break;
	case MSPC_SECFLASH_WARN:
		ret = mspc_record_secflash_diaginfo(MSPC_SECFLASH_WARN_INFO,
						    id, fv, info0, info1);
		break;
	case MSPC_RECORD:
		diaginfo.mod_id = id;
		diaginfo.fi = msg->data[DFT_FAULT_ID_IPC_DATA];
		diaginfo.fv = fv;
		diaginfo.info0 = info0;
		diaginfo.info1 = info1;
		ret = mspc_record_mspc_diaginfo(&diaginfo);
		break;
	default:
		return;
	}

	pr_err("%s record type %x, id %x, fv %x, info0 %x, info1 %x finish, "
	       "ret %d\n", __func__, type, id, fv, info0, info1, ret);
}
#endif

/*
 * @brief      : rdr_hisee_msg_handler
 * @param[in]  : nb, not used now
 * @param[in]  : action, not used now
 * @param[in]  : msg, ipc message buffer
 * @return     : ::int, always is 0
 */
int rdr_hisee_msg_handler(struct notifier_block *nb,
			  unsigned long action,
			  void *msg)
{
	struct ipc_msg *p_ipcmsg = NULL;

	if (!msg) {
		pr_err("%s:msg is NULL!\n", __func__);
		return 0; /* although is failure, still return 0 */
	}

	p_ipcmsg = (struct ipc_msg *)msg;

	/*
	 * 0/6/1 are the index of ipc message, which means can referenc to
	 * ipc msg format.
	 */
	switch (p_ipcmsg->data[0]) {
	case HISEE_VOTE_RES:
		g_vote_val_lpm3 = p_ipcmsg->data[HISEE_MNTN_IPC_DATA6];
		pr_err("%s:vote val from lpm3 is 0x%x!\n",
		       __func__, g_vote_val_lpm3);
		complete(&g_hisee_pwrdebug_complete);
		break;
	case HISEE_LOG_OUT:
		g_log_out_offset = p_ipcmsg->data[1];
		complete(&g_hisee_mntn_save_print_log_complete);
		break;
	case HISEE_EXCEPTION:
#ifdef CONFIG_CRYPTO_CORE
		/* check whether exception happened in ROM */
		if ((p_ipcmsg->data[HISEE_EXC_TYPE_ID] &
		    (u32)HISEE_ROM_EXC_HEAD_MASK) ==
		    (u32)HISEE_ROM_EXC_HEAD_VALUE) {
			g_hisee_exception_modid = (u32)MODID_GENERAL_SEE_EXC_ROM;
			mspc_trigger_upgrade();
		} else {
#endif
		g_hisee_exception_modid = p_ipcmsg->data[1] + MODID_GENERAL_SEE_START;
		if (g_hisee_exception_modid >= (u32)MODID_GENERAL_SEE_EXC_BOTTOM ||
		    g_hisee_exception_modid < (u32)MODID_GENERAL_SEE_START)
			g_hisee_exception_modid = (u32)MODID_GENERAL_SEE_EXC_UNKNOWN;
#ifdef CONFIG_CRYPTO_CORE
		}
#endif
		g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
		g_vote_val_lpm3 = p_ipcmsg->data[HISEE_MNTN_IPC_DATA6];

		g_msg = *p_ipcmsg;
		complete(&g_hisee_mntn_complete);
		break;
	case HISEE_IRQ:
		g_hisee_exception_modid = translate_exc_type(p_ipcmsg->data[1]);
		g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
		g_vote_val_lpm3 = p_ipcmsg->data[HISEE_MNTN_IPC_DATA6];

		g_msg = *p_ipcmsg;
		complete(&g_hisee_mntn_complete);
		break;
	case HISEE_TIME:
		pr_err("%s:sync time with hisee, mark value is:%u\n",
		       __func__, p_ipcmsg->data[1]);
		break;
	default:
#ifdef CONFIG_CRYPTO_CORE
		mspc_record_diaginfo(p_ipcmsg);
#endif
		break;
	}

	return 0;
}

/*
 * @brief      : record dmd info
 * @param[in]  : dmd_no, dmd number
 * @param[in]  : dmd_info, info string buffer of dmd
 * @return     : ::int, 0 on success, other value on failure
 */
int hisee_mntn_record_dmd_info(long dmd_no, const char *dmd_info)
{
#ifdef CONFIG_HUAWEI_DSM
	if (dmd_no < HISEE_DMD_START || dmd_no >= HISEE_DMD_END ||
	    !dmd_info || !g_hisee_dclient) {
		pr_err("%s: para error: %lx\n", __func__, dmd_no);
		return -1;
	}

	pr_err("%s: dmd no: %lx - %s", __func__, dmd_no, dmd_info);
	if (!dsm_client_ocuppy(g_hisee_dclient)) {
		dsm_client_record(g_hisee_dclient, "DMD info:%s", dmd_info);
		dsm_client_notify(g_hisee_dclient, dmd_no);
	}
#endif
	return 0;
}

/*
 * @brief      : notify lpm3 reset hisee
 * @param[in]  : modid, module id
 * @param[in]  : etype, exception type
 * @param[in]  : coreid, core id
 */
void rdr_hisee_reset_common(u32 modid, u32 etype, u64 coreid)
{
	unsigned int msg[HISEE_MNTN_IPC_MSG_LEN] = {0};
	int ret;

#ifdef FACTORY_TEST_HISEE
#ifdef CONFIG_HUAWEI_DSM
	char buf[HISEE_MNTN_PATH_MAXLEN] = {0};

	ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1,
			 "modid:0x%x; etype:0x%x; coreid:0x%llx\n",
			 modid, etype, coreid);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s(): snprintf err.\n", __func__);
		return;
	}

	ret = hisee_mntn_record_dmd_info(DSM_HISEE_POWER_ON_OFF_ERROR_NO,
					 (const char *)buf);
	if (ret != 0)
		pr_err("%s(): nfc_record_dmd_for_hisee return %d failed\n",
		       __func__, ret);
#endif
#endif
	msg[HISEE_MNTN_IPC_MSG_0] = LPM3_HISEE_MNTN;
	msg[HISEE_MNTN_IPC_MSG_1] = HISEE_RESET;

	pr_err(" ====================================\n");
	pr_err(" modid:          [0x%x]\n", modid);
	pr_err(" coreid:         [0x%llx]\n", coreid);
	pr_err(" exce tpye:      [0x%x]\n", etype);
	pr_err(" ====================================\n");

	ret = hisee_mntn_send_msg_to_lpm3(msg, HISEE_MNTN_IPC_MSG_LEN);
	if (ret != 0)
		pr_err("%s: hisee_mntn_send_msg_to_lpm3 fail, ret %d\n", __func__, ret);
}

/*
 * @brief      : rdr_hisee_reset
 * @param[in]  : modid, module id
 * @param[in]  : etype, exception type
 * @param[in]  : coreid, core id
 */
void rdr_hisee_reset(u32 modid, u32 etype, u64 coreid)
{
	rdr_hisee_reset_common(modid, etype, coreid);
}

/*
 * @brief      : get the pointer to the name str of mod
 * @param[in]  : modid, module id
 * @return     : module name string
 */
static char *hisee_mntn_get_mod_name_str(u32 modid)
{
	u32  i;
	char *p_name = NULL;

	for (i = 0; i < ARRAY_SIZE(g_hisee_excetption_info); i++) {
		if (modid == g_hisee_excetption_info[i].e_modid) {
			p_name = (char *)(g_hisee_excetption_info[i].e_from_module);
			break;
		}
	}
	if (!p_name)
		pr_err("%s() p_name is NULL.\n", __func__);
	return p_name;
}

/*
 * @brief      : save hisee log to file system
 * @param[in]  : modid, module id
 * @param[in]  : coreid, exception type
 * @param[in]  : pathname, log path
 * @param[in]  : pfn_cb: callback function
 */
void rdr_hisee_dump_common(u32 modid, u64 coreid,
			   const char *pathname, pfn_cb_dump_done pfn_cb)
{
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	char debug_cont[HISEE_MNTN_PATH_MAXLEN] = {0};
	char *p_name_str = hisee_mntn_get_mod_name_str(modid);
	int ret;

	if (!pathname || !g_logbuf_info.header) {
		pr_err("%s:pointer is NULL !!\n",  __func__);
		return;
	}
	atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID, (u64)HISEE_SMC_GET_LOG,
					 (u64)0x0, (u64)0x0);
	ret = snprintf_s(path, HISEE_MNTN_PATH_MAXLEN,
			 HISEE_MNTN_PATH_MAXLEN - 1,
			 "%s/%s", pathname, HISEE_LOG_FLIENAME);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s(): snprintf err.\n", __func__);
		return;
	}

	/* save hisee log to data/hisi_logs/time/hisee_log */
	ret = mntn_filesys_write_log(path, (void *)((uintptr_t)g_logbuf_info.addr + GENERAL_SEE_LOGBUF_HEADER_LEN),
				     (unsigned int)(g_logbuf_info.header->real_size),
				     HISEE_FILE_PERMISSION);
	if (ret == 0)
		pr_err("%s:hisee log save fail\n", __func__);

	if (p_name_str) {
		ret = mntn_filesys_write_log(
			path, (void *)p_name_str,
			(unsigned int)MODULE_NAME_LEN, HISEE_FILE_PERMISSION);
		pr_err("mod name:      [%s]\n", p_name_str);
	} else {
		ret = mntn_filesys_write_log(
			path, (void *)(&modid),
			(unsigned int)sizeof(modid), HISEE_FILE_PERMISSION);
	}

	pr_err(" ====================================\n");
	if (ret == 0)
		pr_err("%s:hisee mod id save fail\n", __func__);

	/* save fi fv ss and sv in hisee_log */
	(void)memset_s((void *)debug_cont, sizeof(debug_cont), 0, sizeof(debug_cont));
	/* 0/1/2/3/4/5/6 are ipc message data index */
	ret = snprintf_s(debug_cont, HISEE_MNTN_PATH_MAXLEN, HISEE_MNTN_PATH_MAXLEN - 1,
			 "fi[0x%x] fv[0x%x] ss[0x%x] sc[0x%x] vote lpm3[0x%x] vote atf[0x%x] rpmb %d\n",
			 g_msg.data[HISEE_MNTN_IPC_DATA2], g_msg.data[HISEE_MNTN_IPC_DATA3], g_msg.data[HISEE_MNTN_IPC_DATA4],
			 g_msg.data[HISEE_MNTN_IPC_DATA5], g_msg.data[HISEE_MNTN_IPC_DATA6], g_vote_val_atf,
			 g_rpmb_status_flag);
	if (ret == HISEE_SECLIB_ERROR) {
		pr_err("%s(): snprintf err.\n", __func__);
		return;
	}

	ret = mntn_filesys_write_log(
		path, (void *)debug_cont,
		(unsigned int)HISEE_MNTN_PATH_MAXLEN, HISEE_FILE_PERMISSION);
	if (ret == 0)
		pr_err("%s:fifvsssc save fail\n", __func__);

	/* save to 8M */
	ret = memcpy_s(g_hisee_mntn_addr, g_hisee_info.log_len,
		       g_logbuf_info.addr, (unsigned long)g_hisee_info.log_len);
	if (ret != EOK) {
		pr_err("%s(): memcpy err.\n", __func__);
		return;
	}
	(void)memset_s(&g_msg, sizeof(struct ipc_msg), 0, sizeof(struct ipc_msg));
	if (pfn_cb)
		pfn_cb(modid, coreid);
}

/*
 * @brief      : do hisee dump using rdr
 * @param[in]  : modid , module id
 * @param[in]  : etype , exception type
 * @param[in]  : coreid , core id
 * @param[in]  : pathname , path name string
 * @param[in]  : pfn_cb , callback function
 * @return     : void
 */
void rdr_hisee_dump(u32 modid, u32 etype, u64 coreid,
		    char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (!pathname) {
		pr_err("%s:pointer is NULL !!\n",  __func__);
		return;
	}
	pr_err(" ====================================\n");
	pr_err(" modid:          [0x%x]\n",   modid);
	pr_err(" coreid:         [0x%llx]\n", coreid);
	pr_err(" exce tpye:      [0x%x]\n",   etype);
	pr_err(" path name:      [%s]\n",     pathname);

	rdr_hisee_dump_common(modid, coreid, pathname, pfn_cb);
}

/*
 * @brief      : register hisee dump and reset function
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_register_core(void)
{
	int ret;
	struct rdr_module_ops_pub s_soc_ops = {0};

	s_soc_ops.ops_dump = rdr_hisee_dump;
	s_soc_ops.ops_reset = rdr_hisee_reset;
	/* register hisee core dump and reset function */
	ret = rdr_register_module_ops((u64)RDR_GENERAL_SEE, &s_soc_ops, &g_hisee_info);
	if (ret < 0) {
		pr_err("%s:hisee core register fail, ret = [%d]\n",
		       __func__, ret);
		return ret;
	};
	return 0;
}

/*
 * @brief      : register hisee exception
 */
static void hisee_register_exception(void)
{
	u32 ret;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_hisee_excetption_info); i++) {
		/* error return 0, ok return modid */
		ret = rdr_register_exception(&g_hisee_excetption_info[i]);
		if (ret == 0)
			pr_err("register hisee exception fail [%u]\n", i);
	}
}

#ifdef CONFIG_GENERAL_SEE_MNTN_TEST
/*
 * @brief      : set mntn ctrl level according to the nv value
 * @param[in]  : value, hisee mntn level
 * @return     : void
 */
static void hisee_mntn_set_ctrl_level_handler(int value)
{
	unsigned int msg[HISEE_MNTN_IPC_MSG_LEN << 1] = {0};
	int ret;

	msg[HISEE_MNTN_IPC_MSG_0] = LPM3_HISEE_MNTN;
	msg[HISEE_MNTN_IPC_MSG_1] = HISEE_MNTN_CTRL;
	msg[HISEE_MNTN_IPC_MSG_3] = HISEE_MNTN_LOG_CTRL_OUT_2_KERNEL;
	switch (value) {
	case '0':
		msg[HISEE_MNTN_IPC_MSG_2] = (u32)HISEE_MNTN_DISABLE;
		break;
	case '1':
		msg[HISEE_MNTN_IPC_MSG_2] = (u32)HISEE_MNTN_USR;
		break;
	case '2':
		msg[HISEE_MNTN_IPC_MSG_2] = (u32)HISEE_MNTN_BETA;
		break;
	case '3':
		msg[HISEE_MNTN_IPC_MSG_2] = (u32)HISEE_MNTN_ENG;
		break;
	case '4':
		msg[HISEE_MNTN_IPC_MSG_2] = (u32)HISEE_MNTN_ENG;
		msg[HISEE_MNTN_IPC_MSG_3] = HISEE_MNTN_LOG_CTRL_NOT_2_KERNEL;
		break;
	default:
		break;
	}

	ret = hisee_mntn_send_msg_to_lpm3(msg, HISEE_MNTN_IPC_MSG_LEN << 1);
	if (ret != 0)
		pr_err("%s: hisee_mntn_send_msg_to_lpm3 fail, ret %d\n", __func__, ret);
	pr_err("%s:hisee mntn ctrl level is %u\n",
	       __func__, msg[HISEE_MNTN_IPC_MSG_2]);
}

static void hisee_mntn_set_ctrl_level(void)
{
	int value = get_mntn_switch_value(MNTN_GENERAL_SEE);

	hisee_mntn_set_ctrl_level_handler(value);
}

/*
 * @brief      : hisee mntn test, only for test
 */
void hisee_mntn_make_data_full(void)
{
	u64 total_cnt = 0;
	u32 cnt_1;
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	u8 *p_buf = kmalloc((size_t)DATA_FULL_BUF_LEN, GFP_KERNEL);
	int ret;

	if (p_buf) { /* 0xab is clean value */
		(void)memset_s((void *)p_buf, DATA_FULL_BUF_LEN, 0xab,
			       (size_t)DATA_FULL_BUF_LEN);
		pr_err(" ============start to make data full++!!==============\n");
		ret = snprintf_s(path, HISEE_MNTN_PATH_MAXLEN,
				 HISEE_MNTN_PATH_MAXLEN - 1,
				 "%s%s", PATH_ROOT, "hisee_datafull");
		if (ret == HISEE_SECLIB_ERROR) {
			pr_err("%s(): snprintf err.\n", __func__);
			kfree(p_buf);
			return;
		}

		while (1) {
			/* save hisee log to data/hisi_logs/hisee/hisee_log */
			cnt_1 = (u32)mntn_filesys_write_log(
					(const char *)path,
					(void *)p_buf,
					(unsigned int)DATA_FULL_BUF_LEN,
					HISEE_FILE_PERMISSION);
			total_cnt = total_cnt + cnt_1;
			if (cnt_1 < DATA_FULL_BUF_LEN || total_cnt > rdr_get_logsize())
				break;
		}
		kfree(p_buf);
		pr_err(" ============start to make data full --!!==============\n");
	}
}

static int _mntn_test_function_0(u32 modid, unsigned int *msg, size_t size)
{
	u32 i;
	u32 int_id = HISEE_INVALID_INT_ID;

	if (modid < (u32)DFX_BB_MOD_GENERAL_SEE_START ||
	    modid > (u32)DFX_BB_MOD_GENERAL_SEE_END)
		return -ENOEXEC;

	for (i = 0; i < ARRAY_SIZE(g_hisee_exc_trans); i++)
		if (modid == g_hisee_exc_trans[i].module_value) {
			int_id = g_hisee_exc_trans[i].int_id_value;
			break;
		}
	if (size < HISEE_MNTN_MSG_LEN) {
		pr_err("hisee mntn:invalid size:0x%x\n", (unsigned int)size);
		return HISEE_ERROR;
	}
	if (int_id != HISEE_INVALID_INT_ID) {
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_IRQ;
		msg[HISEE_MNTN_IPC_MSG_3] = int_id;
	} else {
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_EXCEPTION;
		msg[HISEE_MNTN_IPC_MSG_3] = modid - MODID_GENERAL_SEE_START;
	}
	return 0;
}

enum {
	HISEE_MNTN_TC_MOD_EXCEPTION_ID = 0,
	HISEE_MNTN_TC_PRINT_PLAIN_ID   = 1,
	HISEE_MNTN_TC_RUN_COS_ID       = 2,
	HISEE_MNTN_TC_PRINT_ERR_ID     = 3,
	HISEE_MNTN_TC_PRINT_CRYPTO_ID  = 4,
	HISEE_MNTN_TC_PRINT_LEVEL_ID   = 5,
};
/*
 * @brief      : hisee mntn test, only for test
 */
int hisee_mntn_test(u32 data1, u32 data2)
{
	int ret;
	unsigned int msg[HISEE_MNTN_MSG_LEN] = {0};
	u32 modid = data2;

	msg[HISEE_MNTN_IPC_MSG_0] = LPM3_HISEE_MNTN;
	msg[HISEE_MNTN_IPC_MSG_1] = HISEE_TEST;
	/*
	 * data1 is mntn test item id, data2 is the mntn test params,
	 * which meaning are defined by self.
	 */
	switch (data1) {
	case HISEE_MNTN_TC_MOD_EXCEPTION_ID:
		ret = _mntn_test_function_0(modid, msg, HISEE_MNTN_MSG_LEN);
		if (ret != 0)
			return ret;
		break;
	case HISEE_MNTN_TC_PRINT_PLAIN_ID:
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_PRINT_PLAIN;
		msg[HISEE_MNTN_IPC_MSG_3] = data2;
		break;
	case HISEE_MNTN_TC_RUN_COS_ID:
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_RUN_COS;
		msg[HISEE_MNTN_IPC_MSG_3] = data2;
		break;
	case HISEE_MNTN_TC_PRINT_ERR_ID:
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_PRINT_ERR;
		msg[HISEE_MNTN_IPC_MSG_3] = data2;
		break;
	case HISEE_MNTN_TC_PRINT_CRYPTO_ID:
		msg[HISEE_MNTN_IPC_MSG_2] = HISEE_TEST_PRINT_CRYPTO;
		msg[HISEE_MNTN_IPC_MSG_3] = data2;
		break;
	case HISEE_MNTN_TC_PRINT_LEVEL_ID:
		hisee_mntn_set_ctrl_level_handler((int)data2);
		break;
	default:
		break;
	}
	ret = hisee_mntn_send_msg_to_lpm3(msg, HISEE_MNTN_MSG_LEN);
	return ret;
}
#endif

#if defined(CONFIG_GENERAL_SEE_MNTN_TEST) || defined(CONFIG_CRYPTO_CORE)
/*
 * @brief      : dump hisee data when hisee is running
 */
void hisee_mntn_debug_dump(void)
{
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	int ret;

	if (g_hisee_mntn_state == HISEE_STATE_INVALID) {
		pr_err(" hisee_mntn isn't initial\n");
		return;
	}
	pr_err(" ============somebody is using hisee mntn debug!!==============\n");
	atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID, (u64)HISEE_SMC_GET_LOG,
					 (u64)0x0, (u64)0x0);
	if (!g_logbuf_info.header || g_logbuf_info.header->real_size == 0) {
		pr_err("Hisee is pwr off!!!\n");
	} else {
		ret = snprintf_s(path, HISEE_MNTN_PATH_MAXLEN, HISEE_MNTN_PATH_MAXLEN - 1,
				 "%s%s", PATH_ROOT, HISEE_LOG_FLIENAME);
		if (ret == HISEE_SECLIB_ERROR) {
			pr_err("%s(): snprintf err.\n", __func__);
			return;
		}

		/* save hisee log to data/hisi_logs/hisee/hisee_log */
		ret = mntn_filesys_write_log(path,
					     (void *)((uintptr_t)g_logbuf_info.addr + GENERAL_SEE_LOGBUF_HEADER_LEN),
					     (unsigned int)(g_logbuf_info.header->real_size),
					     HISEE_FILE_PERMISSION);
		if (ret == 0)
			pr_err("%s:hisee log save fail\n", __func__);
	}
}
#endif

/*
 * @brief      : hisee_mntn_prepare_logbuf
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_mntn_prepare_logbuf(struct platform_device *pdev)
{
	g_hisee_mntn_addr = dfx_bbox_map((phys_addr_t)g_hisee_info.log_addr,
					  (size_t)g_hisee_info.log_len);
	if (!g_hisee_mntn_addr) {
		pr_err("%s:memory map fail\n", __func__);
		return -EFAULT;
	}

	if (g_hisee_info.log_len <= GENERAL_SEE_LOGBUF_HEADER_LEN) {
		pr_err("%s:memory size %u too small\n", __func__, g_hisee_info.log_len);
		return -ENOMEM;
	}

	g_logbuf_info.addr = dma_alloc_coherent(&pdev->dev, (size_t)g_hisee_info.log_len,
						&g_hisee_log_phy, GFP_KERNEL);
	if (!g_logbuf_info.addr) {
		pr_err("%s:memory alloc fail\n", __func__);
		return -ENOMEM;
	}
	g_logbuf_info.header = (struct general_see_logbuf_header *)g_logbuf_info.addr;
	g_logbuf_info.header->real_size = 0;
	pr_info("%s : v:%pK, phy : %llx\n", __func__,
		g_logbuf_info.addr, (u64)g_hisee_log_phy);
	return 0;
}

/*
 * @brief      : print cos info
 */
void hisee_mntn_update_local_ver_info(void)
{
#ifdef CONFIG_CRYPTO_CORE
	struct mspc_access_info info;

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));
	info.access_type = MSPC_FLAG_INFO_READ;
	if (mspc_access_partition((char *)&g_curr_ver_mntn,
				  sizeof(g_curr_ver_mntn), &info) != MSPC_OK)
		pr_err("%s:Fail to update ver info\n", __func__);
	else
		hisee_mntn_print_cos_info();
#else /* CONFIG_CRYPTO_CORE */
	g_curr_ver_mntn.magic = 0;

	if (access_hisee_image_partition((char *)&g_curr_ver_mntn, sizeof(g_curr_ver_mntn),
					 SW_VERSION_READ_TYPE) != HISEE_OK)
		pr_err("%s:Fail to update ver info\n", __func__);
	else
		hisee_mntn_print_cos_info();
#endif /* CONFIG_CRYPTO_CORE */
}

#ifdef CONFIG_CRYPTO_CORE
void hisee_mntn_print_cos_info(void)
{
	pr_err("%s:%x %llx\n", __func__, g_curr_ver_mntn.magic,
	       (u64)g_curr_ver_mntn.timestamp.value);
}

#else /* CONFIG_CRYPTO_CORE */
/*
 * @brief      : print cos info
 */
void hisee_mntn_print_cos_info(void)
{
	int i;

	pr_err("%s:%x %llx", __func__, g_curr_ver_mntn.magic,
	       (u64)g_curr_ver_mntn.img_timestamp.value);

	for (i = 0; i < HISEE_SUPPORT_COS_FILE_NUMBER; i++) {
#ifdef CONFIG_GENERAL_SEE_SUPPORT_MULTI_COS
		if (i >= HISEE_HALF_SW_VERSION_NUMBER)
			pr_err(" %d", g_curr_ver_mntn.img_version_num1[i - HISEE_HALF_SW_VERSION_NUMBER]);
		else
			pr_err(" %d", g_curr_ver_mntn.img_version_num[i]);
#else
		pr_err(" %d", g_curr_ver_mntn.img_version_num[i]);
#endif
	}
	pr_err("\n");

	/* print misc info */
	pr_err("misc info: %s:%x %llx", __func__, g_misc_version_mntn.magic,
	       (u64)g_misc_version_mntn.img_timestamp.value);
	for (i = 0; i < HISEE_SUPPORT_COS_FILE_NUMBER; i++)
		pr_err(" %d", g_misc_version_mntn.img_version_num[i]);

	pr_err("\n");
}
#endif /* CONFIG_CRYPTO_CORE */

#ifndef CONFIG_CRYPTO_CORE
/*
 * @brief      : don't allow to power up hisee if cur phone is dm and just
 *               power up booting
 * @return     : ::int, 0 allow to powerup, other value don't allow to powerup
 */
int hisee_mntn_can_power_up_hisee(void)
{
	int ret = HISEE_OK;

	if (check_hisee_is_fpga())
		return ret;

#ifndef FACTORY_TEST_HISEE
	/* get the current mode */
	if (g_hisee_lcs_mode_mntn == 0)
		if (get_hisee_lcs_mode(&g_hisee_lcs_mode_mntn) != HISEE_OK)
			pr_err("%s:failt to get lcs mode\n", __func__);

	if (g_hisee_lcs_mode_mntn == HISEE_DM_MODE_MAGIC)
		if (g_curr_ver_mntn.magic == 0) {
			pr_err("%s:cos hasn't been upgraded\n", __func__);
			ret = HISEE_ERROR;
		}

#endif
	return ret;
}
#endif /* CONFIG_CRYPTO_CORE */

/*
 * @brief      : update local variable of lcs mode
 */
void hisee_mntn_update_local_lcsmode_val(void)
{
	g_hisee_lcs_mode_mntn = 0;
}

/*
 * @brief      : get the vote value in lpm3
 * @return     : vote value
 */
u32 hisee_mntn_get_vote_val_lpm3(void)
{
	return g_vote_val_lpm3;
}

/*
 * @brief      : get the vote value in atf
 * @return     : vote value
 */
u32 hisee_mntn_get_vote_val_atf(void)
{
	return g_vote_val_atf;
}

#ifdef CONFIG_CRYPTO_CORE
u64 mspc_mntn_get_vote_val_tee(void)
{
	return g_vote_val_tee;
}
#endif

/*
 * @brief      : to get vote value from lpm3 and atf
 * @return     : vote value
 */
int hisee_mntn_collect_vote_value_cmd(void)
{
	int ret = HISEE_OK;
	unsigned int msg[HISEE_MNTN_IPC_MSG_LEN << 1] = {0};
	unsigned long time_left;

	if (g_need_run_flag == HISEE_ERROR) {
		ret = HISEE_ERROR;
	} else {
		g_need_run_flag = HISEE_ERROR;
		msg[HISEE_MNTN_IPC_MSG_0] = LPM3_HISEE_MNTN;
		msg[HISEE_MNTN_IPC_MSG_1] = HISEE_GET_VOTE;
		hisee_mntn_send_msg_to_lpm3(msg, HISEE_MNTN_IPC_MSG_LEN);
		g_vote_val_atf = (u32)atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID,
				 (u64)HISEE_SMC_GET_ATF_VOTE, (u64)0, (u64)0);
#ifdef CONFIG_CRYPTO_CORE
		g_vote_val_tee = atfd_hisi_service_hisee_mntn_smc(HISEE_MNTN_ID,
				 (u64)HISEE_SMC_GET_TEE_VOTE, (u64)0, (u64)0);
#endif
		/* wait for lpm3 reply */
		time_left = wait_for_completion_timeout(
				&g_hisee_pwrdebug_complete,
				msecs_to_jiffies(WAIT_LPM3_REPLY_TIMEOUT));
		if (time_left == 0)
			pr_err("%s: no time is left!!\n", __func__);

		g_need_run_flag = HISEE_OK;
		pr_err("%s: vote_val from atf is 0x%x!\n",
		       __func__, g_vote_val_atf);
	}
	return ret;
}

/*
 * @brief      : hisee mntn initialization
 * @return     : ::int, 0 on success, other value on failure
 */
static int hisee_mntn_probe(struct platform_device *pdev)
{
	int ret;

#ifdef CONFIG_GENERAL_SEE_MNTN_TEST
	/*
	 * set ctrl level, usr, beta, eng, disable,
	 * etc according to the value of himntn nv
	 */
	hisee_mntn_set_ctrl_level();
#endif

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret != 0) {
		pr_err("%s: init failed, ret.%d\n", __func__, ret);
		return ret;
	}

	/* register hisee exception */
	hisee_register_exception();

	/* register hisee dump and reset function */
	ret = hisee_register_core();
	if (ret < 0)
		return ret;

	ret = hisee_mntn_prepare_logbuf(pdev);
	if (ret < 0)
		return ret;
	/* initialization mailbox */
	g_hisee_ipc_block.next = NULL;
	g_hisee_ipc_block.notifier_call = rdr_hisee_msg_handler;

	ret = RPROC_MONITOR_REGISTER(IPC_LPM3_ACPU_MBX_1, &g_hisee_ipc_block);
	if (ret != 0) {
		pr_err("%s:RPROC_MONITOR_REGISTER failed\n", __func__);
		return ret;
	}
	init_completion(&g_hisee_mntn_complete);
	g_hisee_mntn_thread = kthread_run(rdr_hisee_thread, NULL, "hisee_mntn");
	if (!g_hisee_mntn_thread)
		pr_err("create hisee mntn thread failed.\n");

	atfd_hisi_service_hisee_mntn_smc(
		HISEE_MNTN_ID, (u64)HISEE_SMC_INIT,
		g_hisee_log_phy, (u64)g_hisee_info.log_len);

	g_hisee_mntn_print_verinfo = kthread_run(hisee_mntn_printverinfo_thread,
						 NULL, "hiseeprint_mntn");
	if (!g_hisee_mntn_print_verinfo)
		pr_err("fail to create the thread that prints cos ver info\n");

	init_completion(&g_hisee_mntn_save_print_log_complete);
	g_hisee_mntn_save_print_log_task = kthread_run(hisee_mntn_save_print_log_task,
						       NULL,
						       "hisee_mntn_log_save");
	if (!g_hisee_mntn_save_print_log_task)
		pr_err("fail to create the thread that save print log\n");

#ifdef CONFIG_HUAWEI_DSM
	if (!g_hisee_dclient)
		g_hisee_dclient = dsm_register_client(&g_dsm_hisee);
#endif

	g_hisee_mntn_state = HISEE_STATE_READY;

	pr_err("exit %s\n", __func__);
	return 0;
}

static int hisee_mntn_remove(struct platform_device *pdev)
{
	dma_free_coherent(&pdev->dev, (size_t)g_hisee_info.log_len,
			  &g_hisee_log_phy, GFP_KERNEL);

	if (g_hisee_mntn_thread) {
		kthread_stop(g_hisee_mntn_thread);
		g_hisee_mntn_thread = NULL;
	}
	if (g_hisee_mntn_print_verinfo) {
		kthread_stop(g_hisee_mntn_print_verinfo);
		g_hisee_mntn_print_verinfo = NULL;
	}
	if (g_hisee_mntn_save_print_log_task) {
		kthread_stop(g_hisee_mntn_save_print_log_task);
		g_hisee_mntn_save_print_log_task = NULL;
	}
	return 0;
}

static const struct of_device_id g_hisee_mntn_match[] = {
	{ .compatible = "hisee-mntn" },
	{ }
};

static struct platform_driver g_hisee_mntn_driver = {
	.probe = hisee_mntn_probe,
	.remove = hisee_mntn_remove,
	.driver = {
		   .name = "hisee-mntn",
		   .of_match_table = g_hisee_mntn_match,
	},
};

static int __init hisee_mntn_init(void)
{
	return platform_driver_register(&g_hisee_mntn_driver);
}

static void __exit hisee_mntn_exit(void)
{
	platform_driver_unregister(&g_hisee_mntn_driver);
}

module_init(hisee_mntn_init);
module_exit(hisee_mntn_exit);

MODULE_LICENSE("GPL");