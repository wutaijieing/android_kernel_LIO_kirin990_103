/* Copyright (c) 2013-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * MMC fault inject - add fault inject interface to the mmc.
 * This infrastructure can be used for debugging the driver from userspace.
 *
 */

#include <linux/random.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fault-inject.h>
#include <linux/mmc/emmc_fault_inject.h>
#include <linux/mmc/cmdq_hci.h>
#include <linux/mmc/host.h>
#include <sdhci.h>

#ifdef CONFIG_EMMC_FAULT_INJECT
#define DEFAULT_ERR_INDEX (0xFFFFFFFF)
#define BUFFER_SIZE 128

#define CQ_RED_ERR_INJ  (BIT(0))  /*cmdq response error*/
#define CQ_DATA_ERR_INJ (BIT(1)) /*cmdq data err*/
#define CQ_CMD_ERR_INJ  (BIT(2))  /*cmdq cmd err*/
#define CQ_ADMA_ERR_INJ (BIT(3)) /*adma or other err*/
#define CQ_RESET_INJ    (BIT(4)) /*reset when err*/
#define CQ_DISCARD_ERR_INJ	(BIT(5)) /*cmdq discard err*/

#define CQ_TIMEOUT_INJ  (BIT(0)) /*reset when err*/


struct fault_inject_files {
	struct dentry *debugfs_root;
	struct dentry *err_inj_scenario;
	struct dentry *err_inj_err_index;
	struct dentry *err_inj_stats;
	u32 err_inj_scenario_mask;
	struct fault_attr fail_attr;
};


static DECLARE_FAULT_ATTR(fail_default_attr);
static char *fail_request;
static struct fault_inject_files fault_inject_files;
module_param(fail_request, charp, S_IRUGO);

/**
 * struct mmcdbg_err_scenario - error scenario use case
 * @name: the name of the error scenario
 * @err_code_arr: error codes array for this error scenario
 * @num_err_codes: number of error codes in err_code_arr
 */
struct mmcdbg_err_scenario {
	const char *name;
	const int *err_code_arr;
	u32 num_err_codes;
	u32 err_code_index;
	u32 num_err_injected;
};

/*
 * the following static arrays are aggregation of possible errors
 * that might occur during the relevant error scenario
 */
static const int err_inject_legacy_intr_err_codes[] = {
	SDHCI_INT_TIMEOUT,
	SDHCI_INT_CRC,
	SDHCI_INT_END_BIT,
	SDHCI_INT_INDEX,

	SDHCI_INT_DATA_TIMEOUT,
	SDHCI_INT_DATA_CRC,
	SDHCI_INT_DATA_END_BIT,
	SDHCI_INT_ADMA_ERROR
};

static const int err_inject_cmdq_intr_err_codes[] = {
	CQ_RED_ERR_INJ,
	CQ_DATA_ERR_INJ,
	CQ_CMD_ERR_INJ,
	CQ_ADMA_ERR_INJ,
	CQ_RESET_INJ,
	CQ_DISCARD_ERR_INJ
};

static const int err_inject_cmdq_tmout_err_codes[] = {
	CQ_TIMEOUT_INJ,
};



static struct mmcdbg_err_scenario err_scen_arr[] = {
	{
		"ERR_LEGACY_INJECT_INTR",
		err_inject_legacy_intr_err_codes,
		ARRAY_SIZE(err_inject_legacy_intr_err_codes),
		DEFAULT_ERR_INDEX,
	},
	{
		"ERR_CMDQ_INTR_INJECT",
		err_inject_cmdq_intr_err_codes,
		ARRAY_SIZE(err_inject_cmdq_intr_err_codes),
		DEFAULT_ERR_INDEX,
	},
	{
		"ERR_CMDQ_TIMEOUT_INJECT",
		err_inject_cmdq_tmout_err_codes,
		ARRAY_SIZE(err_inject_cmdq_tmout_err_codes),
		DEFAULT_ERR_INDEX,
	},
};


