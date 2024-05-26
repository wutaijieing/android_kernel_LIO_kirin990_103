#define LOG_TAG         "EFCtrl"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_efctrl.h"

#define EFCTRL_CMD_SEL               (0x0000)
#define EFCTRL_CMD_NVR               (0x0002)
#define EFCTRL_FLASH_ADDR            (0x0004)
#define EFCTRL_SRAM_ADDR             (0x0008)
#define EFCTRL_DATA_LENGTH           (0x000C)
#define EFCTRL_START_DEXC            (0x0010)
#define EFCTRL_RELEASE_FLASH         (0x0014)
#define EFCTRL_HW_STATE              (0x0018)
#define EFCTRL_CRC_RESULT            (0x001C)
#define EFCTRL_SRAM_CRC_START        (0x0020)
#define EFCTRL_FLASH_CRC_START       (0x0022)
#define EFCTRL_SF_BUSY               (0x0024)
#define EFCTRL_ET_OP_CNT             (0x003C)

/** Constants for register @ref rSFCTRLv2_CMD_SEL */
enum efctrl_cmd {
	EFCTRL_CMD_FAST_READ = 0x01u,
	EFCTRL_CMD_SE = 0x04u,
	EFCTRL_CMD_CE = 0x05u,
	EFCTRL_CMD_PP = 0x06u,
	EFCTRL_CMD_AUTO_PP = 0x07u,
};

/** emb flash controller v2 operation flags. */
enum efctrl_opflags {
	EFCTRL_OPFLAG_READ = BIT(0),
	EFCTRL_OPFLAG_SET_FLASH_ADDR = BIT(1),
	EFCTRL_OPFLAG_SRAM_DATA_XCHG = BIT(2),
	EFCTRL_OPFLAG_SET_DATA_LENGTH = BIT(3),
	EFCTRL_OPFLAG_CE = BIT(4),
	EFCTRL_OPFLAG_SE = BIT(5),
};

#define EFCTRL_CMD_RDSR_FLAGS    \
	(EFCTRL_OPFLAG_READ | \
	EFCTRL_OPFLAG_SRAM_DATA_XCHG)

#define EFCTRL_CMD_BE_FLAGS    (EFCTRL_OPFLAG_SET_FLASH_ADDR)

#define EFCTRL_CMD_CE_FLAGS    (EFCTRL_OPFLAG_CE)

#define EFCTRL_CMD_SE_FLAGS  \
		(EFCTRL_OPFLAG_SET_FLASH_ADDR | \
		EFCTRL_OPFLAG_SE)

#define EFCTRL_CMD_PP_FLAGS \
	(EFCTRL_OPFLAG_SET_FLASH_ADDR | \
	EFCTRL_OPFLAG_SRAM_DATA_XCHG  | \
	EFCTRL_OPFLAG_SET_DATA_LENGTH)

#define EFCTRL_CMD_AUTO_PP_FLAGS \
	(EFCTRL_OPFLAG_SET_FLASH_ADDR | \
	EFCTRL_OPFLAG_SRAM_DATA_XCHG  | \
	EFCTRL_OPFLAG_SET_DATA_LENGTH)

#define EFCTRL_CMD_FAST_READ_FLAGS  \
	(EFCTRL_OPFLAG_READ | \
	EFCTRL_OPFLAG_SET_FLASH_ADDR | \
	EFCTRL_OPFLAG_SRAM_DATA_XCHG  | \
	EFCTRL_OPFLAG_SET_DATA_LENGTH)

#define EFCTRL_EF_OP_CNT_CE_CFG   0xC0D26DDD
#define EFCTRL_EF_OP_CNT_SE_CFG   0xC03C107A

#define efctrl_reg_addr(cts_dev, offset)  \
	((cts_dev)->hwdata->efctrl->reg_base + offset)

static int wait_efctrl_xfer_comp(const struct cts_device *cts_dev)
{
	int retries = 0;
	u8 status;

	do {
		efctrl_read_reg_check_ret(readb, EFCTRL_SF_BUSY, &status);
		if (status == 0) {
			efctrl_write_reg_check_ret(writeb,
						   EFCTRL_RELEASE_FLASH, 0x01);
			return status;
		}

		mdelay(1);
	} while (status && retries++ < 1000);
	efctrl_write_reg_check_ret(writeb, EFCTRL_RELEASE_FLASH, 0x01);
	return -ETIMEDOUT;
}

static int efctrl_transfer(const struct cts_device *cts_dev,
			   u8 cmd, void *data, u32 flash_addr, u32 sram_addr,
			   size_t size, u32 flags)
{
	int ret;

	efctrl_write_reg_check_ret(writeb, EFCTRL_CMD_SEL, cmd);

	if (flags & EFCTRL_OPFLAG_SRAM_DATA_XCHG) {
		efctrl_write_reg_check_ret(writel, EFCTRL_SRAM_ADDR, sram_addr);

		if ((!(flags & EFCTRL_OPFLAG_READ)) && data) {
			ret = cts_sram_writesb(cts_dev, sram_addr, data, size);
			if (ret) {
				TS_LOG_ERR
				    ("Write data to exchange sram 0x%06x size %zu failed %d",
				     sram_addr, size, ret);
				return ret;
			}
		}
	}

	if (flags & EFCTRL_OPFLAG_SET_FLASH_ADDR) {
		efctrl_write_reg_check_ret(writel, EFCTRL_FLASH_ADDR,
					   flash_addr);
	}
	if (flags & EFCTRL_OPFLAG_SET_DATA_LENGTH) {
		efctrl_write_reg_check_ret(writel, EFCTRL_DATA_LENGTH,
					   (u32) size);
	}

	efctrl_write_reg_check_ret(writeb, EFCTRL_START_DEXC, 1);

	if (wait_efctrl_xfer_comp(cts_dev) != 0) {
		TS_LOG_ERR("Wait efctrl ready failed %d", ret);
		return ret;
	}

	if ((flags & EFCTRL_OPFLAG_READ) && data) {
		ret = cts_sram_readsb(cts_dev, sram_addr, data, size);
		if (ret) {
			TS_LOG_ERR
			    ("Read data from exchange sram 0x%06x size %zu failed %d",
			     sram_addr, size, ret);
			return ret;
		}
	}

	return 0;
}

