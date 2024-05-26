/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2017-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/export.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>
#include <linux/notifier.h>

static ATOMIC_NOTIFIER_HEAD(powerkey_notifier_list);

int powerkey_register_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&powerkey_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(powerkey_register_notifier);

int powerkey_unregister_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(
		&powerkey_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(powerkey_unregister_notifier);

int call_powerkey_notifiers(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&powerkey_notifier_list,
		val, v);
}
EXPORT_SYMBOL_GPL(call_powerkey_notifiers);
