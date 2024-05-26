/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include "hisi_isr_utils.h"
#include "hisi_disp_debug.h"

#define get_bits_len(nr) (sizeof(nr) * BITS_PER_BYTE)

// uint32_t isr_addr, uint32_t isr_mask
void hisi_disp_isr_open()
{

}

void hisi_disp_isr_close()
{

}

void hisi_disp_isr_enable(struct hisi_disp_isr *isr_ctrl)
{
	if (!isr_ctrl) {
		disp_pr_err( "isr_ctrl is NULL\n");
		return;
	}

	disp_pr_info("enable_irq no= %d", isr_ctrl->irq_no);

	enable_irq(isr_ctrl->irq_no);
}

void hisi_disp_isr_disable(struct hisi_disp_isr *isr_ctrl)
{
	if (!isr_ctrl) {
		disp_pr_err( "isr_ctrl is NULL\n");
		return;
	}

	disable_irq(isr_ctrl->irq_no);
}


static struct hisi_isr_state_node *hisi_disp_isr_get_state_node(struct list_head *isr_state_list, uint32_t unmask_bit)
{
	struct hisi_isr_state_node *unmask_node = NULL;
	struct hisi_isr_state_node *_node_ = NULL;

	if (list_empty(isr_state_list))
		return NULL;

	list_for_each_entry_safe(unmask_node, _node_, isr_state_list, list_node) {
		if (unmask_node->unmask_bit == unmask_bit)
			return unmask_node;
	}

	return NULL;
}

static void hisi_disp_isr_init_states(struct list_head *isr_state_list, uint32_t unmask)
{
	uint32_t bit;
	struct hisi_isr_state_node *node;
	uint32_t bits_len = get_bits_len(unmask);

	for_each_set_bit(bit, (ulong*)&unmask, bits_len) {
		node = kzalloc(sizeof(*node), GFP_KERNEL);
		if (!node) {
			disp_pr_err("alloc isr node fail");
			return;
		}

		node->unmask_bit = BIT(bit);
		disp_pr_info("node->unmask_bit=0x%x,bit = %d", node->unmask_bit,bit);

		RAW_INIT_NOTIFIER_HEAD(&node->irq_nofitier);

		list_add_tail(&node->list_node, isr_state_list);
	}
}

void hisi_disp_isr_init_state_lists(struct hisi_disp_isr *isr_ctrl)
{
	uint32_t i;

	for (i = 0; i < IRQ_UNMASK_MAX; i++) {
		INIT_LIST_HEAD(&isr_ctrl->isr_state_lists[i]);
		hisi_disp_isr_init_states(&isr_ctrl->isr_state_lists[i], isr_ctrl->irq_states[i]);

		disp_pr_info("state[%d]=0x%x", i, isr_ctrl->irq_states[i]);
	}
}

void hisi_disp_isr_setup(struct hisi_disp_isr *isr_ctrl)
{
	int ret;
	disp_pr_info(" ++++ ");
	ret = request_irq(isr_ctrl->irq_no, isr_ctrl->isr_ops->handle_irq, 0, isr_ctrl->irq_name, isr_ctrl->parent);
	if (ret) {
		disp_pr_err("setup irq fail %s, isr_ctrl->irq_no %d", isr_ctrl->irq_name, isr_ctrl->irq_no);
		return;
	}

	disable_irq(isr_ctrl->irq_no);
	disp_pr_info(" ---- ");
}

int hisi_disp_isr_register_listener(struct hisi_disp_isr *isr_ctrl, struct notifier_block *nb,
		uint32_t isr_state, uint32_t unmask_bit)
{
	struct list_head *isr_state_list = &isr_ctrl->isr_state_lists[isr_state];
	struct hisi_isr_state_node *state_node;
	disp_pr_info(" ++++ ");
	state_node = hisi_disp_isr_get_state_node(isr_state_list, unmask_bit);
	if (!state_node) {
		disp_pr_err("get state node fail, state = 0x%x, unmask_bit=0x%x", isr_state, unmask_bit);
		return -1;
	}
	disp_pr_info(" ---- ");

	return raw_notifier_chain_register(&state_node->irq_nofitier, nb);
}

int hisi_disp_isr_unregister_listener(struct hisi_disp_isr *isr_ctrl, struct notifier_block *nb,
		uint32_t isr_state, uint32_t unmask_bit)
{
	struct list_head *isr_state_list = &isr_ctrl->isr_state_lists[isr_state];
	struct hisi_isr_state_node *state_node;

	state_node = hisi_disp_isr_get_state_node(isr_state_list, unmask_bit);
	if (!state_node) {
		disp_pr_err("get state node fail, state = 0x%x, unmask_bit=0x%x", isr_state, unmask_bit);
		return -1;
	}

	return raw_notifier_chain_unregister(&state_node->irq_nofitier, nb);
}

int hisi_disp_isr_notify_listener(struct hisi_disp_isr *isr_ctrl, uint32_t isr_state_type, uint32_t isr_state)
{
	struct list_head *isr_state_list = &isr_ctrl->isr_state_lists[isr_state_type];
	struct hisi_isr_state_node *state_node = NULL;
	uint32_t bits_len = get_bits_len(isr_state);
	uint32_t unmask_state = isr_state & isr_ctrl->irq_states[isr_state_type];
	uint32_t unmask_bit;
	uint32_t i;
	//disp_pr_info(" ++++ ");
	//disp_pr_info("bits_len = %u, unmask_state = %u", bits_len, unmask_state);
	for_each_set_bit(i, (ulong*)&unmask_state, bits_len) {
		unmask_bit = BIT(i);
		state_node = hisi_disp_isr_get_state_node(isr_state_list, unmask_bit);
		if (!state_node) {
			disp_pr_err("get state node fail, state = 0x%x, unmask_bit=0x%x", isr_state, unmask_bit);
			continue;
		}
		raw_notifier_call_chain(&state_node->irq_nofitier, unmask_bit, isr_ctrl->parent);
		//disp_pr_info("notify listener unmask_bit = %u", i);
	}
	//disp_pr_info(" ---- ");
	return 0;
}



