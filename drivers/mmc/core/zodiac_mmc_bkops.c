/*
 * zodiac emmc reset read write bkops
 * Copyright (c) Zodiac Technologies Co., Ltd. 2016-2019. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/blkdev.h>
#include <linux/mas_bkops_core.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/version.h>

#include "card.h"
#include "core.h"
#include "mmc_ops.h"
#include "queue.h"

#define MMC_BKOPS_FLAG BKOPS_CHK_TIME_INTERVAL
#define BKOPS_STATUS_MAX_MMC 4
#define MICRON_2D_CID_MASK_LEN 9
#define MMC_CONVERT_CID_BUFFER_SIZE 12

static int mmc_bkops_status_query(void *bkops_data, u32 *status)
{
	int ret = 0;
	struct mmc_card *card = bkops_data;
#ifdef CONFIG_MAS_DEBUG_FS
	struct mas_bkops *bkops = card->mmc_bkops;
#endif

	if (!card)
		rdr_syserr_process_for_ap((u32)MODID_AP_S_PANIC_STORAGE, 0ull, 0ull);

	mmc_get_card(card, NULL);

	if (mmc_card_suspended(card->host->card))
		goto put_card;

	ret = mmc_read_bkops_status(card);
#ifdef CONFIG_MAS_DEBUG_FS
	if (bkops->bkops_debug_ops.sim_critical_bkops) {
		card->ext_csd.raw_bkops_status = EXT_CSD_BKOPS_LEVEL_2;
		bkops->bkops_debug_ops.sim_critical_bkops = 0;
	}
	if (bkops->bkops_debug_ops.sim_bkops_query_fail) {
		pr_err("simulate bkops query failure\n");
		bkops->bkops_debug_ops.sim_bkops_query_fail = false;
		ret = -EAGAIN;
	}
#endif
	if (ret) {
		pr_err("%s: Failed to read bkops status: %d\n", mmc_hostname(card->host), ret);
#ifdef CONFIG_MAS_DEBUG_FS
		rdr_syserr_process_for_ap((u32)MODID_AP_S_PANIC_STORAGE, 0ull, 0ull);
#else
		*status = 0;
		goto put_card;
#endif
	}

	*status = card->ext_csd.raw_bkops_status;

put_card:
	mmc_put_card(card, NULL);

	return ret;
}

int mmc_start_bkops(struct mmc_card *card, bool from_exception)
{
	int err;
#ifdef CONFIG_MAS_DEBUG_FS
	struct mas_bkops *bkops = card->mmc_bkops;
#endif

	if (!card->ext_csd.man_bkops_en || mmc_card_doing_bkops(card))
		return 0;

	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BKOPS_START, 1, 0, 0, false, true);

#ifdef CONFIG_MAS_DEBUG_FS
	if (bkops->bkops_debug_ops.sim_bkops_start_fail) {
		pr_err("simulate bkops start failure\n");
		bkops->bkops_debug_ops.sim_bkops_start_fail = false;
		err = -EAGAIN;
	}
#endif
	if (err) {
		pr_err("%s: Error %d starting bkops\n", mmc_hostname(card->host), err);
#ifdef CONFIG_MAS_DEBUG_FS
		mmc_process_ap_err(card);
		bkops->bkops_stats.bkops_start_fail_count++;
#else
		goto out;
#endif
	}
	mmc_card_set_doing_bkops(card);
#ifdef CONFIG_MAS_DEBUG_FS
	pr_info("BKOPS started\n");
	if (bkops->bkops_debug_ops.sim_bkops_abort) {
		err = mmc_stop_bkops(card);
		if (err) {
			pr_err("mmc bkops stop failure\n");
			goto out;
		}
	}
#endif

out:
	return err;
}

static void zodiac_mmc_send_hpi(struct mmc_card *card)
{
	int err;
	int retry_count = 4;
	struct mmc_host *host = card->host;
#ifdef CONFIG_MAS_DEBUG_FS
	struct mas_bkops *bkops = card->mmc_bkops;
	struct bkops_debug_ops *bkops_debug_ops_p = &(bkops->bkops_debug_ops);
#endif

#ifdef CONFIG_MAS_DEBUG_FS
	if (bkops_debug_ops_p->sim_bkops_stop_fail) {
		pr_err("simulate bkops stop failure\n");
		err = -EAGAIN;
		goto sim_bkops_stop_fail;
	}
#endif

	do {
		err = mmc_interrupt_hpi(card);
		if (!err || (err == -EINVAL))
			break;
	} while (retry_count--);
#ifdef CONFIG_MAS_DEBUG_FS
sim_bkops_stop_fail:
#endif
	if ((retry_count <= 0) || (err && (err != -EINVAL))) {
		pr_err("%s %d HPI failed! do zodiac_mmc_reset\n", __func__, __LINE__);
#ifdef CONFIG_MAS_DEBUG_FS
		if (!bkops_debug_ops_p->sim_bkops_stop_fail)
			dump_stack();

		bkops->bkops_stats.bkops_stop_fail_count++;
#endif
		if (!(host->bus_ops->reset) || host->bus_ops->reset(host)) {
			pr_err("%s %d zodiac_mmc_reset Failed\n", __func__, __LINE__);
			dump_stack();
		}
	}
}

/*
 * mmc_stop_bkops - stop ongoing BKOPS
 * @card: MMC card to check BKOPS
 *
 * Send HPI command to stop ongoing background operations to
 * allow rapid servicing of foreground operations, e.g. read/
 * writes. Wait until the card comes out of the programming state
 * to avoid errors in servicing read/write requests.
 */
