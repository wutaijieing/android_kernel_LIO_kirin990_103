#ifndef CTS_CORE_H
#define CTS_CORE_H

#include "cts_config.h"
#include "cts_test.h"
#include "huawei_ts_kit.h"

enum cts_dev_hw_reg {
	CTS_DEV_HW_REG_HARDWARE_ID = 0x40000u,
	CTS_DEV_HW_REG_BOOT_MODE = 0x40010u,
	CTS_DEV_HW_REG_BOOT_STATUS = 0x40011u,
};

enum cts_dev_boot_mode {
	CTS_DEV_BOOT_MODE_FLASH = 1,
	CTS_DEV_BOOT_MODE_I2C_PROGRAM = 2,
	CTS_DEV_BOOT_MODE_SRAM = 3,
};
#define CTS_INFO_PROJECT_ID_ADDR 0xC60C
#define CTS_INFO_PROJECT_ID_LEN  10

enum {
	CTS_DEV_POWER_MODE_ACTIVE = 0,
	CTS_DEV_POWER_MODE_MONITOR,
	CTS_DEV_POWER_MODE_HIBERNATE,
	CTS_DEV_POWER_MODE_GESTURE_WAKEUP
};
/* I2C addresses(7bits), transfer size and bitrate */
#define CTS_DEV_NORMAL_MODE_I2CADDR         (0x42)
#define CTS_DEV_PROGRAM_MODE_I2CADDR        (0x30)
#define CTS_DEV_NORMAL_MODE_ADDR_WIDTH      (2)
#define CTS_DEV_PROGRAM_MODE_ADDR_WIDTH     (3)

/* Chipone firmware register addresses under normal mode */
enum cts_device_fw_reg {
	CTS_DEVICE_FW_REG_WORK_MODE = 0x0000,
	CTS_DEVICE_FW_REG_SYS_BUSY = 0x0001,
	CTS_DEVICE_FW_REG_DATA_READY = 0x0002,
	CTS_DEVICE_FW_REG_CMD = 0x0004,
	CTS_DEVICE_FW_REG_POWER_MODE = 0x0005,
	CTS_DEVICE_FW_REG_CHARGER_STATUS = 0x0007,
	CTS_DEVICE_FW_REG_FW_LIB_MAIN_VERSION = 0x0009,
	CTS_DEVICE_FW_REG_CHIP_TYPE = 0x000A,
	CTS_DEVICE_FW_REG_FW_VERSION = 0x000C,
	CTS_DEVICE_FW_REG_GET_RAW_CFG = 0x0012,
	CTS_DEVICE_FW_REG_GET_PANNEL_ID = 0x0043,
	CTS_DEVICE_FW_REG_GET_SHORT_TEST_STATUS = 0x0046,
	CTS_DEVICE_FW_REG_GET_SHORT_OPEN_TEST_START_FLAG = 0x0047,
	CTS_DEVICE_FW_REG_COMPENSATE_CAP_READY = 0x004E,
	CTS_DEVICE_FW_REG_PROJECT_ID = 0x005A,
	CTS_DEVICE_FW_REG_GET_JITTER_FLAG = 0x0064,
	CTS_DEVICE_FW_REG_TOUCH_INFO = 0x1000,
	CTS_DEVICE_FW_REG_RAW_DATA = 0x2000,
	CTS_DEVICE_FW_REG_RAW_DATA_SELF_CAP = 0x2080,
	CTS_DEVICE_FW_REG_DIFF_DATA = 0x3000,
	CTS_DEVICE_FW_REG_JITTER_MUTUAL_DATA = 0x5140,
	CTS_DEVICE_FW_REG_JITTER_SELF_DATA = 0x5180,
	CTS_DEVICE_FW_REG_BASELINE = 0x6000,
	CTS_DEVICE_FW_REG_PANEL_PARAM = 0x8000,
	CTS_DEVICE_FW_REG_X_RESOLUTION = 0x8000,
	CTS_DEVICE_FW_REG_Y_RESOLUTION = 0x8002,
	CTS_DEVICE_FW_REG_NUM_TX = 0x8004,
	CTS_DEVICE_FW_REG_NUM_RX = 0x8005,
	CTS_DEVICE_FW_REG_TX_ORDER = 0x8006,
	CTS_DEVICE_FW_REG_RX_ORDER = 0x8012,
	CTS_DEVICE_FW_REG_MAX_TOUCH_NUM = 0x8022,
	CTS_DEVICE_FW_REG_X_Y_SWAP = 0x8083,
	CTS_DEVICE_FW_REG_INT_MODE = 0x8084,
	CTS_DEVICE_FW_REG_INT_KEEP_TIME = 0x8085,
	CTS_DEVICE_FW_REG_LOW_POWER_EN = 0x8168,
	CTS_DEVICE_FW_REG_SHORT_DATA = 0xB000,
	CTS_DEVICE_FW_REG_OPEN_DATA = 0xB040,
	CTS_DEVICE_FW_REG_DEBUG_INTF = 0xF000,
};

