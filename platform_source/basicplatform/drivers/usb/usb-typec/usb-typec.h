/*
 * Copyright (C) 2021 Hisilicon
 * Author: Hisillicon <>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LOCAL_USB_TYPEC_H_
#define _LOCAL_USB_TYPEC_H_

#include <securec.h>

#define APDO_MAX_NUM 7

enum usb_cable_event {
	USB_CABLE_DP_IN,
	USB_CABLE_DP_OUT,
};

struct hisi_usb_apdo_info {
	int num;
	int min_volt[APDO_MAX_NUM];
	int max_volt[APDO_MAX_NUM];
	int cur[APDO_MAX_NUM];
};

struct hisi_usb_typec_adapter_ops {
	int (*otg_pwr_src)(void);
	bool (*force_vconn)(void);
	void (*set_vconn)(int enable);
	void (*monitor_cc_change)(uint8_t cc1, uint8_t cc2);
	void (*dfp_inform_id)(uint32_t *payload, uint32_t size);
	bool (*charger_type_pd)(void);
	void (*cc_ovp_dmd_report)(const void *dmd_msg);
	int (*handle_pe_event)(uint32_t evt_id, void *data);
	void (*max_power)(int power);
	void (*reset_product)(void);
	int (*pd_dp_state_notify)(uint32_t, uint32_t, uint32_t, uint32_t);
	void (*set_combphy_status)(enum tcpc_mux_ctrl_type);
	void (*set_last_hpd_status)(bool status);
	void (*usb_send_event)(enum usb_cable_event event);
	int (*register_pd_dpm)(void);
};

int hisi_usb_typec_register_ops(struct hisi_usb_typec_adapter_ops *ops);
int hisi_usb_typec_update_wake_lock(bool wake_lock_en);
void hisi_usb_typec_issue_hardreset(void *dev_data);
bool hisi_usb_pd_get_hw_dock_svid_exist(void *client);
int hisi_usb_typec_mark_direct_charging(void *data, bool direct_charging);
void hisi_usb_typec_set_pd_adapter_voltage(int voltage_mv, void *dev_data);
void hisi_usb_typec_send_uvdm(uint32_t *data, uint8_t cnt,
		bool wait_resp, void *dev_data);
int hisi_usb_get_cc_state(void);
bool hisi_usb_check_cc_vbus_short(void);
void hisi_usb_set_cc_mode(int mode);
int hisi_usb_pd_dpm_data_role_swap(void *client);
void hisi_usb_detect_emark_cable(void *client);
void hisi_usb_force_enable_drp(int mode);
int hisi_usb_disable_pd(void *client, bool disable);
int hisi_usb_typec_direct_charge_cable_detect(void);

#ifdef CONFIG_USB_PD_REV30
bool hisi_usb_typec_is_support_pps(void);
int hisi_usb_typec_get_source_apdo_cap(struct hisi_usb_apdo_info *para);
int hisi_usb_typec_set_pps_start(int mv, int ma);
int hisi_usb_typec_set_pps_end(void);
int hisi_usb_typec_get_max_current(void);
int hisi_usb_typec_get_temp(void);
int hisi_usb_typec_get_pps_adapter_output(int *mv, int *ma);
int hisi_typec_usb_get_product_id(void);
int hisi_typec_usb_get_vendor_id(void);
int hisi_typec_usb_get_version(int *fw_ver, int *hw_ver);
#else
static inline bool hisi_usb_typec_is_support_pps(void) { return false; }
static inline int hisi_usb_typec_get_source_apdo_cap(struct hisi_usb_apdo_info *para)
{
	return memset_s(para, sizeof(*para),
			0, sizeof(struct hisi_usb_apdo_info));
}
static inline int hisi_usb_typec_set_pps_start(int mv, int ma) { return 0; }
static inline int hisi_usb_typec_set_pps_end(void) { return 0;};
static inline int hisi_usb_typec_get_max_current(void) { return 0; }
static inline int hisi_usb_typec_get_temp(void) { return 0;}
static inline int hisi_usb_typec_get_pps_adapter_output(int *mv, int *ma) { *mv = 0; *ma = 0; return 0; }
static inline int hisi_typec_usb_get_product_id(void) { return 0; }
static inline int hisi_typec_usb_get_vendor_id(void) { return 0; }
static inline int hisi_typec_usb_get_version(int *fw_ver, int *hw_ver)
{
	*fw_ver = 0;
	*hw_ver = 0;
	return 0;
}
#endif /* CONFIG_USB_PD_REV30 */

#endif