#ifdef CONFIG_MMC_CQ_HCI
static bool
mmcdbg_cmdq_intr_fail_request(struct mmc_host *mmc, u32 *err_no)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	u32 tag = 0;
	u32 err_tag = 0;
	bool result = true;

	pr_info("%s: fault-inject error: 0x%x\n",
			__func__, *err_no);

	/*find a transfered tag*/
	for (tag = 0; tag < 32; tag++) {
		if (cq_host->mrq_slot[tag] != NULL)
			break;
		return false;
	}

	pr_info("err tag is 0x%x\n", tag);
	cq_host->inject_para->fake_dbl = 0;
	switch (*err_no) {
	case CQ_RESET_INJ:
		cq_host->inject_para->need_reset = true;
		/* just select a CQ_CMD_ERR_INJ scene to check reset process */
		cq_host->inject_para->status = 0;
		err_tag = (tag & 0x1f) << 8;
		cq_host->inject_para->err_info = (CQTERRI_RES_ERR | err_tag);
		break;

	case CQ_DATA_ERR_INJ:
		cq_host->inject_para->status = 0;
		if (31 == tag) {
			/*if dcmd, it can not be data err*/
			result = false;
			goto out;
		}
		err_tag = (tag & 0x1f) << 24;
		cq_host->inject_para->err_info = (CQTERRI_DAT_ERR | err_tag);

		break;

	case CQ_CMD_ERR_INJ:
		cq_host->inject_para->status = 0;
		err_tag = (tag & 0x1f) << 8;
		cq_host->inject_para->err_info = (CQTERRI_RES_ERR | err_tag);

		break;

	case CQ_RED_ERR_INJ:
		cq_host->inject_para->status = CQIS_RED;
		cq_host->inject_para->err_info = 0;
		cq_host->inject_para->fake_dbl = BIT(tag);

		break;

	case CQ_ADMA_ERR_INJ:
		cq_host->inject_para->status = 0;
		cq_host->inject_para->err_info = 0;
		cq_host->inject_para->fake_dbl = BIT(tag);
		break;

	case CQ_DISCARD_ERR_INJ:
		if (tag != 31 && cq_host->mrq_slot[31]
		   && (req_op(cq_host->mrq_slot[31]->req) == REQ_OP_DISCARD)) {
			pr_err("error inject: slots has discard request\n");
			cq_host->inject_para->status = 0;
			err_tag = (tag & 0x1f) << 8;
			cq_host->inject_para->err_info = (CQTERRI_RES_ERR | err_tag);
		} else {
			result = false;
			atomic_inc(&fault_inject_files.fail_attr.times);
		}

		break;

	default:
		dev_err(mmc_dev(mmc), "invalid fault interrupt");
		/* some configurations ignore panics caused by BUG() */
		break;
	}
out:
	if (result) {
		//if we want to inject fault, we need clear tcn and interrupt first
		cmdq_writel(cq_host, 0xffffffff, CQTCN);
		cmdq_writel(cq_host, 0xffffffff, CQIS);
	}
	return result;
}
#endif

static bool
mmcdbg_find_err_code(enum mmcdbg_err_inject_scenario usecase,
			 int *ret, u32 *index)
{
	struct mmcdbg_err_scenario *err_scen = &err_scen_arr[usecase];
	u32 err_code_index;

	if (!err_scen->num_err_codes)
		return false;

	if ((err_scen->err_code_index < err_scen->num_err_codes)) {
		err_code_index = err_scen->err_code_index;
	} else if (err_scen->err_code_index == DEFAULT_ERR_INDEX) {
		err_code_index = prandom_u32() % err_scen->num_err_codes;
	} else {
		pr_err("mmc fault inject invalid err code\n");
		return false;
	}
	*index = err_code_index;
	*ret = err_scen->err_code_arr[err_code_index];
	return true;
}

