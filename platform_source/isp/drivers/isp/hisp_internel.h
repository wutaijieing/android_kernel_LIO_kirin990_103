/*
 * hisp_internel.h
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#ifndef _HISP_INTERNEL_H
#define _HISP_INTERNEL_H

#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/pci.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>

#define ISP_NORMAL_MODE 0
#define ISP_PCIE_MODE   1

#define ISP_MAX_NUM_MAGIC            (0xFFFFFFFF)
#define DUMP_ISP_BOOT_SIZE           (64)
#define ISP_CPU_POWER_DOWN           (1 << 7)

enum hisp_rdr_enable_mask
{
	RDR_LOG_TRACE       = 0x1,
	RDR_LOG_LAST_WORD   = 0x2,
	RDR_LOG_ALGO        = 0x4,
	RDR_LOG_CVDR        = 0x8,
	RDR_LOG_IRQ         = 0x10,
	RDR_LOG_TASK        = 0x20,
	RDR_LOG_CPUP        = 0x40,
	RDR_LOG_MONITOR     = 0x80,
};

#define ISPCPU_RDR_USE_APCTRL        (1UL << 31)
#define ISPCPU_DEFAULT_RDR_LEVEL     (RDR_LOG_TRACE | RDR_LOG_LAST_WORD)

#define ISPCPU_LOG_USE_APCTRL        (1UL << 31)
#define ISPCPU_LOG_TIMESTAMP_FPGAMOD (1 << 30)

#define IRQ_NUM                      (64)
#define MONITOR_CHAN_NUM             4

struct level_switch_s {
	unsigned int level;
	char enable_cmd[8];
	char disable_cmd[8];
	char info[32];
};

enum rpmsg_client_choice {
	ISP_DEBUG_RPMSG_CLIENT  = 0x1111,
	INVALID_CLIENT          = 0xFFFF,
};

enum hisp_vring_e {
	HISP_VRING0          = 0x0,
	HISP_VRING1,
    HISP_VRING_MAX,
};

enum hisp_sec_boot_mem_type {
	HISP_SEC_TEXT       = 0,
	HISP_SEC_DATA       = 1,
	HISP_SEC_BOOT_MAX_TYPE
};

enum hisp_sec_mem_pool_type {
	HISP_DYNAMIC_POOL        = 0,
	HISP_SEC_POOL            = 1,
	HISP_ISPSEC_POOL         = 2,
	HISP_SEC_POOL_MAX_TYPE
};

enum hisp_sec_rsv_mem_type {
	HISP_SEC_VR0       = 0,
	HISP_SEC_VR1       = 1,
	HISP_SEC_VQ        = 2,
	HISP_SEC_SHARE     = 3,
	HISP_SEC_RDR       = 4,
	HISP_SEC_RSV_MAX_TYPE
};

struct isp_plat_cfg {
	unsigned int platform_id;
	unsigned int isp_local_timer;
	unsigned int perf_power;
};

struct bw_boot_status {
	unsigned int entry:1;
	unsigned int invalid_tlb:1;
	unsigned int enable_mmu:1;
	unsigned int reserved:29;
};

struct fw_boot_status {
	unsigned int entry:1;
	unsigned int hard_boot_init:1;
	unsigned int hard_drv_init:1;
	unsigned int app_init:1;
	unsigned int reserved:28;
};

struct hisp_shared_para {
	struct isp_plat_cfg plat_cfg;
	int rdr_enable;
	unsigned int rdr_enable_type;
	unsigned char irq[IRQ_NUM];
	int log_flush_flag;
	unsigned int log_head_size;
	unsigned int log_cache_write;
	struct bw_boot_status bw_stat;
	struct fw_boot_status fw_stat;
	unsigned int logx_switch;
	u64 bootware_paddr;
	u64 dynamic_pgtable_base;
	unsigned long long  coresight_addr_da;
	unsigned long long  coresight_addr_vir;
	unsigned int perf_switch;
	u64 perf_addr;
	u32 coredump_addr;
	u32 exc_flag;/*bit 0:exc cur ;bit 1:ion flag ; bit 2:rtos dump over  bit3:handle over*/
	u64 mdc_pa_addr;
	unsigned int monitor_addr[MONITOR_CHAN_NUM];
	unsigned int monitor_ctrl;
	unsigned int monitor_pa_va;
	unsigned int monitor_hit_flag;
	unsigned int clk_value[ISP_CLK_MAX];
	unsigned char isp_efuse;
	unsigned int chip_flag;
};

