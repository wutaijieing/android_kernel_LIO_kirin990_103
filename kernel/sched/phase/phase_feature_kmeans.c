/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature_kmeans.c - arm64-specific Kernel Phase-Algorithm
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/phase.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/build_bug.h>
#include <../kernel/sched/sched.h>
#include "phase_feature.h"

#define CREATE_TRACE_POINTS
#include <trace/events/km_cluster.h>

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
#endif

#define SUM_LEN			16

/* Helpers for Phase Decision Tree Algorithm interface */
#define MODEL_FORWARD_PATH "/mnt/FG_KMEANS_FORWARD.cfg"
#define MODEL_REVERSE_PATH "/mnt/FG_KMEANS_REVERSE.cfg"
#define NR_MAX_PMU_EVENTS	5

/*
 * struct kmeans_lsvm layout:
 *       ---------------------------------------------
 *       |   int type                                |
 *       ---------------------------------------------
 *       |   int nr_cluster                          |
 *       ---------------------------------------------
 *       |   int dim_cluster                         |
 *       ---------------------------------------------
 *       |   int nr_lsvm                             |
 *       ---------------------------------------------
 *       |   int dim_lsvm                            |
 *       ---------------------------------------------
 *       |   int nr_threshold                        |
 *       ---------------------------------------------
 *       |   int data_scaler                         |
 *       ---------------------------------------------
 *       |   int model_scaler                        |
 *       ---------------------------------------------
 *       |   int *buffer_scaler                      |
 *       ---------------------------------------------
 *       |   u64* buffer                             |
 *       ---------------------------------------------
 *       |   int **cluster2dim                       |
 *       ---------------------------------------------
 *       |   struct lsvm **lsvm                      |
 *       ---------------------------------------------
 *       |   buffer_scaller[dim_cluster]             |
 *       ---------------------------------------------
 *       |   buffer[dim_lsvm]                        |
 *       ---------------------------------------------
 *       |   cluster2dim[0]                          |
 *       ---------------------------------------------
 *       |     ...                                   |
 *       ---------------------------------------------
 *       |   cluster2dim[nr_cluster-1]               |
 *       ---------------------------------------------
 *       |   cluster2dim[0][dim_cluster]             |
 *       ---------------------------------------------
 *       |    ...                                    |
 *       ---------------------------------------------
 *       |   cluster2dim[nr_cluster-1][dim_cluster-1]|
 *       ---------------------------------------------
 *       |                                           |
 *       |    ...                                    |
 *       |    struct lsvm                            |
 *       |    ...                                    |
 *       |                                           |
 *       |--------------------------------------------
 */
struct lsvm {
	int gain;
	int **lsvm2dim; /* 2-dims [nr_cluster][dim_lsvm] */
	int lower;
	int upper;
	int threshold;
};

struct kmeans_lsvm {
	int type;
	int nr_cluster;
	int dim_cluster;
	int nr_lsvm;
	int dim_lsvm;
	int nr_threshold;
	int data_scaler;
	int model_scaler;
	int *buffer_scaler; /* 1-dims [dim_cluster] */
	u64 *buffer;        /* 1-dims [dim_lsvm] */
	int **cluster2dim;  /* 2-dims [nr_cluster][dim_cluster] */
	struct lsvm **lsvm; /* lsvm[nr_lsvm] */
};

#define PHASE_EVENT_LIST		\
	EVENT(CYCLES),			\
	EVENT(INST_RETIRED),		\
	EVENT(BR_MIS_PRED),		\
	EVENT(ROB_FLUSH),		\
	EVENT(OP_SPEC),			\
	EVENT(DSP_STALL),		\
	EVENT(L1D_CACHE),		\
	EVENT(IF_RSURC_CHK_OK)

enum {
#define EVENT(e) e##_INDEX
	PHASE_EVENT_LIST
#undef EVENT
};

static int kmeans_pevents[PHASE_PEVENT_NUM] = {
#define EVENT(e) e
	PHASE_EVENT_LIST,
#undef EVENT
	PHASE_EVENT_FINAL_TERMINATOR,
};

static atomic_t kmeans_lsvm_created = ATOMIC_INIT(0);
static struct kmeans_lsvm *pkmeans_lsvm[NR_PREDICT_MODE];