static int efctrl_rdid(const struct cts_device *cts_dev, u32 *id)
{
	int ret = 0;

	*id = ICN_EMBEDDED_FLASH;
	return ret;
}

static int efctrl_se(const struct cts_device *cts_dev, u32 sector_addr)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_SE, NULL,
			       sector_addr, 0, 0, EFCTRL_CMD_SE_FLAGS);
}

static int efctrl_be(const struct cts_device *cts_dev, u32 block_addr)
{
	TS_LOG_ERR("ICN_EMBEDDED_FLASH not support block earse");
	return 0;
}

static int efctrl_ce(const struct cts_device *cts_dev)
{
	TS_LOG_ERR("chip earse");
	return efctrl_transfer(cts_dev, EFCTRL_CMD_CE, NULL,
			       0, 0, 0, EFCTRL_CMD_CE_FLAGS);
}

static int efctrl_pp(const struct cts_device *cts_dev,
		     u32 flash_addr, const void *src, size_t size)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_PP, (void *)src,
			       flash_addr,
			       cts_dev->hwdata->efctrl->xchg_sram_base, size,
			       EFCTRL_CMD_PP_FLAGS);
}

static int efctrl_auto_page_program_flash_from_sram(const struct cts_device
						    *cts_dev, u32 flash_addr,
						    u32 sram_addr, size_t size)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_AUTO_PP, NULL,
			       flash_addr, sram_addr, size,
			       EFCTRL_CMD_AUTO_PP_FLAGS);
}

static int efctrl_page_program_flash_from_sram(const struct cts_device *cts_dev,
					       u32 flash_addr, u32 sram_addr,
					       size_t len)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_PP, NULL,
			       flash_addr, sram_addr, len, EFCTRL_CMD_PP_FLAGS);
}

static int efctrl_read(const struct cts_device *cts_dev,
		       u32 flash_addr, void *dst, size_t size)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_FAST_READ, dst,
			       flash_addr,
			       cts_dev->hwdata->efctrl->xchg_sram_base, size,
			       EFCTRL_CMD_FAST_READ_FLAGS);
}

static int efctrl_nvr_enable(const struct cts_device *cts_dev, bool enable)
{
	int ret = NO_ERR;

	if (enable)
		efctrl_write_reg_check_ret(writeb, EFCTRL_CMD_NVR, 0x01);
	else
		efctrl_write_reg_check_ret(writeb, EFCTRL_CMD_NVR, 0x00);

	return ret;
}

static int efctrl_read_flash_to_sram(const struct cts_device *cts_dev,
				     u32 flash_addr, u32 sram_addr, size_t size)
{
	return efctrl_transfer(cts_dev, EFCTRL_CMD_FAST_READ, NULL,
			       flash_addr, sram_addr, size,
			       EFCTRL_CMD_FAST_READ_FLAGS);
}

int efctrl_calc_sram_crc(const struct cts_device *cts_dev,
			 u32 sram_addr, size_t size, u32 *crc)
{
	int ret;

	efctrl_write_reg_check_ret(writel, EFCTRL_SRAM_ADDR, sram_addr);
	efctrl_write_reg_check_ret(writel, EFCTRL_DATA_LENGTH, (u32) size);
	efctrl_write_reg_check_ret(writeb, EFCTRL_SRAM_CRC_START, 1);

	if (wait_efctrl_xfer_comp(cts_dev) != 0) {
		TS_LOG_ERR("Wait efctrl ready failed %d", ret);
		return ret;
	}

	efctrl_read_reg_check_ret(readl, EFCTRL_CRC_RESULT, crc);

	return 0;
}

int efctrl_calc_flash_crc(const struct cts_device *cts_dev,
			  u32 flash_addr, size_t size, u32 *crc)
{
	int ret;

	TS_LOG_INFO("Calc crc from flash 0x%06x size %zu", flash_addr, size);

	efctrl_write_reg_check_ret(writel, EFCTRL_FLASH_ADDR, flash_addr);
	efctrl_write_reg_check_ret(writel, EFCTRL_DATA_LENGTH, (u32) size);
	efctrl_write_reg_check_ret(writeb, EFCTRL_FLASH_CRC_START, 1);

	if (wait_efctrl_xfer_comp(cts_dev) != 0) {
		TS_LOG_ERR("Wait efctrl ready failed %d", ret);
		return ret;
	}

	efctrl_read_reg_check_ret(readl, EFCTRL_CRC_RESULT, crc);

	return 0;
}

const struct cts_efctrl_ops cts_efctrl_ops = {
	.rdid = efctrl_rdid,
	.se = efctrl_se,
	.be = efctrl_be,
	.ce = efctrl_ce,
	.read = efctrl_read,
	.read_to_sram = efctrl_read_flash_to_sram,
	.program = efctrl_pp,
	.auto_page_program_from_sram = efctrl_auto_page_program_flash_from_sram,
	.page_program_from_sram = efctrl_page_program_flash_from_sram,
	.calc_sram_crc = efctrl_calc_sram_crc,
	.calc_flash_crc = efctrl_calc_flash_crc,
	.nvr_enable = efctrl_nvr_enable,
};