struct isp_a7mapping_s {
	unsigned int a7va;
	unsigned int size;
	unsigned int prot;
	unsigned int offset;
	unsigned int reserve;
	unsigned long a7pa;
	void *apva;
};

struct hisp_ca_meminfo {
	unsigned int type;
	unsigned int da;
	unsigned int size;
	unsigned int prot;
	unsigned int sec_flag;/* SEC or NESC*/
	int sharefd;
	u64 pa;
};

#ifdef DEBUG_HISP
struct isp_pcie_cfg {
	struct pci_dev *isp_pci_dev;
	void __iomem *isp_pci_reg;
	unsigned int isp_pci_addr;
	unsigned int isp_pci_size;
	unsigned int isp_pci_irq;
	unsigned int isp_pci_inuse_flag;
	unsigned int isp_pci_ready;
};
#endif

/* hisp rproc api */
int hisp_rproc_set_domain(struct rproc *rproc,
                struct iommu_domain *domain);
struct iommu_domain *hisp_rproc_get_domain(struct rproc *rproc);
void hisp_rproc_clr_domain(struct rproc *rproc);
void *hisp_rproc_get_imgva(struct rproc *rproc);
void hisp_rproc_get_vqmem(struct rproc *rproc, u64 *pa,
		size_t *size, u32 *flags);
int hisp_rproc_vqmem_init(struct rproc *rproc);
u64 hisp_rproc_get_vringmem_pa(struct rproc *rproc, int id);
void hisp_rproc_set_rpmsg_ready(struct rproc *rproc);

/* hisp smc api in hisp_smc.c */
int hisp_smc_setparams(u64 shrd_paddr);
int hisp_smc_cpuinfo_dump(u64 param_pa_addr);
void hisp_smc_isp_init(u64 pgd_base);
void hisp_smc_isp_exit(void);
int hisp_smc_ispcpu_init(void);
void hisp_smc_ispcpu_exit(void);
int hisp_smc_ispcpu_map(void);
void hisp_smc_ispcpu_unmap(void);
void hisp_smc_set_nonsec(void);
int hisp_smc_disreset_ispcpu(void);
void hisp_smc_ispsmmu_ns_init(u64 pgt_addr);
int hisp_smc_get_ispcpu_idle(void);
int hisp_smc_send_fiq2ispcpu(void);
int hisp_smc_isptop_power_up(void);
int hisp_smc_isptop_power_down(void);
int hisp_smc_phy_csi_connect(u64 info_addr);
int hisp_smc_phy_csi_disconnect(void);
int hisp_smc_qos_cfg(void);
int hisp_smc_csi_mode_sel(u64 mode);
int hisp_smc_i2c_pad_type(void);
int hisp_smc_i2c_deadlock(u64 type);

#ifdef DEBUG_HISP
int hisp_smc_ispnoc_r8_power_up(u64 pcie_flag, u64 pcie_addr);
int hisp_smc_ispnoc_r8_power_down(u64 pcie_flag, u64 pcie_addr);
int hisp_smc_media2_vcodec_power_up(u64 pcie_flag, u64 pcie_addr);
int hisp_smc_media2_vcodec_power_down(u64 pcie_flag, u64 pcie_addr);
#endif
int hisp_smc_vq_map(void);
int hisp_smc_vq_unmap(void);

/* hisp dpm api in hisp_dpm.c */
#ifdef CONFIG_HISP_DPM
void hisp_dpm_probe(void);
void hisp_dpm_release(void);
#endif

/* hisp fstcma api in hisp_cma.c */
void *hisp_fstcma_alloc(dma_addr_t *dma_handle, size_t size, gfp_t flag);
void hisp_fstcma_free(void *va, dma_addr_t dma_handle, size_t size);

/* hisp dump api in hisp_dump.c */
void hisp_boot_stat_dump(void);
int hisp_mntn_dumpregs(void);

/* hisp ca api in hisp_ca.c */
int hisp_ca_ta_open(void);
int hisp_ca_ta_close(void);
int hisp_ca_ispcpu_disreset(void);
int hisp_ca_ispcpu_reset(void);
int hisp_ca_sfdmem_map(struct hisp_ca_meminfo *buffer);
int hisp_ca_sfdmem_unmap(struct hisp_ca_meminfo *buffer);
int hisp_ca_dynmem_map(
	struct hisp_ca_meminfo *buffer, struct device *dev);
int hisp_ca_dynmem_unmap(
	struct hisp_ca_meminfo *buffer, struct device *dev);
