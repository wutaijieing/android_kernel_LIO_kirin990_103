/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM hck_regulator
#define TRACE_INCLUDE_PATH trace/hooks/hck_hook
#if !defined(_TRACE_HCK_HOOK_REGULATOR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HCK_HOOK_REGULATOR_H
#include <linux/tracepoint.h>
#include <trace/hooks/hck_hook/hck_vendor_hooks.h>

/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */

struct regulator_dev;

DECLARE_HCK_HOOK(hck_vh_regulator_on_off,
		TP_PROTO(struct regulator_dev *rdev),
		TP_ARGS(rdev));

DECLARE_HCK_HOOK(hck_vh_regulator_set_vol,
		TP_PROTO(struct regulator_dev *rdev, int max_uv, int min_uv),
		TP_ARGS(rdev, max_uv, min_uv));

DECLARE_HCK_HOOK(hck_vh_regulator_set_mode,
		TP_PROTO(struct regulator_dev *rdev, u8 mode),
		TP_ARGS(rdev, mode));

#endif /* _TRACE_HCK_HOOK_REGULATOR_H */
/* This part must be outside protection */
#include <trace/define_trace.h>