/* Hardware IDs, read from hardware id register */
enum cts_dev_hwid {
	CTS_DEV_HWID_ICNT8918 = 0x8918u,
	CTS_DEV_HWID_ANY = 0u,
	CTS_DEV_HWID_INVALID = 0xFFFFu,
};

enum cts_dev_fwid {
	CTS_DEV_FWID_ICNT8918 = 0x8918u,
	CTS_DEV_FWID_ANY = 0u,
	CTS_DEV_FWID_INVALID = 0xFFFFu
};

/* Commands written to firmware register @ref CTS_DEVICE_FW_REG_CMD under normal mode */
enum cts_firmware_cmd {
	CTS_CMD_RESET = 0x01,
	CTS_CMD_SUSPEND = 0x02,
	CTS_CMD_ENTER_WRITE_PARA_TO_FLASH_MODE = 0x03,
	CTS_CMD_WRITE_PARA_TO_FLASH = 0x04,
	CTS_CMD_WRTITE_INT_HIGH = 0x05,
	CTS_CMD_WRTITE_INT_LOW = 0x06,
	CTS_CMD_RELASE_INT_TEST = 0x07,
	CTS_CMD_ENABLE_READ_RAWDATA = 0x20,
	CTS_CMD_DISABLE_READ_RAWDATA = 0x21,
	CTS_CMD_SUSPEND_WITH_GESTURE = 0x40,
	CTS_CMD_QUIT_GESTURE_MONITOR = 0x41,
	CTS_CMD_CHARGER_ATTACHED = 0x55,
	CTS_CMD_CHARGER_DETACHED = 0x66,
	CTS_CMD_MONITOR_OFF = 0xA4,
	CTS_CMD_FREQ_HOP_OFF = 0xA5,
	CTS_CMD_SHORT_OPEN_TEST = 0xA6,
	CTS_CMD_MONITOR_ON = 0xA7,
	CTS_CMD_FREQ_HOP_ON = 0xA8,
	CTS_CMD_LOW_POWER_OFF = 0xA9,
	CTS_CMD_LOW_POWER_ON = 0xAA,
	CTS_CMD_JITTER_TEST_START = 0xAB,
	CTS_CMD_JITTER_TEST_EXIT = 0xAC,
};

#define CTS_DEVICE_TOUCH_EVENT_NONE         (0)
#define CTS_DEVICE_TOUCH_EVENT_DOWN         (1)
#define CTS_DEVICE_TOUCH_EVENT_MOVE         (2)
#define CTS_DEVICE_TOUCH_EVENT_STAY         (3)
#define CTS_DEVICE_TOUCH_EVENT_UP           (4)

#pragma pack(1)
/** Touch message read back from chip */
struct cts_device_touch_msg {
	u8 id;
	__le16 x;
	__le16 y;
	u8 pressure;
	u8 event;
};

/** Touch information read back from chip */
struct cts_device_touch_info {
	u8 num_msg;
	struct cts_device_touch_msg msgs[CFG_CTS_MAX_TOUCH_NUM];
};

#pragma pack()

struct cts_device;

enum cts_work_mode {
	CTS_WORK_MODE_UNKNOWN = 0,
	CTS_WORK_MODE_SUSPEND,
	CTS_WORK_MODE_NORMAL_ACTIVE,
	CTS_WORK_MODE_NORMAL_IDLE,
	CTS_WORK_MODE_GESTURE_ACTIVE,
	CTS_WORK_MODE_GESTURE_IDLE,
};