static bool mmcdbg_should_fail(struct mmc_host *mmc,
	enum mmcdbg_err_inject_scenario usecase, int *opt_ret,
	u32 *err_code_index)
{
	if (usecase >= ERR_INJECT_MAX_ERR_SCENARIOS) {
		dev_err(mmc->parent, "%s: invalid usecase value (%d)\n", __func__,
			usecase);
		return false;
	}

	if (!mmcdbg_find_err_code(usecase, opt_ret, err_code_index)) {
		 pr_err("fail to find err code\n");
		return false;
	}

	if (!should_fail(&fault_inject_files.fail_attr, 1))
		return false;

	return true;
}

bool mmcdbg_error_inject_dispatcher(struct mmc_host *mmc,
			enum mmcdbg_err_inject_scenario usecase,
			int success_value, int *ret_value, bool not_inj)
{
	int opt_ret = 0;
	u32 err_code_index = 0;
	bool inject = false;

#ifdef CONFIG_MMC_CQ_HCI
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
#endif

	if(likely(!(fault_inject_files.err_inj_scenario_mask &
						BIT(usecase))))
		return false;

	/* sanity check and verify error scenario bit */
	if ((unlikely(!mmc || !ret_value)) || not_inj) {
		return false;
	}

#ifdef CONFIG_MMC_CQ_HCI
	if (unlikely((!cq_host)||(!cq_host->inject_para))) {
		pr_err("cq_host or inject para is NULL\n");
		return false;
	}
#endif

	/* if an error already occurred/injected */
	if (*ret_value != success_value) {
		pr_err("*ret_val 0x%x, success_value 0x%x\n", *ret_value, success_value);
		return false;
	}

	if (!mmcdbg_should_fail(mmc, usecase, &opt_ret,
		    &err_code_index)) {
		goto out;
	}

	switch (usecase) {
	case ERR_INJECT_LEGACY_INTR:
		break;
#ifdef CONFIG_MMC_CQ_HCI
	case ERR_INJECT_CMDQ_INTR:
		if (!mmcdbg_cmdq_intr_fail_request(mmc, &opt_ret))
			goto out;
		break;
#endif
	default:
		dev_err(mmc_dev(mmc), "%s: unsupported error scenario\n", __func__);
		goto out;
	}

	inject = true;
	*ret_value |= opt_ret;
	err_scen_arr[usecase].num_err_injected++;  /*lint !e661*/
	dev_err(mmc_dev(mmc),
		"%s: Error Code: %d (0x%x), Index [%d], Scenario: \"%s\", ",
		__func__, *ret_value, *ret_value, err_code_index,
		err_scen_arr[usecase].name);  /*lint !e661*/
out:
	/**
	 * here it's guaranteed that ret_value has the correct value,
	 * whether it was assigned with a new value, or kept its own
	 * original incoming value
	 */
	return inject;
}

bool mmcdbg_cq_timeout_inj(struct mmc_request *mrq,
	enum mmcdbg_err_inject_scenario usecase)
{
	int opt_ret = 0;
	u32 err_code_index = 0;
	bool inject = false;

	if (likely(!(fault_inject_files.err_inj_scenario_mask &
						BIT(usecase))))
		return false;

	if (unlikely((!mrq) || (!mrq->host)))
		return false;

	if (!mmcdbg_should_fail(mrq->host, usecase, &opt_ret,
		    &err_code_index)) {
		goto out;
	}

	if((mrq->cmd && mrq->cmd->error) ||
		(mrq->data && mrq->data->error) ||
		(mrq->cmdq_req->resp_err)) {
		dev_err(mmc_dev(mrq->host),
			"%s already in err status.\n", __func__);
		goto out;
	}

	inject = true;
	err_scen_arr[usecase].num_err_injected++;  /*lint !e661*/
	dev_err(mmc_dev(mrq->host),
		"%s: Error Code: %d Index [%d], Scenario: \"%s\", ",
		__func__, opt_ret, err_code_index,
		err_scen_arr[usecase].name);  /*lint !e661*/
out:
	return inject;


}

