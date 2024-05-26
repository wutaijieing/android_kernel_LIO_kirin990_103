// SPDX-License-Identifier: GPL-2.0
/*
 * xHCI host controller driver
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 */

#include "xhci.h"
#include <linux/platform_drivers/chip_usb_log.h>

/* Add verbose debugging later, just print everything for now */

void xhci_dump_op_regs(struct xhci_hcd *xhci)
{
	if (!xhci) {
		hiusb_pr_err("xhc is NULL\n");
		return;
	}

	hiusb_pr_err("USBCMD = 0x%x", readl(&xhci->op_regs->command));
	hiusb_pr_err("USBSTS = 0x%x", readl(&xhci->op_regs->status));
	hiusb_pr_err("PAGESIZE = 0x%x", readl(&xhci->op_regs->page_size));
	hiusb_pr_err("DNCTRL = 0x%x", readl(&xhci->op_regs->dev_notification));
	hiusb_pr_err("CRCR = 0x%x", readl(&xhci->op_regs->cmd_ring));
	hiusb_pr_err("DCBAAP = 0x%x", readl(&xhci->op_regs->dcbaa_ptr));
	hiusb_pr_err("CONFIG = 0x%x", readl(&xhci->op_regs->config_reg));
}

void xhci_dump_portsc(struct xhci_hcd *xhci)
{
	unsigned int u2_port_nums;
	unsigned int u3_port_nums;
	struct xhci_hub *rhub;
	u32 portsc;

	hiusb_pr_err("+\n");
	if (!xhci) {
		hiusb_pr_err("xhc is NULL\n");
		return;
	}

	rhub = &xhci->usb2_rhub;
	u2_port_nums = rhub->num_ports;
	while (u2_port_nums--) {
		portsc = readl(rhub->ports[u2_port_nums]->addr);
		hiusb_pr_err("u2_port_num:%u, portsc[%s]", u2_port_nums,
				xhci_decode_portsc(portsc));
	}

	rhub = &xhci->usb3_rhub;
	u3_port_nums = rhub->num_ports;
	while (u3_port_nums--) {
		portsc = readl(rhub->ports[u3_port_nums]->addr);
		hiusb_pr_err("u3_port_num:%u, portsc[%s]", u3_port_nums,
				xhci_decode_portsc(portsc));
	}

	hiusb_pr_err("-\n");
}

char *xhci_get_slot_state(struct xhci_hcd *xhci,
		struct xhci_container_ctx *ctx)
{
	struct xhci_slot_ctx *slot_ctx = xhci_get_slot_ctx(xhci, ctx);
	int state = GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state));

	return xhci_slot_state_string(state);
}

void xhci_dbg_trace(struct xhci_hcd *xhci, void (*trace)(struct va_format *),
			const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	xhci_dbg(xhci, "%pV\n", &vaf);
	trace(&vaf);
	va_end(args);
}
EXPORT_SYMBOL_GPL(xhci_dbg_trace);