/** Chip hardware data, will never change */
struct cts_device_hwdata {
	char *name;
	char project_id[CTS_INFO_PROJECT_ID_LEN + 1];
	u16 hwid;
	u16 fwid;
	u8 num_row;
	u8 num_col;
	u32 sram_size;
	/* Address width under program mode */
	u8 program_addr_width;
	u16 ver_offset;
	const struct cts_efctrl *efctrl;
};

/** Chip firmware data */
struct cts_device_fwdata {
	u16 version;
	u16 res_x;
	u16 res_y;
	u8 rows;
	u8 cols;
	bool flip_x;
	bool flip_y;
	u8 swap_axes;
	u8 int_mode;
	u8 esd_method;
	u16 lib_version;
	u16 int_keep_time;
	u16 rawdata_target;
	u16 pannel_id;
};

/** Chip runtime data */
struct cts_device_rtdata {
	u8 slave_addr;
	int addr_width;
	bool program_mode;
	bool has_flash;
	bool suspended;
	bool updating;
	bool testing;
	bool gesture_wakeup_enabled;
	bool charger_exist;
	bool fw_log_redirect_enabled;
	bool glove_mode_enabled;
};

struct cts_device {
	struct rt_mutex dev_lock;
	struct cts_platform_data *pdata;
	const struct cts_device_hwdata *hwdata;
	struct cts_device_fwdata fwdata;
	struct cts_device_rtdata rtdata;
	const struct cts_flash *flash;
	bool enabled;
};

struct cts_platform_data;
struct ts_kit_device_data;
struct cts_firmware;

struct chipone_ts_data {
	struct ts_kit_device_data *tskit_data;
	u32 hide_fw_name;
	char boot_firmware_filename[256];
	const struct cts_firmware *cached_firmware;
	bool use_pinctrl;
	struct pinctrl *pinctrl;
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	struct pinctrl_state *pinctrl_state_reset_high;
	struct pinctrl_state *pinctrl_state_reset_low;
	struct pinctrl_state *pinctrl_state_release;
	struct pinctrl_state *pinctrl_state_int_high;
	struct pinctrl_state *pinctrl_state_int_low;
	struct pinctrl_state *pinctrl_state_as_int;
#else
	struct pinctrl_state *pinctrl_state_default;
	struct pinctrl_state *pinctrl_state_idle;
#endif
	bool use_huawei_charger_detect;
	struct mutex wrong_touch_lock;
	/* Project settings */
	char project_id[CTS_INFO_PROJECT_ID_LEN + 1];
	u8 boot_hw_reset_timing[3];
	u8 resume_hw_reset_timing[3];
	/* Factory test */
	bool dump_factory_test_data_to_console;
	bool dump_factory_test_data_to_buffer;
	bool dump_factory_test_data_to_file;
	bool print_factory_test_data_only_fail;
	char factory_test_data_directory[512];
	u8 invalid_node_num;
	u32 invalid_nodes[TX_MAX_CHANNEL_NUM * TX_MAX_CHANNEL_NUM];
	bool test_reset_pin;
	int reset_pin_test_result;
	struct cts_test_param reset_pin_test_param;

	bool test_int_pin;
	int int_pin_test_result;
	struct cts_test_param int_pin_test_param;

	bool test_rawdata;
	int rawdata_test_result;
	int selfrawdata_test_result;
	int rawdata_test_min;
	int rawdata_test_max;
	int self_rawdata_test_min;
	int self_rawdata_test_max;
	int rawdata_test_data_size;
	struct cts_rawdata_test_priv_param rawdata_test_priv_param;
	struct cts_test_param rawdata_test_param;

	bool test_deviation;
	int deviation_test_result;
	int selfdeviation_test_result;
	int deviation_test_min;
	int deviation_test_max;
	int self_deviation_test_min;
	int self_deviation_test_max;
	int deviation_test_data_size;
	struct cts_deviation_test_priv_param deviation_test_priv_param;
	struct cts_test_param deviation_test_param;