void mmcdbg_cmdq_inj_fill_status(struct cmdq_host *cq_host, u32 *status)
{
	if (unlikely( (!cq_host)|| (!cq_host->inject_para)))
		return;
	*status = cq_host->inject_para->status;
}

void mmcdbg_cmdq_inj_fill_errinfo(struct cmdq_host *cq_host, u32 *errinfo)
{
	if (unlikely( (!cq_host)|| (!cq_host->inject_para)))
		return;
	*errinfo = cq_host->inject_para->err_info;
}

void mmcdbg_cmdq_inj_fake_dbl(struct cmdq_host *cq_host,
			u32 *dbl)
{
	if (unlikely( (!cq_host)|| (!cq_host->inject_para) ))
		return;
	*dbl |= cq_host->inject_para->fake_dbl;
}


bool mmcdbg_cmdq_inj_need_reset(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	if (unlikely( (!cq_host)|| (!cq_host->inject_para)))
		return false;
	return cq_host->inject_para->need_reset;
}

void mmcdbg_cmdq_inj_clear_reset(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	if (unlikely( (!cq_host)|| (!cq_host->inject_para)))
		return;
	cq_host->inject_para->need_reset = false;
}

bool mmc_is_reset(void)
{
	return g_mmc_reset_status;
}

static int mmcdbg_err_inj_scenario_read(struct seq_file *file, void *data)
{
	enum mmcdbg_err_inject_scenario err_case;

	seq_printf(file, "%-40s %-17s %-15s\n",
		   "Error Scenario:", "Bit[#]", "STATUS");

	for (err_case = ERR_INJECT_LEGACY_INTR;
		err_case < ERR_INJECT_MAX_ERR_SCENARIOS; err_case++)
		seq_printf(file, "%-40s 0x%-15lx %-15s\n",
			   err_scen_arr[err_case].name,
			   BIT(err_case),
			   fault_inject_files.err_inj_scenario_mask &
				BIT(err_case) ? "ENABLE" : "DISABLE");

	seq_printf(file, "bitwise of error scenario is 0x%x\n\n",
		   fault_inject_files.err_inj_scenario_mask);

	seq_puts(file, "usage example:\n");
	seq_puts(file, "echo 0x1 > /sys/kernel/debug/mmc_fault_inject/err_inj_scenario\n");
	seq_puts(file, "in order to enable ERR_INJECT_LEGACY_INTR\n");

	return 0;
}

static
int mmcdbg_err_inj_scenario_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			mmcdbg_err_inj_scenario_read, inode->i_private);
}

static ssize_t mmcdbg_err_inj_scenario_write(struct file *file,
					 const char __user *ubuf, size_t cnt,
					 loff_t *ppos)
{
	int ret;
	int err_scen = 0;


	ret = kstrtoint_from_user(ubuf, cnt, 0, &err_scen);
	if (ret) {
		pr_err("%s: Invalid argument\n", __func__);
		return ret;
	}

	fault_inject_files.err_inj_scenario_mask = err_scen;

	return cnt;
}

static const struct file_operations mmcdbg_err_inj_scenario_ops = {
	.open		= mmcdbg_err_inj_scenario_open,
	.read		= seq_read,
	.write		= mmcdbg_err_inj_scenario_write,
};

static int mmcdbg_err_inj_stats_read(struct seq_file *file, void *data)
{
	int err;

	seq_printf(file, "%-40s %-20s\n",
		   "Error Scenario:", "Num of Errors Injected");

	for (err = 0; err < ERR_INJECT_MAX_ERR_SCENARIOS; err++)
		seq_printf(file, "%-40s %-20d\n",
			err_scen_arr[err].name,
			err_scen_arr[err].num_err_injected);

	return 0;
}

static
int mmcdbg_err_inj_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			mmcdbg_err_inj_stats_read, inode->i_private);
}

