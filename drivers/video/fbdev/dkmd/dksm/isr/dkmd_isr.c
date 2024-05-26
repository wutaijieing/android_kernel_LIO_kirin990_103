/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/bitops.h>
#include "dkmd_log.h"
#include "dkmd_isr.h"

static void dkmd_isr_init_listener_list(struct dkmd_isr *isr_ctrl)
{
	int32_t i;

	for (i = 0; i < ISR_LISTENER_LIST_COUNT; ++i)
		INIT_LIST_HEAD(&isr_ctrl->isr_listener_list[i]);
}

static struct list_head* dkmd_isr_get_listener_list(struct dkmd_isr *isr_ctrl, uint32_t listen_bit)
{
	return &isr_ctrl->isr_listener_list[get_count_order(listen_bit)];
}

static void dkmd_isr_release(struct dkmd_isr *isr_ctrl)
{
	if (atomic_read(&isr_ctrl->refcount) != 0)
		return;

	if (isr_ctrl->irq_no > 0)
		free_irq(isr_ctrl->irq_no, isr_ctrl);
}

static void dkmd_isr_enable(struct dkmd_isr *isr_ctrl)
{
	if (atomic_read(&isr_ctrl->refcount) == 0)
		enable_irq(isr_ctrl->irq_no);

	atomic_inc(&isr_ctrl->refcount);
}

static void dkmd_isr_disable(struct dkmd_isr *isr_ctrl)
{
	if (atomic_read(&isr_ctrl->refcount) == 0)
		return;

	if (atomic_dec_and_test(&isr_ctrl->refcount))
		disable_irq(isr_ctrl->irq_no);
}

static void dkmd_isr_enable_no_ref(struct dkmd_isr *isr_ctrl)
{
	if (atomic_read(&isr_ctrl->refcount) != 0)
		enable_irq(isr_ctrl->irq_no);
}

static void dkmd_isr_disable_no_ref(struct dkmd_isr *isr_ctrl)
{
	if (atomic_read(&isr_ctrl->refcount) != 0)
		disable_irq(isr_ctrl->irq_no);
}

static void dkmd_isr_handle_func(struct dkmd_isr *isr_ctrl, uint32_t handle_evnet)
{
	dpu_pr_info("irq_name:%s irq_no:%d refcount:%u handle_evnet:%u",
		isr_ctrl->irq_name, isr_ctrl->irq_no, atomic_read(&isr_ctrl->refcount), handle_evnet);
	switch (handle_evnet) {
	case DKMD_ISR_RELEASE:
		dkmd_isr_release(isr_ctrl);
		break;
	case DKMD_ISR_DISABLE:
		dkmd_isr_disable(isr_ctrl);
		break;
	case DKMD_ISR_ENABLE:
		dkmd_isr_enable(isr_ctrl);
		break;
	case DKMD_ISR_ENABLE_NO_REF:
		dkmd_isr_enable_no_ref(isr_ctrl);
		break;
	case DKMD_ISR_DISABLE_NO_REF:
		dkmd_isr_disable_no_ref(isr_ctrl);
		break;
	default:
		return;
	}
}

void dkmd_isr_setup(struct dkmd_isr *isr_ctrl)
{
	int32_t ret;

	dkmd_isr_init_listener_list(isr_ctrl);
	isr_ctrl->handle_func = dkmd_isr_handle_func;

	if (isr_ctrl->irq_no < 0) {
		dpu_pr_warn("%s error irq_no: %d", isr_ctrl->irq_name, isr_ctrl->irq_no);
		return;
	}

	ret = request_irq(isr_ctrl->irq_no, isr_ctrl->isr_fnc, 0, isr_ctrl->irq_name, isr_ctrl);
	if (ret) {
		dpu_pr_err("setup irq fail %s, isr_ctrl->irq_no: %d", isr_ctrl->irq_name, isr_ctrl->irq_no);
		return;
	}
	/* default disable irq */
	disable_irq(isr_ctrl->irq_no);
}

int32_t dkmd_isr_register_listener(struct dkmd_isr *isr_ctrl,
	struct notifier_block *nb, uint32_t listen_bit, void *listener_data)
{
	struct dkmd_isr_listener_node *listener_node = NULL;
	struct list_head *listener_list = NULL;

	if (!isr_ctrl) {
		dpu_pr_err("isr_ctrl is null ptr");
		return -1;
	}

	listener_list = dkmd_isr_get_listener_list(isr_ctrl, listen_bit);
	if (!listener_list) {
		dpu_pr_err("get listener_list fail, listen_bit=0x%x", listen_bit);
		return -1;
	}

	listener_node = kzalloc(sizeof(*listener_node), GFP_KERNEL);
	if (!listener_node) {
		dpu_pr_err("alloc listener node fail, listen_bit=0x%x", listen_bit);
		return -1;
	}

	listener_node->listen_bit = listen_bit;
	listener_node->data = listener_data;

	list_add_tail(&listener_node->list_node, listener_list);

	return raw_notifier_chain_register(&listener_node->irq_nofitier, nb);
}

int32_t dkmd_isr_unregister_listener(struct dkmd_isr *isr_ctrl, struct notifier_block *nb, uint32_t listen_bit)
{
	struct list_head *listener_list = NULL;
	struct dkmd_isr_listener_node *listener_node = NULL;
	struct dkmd_isr_listener_node *_node_ = NULL;

	if (!isr_ctrl) {
		dpu_pr_err("isr_ctrl is null ptr");
		return -1;
	}

	listener_list = dkmd_isr_get_listener_list(isr_ctrl, listen_bit);

	list_for_each_entry_safe(listener_node, _node_, listener_list, list_node) {
		if (listener_node->listen_bit != listen_bit)
			continue;

		raw_notifier_chain_unregister(&listener_node->irq_nofitier, nb);

		list_del(&listener_node->list_node);
		kfree(listener_node);
	}

	return 0;
}

int32_t dkmd_isr_notify_listener(struct dkmd_isr *isr_ctrl, uint32_t listen_bit)
{
	struct list_head *listener_list = NULL;
	struct dkmd_isr_listener_node *listener_node = NULL;
	struct dkmd_isr_listener_node *_node_ = NULL;

	if (!isr_ctrl) {
		dpu_pr_err("isr_ctrl is null ptr");
		return -1;
	}

	/**
	 * @brief listen_bit MUST be single bit!!!
	 *
	 */
	listener_list = dkmd_isr_get_listener_list(isr_ctrl, listen_bit);

	list_for_each_entry_safe(listener_node, _node_, listener_list, list_node) {
		if ((listener_node->listen_bit & listen_bit) == listen_bit)
			raw_notifier_call_chain(&listener_node->irq_nofitier, listen_bit, listener_node->data);
	}

	return 0;
}
