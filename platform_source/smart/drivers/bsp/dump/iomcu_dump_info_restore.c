/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub save dump file process.
 * Create: 2021-11-26
 */

#include "iomcu_dump_priv.h"
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_boot.h>
#include <linux/syscalls.h>
#include "securec.h"
#include <linux/delay.h>
#include <linux/namei.h>

#define CUR_PATH_LEN 64
#define MAX_DUMP_CNT 32
#define DUMP_REGS_SIZE 128
#define DUMP_REGS_MAGIC 0x7a686f75
#define REG_NAME_SIZE 25
#define LOG_EXTRA_NUM 2

struct dump_zone_head {
	uint32_t cnt;
	uint32_t len;
	uint32_t tuncate;
};

struct dump_zone_element_t {
	uint32_t base;
	uint32_t size;
};

enum m7_register_name {
	SOURCE,
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	EXP_SP,
	EXP_LR,
	EXP_PC,
	SAVED_PSR,
	CFSR,
	HFSR,
	DFSR,
	MMFAR,
	BFAR,
	AFSR,
	PSP,
	MSP,
	ARADDR_CHK,
	AWADDR_CHK,
	PERI_USED,
	AO_CNT,
	MAGIC,
};

static char g_dump_restore_dir[PATH_MAXLEN] = SH_DMP_DIR;
static uint32_t *g_dp_regs = NULL;
static uint32_t g_dump_index = (uint32_t)-1;

static const char *const g_sh_regs_name[] = {
	"SOURCE", "R0",   "R1",         "R2",         "R3",        "R4",
	"R5",     "R6",   "R7",         "R8",         "R9",        "R10",
	"R11",    "R12",  "EXP_SP",     "EXP_LR",     "EXP_PC",    "SAVED_PSR",
	"CFSR",   "HFSR", "DFSR",       "MMFAR",      "BFAR",      "AFSR",
	"PSP",    "MSP",  "ARADDR_CHK", "AWADDR_CHK", "PERI_USED", "AO_CNT",
};

static const char *const g_sh_reset_reasons[] = {
	"SH_FAULT_HARDFAULT", // 0
	"SH_FAULT_BUSFAULT",       "SH_FAULT_USAGEFAULT",
	"SH_FAULT_MEMFAULT",       "SH_FAULT_NMIFAULT",
	"SH_FAULT_ASSERT", // 5
	"UNKNOW DUMP FAULT",       "UNKNOW DUMP FAULT",
	"UNKNOW DUMP FAULT",       "UNKNOW DUMP FAULT",
	"UNKNOW DUMP FAULT", // 10
	"UNKNOW DUMP FAULT",       "UNKNOW DUMP FAULT",
	"UNKNOW DUMP FAULT",       "UNKNOW DUMP FAULT",
	"UNKNOW DUMP FAULT",      // 15
	"SH_FAULT_INTERNELFAULT", // 16
	"SH_FAULT_IPC_RX_TIMEOUT", "SH_FAULT_IPC_TX_TIMEOUT",
	"SH_FAULT_RESET",          "SH_FAULT_USER_DUMP",
	"SH_FAULT_RESUME",         "SH_FAULT_REDETECT",
	"SH_FAULT_PANIC",          "SH_FAULT_NOC",
	"SH_FAULT_EXP_BOTTOM", // also use as unknow dump
};
long do_unlinkat(int dfd, struct filename *name);
static int judge_access(const char *filename)
{
	struct path path;
	int error;

	error = kern_path(filename, LOOKUP_FOLLOW, &path);
	if (error)
		return error;
	error = inode_permission(d_inode(path.dentry), MAY_ACCESS);
	path_put(&path);
	return error;
}

static int iomcu_mkdir(const char *pathname, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int error;

	dentry = kern_path_create(AT_FDCWD, pathname, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);
	if (!IS_POSIXACL(path.dentry->d_inode))
		mode &= ~((uint32_t)current_umask());
	error = security_path_mkdir(&path, dentry, mode);
	if (!error)
		error = vfs_mkdir(path.dentry->d_inode, dentry, mode);
	done_path_create(&path, dentry);
	return error;
}