static int build_buffer_scaler(struct kmeans_lsvm *kmeans_lsvm,
			       char *buf, s64 *pos, loff_t size)
{
	/* get buffer scaler */
	kmeans_lsvm->buffer_scaler = (int *)((char *)kmeans_lsvm + sizeof(struct kmeans_lsvm));
	if (parse_one_line(kmeans_lsvm->buffer_scaler, buf + *pos, kmeans_lsvm->dim_cluster) != 0)
		return -EINVAL;

	consume_line(buf, pos, size);
	return 0;
}

static int build_cluster(struct kmeans_lsvm *kmeans_lsvm, char *buf, s64 *pos, loff_t size)
{
	int i;
	int nr_cluster = kmeans_lsvm->nr_cluster;
	int dim_cluster = kmeans_lsvm->dim_cluster;

	/* get kmeans cluster */
	kmeans_lsvm->cluster2dim = (int **)((char *)kmeans_lsvm->buffer + sizeof(u64) * kmeans_lsvm->dim_lsvm);
	/* cluster2dim[0...nr_cluster] */
	for (i = 0; i < kmeans_lsvm->nr_cluster; i++)
		kmeans_lsvm->cluster2dim[i] = (int *)((char *)kmeans_lsvm->cluster2dim +
						      nr_cluster * sizeof(int *) +
						      i * dim_cluster * sizeof(int));

	for (i = 0; i < kmeans_lsvm->nr_cluster; i++) {
		if (parse_one_line(kmeans_lsvm->cluster2dim[i], buf + *pos,
				   kmeans_lsvm->dim_cluster) != 0) {
			pr_err("parse cluster2dim %d column error\n", i);
			return -EINVAL;
		}
		consume_line(buf, pos, size);
	}

	return 0;
}

/*
 * struct lsvm data struct as fellow:
 *           |-------------------------------------------|
 *           |  struct lsmv ** lsvm                      |-----------------------`
 *           |-------------------------------------------|<----------------------`
 *           |  struct lsvm *lsmv0                       |-----------------------`
 *           |-------------------------------------------|                       `
 *           |  struct lsvm *lsmv1                       |                       `
 *           |-------------------------------------------|                       `
 *           |        ...                                |                       `
 *           |-------------------------------------------|                       `
 *     ------| struct lsvm *lsmv[nr_lsvm-1]              |                       `
 *     `     |-------------------------------------------|                       `
 *     `     |  int gain                                 |                       `
 *     `     |  int **lsvm2dim [nr_cluster][dim_lsvm]+++ |                       `
 *     `     |  int lower                              + |                       `
 *     `     |  int upper                              + |                       `
 *     `     |  int threshold                          + |                       `
 *     `     |-----------------------------------------+-|                       `
 *     `     |  int *lsvm2dim0 <++++++++++++++++++++++++ |                       `
 *     `     |-------------------------------------------|                       `
 *     `     |  int *lsvm2dim1                           |  struct lsvm lsvm0<---`
 *     `     |-------------------------------------------|
 *     `     |       ...                                 |
 *     `     |-------------------------------------------|
 *     `     |  int *lsvm2dim[nr_cluster-1]              |
 *     `     |-------------------------------------------|
 *     `     |  int lsvm2dim[0][dim_lsvm]                |
 *     `     |-------------------------------------------|
 *     `     |  int lsvm2dim[1][dim_lsvm]                |
 *     `     |-------------------------------------------|
 *     `     |       ...                                 |
 *     `     |-------------------------------------------|
 *     `     |  int lsvm2dim[nr_cluster-1][dim_lsvm]     |
 *     `     |-------------------------------------------|
 *     `     |                                           |
 *     `     |                                           |
 *     `     |                                           |
 *     `     |             ...                           |
 *     `     |                                           |
 *     `     |                                           |
 *     `     |                                           |
 *     `     |-------------------------------------------|
 *     `     |  int gain                                 |
 *     `     |  int **lsvm2dim                           |
 *     `     |  int lower                                |
 *     `     |  int upper                                |
 *     `     |  int threshold                            |
 *     `     |-------------------------------------------|
 *     `     |  int *lsvm2dim0                           |
 *     `     |-------------------------------------------|
 *     `     |  int *lsvm2dim1                           |
 *     `     |-------------------------------------------|
 *     `     |       ...                                 |
 *     `---->|-------------------------------------------|  struct lsvm lsvm[nr_lsvm-1]
 *           |  int *lsvm2dim[nr_cluster-1]              |
 *           |-------------------------------------------|
 *           |  int lsvm2dim[0][dim_lsvm]                |
 *           |-------------------------------------------|
 *           |  int lsvm2dim[1][dim_lsvm]                |
 *           |-------------------------------------------|
 *           |       ...                                 |
 *           |-------------------------------------------|
 *           |  int lsvm2dim[nr_cluster-1][dim_lsvm]     |
 *           |-------------------------------------------|
 */

