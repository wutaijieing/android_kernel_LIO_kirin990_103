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
#include <linux/notifier.h>
#include <linux/export.h>
#include <linux/types.h>

static BLOCKING_NOTIFIER_HEAD(composer_notifier_list);

int composer_register_listener(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&composer_notifier_list, nb);
}
EXPORT_SYMBOL(composer_register_listener);


int composer_unregister_listener(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&composer_notifier_list, nb);
}
EXPORT_SYMBOL(composer_unregister_listener);


int composer_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&composer_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(composer_notifier_call_chain);