	bool test_noise;
	int noise_test_result;
	int selfnoise_test_result;
	int noise_test_max;
	int noise_test_data_size;
	struct cts_noise_test_priv_param noise_test_priv_param;
	struct cts_test_param noise_test_param;

	bool test_open;
	int open_test_result;
	int open_test_min;
	int open_test_max;
	int open_test_data_size;
	struct cts_test_param open_test_param;

	bool test_short;
	int short_test_result;
	int short_test_min;
	int short_test_data_size;
	struct cts_test_param short_test_param;

	int test_capacitance_via_csvfile;
	int csvfile_use_product_system;

	struct cts_device_touch_info touch_info;

	u8 i2c_fifo_buf[CFG_CTS_MAX_I2C_XFER_SIZE];

	struct device *device;

	struct cts_device cts_dev;

	struct cts_platform_data *pdata;

	struct workqueue_struct *workqueue;
#ifdef CONFIG_CTS_LEGACY_TOOL
	struct proc_dir_entry *procfs_entry;
#endif
};

static inline u32 get_unaligned_le24(const void *p)
{
	const u8 *puc;

	puc = (const u8 *)p;
	return (puc[0] | (puc[1] << 8) | (puc[2] << 16));
}

static inline u32 get_unaligned_be24(const void *p)
{
	const u8 *puc;

	puc = (const u8 *)p;
	return (puc[2] | (puc[1] << 8) | (puc[0] << 16));
}

static inline void put_unaligned_be24(u32 v, void *p)
{
	u8 *puc;

	puc = (u8 *)p;
	puc[0] = (v >> 16) & 0xFF;
}

#define wrap(max, x)        ((max) - 1 - (x))

void cts_lock_device(struct cts_device *cts_dev);
void cts_unlock_device(struct cts_device *cts_dev);

int cts_sram_writeb_retry(const struct cts_device *cts_dev,
			  u32 addr, u8 b, int retry, int delay);
int cts_sram_writew_retry(const struct cts_device *cts_dev,
			  u32 addr, u16 w, int retry, int delay);
int cts_sram_writel_retry(const struct cts_device *cts_dev,
			  u32 addr, u32 l, int retry, int delay);
int cts_sram_writesb_retry(const struct cts_device *cts_dev,
			   u32 addr, const void *src, size_t len,
			   int retry, int delay);
int cts_sram_writesb_check_crc_retry(const struct cts_device *cts_dev,
				     u32 addr, const void *src,
				     size_t len, u32 crc, int retry);

int cts_sram_readb_retry(const struct cts_device *cts_dev,
			 u32 addr, u8 *b, int retry, int delay);
int cts_sram_readw_retry(const struct cts_device *cts_dev,
			 u32 addr, u16 *w, int retry, int delay);
int cts_sram_readl_retry(const struct cts_device *cts_dev,
			 u32 addr, u32 *l, int retry, int delay);
int cts_sram_readsb_retry(const struct cts_device *cts_dev,
			  u32 addr, void *dst, size_t len, int retry,
			  int delay);

int cts_fw_reg_writeb_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u8 b, int retry, int delay);
int cts_fw_reg_writew_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u16 w, int retry, int delay);
int cts_fw_reg_writel_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u32 l, int retry, int delay);
int cts_fw_reg_writesb_retry(const struct cts_device *cts_dev,
			     u32 reg_addr, const void *src, size_t len,
			     int retry, int delay);

int cts_fw_reg_readb_retry(const struct cts_device *cts_dev,
			   u32 reg_addr, u8 *b, int retry, int delay);
int cts_fw_reg_readw_retry(const struct cts_device *cts_dev,
			   u32 reg_addr, u16 *w, int retry, int delay);
int cts_fw_reg_readl_retry(const struct cts_device *cts_dev,
			   u32 reg_addr, u32 *l, int retry, int delay);
int cts_fw_reg_readsb_retry(const struct cts_device *cts_dev, u32 reg_addr,
			    void *dst, size_t len, int retry, int delay);
int cts_fw_reg_readsb_retry_delay_idle(const struct cts_device *cts_dev,
				       u32 reg_addr, void *dst, size_t len,
				       int retry, int delay, int idle);