static int build_lsvm(struct kmeans_lsvm *kmeans_lsvm, char *buf, s64 *pos, loff_t size)
{
	int i, j;
	int nr_lsvm = kmeans_lsvm->nr_lsvm;
	int nr_cluster = kmeans_lsvm->nr_cluster;
	int dim_lsvm = kmeans_lsvm->dim_lsvm;
	void *cluster2dim_end = &(kmeans_lsvm->cluster2dim[nr_cluster - 1][dim_lsvm]);
	struct lsvm *plsvm = NULL;

	/* struct lsvm->lsvm */
	kmeans_lsvm->lsvm = (struct lsvm **)cluster2dim_end;
	/* struct lsvm->lsvm[0...nr_lsvm] */
	for (i = 0; i < nr_lsvm; i++)
		kmeans_lsvm->lsvm[i] = (struct lsvm *)((char *)kmeans_lsvm->lsvm +
				       nr_lsvm * sizeof(struct lsvm *) + /* nr_lsvm * pointer */
				       (i * (sizeof(struct lsvm) + nr_cluster * sizeof(int *) + nr_cluster * dim_lsvm * sizeof(int))));

	/* get lsvm */
	for (i = 0; i < nr_lsvm; i++) {
		plsvm = kmeans_lsvm->lsvm[i];
		get_matches(buf + *pos, "%d", &plsvm->gain);
		consume_line(buf, pos, size);
		/* get lsvm[nr_cluster][dim_lsvm] */
		plsvm->lsvm2dim = (int **)((char *)plsvm + sizeof(struct lsvm));
		for (j = 0; j < nr_cluster; j++) {
			plsvm->lsvm2dim[j] = (int *)((char *)plsvm->lsvm2dim + nr_cluster * sizeof(int *) + j * dim_lsvm * sizeof(int));
			if (parse_one_line(plsvm->lsvm2dim[j], buf + *pos, dim_lsvm) != 0) {
				pr_err("parse lsvm %d lsvm2dim %d column error\n", i, j);
				return -EINVAL;
			}
			consume_line(buf, pos, size);
		}
		/* get lower, upper, threshold */
		get_matches(buf + *pos, "%d %d %d", &plsvm->lower, &plsvm->upper, &plsvm->threshold);
		consume_line(buf, pos, size);
	}

	return 0;
}

static void print_kmeans_lsvm(struct kmeans_lsvm *kmeans_lsvm)
{
	int i, j, k;
	struct lsvm *plsvm = NULL;

	printk("type %d, nr_cluster %d, dim_cluster %d, nr_lsvm %d, dim_lsvm %d, "
	       "nr_threshold %d, data_scaler %d, model_scaler %d\n",
	       kmeans_lsvm->type, kmeans_lsvm->nr_cluster, kmeans_lsvm->dim_cluster,
	       kmeans_lsvm->nr_lsvm, kmeans_lsvm->dim_lsvm, kmeans_lsvm->nr_threshold,
	       kmeans_lsvm->data_scaler, kmeans_lsvm->model_scaler);
	printk("kmeans_lsvm buffer_scaler:\n");
	for (i = 0; i < kmeans_lsvm->dim_lsvm; i++)
		printk("%6d", kmeans_lsvm->buffer_scaler[i]);
	printk("\n");
	printk("kmeans_lsvm cluster:\n");
	for (i = 0; i < kmeans_lsvm->nr_cluster; i++) {
		printk("%2d:", i);
		for (j = 0; j < kmeans_lsvm->dim_cluster; j++)
			printk("%6d", kmeans_lsvm->cluster2dim[i][j]);
		printk("\n");
	}

	printk("kmeans_lsvm lsvm:\n");
	for (i = 0; i < kmeans_lsvm->nr_lsvm; i++) {
		plsvm = kmeans_lsvm->lsvm[i];
		printk("lsmv %2d: gain %d lower %d upper %d threshold %d\n",
		       i, plsvm->gain, plsvm->lower, plsvm->upper, plsvm->threshold);

		for (j = 0; j < kmeans_lsvm->nr_cluster; j++) {
			int **lm = plsvm->lsvm2dim;

			printk("lsvm %2d col: %2d:", i, j);
			for (k = 0; k < kmeans_lsvm->dim_lsvm; k++)
				printk("\t%6d", lm[j][k]);
			printk("\n");
		}
	}
}

