/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature_dctree.c - arm64-specific Kernel Phase-Algorithm
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
#include <linux/build_bug.h>
#include <../kernel/sched/sched.h>
#include "phase_feature.h"

#define CREATE_TRACE_POINTS
#include <trace/events/dc_tree.h>

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
#endif

/* Helpers for Phase Decision Tree Algorithm interface */
#define LEAF_NODE		1
#define MODEL_FORWARD_PATH	"/mnt/RF_FORWARD.cfg"
#define MODEL_REVERSE_PATH	"/mnt/RF_REVERSE.cfg"

#define NR_MAX_PMU_EVENTS	5

enum predict_calc_mode {
	MODE_REGRESSION = 0,
	MODE_CLS,
	NR_PREDICT_CALC_MODE,
};

struct tree_param {
	u16 index;
	u8 type;
	u8 input_index;
	u32 input_value;
	u16 output_value;
	u16 lchild_index;
	u16 rchild_index;
};

struct tree_node {
	u8 type; /* 1 stands for leaf and false for test */
	u8 input_index;
	union {
		u32 input_value;
		u16 output_value;
	} data;
	struct tree_node *lchild;
	struct tree_node *rchild;
};

struct decision_tree {
	struct tree_node **forest;
	unsigned int tree_num;
	u64 *buffer;
	enum predict_calc_mode calc_mode;
	enum phase_predict_mode predict_mode;
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

static int dc_tree_pevents[PHASE_PEVENT_NUM] = {
#define EVENT(e) e
	PHASE_EVENT_LIST,
#undef EVENT
	PHASE_EVENT_FINAL_TERMINATOR,
};

static atomic_t decision_tree_created = ATOMIC_INIT(0);

static struct tree_param *load_param_from_file(const char *model_file_path,
					       u32 **nodes_num, u32 *trees_num,
					       enum predict_calc_mode *mode)
{
	char *buf = NULL;
	struct tree_param *model_param = NULL;
	loff_t pos = 0;
	loff_t size = 0;
	u32 line_idx, tree_index, i;
	u32 total_nodes_num;

	buf = load_from_file(model_file_path, &size);
	if (buf == NULL)
		return NULL;

	/* Get model mode */
	pos = 0;
	get_matches(buf + pos, "%d\n", mode);
	consume_line(buf, &pos, size);

	/* Get total tree numbers and total nodes numbers */
	get_matches(buf + pos, "%d, %d\n", trees_num, &total_nodes_num);
	consume_line(buf, &pos, size);

	*nodes_num = kmalloc(sizeof(u32 *) * (*trees_num), GFP_KERNEL);
	if (*nodes_num == NULL)
		goto out_free_buf;

	model_param = kmalloc(total_nodes_num * sizeof(struct tree_param), GFP_KERNEL);
	if (model_param == NULL)
		goto out_free_nodes;

	line_idx = 0;
	for (tree_index = 0; tree_index < *trees_num; tree_index++) {
		get_matches(buf + pos, "%d\n", &(*nodes_num)[tree_index]);
		consume_line(buf, &pos, size);

		for (i = 0; i < (*nodes_num)[tree_index]; i++) {
			model_param[line_idx].index = i;
			get_matches(buf + pos, "%d, %d, %d, %d, %d, %d\n",
				    &model_param[line_idx].type,
				    &model_param[line_idx].input_index,
				    &model_param[line_idx].input_value,
				    &model_param[line_idx].output_value,
				    &model_param[line_idx].lchild_index,
				    &model_param[line_idx].rchild_index);
			consume_line(buf, &pos, size);
			line_idx++;
		}
	}
	pr_info("load model param from file %s, trees_num=%d, total_nodes_num=%d\n",
		model_file_path, *trees_num, total_nodes_num);

	kfree(buf);
	return model_param;

out_free_nodes:
	kfree(*nodes_num);
	*nodes_num = NULL;
out_free_buf:
	kfree(buf);

	return NULL;
}

static struct tree_node *build_tree(struct tree_param *param, u32 index, u32 num)
{
	struct tree_node *node = NULL;

	if (index < 0 || index >= num)
		return NULL;

	node = kmalloc(sizeof(struct tree_node), GFP_KERNEL);
	if (node == NULL)
		return NULL;

	node->type = param[index].type;
	if (node->type == LEAF_NODE) {
		node->data.output_value = param[index].output_value;
		node->lchild = NULL;
		node->rchild = NULL;
	} else {
		node->input_index = param[index].input_index;
		node->data.input_value = param[index].input_value;
		node->lchild = build_tree(param, param[index].lchild_index, num);
		node->rchild = build_tree(param, param[index].rchild_index, num);
	}