static int __sh_create_dir(char *path, unsigned int len)
{
	int permission;
	int mkdir;
	mm_segment_t old_fs;

	if (len > CUR_PATH_LEN) {
		pr_err("invalid  parameter. len is %d\n", len);
		return -1;
	}
	if (path == NULL) {
		pr_err("null path\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/
	permission = judge_access(path);
	if (permission != 0) {
		pr_info("sh: need create dir %s\n", path);
		mkdir = iomcu_mkdir(path, DIR_LIMIT);
		if (mkdir < 0) {
			pr_err("sh: create dir %s failed! ret = %d\n", path, mkdir);
			set_fs(old_fs);
			return mkdir;
		}

		pr_info("sh: create dir %s successed [%d]!!!\n", path, mkdir);
	}
	set_fs(old_fs);
	return 0;
}

static int sh_create_dir(const char *path)
{
	char cur_path[CUR_PATH_LEN];
	int index = 0;

	if (path == NULL) {
		pr_err("invalid  parameter. path:%pK\n", path);
		return -1;
	}
	(void)memset_s(cur_path, CUR_PATH_LEN, 0, CUR_PATH_LEN);
	if (*path != '/')
		return -1;
	cur_path[index++] = *path++;
	while ((*path != '\0') && (index < CUR_PATH_LEN)) {
		if (*path == '/')
			__sh_create_dir(cur_path, CUR_PATH_LEN);

		cur_path[index] = *path;
		path++;
		index++;
		if (index >= CUR_PATH_LEN) {
			pr_err("[%s]invalid index %d. path:%pK\n", __func__, index, path);
			return -1;
		}
	}
	return 0;
}

static int sh_wait_fs(const char *path)
{
	int permission = 0;
	int ret = 0;
	unsigned int wait_index = 0;
	unsigned int wait_max = 1000;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/

	do {
		permission = judge_access(path);
		if (permission != 0) {
			msleep(10);
			pr_info("[%s] wait ...\n", __func__);
			wait_index++;
		}
	} while ((permission != 0) && (wait_index <= wait_max));
	if (permission != 0) {
		ret = -1;
		pr_err("[%s] cannot access path! errno %d\n", __func__, permission);
	}
	set_fs(old_fs);
	return ret;
}

static int sh_readfs2buf(char *logpath, char *filename, void *buf, u32 len)
{
	int ret = -1;
	ssize_t read_size;
	int flags;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char path[PATH_MAXLEN];

	if ((logpath == NULL) || (filename == NULL) || (buf == NULL) || (len <= 0)) {
		pr_err("invalid  parameter. path:%pK, name:%pK buf:%pK len:0x%x\n", logpath, filename, buf, len);
		goto FIN;
	}

	if (sprintf_s(path, PATH_MAXLEN, "%s/%s", logpath, filename) < 0) {
		pr_err("%s():path length is too large\n", __func__);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/

	if (judge_access(path) != 0)
		goto SET_OLD_FS;

	flags = O_RDWR;
	fp = filp_open(path, flags, FILE_LIMIT);
	if (IS_ERR(fp)) {
		pr_err("%s():open file %s err.\n", __func__, path);
		goto SET_OLD_FS;
	}

	vfs_llseek(fp, 0L, SEEK_SET);
	read_size = vfs_read(fp, buf, len, &(fp->f_pos));
	if (read_size != len)
		pr_err("%s:read file %s exception with ret %u.\n", __func__, path, read_size);
	else
		ret = (int)read_size;

	filp_close(fp, NULL);
SET_OLD_FS:
	set_fs(old_fs);
FIN:
	return ret;
}

static int sh_savebuf2fs(char *logpath, char *filename, void *buf, u32 len, u32 is_append)
{
	int ret = 0;
	ssize_t write_size;
	int flags;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char path[PATH_MAXLEN];

	if ((logpath == NULL) || (filename == NULL) || (buf == NULL) || (len <= 0)) {
		pr_err("invalid  parameter. path:%pK, name:%pK buf:%pK len:0x%x\n", logpath, filename, buf, len);
		ret = -1;
		goto FIN;
	}

	if (sprintf_s(path, PATH_MAXLEN, "%s/%s", logpath, filename) < 0) {
		pr_err("%s():path length is too large\n", __func__);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/

	flags = O_CREAT | O_RDWR | (is_append ? O_APPEND : O_TRUNC);
	fp = filp_open(path, flags, FILE_LIMIT);
	if (IS_ERR(fp)) {
		set_fs(old_fs);
		pr_err("%s():create file %s err.\n", __func__, path);
		ret = -1;
		goto FIN;
	}

	vfs_llseek(fp, 0L, SEEK_END);
	write_size = vfs_write(fp, buf, len, &(fp->f_pos));
	if (write_size != len) {
		pr_err("%s:write file %s exception with ret %u.\n", __func__, path, write_size);
		ret = -1;
		goto FS_CLOSE;
	}

	vfs_fsync(fp, 0);
FS_CLOSE:
	filp_close(fp, NULL);

	ret = (int)vfs_fchown(fp, ROOT_UID, SYSTEM_GID);
	if (ret)
		pr_err("[%s], chown %s uid [%d] gid [%d] failed err [%d]!\n",
			__func__, path, ROOT_UID, SYSTEM_GID, ret);

	set_fs(old_fs);
FIN:
	return ret;
}

static void get_max_dump_cnt(void)
{
	int ret;
	uint32_t index;

	/* find max index */
	ret = sh_readfs2buf(g_dump_restore_dir, "dump_max", &index, sizeof(index));
	if (ret < 0)
		g_dump_index = (uint32_t)-1;
	else
		g_dump_index = index;

	g_dump_index++;
	if (g_dump_index == MAX_DUMP_CNT)
		g_dump_index = 0;
	sh_savebuf2fs(g_dump_restore_dir, "dump_max", &g_dump_index, sizeof(g_dump_index), 0);
}

static void get_sh_dump_regs(uint8_t * sensorhub_dump_buff)
{
	struct dump_zone_head *dzh = NULL;
	uint32_t magic;
#ifdef CONFIG_DFX_DEBUG_FS
	uint32_t i;
	int32_t words = 0;
	int32_t ret;
	char buf[DUMP_REGS_SIZE] = {0};
#endif

	dzh = (struct dump_zone_head *)sensorhub_dump_buff;
	g_dp_regs = (uint32_t *)(sensorhub_dump_buff + sizeof(struct dump_zone_head) +
		sizeof(struct dump_zone_element_t) * dzh->cnt);
	if ((uintptr_t)(g_dp_regs + MAGIC) > (uintptr_t)(sensorhub_dump_buff + SENSORHUB_DUMP_BUFF_SIZE)) {
		pr_err("%s: ramdump maybe failed\n", __func__);
		g_dp_regs = NULL;
		return;
	}
	magic = g_dp_regs[MAGIC];
	pr_info("%s: dump regs magic 0x%x dump addr 0x%pK dp regs addr 0x%pK\n",
		__func__, magic, sensorhub_dump_buff, g_dp_regs);
	if (magic != DUMP_REGS_MAGIC) {
		g_dp_regs = NULL;
		return;
	}

#ifdef CONFIG_DFX_DEBUG_FS
	/* output it to kmsg */
	for (i = SOURCE; i < MAGIC; i++) {
		ret = snprintf_s(buf + words, DUMP_REGS_SIZE - words, REG_NAME_SIZE,
			"%s--%08x, ", g_sh_regs_name[i], g_dp_regs[i]);
		if (ret < 0)
			return;
		words += ret;
		if ((DUMP_REGS_SIZE - words < REG_NAME_SIZE + LOG_EXTRA_NUM) || (i == (MAGIC - 1))) {
			buf[words] = '\n';
			buf[words + 1] = '\0';
			pr_info("%s", buf);
			words = 0;
		}
	}
#endif
}

static int get_dump_reason_idx(void)
{
	struct plat_cfg_str *smplat_scfg = get_plat_cfg_str();

	if (smplat_scfg == NULL)
		return (int)(ARRAY_SIZE(g_sh_reset_reasons) - 1);
	if (smplat_scfg->dump_config.reason >= (uint32_t)ARRAY_SIZE(g_sh_reset_reasons))
		return (int)(ARRAY_SIZE(g_sh_reset_reasons) - 1);
	else
		return smplat_scfg->dump_config.reason;
}

static int write_sh_dump_history(union dump_info dump_info)
{
	int ret = 0;
	char buf[HISTORY_LOG_SIZE];
	struct kstat historylog_stat;
	mm_segment_t old_fs;
	char local_path[PATH_MAXLEN];
	char date[DATATIME_MAXLEN] = {0};
	int print_ret = 0;
#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
	int print_loc = 0;
	union sctrl_sh_boot sh_boot;
#endif

	pr_info("%s: write sensorhub dump history file\n", __func__);
	ret = memset_s(date, DATATIME_MAXLEN, 0, DATATIME_MAXLEN);
	if (ret != EOK)
		pr_err("[%s]memset_s date fail[%d]\n", __func__, ret);
#ifdef CONFIG_DFX_BB
	ret = snprintf_s(date, DATATIME_MAXLEN, DATATIME_MAXLEN - 1, "%s-%08lld", rdr_get_timestamp(), rdr_get_tick());
	if (ret < 0)
		pr_err("[%s]sprintf_s date fail[%d]\n", __func__, ret);
#endif

	ret = memset_s(local_path, PATH_MAXLEN, 0, PATH_MAXLEN);
	if (ret != EOK)
		pr_err("[%s]memset_s local_path fail[%d]\n", __func__, ret);
	ret = sprintf_s(local_path, PATH_MAXLEN, "%s/%s", g_dump_restore_dir, "history.log");
	if (ret < 0)
		pr_err("[%s]sprintf_s local_path fail[%d]\n", __func__, ret);
	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501*/

	/* check file size */
	if (vfs_stat(local_path, &historylog_stat) == 0 && historylog_stat.size > HISTORY_LOG_MAX) {
		pr_info("truncate dump history log\n");
		do_unlinkat(AT_FDCWD, getname(local_path)); /* delete history.log */
	}

	set_fs(old_fs);
	/* write history file */
	ret = memset_s(buf, HISTORY_LOG_SIZE, 0, HISTORY_LOG_SIZE);
	if (ret != EOK)
		pr_err("[%s]memset_s local_path fail[%d]\n", __func__, ret);

	if (g_dp_regs != NULL) {
		print_ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "reason [%s], [%02u], time [%s], regs "
			"[pc-%x,lr-%x,fsr[c-%x,h-%x],far[m-%x,b-%x],peri-%x,ao-%x],dump_info[0x%x]\n",
			g_sh_reset_reasons[get_dump_reason_idx()], g_dump_index, date,
			g_dp_regs[EXP_PC], g_dp_regs[EXP_LR], g_dp_regs[CFSR],
			g_dp_regs[HFSR], g_dp_regs[MMFAR], g_dp_regs[BFAR],
			g_dp_regs[PERI_USED], g_dp_regs[AO_CNT], dump_info.value);
		if (print_ret < 0)
			pr_err("%s write dp regs history failed\n", __func__);
	} else {
		print_ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1,
			"reason [%s], [%02u], time [%s],dump_info[0x%x]",
			g_sh_reset_reasons[get_dump_reason_idx()], g_dump_index, date, dump_info.value);
		if (print_ret < 0)
			pr_err("%s write history failed\n", __func__);
	}
	/* write boot_step and boot_stat to history.log */
#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
	sh_boot = get_boot_stat();
	reset_boot_stat();
	if (print_ret > 0) {
		print_loc += print_ret;
		if (print_loc >= (int)(sizeof(buf) - 1)) {
			pr_err("%s write boot stat failed\n", __func__);
			goto save_buf;
		}
		print_ret = snprintf_s(buf + print_loc, sizeof(buf) - print_loc, sizeof(buf) - print_loc - 1,
				"boot_step [%u], boot_stat [%u]\n", sh_boot.reg.boot_step, sh_boot.reg.boot_stat);
		if (print_ret < 0)
			pr_err("[%s]sprintf_s boot_state fail[%d]\n", __func__, ret);
	}
save_buf:
#endif
	(void)sh_savebuf2fs(g_dump_restore_dir, "history.log", buf, strlen(buf), 1);
	return ret;
}

static int write_sh_dump_file(uint8_t * sensorhub_dump_buff)
{
	int ret;
	char date[DATATIME_MAXLEN];
	char path[PATH_MAXLEN];
	struct dump_zone_head *dzh = NULL;
	struct plat_cfg_str *smplat_scfg = get_plat_cfg_str();

	ret = memset_s(date, DATATIME_MAXLEN, 0, DATATIME_MAXLEN);
	if (ret != EOK)
		pr_err("[%s]memset_s data fail[%d]\n", __func__, ret);
#ifdef CONFIG_DFX_BB
	ret = snprintf_s(date, DATATIME_MAXLEN, DATATIME_MAXLEN - 1, "%s-%08lld", rdr_get_timestamp(), rdr_get_tick());
	if (ret < 0)
		pr_err("[%s]sprintf_s DATATIME_MAXLEN fail[%d]\n", __func__, ret);
#endif

	ret = memset_s(path, PATH_MAXLEN, 0, PATH_MAXLEN);
	if (ret != EOK)
		pr_err("[%s]memset_s path fail[%d]\n", __func__, ret);

	ret = sprintf_s(path, PATH_MAXLEN, "sensorhub-%02u.dmp", g_dump_index);
	if (ret < 0)
		pr_err("[%s]sprintf_s PATH_MAXLEN fail[%d]\n", __func__, ret);
	pr_info("%s: write sensorhub dump  file %s\n", __func__, path);
	pr_err("sensorhub recovery source is %s\n", g_sh_reset_reasons[get_dump_reason_idx()]);
#if (KERNEL_VERSION(4, 4, 0) > LINUX_VERSION_CODE)
	flush_cache_all();
#endif

	/* write share part */
	if ((sensorhub_dump_buff != NULL) && (smplat_scfg != NULL)) {
		dzh = (struct dump_zone_head *)sensorhub_dump_buff;
		sh_savebuf2fs(g_dump_restore_dir, path, sensorhub_dump_buff,
			min(smplat_scfg->dump_config.dump_size, dzh->len), 0);
	}

	return 0;
}

int save_sh_dump_file(uint8_t * sensorhub_dump_buff, union dump_info dump_info)
{
	char dump_fs[PATH_MAXLEN] = SH_DMP_FS;
	if (IS_ENABLE_DUMP == 0) {
		pr_info("%s skipped!\n", __func__);
		return 0;
	}
	if (sh_wait_fs(dump_fs) != 0) {
		pr_err("[%s] fs not ready!\n", __func__);
		return -1;
	}
	pr_info("%s fs ready\n", __func__);
	// check and create dump dir
	if (sh_create_dir(g_dump_restore_dir)) {
		pr_err("%s failed to create dir %s\n", __func__, g_dump_restore_dir);
		return -1;
	}

	if (sensorhub_dump_buff == NULL) {
		pr_err("%s sensorhub_dump_buff is null\n", __func__);
		return -1;
	}

	get_max_dump_cnt();
	get_sh_dump_regs(sensorhub_dump_buff);
	write_sh_dump_history(dump_info);
	write_sh_dump_file(sensorhub_dump_buff);
	return 0;
}