int mmc_stop_bkops(struct mmc_card *card)
{
#ifdef CONFIG_MAS_DEBUG_FS
	struct mmc_host *host = card->host;
	u64 start_time;
	u64 stop_time;
	u64 time_interval;
	struct mas_bkops *bkops = card->mmc_bkops;
	struct bkops_stats *bkops_stats_p = &(bkops->bkops_stats);
	struct bkops_debug_ops *bkops_debug_ops_p = &(bkops->bkops_debug_ops);

	start_time = ktime_get_ns();
#endif
	if (!card->ext_csd.man_bkops_en || !mmc_card_doing_bkops(card))
		return 0;

#ifdef CONFIG_MAS_DEBUG_FS
	if (bkops_debug_ops_p->skip_bkops_stop) {
		pr_err("stop bkops was skipped\n");
		bkops_debug_ops_p->skip_bkops_stop = false;
		mmc_card_clr_doing_bkops(card);
		return 0;
	}

	if ((host->ops->card_busy && host->ops->card_busy(host)) ||
		bkops->bkops_debug_ops.sim_bkops_abort) {
		bkops_stats_p->bkops_abort_count++;
		pr_err("%s ongoing bkops aborted\n", __func__);
	}
#endif

	zodiac_mmc_send_hpi(card);
	mmc_card_clr_doing_bkops(card);
#ifdef CONFIG_MAS_DEBUG_FS
	bkops_stats_p->bkops_stop_count++;
	mas_bkops_update_dur(bkops_stats_p);
	if (bkops_debug_ops_p->sim_bkops_stop_delay) {
		pr_err("simulate bkops stop delay %ums\n", bkops_debug_ops_p->sim_bkops_stop_delay);
		msleep(bkops_debug_ops_p->sim_bkops_stop_delay);
	}

	pr_info("BKOPS stoped\n");
	stop_time = ktime_get_ns();
	time_interval = stop_time - start_time;
	if (time_interval > bkops_stats_p->bkops_max_stop_time)
		bkops_stats_p->bkops_max_stop_time = time_interval;
	bkops_stats_p->bkops_avrg_stop_time = ((bkops_stats_p->bkops_avrg_stop_time *
		(bkops_stats_p->bkops_stop_count - 1)) + time_interval) /
		bkops_stats_p->bkops_stop_count;
#endif

	/* stop BKOPS can't fail, we always return 0 */
	return 0;
}