	return node;
}

static struct tree_node **build_forest(struct tree_param *param,
				       u32 trees_num, u32 *nodes_num)
{
	u32 i, offset;
	struct tree_node **forest = kmalloc(sizeof(struct tree_node *) * trees_num, GFP_KERNEL);

	if (forest == NULL)
		return NULL;

	offset = 0;
	for (i = 0; i < trees_num; i++) {
		forest[i] = build_tree(param + offset, 0, nodes_num[i]);
		offset += nodes_num[i];
	}

	return forest;
}

static void delete_tree(struct tree_node *node)
{
	if (node == NULL)
		return;

	if (node->lchild)
		delete_tree(node->lchild);
	if (node->rchild)
		delete_tree(node->rchild);

	kfree(node);
	node = NULL;
}

static void delete_forest(struct tree_node **forest, unsigned int trees_num)
{
	unsigned int i;

	if (forest == NULL)
		return;

	for (i = 0; i < trees_num; i++)
		delete_tree(forest[i]);

	kfree(forest);
	forest = NULL;
}

static void free_param(struct tree_param *param, unsigned int *nodes_num)
{
	kfree(param);
	kfree(nodes_num);
}

static u16 make_decison(struct tree_node *node, u64 *buffer, unsigned int buffer_size)
{
	struct tree_node *p = node;

	while (p->type != LEAF_NODE) {
		prefetch(p->lchild);
		prefetch(p->rchild);
		if (buffer[p->input_index] <= p->data.input_value)
			p = p->lchild;
		else
			p = p->rchild;
	}

	return p->data.output_value;
}

static bool predict_ipc(struct decision_tree *dtree,
			struct task_struct *pleft,
			struct task_struct *pright,
			enum smt_mode smt)
{
	u16 i;
	u32 ipc = 0;
	bool is_match = false;
	struct tree_node **forest = NULL;
	unsigned int tree_num;
	u64 *buffer = NULL;

	forest = dtree->forest;
	tree_num = dtree->tree_num;
	buffer = dtree->buffer;
	if (!forest || !buffer)
		return is_match;

	for (i = 0; i < tree_num; i++) {
		if (i + 1 < tree_num)
			prefetch(forest[i + 1]);
		ipc += make_decison(forest[i], buffer, NR_MAX_PMU_EVENTS);
	}
	if (dtree->calc_mode == MODE_CLS)
		is_match = ipc > (tree_num >> 1);
	else
		is_match = ipc / tree_num;

	/* print the data which calc by pmu */
	trace_dc_tree_predict(pleft, pright, buffer, smt, is_match);