int cts_hw_reg_writeb_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u8 b, int retry, int delay);
int cts_hw_reg_writew_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u16 w, int retry, int delay);
int cts_hw_reg_writel_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, u32 l, int retry, int delay);
int cts_hw_reg_writesb_retry(const struct cts_device *cts_dev,
			     u32 reg_addr, const void *src, size_t len,
			     int retry, int delay);

int cts_hw_reg_readb_retry(const struct cts_device *cts_dev,
			   u32 reg_addr, u8 *b, int retry, int delay);
int cts_hw_reg_readw_retry(const struct cts_device *cts_dev, u32 reg_addr,
			   u16 *w, int retry, int delay);
int cts_hw_reg_readl_retry(const struct cts_device *cts_dev,
			   u32 reg_addr, u32 *l, int retry, int delay);
int cts_hw_reg_readsb_retry(const struct cts_device *cts_dev,
			    u32 reg_addr, void *dst, size_t len, int retry,
			    int delay);

static inline int cts_fw_reg_writeb(const struct cts_device *cts_dev,
				    u32 reg_addr, u8 b)
{
	return cts_fw_reg_writeb_retry(cts_dev, reg_addr, b, 1, 0);
}

static inline int cts_fw_reg_writew(const struct cts_device *cts_dev,
				    u32 reg_addr, u16 w)
{
	return cts_fw_reg_writew_retry(cts_dev, reg_addr, w, 1, 0);
}

static inline int cts_fw_reg_writel(const struct cts_device *cts_dev,
				    u32 reg_addr, u32 l)
{
	return cts_fw_reg_writel_retry(cts_dev, reg_addr, l, 1, 0);
}

static inline int cts_fw_reg_writesb(const struct cts_device *cts_dev,
				     u32 reg_addr, const void *src, size_t len)
{
	return cts_fw_reg_writesb_retry(cts_dev, reg_addr, src, len, 1, 0);
}

static inline int cts_fw_reg_readb(const struct cts_device *cts_dev,
				   u32 reg_addr, u8 *b)
{
	return cts_fw_reg_readb_retry(cts_dev, reg_addr, b, 1, 0);
}

static inline int cts_fw_reg_readw(const struct cts_device *cts_dev,
				   u32 reg_addr, u16 *w)
{
	return cts_fw_reg_readw_retry(cts_dev, reg_addr, w, 1, 0);
}

static inline int cts_fw_reg_readl(const struct cts_device *cts_dev,
				   u32 reg_addr, u32 *l)
{
	return cts_fw_reg_readl_retry(cts_dev, reg_addr, l, 1, 0);
}

static inline int cts_fw_reg_readsb(const struct cts_device *cts_dev,
				    u32 reg_addr, void *dst, size_t len)
{
	return cts_fw_reg_readsb_retry(cts_dev, reg_addr, dst, len, 1, 0);
}

static inline int cts_fw_reg_readsb_delay_idle(const struct cts_device *cts_dev,
					       u32 reg_addr, void *dst,
					       size_t len, int idle)
{
	return cts_fw_reg_readsb_retry_delay_idle(cts_dev, reg_addr, dst, len,
						  1, 0, idle);
}

static inline int cts_hw_reg_writeb(const struct cts_device *cts_dev,
				    u32 reg_addr, u8 b)
{
	return cts_hw_reg_writeb_retry(cts_dev, reg_addr, b, 1, 0);
}

static inline int cts_hw_reg_writew(const struct cts_device *cts_dev,
				    u32 reg_addr, u16 w)
{
	return cts_hw_reg_writew_retry(cts_dev, reg_addr, w, 1, 0);
}

static inline int cts_hw_reg_writel(const struct cts_device *cts_dev,
				    u32 reg_addr, u32 l)
{
	return cts_hw_reg_writel_retry(cts_dev, reg_addr, l, 1, 0);
}

static inline int cts_hw_reg_writesb(const struct cts_device *cts_dev,
				     u32 reg_addr, const void *src, size_t len)
{
	return cts_hw_reg_writesb_retry(cts_dev, reg_addr, src, len, 1, 0);
}