static ssize_t mmcdbg_err_inj_stats_write(struct file *file,
					 const char __user *ubuf, size_t cnt,
					 loff_t *ppos)
{
	int err;

	for (err = 0; err < ERR_INJECT_MAX_ERR_SCENARIOS; err++)
		err_scen_arr[err].num_err_injected = 0;

	return cnt;
}

static const struct file_operations mmcdbg_err_inj_stats_ops = {
	.open		= mmcdbg_err_inj_stats_open,
	.read		= seq_read,
	.write		= mmcdbg_err_inj_stats_write,
};


static int mmcdbg_err_inj_err_index_read(struct seq_file *file, void *data)
{
	int scenario = 0;
	int ec_index = 0;

	seq_puts(file, "Selected Scenarios and Error Codes are as follows:\n\n");
	for (scenario = 0;
	     scenario < ERR_INJECT_MAX_ERR_SCENARIOS;
	     scenario++) {
		ec_index = err_scen_arr[scenario].err_code_index;
		if ((ec_index >= 0) &&
		    (ec_index < err_scen_arr[scenario].num_err_codes)) /*lint !e574*/
			seq_printf(file, "Scenario: %-35s(Index: %d), Error Code: %d(Index: %d)\n",
				err_scen_arr[scenario].name,
				scenario,
				err_scen_arr[scenario].err_code_arr[ec_index],
				ec_index);
	}
	seq_puts(file,
		"\n\nUsage:\n"
		"To select a specific fault, write 'S E' to err_inj_index, where:\n"
		" S - scenario index\n"
		" E - error code index of the scenario\n"
		"     All available error code index:\n");
	for (scenario = 0;
	     scenario < ERR_INJECT_MAX_ERR_SCENARIOS;
	     scenario++)
		seq_printf(file, "     Scenario: %-35s(Index: %d), Error Code Index Range: [0, %d]\n",
			err_scen_arr[scenario].name,
			scenario,
			err_scen_arr[scenario].num_err_codes - 1);

	seq_puts(file, "\nTo select a random fault, write 'S 0xFFFFFFFF' to "
		       "err_inj_index:\n\n");

	seq_puts(file, "usage example:\n");
	seq_puts(file, "echo 0x1 > /sys/kernel/debug/mmc_fault_inject/err_inj_scenario\n");
	seq_puts(file, "echo 0 1 > /sys/kernel/debug/mmc_fault_inject/err_inj_index\n");
	seq_puts(file, "to enable SDHCI_INT_TIMEOUT in scenario ERR_INJECT_LEGACY_INTR\n");
	return 0;
}

static
int mmcdbg_err_inj_err_index_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			mmcdbg_err_inj_err_index_read, inode->i_private);
}

static ssize_t mmcdbg_err_inj_err_index_write(struct file *file,
					 const char __user *ubuf, size_t cnt,
					 loff_t *ppos)
{
	int ret;
	int scenario = 0;
	int err_code_index = 0;
	char err_index_str[BUFFER_SIZE] = { 0 };
	loff_t buff_pos = 0;

	ret = simple_write_to_buffer(err_index_str, BUFFER_SIZE,
					 &buff_pos, ubuf, cnt);
	ret = sscanf(err_index_str, "%d %d", &scenario, &err_code_index);

	if ((scenario < 0) ||
	    (scenario >= ERR_INJECT_MAX_ERR_SCENARIOS)) {
		pr_err("%s: Invalid scenario: scenario=%d, valid range: [0,%d)\n",
			__func__,
			scenario,
			ERR_INJECT_MAX_ERR_SCENARIOS);
		return cnt;
	}
	if ((err_code_index < 0) ||
	    (err_code_index >= err_scen_arr[scenario].num_err_codes)) { /*lint !e574*/
		pr_info("%s: Invalid err_code_index: err_code_index=%d, valid range: [0,%d)\n",
			__func__,
			err_code_index,
			err_scen_arr[scenario].num_err_codes);
		pr_info("%s: Err_code_index if secenario %d is set to 0x%x\n",
			__func__, scenario, DEFAULT_ERR_INDEX);
		err_scen_arr[scenario].err_code_index = DEFAULT_ERR_INDEX;
	} else {
		err_scen_arr[scenario].err_code_index = err_code_index;
	}
	return cnt;
}