	/* print the raw pmu data */
	trace_dc_tree_predict_pmu(pleft, pright, phase_get_counter(pleft->phase_info, smt),
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

static bool calc_feature_per_cycle(struct phase_info *linfo,
				   struct phase_info *rinfo,
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

static bool phase_calc_feature(struct phase_info *linfo,
			       struct phase_info *rinfo,
			       enum smt_mode mode, u64 *buffer)
{
	if (sysctl_phase_calc_feature_mode > 0)
		return calc_feature_per_inst(linfo, rinfo, mode, buffer);
	else
		return calc_feature_per_cycle(linfo, rinfo, mode, buffer);
}

static bool phase_predict_decision_tree(struct task_struct *left,
					struct task_struct *right,
					enum phase_predict_mode predict_mode)
{
	struct decision_tree *dtree = NULL;

	if (atomic_read(&decision_tree_created) == 0)
		return false;

	dtree = (struct decision_tree *)phase_tk_decision_tree.private_data + predict_mode;
	if (!phase_calc_feature(left->phase_info, right->phase_info, predict_mode, dtree->buffer))
		return true;

	/* test REVERSE_MODE */
	if (predict_mode == REVERSE_MODE) {
		if (sysctl_phase_reverse_enabled == 1)
			return predict_ipc(dtree, left, right, predict_mode);
		else
			return sysctl_phase_reverse_default_val > 0 ? true : false;
	}

	/* test FORWARD_MODE */
	if (predict_mode == FORWARD_MODE) {
		if (sysctl_phase_forward_enabled == 1)
			return predict_ipc(dtree, left, right, predict_mode);
		else
			return sysctl_phase_forward_default_val > 0 ? true : false;
	}

	return false;
}

static int build_decision_tree(struct decision_tree *decision_tree,
			       enum phase_predict_mode predict_mode)
{
	int ret;
	unsigned int tree_num;
	u64 *buffer = NULL;
	struct tree_param *param = NULL;
	struct tree_node **forest = NULL;
	unsigned int *tree_node_num = NULL;
	enum predict_calc_mode calc_mode;
	const char *config_file = (predict_mode == FORWARD_MODE) ?
					MODEL_FORWARD_PATH : MODEL_REVERSE_PATH;

	/* load decision tree from file */
	param = load_param_from_file(config_file, &tree_node_num, &tree_num, &calc_mode);
	if (param == NULL) {
		pr_err("load param from file fail\n");
		return -ENODEV;
	}

	/* build decision forest */
	forest = build_forest(param, tree_num, tree_node_num);
	if (forest == NULL) {
		pr_err("build forest fail\n");
		ret = -ENOMEM;
		goto out_free_param;
	}
	free_param(param, tree_node_num);

	/* create predict buffer */
	buffer = kmalloc(sizeof(u64) * NR_MAX_PMU_EVENTS * 2, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err("malloc buffer fail\n");
		ret = -ENOMEM;
		goto out_del_forest;
	}

	decision_tree->forest = forest;
	decision_tree->tree_num = tree_num;
	decision_tree->buffer = buffer;
	decision_tree->calc_mode = calc_mode;
	decision_tree->predict_mode = predict_mode;

	pr_info("create decision_tree [%d] success\n", predict_mode);

	return 0;

out_del_forest:
	delete_forest(forest, tree_num);
out_free_param:
	free_param(param, tree_node_num);

	return ret;
}

static inline void destroy_decision_tree(struct decision_tree *dtree)
{
	struct tree_node **forest = NULL;
	unsigned int tree_num;
	u64 *buffer = NULL;
	enum phase_predict_mode predict_mode;

	forest = dtree->forest;
	tree_num = dtree->tree_num;
	if (forest != NULL)
		delete_forest(forest, tree_num);

	buffer = dtree->buffer;
	if (buffer != NULL)
		kfree(buffer);

	predict_mode = dtree->predict_mode;
	pr_info("destroy decision_tree [%d] success\n", predict_mode);
}

static int phase_build_decision_trees(void)
{
	int index, i, ret;
	struct decision_tree *decision_tree = NULL;

	BUILD_BUG_ON((int)FORWARD_MODE != (int)ST_MODE);
	BUILD_BUG_ON((int)REVERSE_MODE != (int)SMT_MODE);

	if (atomic_read(&decision_tree_created) != 0)
		return -EEXIST;

	decision_tree = kmalloc(sizeof(struct decision_tree) * NR_PREDICT_MODE,
				GFP_KERNEL);
	if (decision_tree == NULL)
		return -ENOMEM;

	for (index = 0; index < NR_PREDICT_MODE; index++) {
		ret = build_decision_tree(decision_tree + index, index);
		if (ret != 0)
			goto out_free_tree;
	}

	phase_tk_decision_tree.private_data = (void *)decision_tree;
	atomic_inc(&decision_tree_created);

	return 0;

out_free_tree:
	for (i = 0; i < index; i++)
		destroy_decision_tree(decision_tree + i);
	kfree(decision_tree);

	return ret;
}

static inline void phase_destroy_decision_trees(void)
{
	int index;
	struct decision_tree *dtree = NULL;

	atomic_dec(&decision_tree_created);

	dtree = (struct decision_tree *)phase_tk_decision_tree.private_data;
	if (dtree == NULL)
		return;

	for (index = 0; index < NR_PREDICT_MODE; index++)
		destroy_decision_tree(dtree + index);

	kfree(dtree);
	phase_tk_decision_tree.private_data = NULL;
}

static inline int phase_create_decision_tree(void)
{
	return phase_build_decision_trees();
}

static inline void phase_release_decision_tree(void)
{
	phase_destroy_decision_trees();
}

struct phase_think_class phase_tk_decision_tree = {
	.id = PHASE_TK_ID_DECISION_TREE,
	.name = "decision_tree",
	.pevents = dc_tree_pevents,
	.fevents = NULL,
	.create = phase_create_decision_tree,
	.release = phase_release_decision_tree,
	.predict = phase_predict_decision_tree,
	.sched_core_find = phase_sched_core_find_default,
	.next = &phase_tk_kmeans_lsvm,
};
