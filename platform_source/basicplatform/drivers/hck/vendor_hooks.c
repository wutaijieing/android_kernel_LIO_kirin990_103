// SPDX-License-Identifier: GPL-2.0-only
/* endor_hooks.c
 *
 * HiSilicon Common Kernel Vendor Hook Support
 *
 * Copyright 2022 Huawei
 */

#define CREATE_TRACE_POINTS
#include <trace/hooks/hck_hook/hck_vendor_hooks.h>
#include <trace/hooks/hck_hook/hck_regulator.h>
#include <trace/hooks/hck_hook/hck_pcie.h>

/*
 * Export tracepoints that act as a bare tracehook (ie: have no trace event
 * associated with them) to allow external modules to probe them.
 */

EXPORT_TRACEPOINT_SYMBOL_GPL(hck_vh_regulator_on_off);
EXPORT_TRACEPOINT_SYMBOL_GPL(hck_vh_regulator_set_vol);
EXPORT_TRACEPOINT_SYMBOL_GPL(hck_vh_regulator_set_mode);
EXPORT_TRACEPOINT_SYMBOL_GPL(hck_vh_pcie_refclk_host_vote);
EXPORT_TRACEPOINT_SYMBOL_GPL(hck_vh_pcie_release_resource);