static inline int cts_hw_reg_readb(const struct cts_device *cts_dev,
				   u32 reg_addr, u8 *b)
{
	return cts_hw_reg_readb_retry(cts_dev, reg_addr, b, 1, 0);
}

static inline int cts_hw_reg_readw(const struct cts_device *cts_dev,
				   u32 reg_addr, u16 *w)
{
	return cts_hw_reg_readw_retry(cts_dev, reg_addr, w, 1, 0);
}

static inline int cts_hw_reg_readl(const struct cts_device *cts_dev,
				   u32 reg_addr, u32 *l)
{
	return cts_hw_reg_readl_retry(cts_dev, reg_addr, l, 1, 0);
}

static inline int cts_hw_reg_readsb(const struct cts_device *cts_dev,
				    u32 reg_addr, void *dst, size_t len)
{
	return cts_hw_reg_readsb_retry(cts_dev, reg_addr, dst, len, 1, 0);
}

static inline int cts_sram_writeb(const struct cts_device *cts_dev, u32 addr,
				  u8 b)
{
	return cts_sram_writeb_retry(cts_dev, addr, b, 1, 0);
}

static inline int cts_sram_writew(const struct cts_device *cts_dev, u32 addr,
				  u16 w)
{
	return cts_sram_writew_retry(cts_dev, addr, w, 1, 0);
}

static inline int cts_sram_writel(const struct cts_device *cts_dev, u32 addr,
				  u32 l)
{
	return cts_sram_writel_retry(cts_dev, addr, l, 1, 0);
}

static inline int cts_sram_writesb(const struct cts_device *cts_dev, u32 addr,
				   const void *src, size_t len)
{
	return cts_sram_writesb_retry(cts_dev, addr, src, len, 1, 0);
}

static inline int cts_sram_readb(const struct cts_device *cts_dev, u32 addr,
				 u8 *b)
{
	return cts_sram_readb_retry(cts_dev, addr, b, 1, 0);
}

static inline int cts_sram_readw(const struct cts_device *cts_dev, u32 addr,
				 u16 *w)
{
	return cts_sram_readw_retry(cts_dev, addr, w, 1, 0);
}

static inline int cts_sram_readl(const struct cts_device *cts_dev, u32 addr,
				 u32 *l)
{
	return cts_sram_readl_retry(cts_dev, addr, l, 1, 0);
}

static inline int cts_sram_readsb(const struct cts_device *cts_dev,
				  u32 addr, void *dst, size_t len)
{
	return cts_sram_readsb_retry(cts_dev, addr, dst, len, 1, 0);
}

static inline void cts_set_program_addr(struct cts_device *cts_dev)
{
	cts_dev->rtdata.slave_addr = CTS_DEV_PROGRAM_MODE_I2CADDR;
	cts_dev->rtdata.program_mode = true;
	cts_dev->rtdata.addr_width = CTS_DEV_PROGRAM_MODE_ADDR_WIDTH;
}

static inline void cts_set_normal_addr(struct cts_device *cts_dev)
{
	cts_dev->rtdata.slave_addr = CTS_DEV_NORMAL_MODE_I2CADDR;
	cts_dev->rtdata.program_mode = false;
	cts_dev->rtdata.addr_width = CTS_DEV_NORMAL_MODE_ADDR_WIDTH;
}

bool cts_is_device_suspended(const struct cts_device *cts_dev);
int cts_suspend_device(struct cts_device *cts_dev);
int cts_resume_device(struct cts_device *cts_dev);

bool cts_is_device_program_mode(const struct cts_device *cts_dev);
int cts_enter_program_mode(struct cts_device *cts_dev);
int cts_enter_normal_mode(struct cts_device *cts_dev);

int cts_probe_device(struct cts_device *cts_dev);
int cts_set_work_mode(const struct cts_device *cts_dev, u8 mode);
int cts_get_work_mode(const struct cts_device *cts_dev, u8 *mode);
int cts_get_firmware_version(const struct cts_device *cts_dev, u16 *version);
int cts_get_data_ready_flag(const struct cts_device *cts_dev, u8 *flag);
int cts_clr_data_ready_flag(const struct cts_device *cts_dev);
int cts_send_command(const struct cts_device *cts_dev, u8 cmd);
int cts_get_touchinfo(const struct cts_device *cts_dev,
		      struct cts_device_touch_info *touch_info);
