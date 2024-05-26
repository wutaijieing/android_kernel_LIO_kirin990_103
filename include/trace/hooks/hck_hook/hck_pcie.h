/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM hck_pcie
#define TRACE_INCLUDE_PATH trace/hooks/hck_hook
#if !defined(_TRACE_HCK_HOOK_PCIE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HCK_HOOK_PCIE_H
#include <linux/tracepoint.h>
#include <trace/hooks/hck_hook/hck_vendor_hooks.h>
/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */
struct pcie_port;
struct device;
struct list_head;

DECLARE_HCK_HOOK(hck_vh_pcie_refclk_host_vote,
	TP_PROTO(struct pcie_port *pp, u32 vote),
	TP_ARGS(pp, vote));

DECLARE_HCK_HOOK(hck_vh_pcie_release_resource,
	TP_PROTO(struct device *dev, struct list_head *resources),
	TP_ARGS(dev, resources));

#endif /* _TRACE_HCK_HOOK_PCIE_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