static int build_kmeans_lsvm_from_file(struct kmeans_lsvm *kmeans_lsvm,
				       const char *model_file_path)
{
	int ret;
	char *buf = NULL;
	loff_t pos = 0;
	loff_t size = 0;

	buf = load_from_file(model_file_path, &size);
	if (buf == NULL)
		return -ENOMEM;

	/* get model mode */
	get_matches(buf + pos, "%d %d %d %d %d %d %d %d\n",
		    &kmeans_lsvm->type, &kmeans_lsvm->nr_cluster, &kmeans_lsvm->dim_cluster,
		    &kmeans_lsvm->nr_lsvm, &kmeans_lsvm->dim_lsvm, &kmeans_lsvm->nr_threshold,
		    &kmeans_lsvm->data_scaler, &kmeans_lsvm->model_scaler);
	consume_line(buf, &pos, size);

	ret = build_buffer_scaler(kmeans_lsvm, buf, &pos, size);
	if (ret != 0) {
		pr_err("build kmeans_lsvm->buffer_scaler fail\n");
		goto out_free_buf;
	}

	kmeans_lsvm->buffer = (u64 *)((char *)kmeans_lsvm->buffer_scaler + sizeof(int) * kmeans_lsvm->dim_cluster);
	ret = build_cluster(kmeans_lsvm, buf, &pos, size);
	if (ret != 0) {
		pr_err("build kmeans_lsvm->cluster fail\n");
		goto out_free_buf;
	}

	ret = build_lsvm(kmeans_lsvm, buf, &pos, size);
	if (ret != 0) {
		pr_err("build kmeans_lsvm->lsvm fail\n");
		goto out_free_buf;
	}

	print_kmeans_lsvm(kmeans_lsvm);

out_free_buf:
	kfree(buf);

	return ret;
}

static int make_decision(struct kmeans_lsvm *kmeans_lsvm, u64 *buffer)
{
	int i, j;
	int lsvm_col = -1;
	u64 fdis, fdis_min;
	struct lsvm *plsvm = NULL;
	long long sum[SUM_LEN];

	/* calc distance */
	fdis_min = 0xFFFFFFFFFFFFFFFF;
	for (i = 0; i < kmeans_lsvm->nr_cluster; i++) {
		fdis = 0;
		for (j = 0; j < kmeans_lsvm->dim_cluster; j++)
			fdis += ((int)(buffer[j]) - kmeans_lsvm->cluster2dim[i][j]) *
				((int)(buffer[j]) - kmeans_lsvm->cluster2dim[i][j]);

		if (fdis < fdis_min) {
			fdis_min = fdis;
			lsvm_col = i;
		}
	}
	if (lsvm_col < 0 || lsvm_col >= kmeans_lsvm->nr_cluster) {
		pr_err("lsvm_col %d invalid not in [0, %d]\n", lsvm_col, kmeans_lsvm->nr_cluster);
		return 0;
	}

	/*
	 * This example is ONLY ONE lsvm.
	 * Actually there may be THREE lsvms, 1st is highest gain, 2nd is middle gain, and 3rd is lowest gain.
	 * So the decision logic should be as fellow:
	 *               if model can get hightest gain, by calc in 1st lsvm,
	 *			return 1; // Left and Right task are matched
	 *		 else if model can get middle gain, by calc in 2st lsvm,
	 *			return 1;
	 *		 else if model can get lowest gain, by calc in 3rd lsvm,
	 *			return 1; OR return 0;
	 *		 else // HERE NO Gain and NO Matched!
	 *			return 0;
	 */
	for (i = 0; i < kmeans_lsvm->nr_lsvm; i++) {
		sum[i] = 0;
		plsvm = kmeans_lsvm->lsvm[i];
		for (j = 0; j < kmeans_lsvm->dim_lsvm; j++)
			sum[i] += (int)(buffer[j]) * plsvm->lsvm2dim[lsvm_col][j];
		trace_km_cluster_make_decision(fdis_min, lsvm_col, i, sum[i], plsvm->lower, plsvm->upper, plsvm->threshold);
		if (sum[i] >= plsvm->upper)
			return 1;
		else if (sum[i] <= plsvm->lower)
			return 0;
		else if (sum[i] >= plsvm->threshold)
			return 1;
	}

	return 0;
}