int hisp_ca_imgmem_config(struct hisp_ca_meminfo *info, int info_size);
int hisp_ca_imgmem_deconfig(void);
int hisp_ca_rsvmem_config(struct hisp_ca_meminfo *info, int info_size);
int hisp_ca_rsvmem_deconfig(void);
void hisp_ca_probe(void);
void hisp_ca_remove(void);

/* hisp pwr api in hisp_pwr.c */
int hisp_bsp_read_bin(const char *partion_name, unsigned int offset,
	unsigned int length, char *buffer);
int hisp_dvfs_enable(void);
unsigned int hisp_smmuv3_mode(void);
int hisp_pwr_subsys_powerup(void);
int hisp_pwr_subsys_powerdn(void);
int hisp_pwr_core_nsec_init(u64 pgd_base);
int hisp_pwr_core_nsec_exit(void);
int hisp_pwr_core_sec_init(u64 phy_pgd_base);
int hisp_pwr_core_sec_exit(void);
int hisp_pwr_cpu_sec_init(void);
int hisp_pwr_cpu_sec_exit(void);
int hisp_pwr_cpu_nsec_dst(u64 remap_addr, unsigned int canary);
int hisp_pwr_cpu_nsec_rst(void);
int hisp_pwr_cpu_sec_dst(int ispmem_reserved, unsigned long image_addr);
int hisp_pwr_cpu_sec_rst(void);
int hisp_pwr_cpu_ca_dst(struct hisp_ca_meminfo *info, int info_size);
int hisp_pwr_cpu_ca_rst(void);
unsigned long hisp_pwr_getclkrate(unsigned int type);
int hisp_pwr_nsec_setclkrate(unsigned int type, unsigned int rate);
int hisp_pwr_sec_setclkrate(unsigned int type, unsigned int rate);
int hisp_ispcpu_qos_cfg(void);
int hisp_pwr_probe(struct platform_device *pdev);
int hisp_pwr_remove(struct platform_device *pdev);

/* hisp mempool api in hisp_mempool.c */
int hisp_mempool_init(unsigned int addr, unsigned int size);
void hisp_mempool_exit(void);

/* hisp nsec api in hisp_nsec.c */
int hisp_nsec_bin_mode(void);
int hisp_wakeup_binload_kthread(void);
u64 hisp_nsec_get_remap_pa(void);
u64 hisp_get_ispcpu_shrmem_nsecpa(void);
void *hisp_get_ispcpu_shrmem_nsecva(void);
int hisp_request_nsec_rsctable(struct rproc *rproc);
int hisp_loadbin_debug_elf(struct rproc *rproc);
int hisp_loadbin_segments(struct rproc *rproc);
void hisp_nsec_boot_dump(struct rproc *rproc, unsigned int size);
int hisp_nsec_jpeg_powerup(void);
int hisp_nsec_jpeg_powerdn(void);
int hisp_nsec_device_enable(struct rproc *rproc);
int hisp_nsec_device_disable(void);
u64 hisp_nsec_get_pgd(void);
int hisp_nsec_probe(struct platform_device *pdev);
int hisp_nsec_remove(struct platform_device *pdev);
#ifdef DEBUG_HISP
struct isp_pcie_cfg *hisp_get_pcie_cfg(void);
int hisp_check_pcie_stat(void);
#endif

/* hisp sec api in hisp_sec.c */
int hisp_ca_boot_mode(void);
u64 hisp_get_ispcpu_shrmem_secpa(void);
void *hisp_get_ispcpu_shrmem_secva(void);
void *hisp_get_dyna_array(void);
struct isp_a7mapping_s *hisp_get_ap_dyna_mapping(void);
int hisp_sec_jpeg_powerup(void);
int hisp_sec_jpeg_powerdn(void);
u64 hisp_get_param_info_pa(void);
void *hisp_get_param_info_va(void);
void hisp_sec_set_ispcpu_palist(void *listmem,
		struct scatterlist *sg, unsigned int size);
int hisp_request_sec_rsctable(struct rproc *rproc);
u64 hisp_sec_get_remap_pa(void);
unsigned int hisp_pool_mem_addr(unsigned int pool_num);
int hisp_sec_pwron_prepare(void);
int hisp_sec_device_enable(void);
int hisp_sec_device_disable(void);
int hisp_sec_probe(struct platform_device *pdev);
int hisp_sec_remove(struct platform_device *pdev);