static const struct file_operations mmcdbg_err_inj_err_index_ops = {
	.open		= mmcdbg_err_inj_err_index_open,
	.read		= seq_read,
	.write		= mmcdbg_err_inj_err_index_write,
};
static void mmcdbg_setup_fault_injection(void)
{
	struct dentry *fault_dir;

	fault_inject_files.fail_attr = fail_default_attr;

	if (fail_request)
		setup_fault_attr(&fault_inject_files.fail_attr, fail_request);

	/* suppress dump stack every time failure is injected */
	fault_inject_files.fail_attr.verbose = 0;

	fault_dir = fault_create_debugfs_attr("inject_fault",
					fault_inject_files.debugfs_root,
					&fault_inject_files.fail_attr);

	if (IS_ERR(fault_dir)) {
		pr_err("%s: failed to create debugfs entry for fault injection\n",
			__func__);
		return;
	}

	fault_inject_files.err_inj_scenario =
		debugfs_create_file("err_inj_scenario",
				   S_IRUGO | S_IWUGO,
				   fault_inject_files.debugfs_root, NULL,
				   &mmcdbg_err_inj_scenario_ops);

	if (!fault_inject_files.err_inj_scenario) {
		pr_err("%s: Could not create debugfs entry for err_scenario",
				__func__);
		goto fail_err_inj_scenario;
	}

	fault_inject_files.err_inj_stats =
		debugfs_create_file("err_inj_stats", S_IRUSR | S_IWUSR,
					fault_inject_files.debugfs_root, NULL,
					&mmcdbg_err_inj_stats_ops);
	if (!fault_inject_files.err_inj_stats) {
		pr_err("%s:  failed create err_inj_stats debugfs entry\n",
			__func__);
		goto fail_err_inj_stats;
	}

	fault_inject_files.err_inj_err_index =
		debugfs_create_file("err_inj_index", S_IRUSR | S_IWUSR,
					fault_inject_files.debugfs_root, NULL,
					&mmcdbg_err_inj_err_index_ops);
	if (!fault_inject_files.err_inj_err_index) {
		pr_err("%s:  failed create err_inj_index debugfs entry\n",
			__func__);
		goto fail_err_inj_err_index;
	}
	return;

fail_err_inj_err_index:
	debugfs_remove(fault_inject_files.err_inj_stats);
fail_err_inj_stats:
	debugfs_remove(fault_inject_files.err_inj_scenario);
fail_err_inj_scenario:
	debugfs_remove_recursive(fault_dir);
}

void mmc_fault_inject_fs_setup(void)
{
	if (NULL == fault_inject_files.debugfs_root)
		fault_inject_files.debugfs_root
			= debugfs_create_dir("mmc_fault_inject", NULL);
	else
		return;
	if (IS_ERR(fault_inject_files.debugfs_root))
		/* Don't complain -- debugfs just isn't enabled */
		goto err_no_root;
	if (!fault_inject_files.debugfs_root) {
		/*
		 * Complain -- debugfs is enabled, but it failed to
		 * create the directory
		 */
		pr_err("%s: NULL debugfs root directory, exiting", __func__);
		goto err_no_root;
	}

	mmcdbg_setup_fault_injection();

	return;

err_no_root:
	pr_err("%s: failed to initialize debugfs\n", __func__);
}

void mmc_fault_inject_fs_remove(void)
{
	debugfs_remove_recursive(fault_inject_files.debugfs_root);
}

#endif /* End of CONFIG_MMC_FAULT_INJECT */