static bool predict_kmeans_lsvm(struct kmeans_lsvm *kmeans_lsvm,
				struct task_struct *pleft,
				struct task_struct *pright,
				enum smt_mode smt)
{
	int i;
	u64 *buffer = NULL;
	bool is_match = false;

	buffer = kmeans_lsvm->buffer;
	/* bias scaler: 1 * scaler */
	buffer[10] = kmeans_lsvm->data_scaler;
	trace_km_cluster_predict_buffer(pleft, pright, buffer, smt);
	for (i = 0; i < kmeans_lsvm->dim_lsvm; i++)
		buffer[i] = (int)buffer[i] * kmeans_lsvm->buffer_scaler[i];

	is_match = make_decision(kmeans_lsvm, buffer);
	trace_km_cluster_predict_pmu(pleft, pright, phase_get_counter(pleft->phase_info, smt),
				     phase_get_counter(pright->phase_info, smt), smt, is_match);

	return is_match;
}

static bool calc_feature_per_inst(struct phase_info *linfo, struct phase_info *rinfo,
				  enum smt_mode smt, u64 *buffer)
{
	u64 *ldata = phase_get_counter(linfo, smt)->data;
	u64 *rdata = phase_get_counter(rinfo, smt)->data;

	if (ldata[INST_RETIRED_INDEX] < 10000 || rdata[INST_RETIRED_INDEX] < 10000)
		return false;

	/* calc left task feature */
	buffer[0] = PHASE_SCALE(ldata[BR_MIS_PRED_INDEX] +
			ldata[ROB_FLUSH_INDEX]) / ldata[INST_RETIRED_INDEX]; /* FLUSH_CNT */
	buffer[1] = PHASE_SCALE(ldata[CYCLES_INDEX]) / ldata[INST_RETIRED_INDEX]; /* CPI */
	buffer[2] = PHASE_SCALE(ldata[DSP_STALL_INDEX]) / ldata[INST_RETIRED_INDEX]; /* DISPATCH_STALL_CNT */
	buffer[3] = PHASE_SCALE(ldata[L1D_CACHE_INDEX]) / ldata[INST_RETIRED_INDEX]; /* L1D_CACHE */
	buffer[4] = PHASE_SCALE(ldata[CYCLES_INDEX] -
			ldata[IF_RSURC_CHK_OK_INDEX]) / ldata[INST_RETIRED_INDEX]; /* INSTQ_STALL_CNT */

	/* calc right task feature */
	buffer[5] = PHASE_SCALE(rdata[BR_MIS_PRED_INDEX] +
			rdata[ROB_FLUSH_INDEX]) / rdata[INST_RETIRED_INDEX]; /* FLUSH_CNT */
	buffer[6] = PHASE_SCALE(rdata[CYCLES_INDEX]) / rdata[INST_RETIRED_INDEX]; /* CPI */
	buffer[7] = PHASE_SCALE(rdata[DSP_STALL_INDEX]) / rdata[INST_RETIRED_INDEX]; /* DISPATCH_STALL_CNT */
	buffer[8] = PHASE_SCALE(rdata[L1D_CACHE_INDEX]) / rdata[INST_RETIRED_INDEX]; /* L1D_CACHE */
	buffer[9] = PHASE_SCALE(rdata[CYCLES_INDEX] -
			rdata[IF_RSURC_CHK_OK_INDEX]) / rdata[INST_RETIRED_INDEX]; /* INSTQ_STALL_CNT */

	return true;
}

static bool calc_feature_per_cycle(struct phase_info *linfo, struct phase_info *rinfo,
				   enum smt_mode smt, u64 *buffer)
{
	u64 *ldata = phase_get_counter(linfo, smt)->data;
	u64 *rdata = phase_get_counter(rinfo, smt)->data;

	if (ldata[INST_RETIRED_INDEX] < 10000 || rdata[INST_RETIRED_INDEX] < 10000)
		return false;