/* hisp share api */
struct hisp_shared_para *hisp_share_get_para(void);
int hisp_share_set_para(void);
void hisp_share_update_clk_value(int type,unsigned int value);
void hisp_lock_sharedbuf(void);
void hisp_unlock_sharedbuf(void);
void hisp_rproc_state_lock(void);
void hisp_rproc_state_unlock(void);
int hisp_log_beta_mode(void);
u32 hisp_share_get_exc_flag(void);
unsigned int hisp_wait_excflag_timeout(unsigned int flag, unsigned int time);
int hisp_nsec_set_pgd(void);

/* hisp device api in hisp_device.c */
int hisp_ispcpu_is_powerup(void);
struct device *hisp_get_device(void);
int hisp_nsec_boot_mode(void);
int hisp_sec_boot_mode(void);
int hisp_get_last_boot_state(void);
void hisp_set_last_boot_state(int state);
enum hisp_rproc_case_attr hisp_get_boot_mode(void);
unsigned long hisp_get_clk_rate(unsigned int type);
u64 hisp_get_ispcpu_shrmem_pa(void);
void *hisp_get_ispcpu_shrmem_va(void);
u64 hisp_get_ispcpu_remap_pa(void);
int hisp_get_ispcpu_cfginfo(void);
int hisp_rproc_enable_status(void);
int hisp_wait_rpmsg_completion(struct rproc *rproc);
int hisp_bypass_power_updn(void);
void __iomem *hisp_dev_get_regaddr(unsigned int type);

/* hisp rdr api in hisp_rdr.c */
#ifdef CONFIG_HISP_RDR
void hisp_send_fiq2ispcpu(void);
int hisp_rdr_init(void);
void hisp_rdr_exit(void);
#else
static inline void hisp_send_fiq2ispcpu(void) { return; }
static inline int hisp_rdr_init(void) { return 0; }
static inline void hisp_rdr_exit(void) { return; }
#endif

/* hisp log api in hisp_log.c */
#ifdef CONFIG_HISP_RDR
void hisploglevel_update(void);
int hisplogcat_sync(void);
int hisplogcat_start(void);
void hisplogcat_stop(void);
void hisplog_clear_info(void);
#else
static inline void hisploglevel_update(void) { return; }
static inline int hisplogcat_sync(void) { return -1; }
static inline int hisplogcat_start(void) { return -1; }
static inline void hisplogcat_stop(void) { return; }
static inline void hisplog_clear_info(void) { return; }
#endif

/* hisp rpmsg api in hisp_rpm.c */
int hisp_rpmsg_rdr_init(void);
int hisp_rpmsg_rdr_deinit(void);

/* extern api not in hisp drivers */
#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
extern int hisp120_get_lowpower(void);
#endif
extern unsigned int a7_mmu_map(struct scatterlist *sgl, unsigned int size,
		unsigned int prot, unsigned int flag);
extern void a7_mmu_unmap(unsigned int va, unsigned int size);
extern int hw_is_fpga_board(void);/* form camera hal*/
extern u64 isp_getcurtime(void);
extern int rpmsg_sensor_ioctl(unsigned int cmd, int index, char* name);
extern void *rproc_da_to_va(struct rproc *rproc, u64 da,
		size_t len, bool *is_iomem);
extern irqreturn_t rproc_vq_interrupt(struct rproc *rproc, int notifyid);

/* hisp debug api */
#ifdef DEBUG_HISP
#define NONSEC_MAX_SIZE 64
ssize_t hisp_rdr_store(struct device *pdev,
		struct device_attribute *attr, const char *buf, size_t count);
ssize_t hisp_rdr_show(struct device *pdev,
		struct device_attribute *attr, char *buf);
ssize_t hisp_clk_show(struct device *pdev,
		struct device_attribute *attr, char *buf);
ssize_t hisp_clk_store(struct device *pdev,
		struct device_attribute *attr, const char *buf, size_t count);
ssize_t hisp_power_show(struct device *pdev,
		struct device_attribute *attr, char *buf);
ssize_t hisp_power_store(struct device *pdev,
		struct device_attribute *attr, const char *buf, size_t count);
ssize_t hisp_sec_test_regs_show(struct device *pdev,
		struct device_attribute *attr, char *buf);
ssize_t hisp_boot_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf);
ssize_t hisp_boot_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
ssize_t hisp_loadbin_update_show(struct device *pdev,
		struct device_attribute *attr, char *buf);
#endif

#endif /* _HISP_INTERNEL_H */