int cts_get_panel_param(const struct cts_device *cts_dev,
			void *param, size_t size);
int cts_set_panel_param(const struct cts_device *cts_dev,
			const void *param, size_t size);
int cts_get_x_resolution(const struct cts_device *cts_dev, u16 *resolution);
int cts_get_y_resolution(const struct cts_device *cts_dev, u16 *resolution);
int cts_get_num_rows(const struct cts_device *cts_dev, u8 *num_rows);
int cts_get_num_cols(const struct cts_device *cts_dev, u8 *num_cols);
int cts_init_fwdata(struct cts_device *cts_dev);

enum cts_get_touch_data_flags {
	CTS_GET_TOUCH_DATA_FLAG_ENABLE_GET_TOUCH_DATA_BEFORE = BIT(0),
	CTS_GET_TOUCH_DATA_FLAG_CLEAR_DATA_READY = BIT(1),
	CTS_GET_TOUCH_DATA_FLAG_REMOVE_TOUCH_DATA_BORDER = BIT(2),
	CTS_GET_TOUCH_DATA_FLAG_FLIP_TOUCH_DATA = BIT(3),
	CTS_GET_TOUCH_DATA_FLAG_DISABLE_GET_TOUCH_DATA_AFTER = BIT(4),
};
int cts_enable_get_shortdata(const struct cts_device *cts_dev);
int cts_disable_get_shortdata(const struct cts_device *cts_dev);
int cts_enable_get_rawdata(const struct cts_device *cts_dev);
int cts_disable_get_rawdata(const struct cts_device *cts_dev);
int cts_get_rawdata(const struct cts_device *cts_dev, void *buf);
int cts_get_diffdata(const struct cts_device *cts_dev, void *buf);
int cts_get_jitter(const struct cts_device *cts_dev, void *buf);
int cts_get_fwid(struct cts_device *cts_dev, u16 *fwid);
int cts_get_hwid(struct cts_device *cts_dev, u16 *hwid);
int cts_get_short_test_status(const struct cts_device *cts_dev, u8 *status);
int chipone_chip_reset(void);

#ifdef CFG_CTS_GESTURE
void cts_enable_gesture_wakeup(struct cts_device *cts_dev);
void cts_disable_gesture_wakeup(struct cts_device *cts_dev);
bool cts_is_gesture_wakeup_enabled(const struct cts_device *cts_dev);
int cts_get_gesture_info(const struct cts_device *cts_dev,
	void *gesture_info, bool trace_point);
#else
static inline void cts_enable_gesture_wakeup(struct cts_device *cts_dev)
{
};

static inline void cts_disable_gesture_wakeup(struct cts_device *cts_dev)
{
};

static inline bool cts_is_gesture_wakeup_enabled(const struct cts_device
						 *cts_dev)
{
};

static inline int  cts_get_gesture_info(const struct cts_device *cts_dev,
	void *gesture_info, bool trace_point)
{
};
#endif
#ifdef CONFIG_CTS_LEGACY_TOOL
int cts_tool_init(struct chipone_ts_data *cts_data);
void cts_tool_deinit(struct chipone_ts_data *data);
#else
static inline int cts_tool_init(struct chipone_ts_data *cts_data)
{
	return 0;
}

static inline void cts_tool_deinit(struct chipone_ts_data *data)
{
}
#endif
bool cts_is_device_enabled(const struct cts_device *cts_dev);
int cts_start_device(struct cts_device *cts_dev);
int cts_stop_device(struct cts_device *cts_dev);
bool cts_is_fwid_valid(u16 fwid);
int cts_prog_get_project_id(struct cts_device *cts_dev);
int cts_normal_get_project_id(struct cts_device *cts_dev);
int cts_get_opendata(const struct cts_device *cts_dev, void *buf);
int cts_start_fw_short_openetest(const struct cts_device *cts_dev);
int cts_get_selfcap_diffdata(const struct cts_device *cts_dev, void *buf);
int cts_get_selfcap_rawdata(const struct cts_device *cts_dev, void *buf);
#endif