static int mmc_bkops_start_stop(void *bkops_data, int start)
{
	int ret;
	struct mmc_card *card = (struct mmc_card *)bkops_data;

	if (!card)
		rdr_syserr_process_for_ap((u32)MODID_AP_S_PANIC_STORAGE, 0ull, 0ull);

	mmc_get_card(card, NULL);
	if (mmc_card_suspended(card->host->card)) {
		ret = -EAGAIN;
		goto put_card;
	}
	if (start == BKOPS_STOP) {
		ret = mmc_stop_bkops(card);
	} else if (start == BKOPS_START) {
		ret = mmc_start_bkops(card, false);
	} else {
		pr_err("%s Invalid start value: %d\n", __func__, start);
		ret = -EINVAL;
	}
put_card:
	mmc_put_card(card, NULL);

	return ret;
}

struct bkops_ops mmc_bkops_ops = {
	.bkops_start_stop = mmc_bkops_start_stop,
	.bkops_status_query = mmc_bkops_status_query,
};

/*
 * first 9 Bytes of CID of Micron 2D eMMC that needs BKOPS
 * 0x13014e51334a393756
 * 0x13014e51334a393656
 * 0x13014e51334a393652
 */
static u8 micron_mmc_bkops_cid1[MICRON_2D_CID_MASK_LEN] = {
	0x13, 0x01, 0x4e, 0x51, 0x33, 0x4a, 0x39, 0x37, 0x56
};
static u8 micron_mmc_bkops_cid2[MICRON_2D_CID_MASK_LEN] = {
	0x13, 0x01, 0x4e, 0x51, 0x33, 0x4a, 0x39, 0x36, 0x56
};
static u8 micron_mmc_bkops_cid3[MICRON_2D_CID_MASK_LEN] = {
	0x13, 0x01, 0x4e, 0x51, 0x33, 0x4a, 0x39, 0x36, 0x52
};

bool zodiac_mmc_is_bkops_needed(struct mmc_card *card)
{
	char converted_cid[MMC_CONVERT_CID_BUFFER_SIZE];

	/* convert ending */
	((int *)converted_cid)[0] = be32_to_cpu(card->raw_cid[0]);
	((int *)converted_cid)[1] = be32_to_cpu(card->raw_cid[1]);
	((int *)converted_cid)[2] = be32_to_cpu(card->raw_cid[2]);

	return !memcmp(converted_cid, micron_mmc_bkops_cid1, MICRON_2D_CID_MASK_LEN) ||
	       !memcmp(converted_cid, micron_mmc_bkops_cid2, MICRON_2D_CID_MASK_LEN) ||
	       !memcmp(converted_cid, micron_mmc_bkops_cid3, MICRON_2D_CID_MASK_LEN);
}

#ifdef CONFIG_MAS_DEBUG_FS
static const char *bkops_status_str_mmc[BKOPS_STATUS_MAX_MMC] = {
	"bkops none", "bkops non critical",
	"bkops perf impacted", "bkops critical",
};
#endif
int zodiac_mmc_manual_bkops_config(struct request_queue *q)
{
	struct mmc_queue *mq = q->queuedata;
	struct mmc_card *card = mq->card;
	struct mas_bkops *mmc_bkops = NULL;

	mmc_bkops = mas_bkops_alloc();
	if (!mmc_bkops)
		return -ENOMEM;

	mmc_bkops->dev_type = BKOPS_DEV_MMC;
	mmc_bkops->bkops_ops = &mmc_bkops_ops;
	mmc_bkops->bkops_data = card;
	mmc_bkops->bkops_flag |= MMC_BKOPS_FLAG;
	mmc_bkops->bkops_check_interval = BKOPS_DEF_CHECK_INTERVAL;
	card->mmc_bkops = mmc_bkops;
#ifdef CONFIG_MAS_DEBUG_FS
	mas_bkops_set_status_str(mmc_bkops, BKOPS_STATUS_MAX_MMC, bkops_status_str_mmc);
#endif
	if (mas_bkops_enable(q, mmc_bkops, card->debugfs_root))
		goto free_mmc_bkops;

	return 0;
free_mmc_bkops:
	kfree(mmc_bkops);
	return -EAGAIN;
}