	/* calc left task feature */
	buffer[0] = PHASE_SCALE(ldata[BR_MIS_PRED_INDEX] +
			ldata[ROB_FLUSH_INDEX]) / ldata[CYCLES_INDEX]; /* FLUSH_CNT */
	buffer[1] = PHASE_SCALE(ldata[OP_SPEC_INDEX]) / ldata[CYCLES_INDEX]; /* NUOPS */
	buffer[2] = PHASE_SCALE(ldata[DSP_STALL_INDEX]) / ldata[CYCLES_INDEX]; /* DISPATCH_STALL_CNT */
	buffer[3] = PHASE_SCALE(ldata[L1D_CACHE_INDEX]) / ldata[CYCLES_INDEX]; /* L1D_CACHE */
	buffer[4] = PHASE_SCALE(ldata[CYCLES_INDEX] -
			ldata[IF_RSURC_CHK_OK_INDEX]) / ldata[CYCLES_INDEX]; /* INSTQ_STALL_CNT */

	/* calc right task feature */
	buffer[5] = PHASE_SCALE(rdata[BR_MIS_PRED_INDEX] +
			rdata[ROB_FLUSH_INDEX]) / rdata[CYCLES_INDEX]; /* FLUSH_CNT */
	buffer[6] = PHASE_SCALE(rdata[OP_SPEC_INDEX]) / rdata[CYCLES_INDEX]; /* NUOPS */
	buffer[7] = PHASE_SCALE(rdata[DSP_STALL_INDEX]) / rdata[CYCLES_INDEX]; /* DISPATCH_STALL_CNT */
	buffer[8] = PHASE_SCALE(rdata[L1D_CACHE_INDEX]) / rdata[CYCLES_INDEX]; /* L1D_CACHE */
	buffer[9] = PHASE_SCALE(rdata[CYCLES_INDEX] -
			rdata[IF_RSURC_CHK_OK_INDEX]) / rdata[CYCLES_INDEX]; /* INSTQ_STALL_CNT */

	return true;
}

static bool phase_calc_feature(struct phase_info *linfo, struct phase_info *rinfo,
			       enum smt_mode mode, u64 *buffer)
{
	if (!!sysctl_phase_calc_feature_mode)
		return calc_feature_per_inst(linfo, rinfo, mode, buffer);
	else
		return calc_feature_per_cycle(linfo, rinfo, mode, buffer);

	return true;
}

static bool phase_predict_kmeans_lsvm(struct task_struct *left,
				      struct task_struct *right,
				      enum phase_predict_mode predict_mode)
{
	if (atomic_read(&kmeans_lsvm_created) == 0)
		return false;

	if (!phase_calc_feature(left->phase_info, right->phase_info,
				predict_mode, pkmeans_lsvm[predict_mode]->buffer))
		return true;

	/* test REVERSE_MODE */
	if (predict_mode == REVERSE_MODE) {
		if (sysctl_phase_reverse_enabled == 1)
			return predict_kmeans_lsvm(pkmeans_lsvm[predict_mode],
						   left, right, predict_mode);
		else
			return sysctl_phase_reverse_default_val > 0 ? true : false;
	}

	/* test FORWARD_MODE */
	if (predict_mode == FORWARD_MODE) {
		if (sysctl_phase_forward_enabled == 1)
			return predict_kmeans_lsvm(pkmeans_lsvm[predict_mode],
						   left, right, predict_mode);
		else
			return sysctl_phase_forward_default_val > 0 ? true : false;
	}

	return false;
}

static size_t get_kmeans_size_from_cfg(void)
{
	char *buf = NULL;
	loff_t pos = 0;
	loff_t size = 0;
	size_t kmeans_size;
	struct kmeans_lsvm tmp;

	buf = load_from_file(MODEL_FORWARD_PATH, &size);
	if (buf == NULL)
		return 0;

	/* get model mode */
	get_matches(buf + pos, "%d %d %d %d %d %d %d %d\n",
		    &tmp.type, &tmp.nr_cluster,
		    &tmp.dim_cluster, &tmp.nr_lsvm,
		    &tmp.dim_lsvm, &tmp.nr_threshold,
		    &tmp.data_scaler, &tmp.model_scaler);
	consume_line(buf, &pos, size);

	/* kzalloc kmeans */
	kmeans_size = sizeof(struct kmeans_lsvm) +
		      tmp.dim_cluster * sizeof(int) + /* buffer_scaler */
		      tmp.dim_lsvm * sizeof(u64) + /* buffer */
		      tmp.nr_cluster * sizeof(int *) + /* cluster2dim[cluster] */
		      tmp.nr_cluster * tmp.dim_cluster * sizeof(int) + /* [nr_cluster][dim_cluster] */
		      /* nr_lsvm struct lsvm datas */
		      tmp.nr_lsvm * sizeof(struct lsvm *) + /* nr_lsvm * sizeof(struct lsvm *) */
		      tmp.nr_lsvm * sizeof(struct lsvm) + /* nr_lsvm * sizeof(struct lsvm) */
		      tmp.nr_lsvm * tmp.nr_cluster * sizeof(int *) + /* nr_lsvm * lsvm2dim[nr_cluster] */
		      tmp.nr_lsvm * tmp.nr_cluster * tmp.dim_lsvm * sizeof(int); /* nr_lsvm * [nr_cluster][dim_lsvm] */
	kfree(buf);
	return kmeans_size;
}

