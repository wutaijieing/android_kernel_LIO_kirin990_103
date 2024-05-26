/*
 * Remote Processor - Histar ISP remoteproc platform data.
 * include/linux/platform_data/remoteproc_isp.h
 *
 * Copyright (c) 2013-2014 ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PLAT_REMOTEPROC_HISP_H
#define _PLAT_REMOTEPROC_HISP_H

#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include <linux/gpio.h>
#include <linux/clk.h>

enum hisp_clk_type {
	ISPCPU_CLK   = 0,
	ISPFUNC_CLK  = 1,
	ISPI2C_CLK   = 2,
	VIVOBUS_CLK  = 3,
	ISPFUNC2_CLK = 3,
	ISPFUNC3_CLK = 4,
	ISPFUNC4_CLK = 5,
	ISPFUNC5_CLK = 6,
	ISP_SYS_CLK  = 7,
	ISPI3C_CLK   = 8,
	ISP_CLK_MAX
};

enum hisp_rproc_case_attr {
	SEC_CASE = 0,
	NONSEC_CASE,
	INVAL_CASE,
};

enum hisp_phy_mode_e {
	HISP_PHY_MODE_DPHY = 0,
	HISP_PHY_MODE_CPHY,
	HISP_PHY_MODE_MAX,
};

enum hisp_phy_freq_mode_e {
	HISP_PHY_AUTO_FREQ = 0,
	HISP_PHY_MANUAL_FREQ,
	HISP_PHY_FREQ_MODE_MAX,
};

enum hisp_phy_work_mode_e {
	HISP_PHY_SINGLE_MODE = 0,
	HISP_PHY_DUAL_MODE_SENSORA,
	HISP_PHY_DUAL_MODE_SENSORB,
	HISP_PHY_WORK_MODE_MAX,
};

struct hisp_phy_info_t {
	unsigned int is_master_sensor;
	unsigned int phy_id;
	enum hisp_phy_mode_e phy_mode;
	enum hisp_phy_freq_mode_e phy_freq_mode;
	unsigned int phy_freq;
	enum hisp_phy_work_mode_e phy_work_mode;
};

/* hisp old sec boot api */
extern int hisp_secmem_size_get(unsigned int *trusted_mem_size);
extern int hisp_secmem_pa_set(u64 sec_boot_phymem_addr,
			unsigned int trusted_mem_size);
extern int hisp_secmem_pa_release(void);
/* hisp new sec boot api */
extern int hisp_secmem_ca_map(unsigned int pool_num, int sharefd,
			unsigned int size);
extern int hisp_secmem_ca_unmap(unsigned int pool_num);
extern int hisp_secboot_memsize_get_from_type(unsigned int type,
			unsigned int *size);
extern int hisp_secboot_info_set(unsigned int type, int sharefd);
extern int hisp_secboot_info_release(unsigned int type);
extern int hisp_secboot_prepare(void);
extern int hisp_secboot_unprepare(void);
extern int hisp_sec_ta_enable(void);
extern int hisp_sec_ta_disable(void);

/* hisp device api */
extern int hisp_set_boot_mode(enum hisp_rproc_case_attr);
extern int hisp_set_clk_rate(unsigned int type, unsigned int rate);
extern int hisp_dev_setpinctl(struct pinctrl *isp_pinctrl,
	struct pinctrl_state *pinctrl_def, struct pinctrl_state *pinctrl_idle);
extern int hisp_dev_setclkdepend(struct clk *aclk_dss,
			struct clk *pclk_dss);
extern int hisp_device_start(void);
extern int hisp_device_stop(void);
extern int hisp_device_attach(struct rproc *rproc);
extern int hisp_rproc_enable(void);
extern int hisp_rproc_disable(void);
extern int hisp_jpeg_powerdn(void);
extern int hisp_jpeg_powerup(void);
extern int hisp_dev_select_def(void);
extern int hisp_dev_select_idle(void);
extern int hisp_device_probe(struct platform_device *pdev);
extern int hisp_device_remove(struct platform_device *pdev);
extern void hisp_notify_probe(struct platform_device *pdev);
extern void hisp_notify_remove(void);

/* hisp phy config api */
extern int hisp_phy_csi_connect(struct hisp_phy_info_t *phy_info,
		unsigned int csi_index);
extern int hisp_phy_csi_disconnect(void);
extern int hisp_csi_mode_sel(unsigned int mode);
extern int hisp_i2c_pad_sel(void);

/* hisp rdr api */
#ifdef CONFIG_HISP_RDR
extern u64 hisp_rdr_addr_get(void);
extern void __iomem *hisp_rdr_va_get(void);
#else
static inline u64 hisp_rdr_addr_get(void) { return 0; }
static inline void __iomem *hisp_rdr_va_get(void) { return NULL; }
#endif

/* hisp rpmsg debug api */
extern void hisp_sendin(void *data);
extern void hisp_sendx(void);
extern void hisp_recvtask(void);
extern void hisp_recvthread(void);
extern void hisp_recvin(void *data);
extern void hisp_recvx(void *data);
extern void hisp_recvdone(void *data);
extern void hisp_rpmsgrefs_dump(void);
extern void hisp_rpmsgrefs_reset(void);
extern void hisp_dump_rpmsg_with_id(const unsigned int message_id);

/* hisp rpmsg debug api */
static inline int hisp_mdcmem_pa_set(u64 mdc_phymem_addr, unsigned int mdc_mem_size, int shared_fd) {
	(void)mdc_phymem_addr; (void)mdc_mem_size; (void)shared_fd; return 0; }
static inline int hisp_mdcmem_pa_release(void) { return 0; }

#endif /* _PLAT_REMOTEPROC_HISP_H */