static inline int build_kmeans_lsvm(struct kmeans_lsvm *kmeans_lsvm,
				    enum phase_predict_mode predict_mode)
{
	int ret;
	const char *config_file = (predict_mode == FORWARD_MODE) ?
					MODEL_FORWARD_PATH : MODEL_REVERSE_PATH;

	/* load kmeans lsvm from file and build kmeans lsvm */
	ret = build_kmeans_lsvm_from_file(kmeans_lsvm, config_file);
	if (ret != 0) {
		pr_err("build kmeans lsvm fail\n");
		return ret;
	}

	pr_info("create kmeans_lsvm mode [%d] success\n", predict_mode);
	return ret;
}

static int phase_build_kmeans_lsvms(void)
{
	int index, i, ret;
	size_t kmeans_size;
	struct kmeans_lsvm *kmeans_lsvm = NULL;

	BUILD_BUG_ON((int)FORWARD_MODE != (int)ST_MODE);
	BUILD_BUG_ON((int)REVERSE_MODE != (int)SMT_MODE);

	if (atomic_read(&kmeans_lsvm_created) != 0)
		return -EEXIST;

	/* get kzalloc kmeans lsvm size */
	kmeans_size = get_kmeans_size_from_cfg();
	if (kmeans_size == 0) {
		pr_err("get kmeans_lsvm size 0 error\n");
		return -EINVAL;
	}

	kmeans_lsvm = kzalloc(kmeans_size * NR_PREDICT_MODE, GFP_KERNEL);
	if (kmeans_lsvm == NULL) {
		pr_err("kzalloc %zu error\n", kmeans_size * NR_PREDICT_MODE);
		return -ENOMEM;
	}

	for (index = 0; index < NR_PREDICT_MODE; index++) {
		pkmeans_lsvm[index] = (struct kmeans_lsvm *)((char *)kmeans_lsvm + index * kmeans_size);
		ret = build_kmeans_lsvm(pkmeans_lsvm[index], index);
		if (ret != 0)
			goto out_del_kmeans;
	}

	phase_tk_kmeans_lsvm.private_data = (void *)kmeans_lsvm;
	atomic_inc(&kmeans_lsvm_created);

	return 0;

out_del_kmeans:
	for (i = 0; i < index; i++)
		pkmeans_lsvm[index] = NULL;

	kfree(kmeans_lsvm);

	return ret;
}

static inline void phase_destroy_kmeans_lsvms(void)
{
	int index;
	struct kmeans_lsvm *kmeans_lsvm = NULL;

	atomic_dec(&kmeans_lsvm_created);

	kmeans_lsvm = (struct kmeans_lsvm *)phase_tk_kmeans_lsvm.private_data;
	if (kmeans_lsvm == NULL)
		return;

	for (index = 0; index < NR_PREDICT_MODE; index++)
		pkmeans_lsvm[index] = NULL;

	kfree(kmeans_lsvm);
	phase_tk_kmeans_lsvm.private_data = NULL;
}

static inline int phase_create_kmeans_lsvm(void)
{
	return phase_build_kmeans_lsvms();
}

static inline void phase_release_kmeans_lsvm(void)
{
	phase_destroy_kmeans_lsvms();
}

struct phase_think_class phase_tk_kmeans_lsvm = {
	.id = PHASE_TK_ID_KMEANS_CLUTER,
	.name = "kmeans_lsvm",
	.pevents = kmeans_pevents,
	.fevents = NULL,
	.create = phase_create_kmeans_lsvm,
	.release = phase_release_kmeans_lsvm,
	.predict = phase_predict_kmeans_lsvm,
	.sched_core_find = phase_sched_core_find_default,
	.next = NULL,
};
