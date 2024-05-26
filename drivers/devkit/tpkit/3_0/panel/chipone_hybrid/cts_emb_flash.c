#define LOG_TAG         "Flash"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_efctrl.h"
#include "cts_emb_flash.h"

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static const struct cts_flash cts_flashes[] = {
	{"Icnt8918_embflash",
	 ICN_EMBEDDED_FLASH, 0, 128, 512, 0, 0xA00, 0xCA00},

};

static const struct cts_flash *find_flash_by_jedec_id(u32 jedec_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cts_flashes); i++) {
		if (cts_flashes[i].jedec_id == jedec_id)
			return &cts_flashes[i];
	}

	return NULL;
}

static int probe_flash(struct cts_device *cts_dev)
{
	int ret;

	TS_LOG_INFO("Probe flash");

	if (cts_dev->hwdata->efctrl->ops->rdid != NULL) {
		u32 id;

		TS_LOG_INFO("Read JEDEC ID");

		ret = cts_dev->hwdata->efctrl->ops->rdid(cts_dev, &id);
		if (ret) {
			TS_LOG_ERR("Read JEDEC ID failed %d", ret);
			return ret;
		}

		cts_dev->flash = find_flash_by_jedec_id(id);
		if (cts_dev->flash == NULL) {
			TS_LOG_ERR("Unknown JEDEC ID: %06x", id);
			return -ENODEV;
		}

		TS_LOG_INFO("Flash type: '%s'", cts_dev->flash->name);
		return 0;
	} else {
		TS_LOG_ERR("Probe flash while rdid == NULL");
		return -ENOTSUPP;
	}
}

static int erase_chip_retry(const struct cts_device *cts_dev, int retry)
{
	int ret, retries;

	retries = 0;
	do {
		retries++;
		ret = cts_dev->hwdata->efctrl->ops->ce(cts_dev);
		if (ret) {
			TS_LOG_ERR("Erase chip  failed %d retries %d", ret,
				   retries);
			continue;
		}
		break;
	} while (retries < retry);

	return ret;
}

/** Erase the whole flash */
static inline int erase_chip(const struct cts_device *cts_dev)
{
	return erase_chip_retry(cts_dev, CTS_FLASH_ERASE_DEFAULT_RETRY);
}

/** Make sure sector addr is sector aligned && < flash total size */
static int erase_sector_retry(const struct cts_device *cts_dev,
			      u32 sector_addr, int retry)
{
	int ret, retries;

	TS_LOG_INFO("  Erase sector 0x%06x", sector_addr);

	retries = 0;
	do {
		retries++;
		ret = cts_dev->hwdata->efctrl->ops->se(cts_dev, sector_addr);
		if (ret) {
			TS_LOG_ERR("Erase sector 0x%06x failed %d retries %d",
				   sector_addr, ret, retries);
			continue;
		}
		break;
	} while (retries < retry);

	return ret;
}

/** Make sure sector addr is sector aligned && < flash total size */
static inline int erase_sector(const struct cts_device *cts_dev,
			       u32 sector_addr)
{
	return erase_sector_retry(cts_dev, sector_addr,
				  CTS_FLASH_ERASE_DEFAULT_RETRY);
}

/** Make sure block addr is block aligned && < flash total size */
static int erase_block_retry(const struct cts_device *cts_dev,
			     u32 block_addr, int retry)
{
	int ret, retries;

	TS_LOG_INFO("  Erase block  0x%06x", block_addr);

	retries = 0;
	do {
		retries++;

		ret = cts_dev->hwdata->efctrl->ops->be(cts_dev, block_addr);
		if (ret) {
			TS_LOG_ERR("Erase block 0x%06x failed %d retries %d",
				   block_addr, ret, retries);
			continue;
		}
	} while (retries < retry);

	return ret;
}

/** Make sure block addr is block aligned && < flash total size */
static inline int erase_block(const struct cts_device *cts_dev, u32 block_addr)
{
	return erase_block_retry(cts_dev, block_addr,
				 CTS_FLASH_ERASE_DEFAULT_RETRY);
}

int cts_prepare_flash_operation(struct cts_device *cts_dev)
{
	int ret;
	bool program_mode = cts_is_device_program_mode(cts_dev);
	bool enabled = cts_is_device_enabled(cts_dev);

	if (cts_dev == NULL) {
		TS_LOG_ERR("%s:cts_dev is null\n", __func__);
		return -EINVAL;
	}

	TS_LOG_INFO("Prepare for flash operation");
	if (enabled) {
		ret = cts_stop_device(cts_dev);
		if (ret) {
			TS_LOG_ERR("Stop device failed %d", ret);
			return ret;
		}
	}
	if (!program_mode) {
		ret = cts_enter_program_mode(cts_dev);
		if (ret) {
			TS_LOG_ERR("Enter program mode failed %d", ret);
			return ret;
		}
	}
	if (cts_dev->flash == NULL) {
		TS_LOG_INFO("Flash is not initialized, try to probe");
		ret = probe_flash(cts_dev);
		if (ret) {
			cts_dev->rtdata.has_flash = false;
			TS_LOG_INFO("Probe flash failed %d", ret);
			return ret;
		}
	}
	cts_dev->rtdata.has_flash = true;
	TS_LOG_INFO("Flash has been initialized");
	return ret;
}

int cts_read_flash(const struct cts_device *cts_dev,
			 u32 flash_addr, void *dst, size_t size, int retry)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret;

	size = min(size, flash->total_size - flash_addr);

	TS_LOG_INFO("Read actually from 0x%06x size %zu", flash_addr, size);

	while (size) {
		size_t l;

		l = min(efctrl->xchg_sram_size, size);

		ret = efctrl->ops->read(cts_dev, flash_addr, dst, l);
		if (ret < 0) {
			TS_LOG_ERR("Read from 0x%06x size %zu failed %d",
				   flash_addr, size, ret);
			return ret;
		}

		dst += l;
		size -= l;
		flash_addr += l;
	}

	if (flash_addr > flash->total_size - flash->nvr_size) {
		ret = efctrl->ops->nvr_enable(cts_dev, false);
		if (ret < 0) {
			TS_LOG_ERR("Disable nvr sector read ");
			return ret;
		}
	}
	return ret;
}
int cts_read_flash_retry(const struct cts_device *cts_dev,
			 u32 flash_addr, void *dst, size_t size, int retry)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret;

	TS_LOG_INFO("Read from 0x%06x size %zu", flash_addr, size);

	if (cts_dev == NULL) {
		TS_LOG_ERR("%s:cts_dev is null\n", __func__);
		return -EINVAL;
	}

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL ||
	    efctrl->ops == NULL || efctrl->ops->read == NULL) {
		TS_LOG_ERR("Read not supported");
		return -ENOTSUPP;
	}

	if (flash_addr > flash->total_size) {
		TS_LOG_ERR("Read from 0x%06x > flash size 0x%06zx",
			   flash_addr, flash->total_size);
		return -EINVAL;
	} else if (flash_addr > flash->total_size - flash->nvr_size) {
		TS_LOG_INFO("Read fro nvr sector from 0x%06x ", flash_addr);
		ret = efctrl->ops->nvr_enable(cts_dev, true);
		if (ret < 0) {
			TS_LOG_ERR("Enable nvr sector read ");
			return ret;
		}
	}
	return ret;
}

int cts_read_flash_to_sram_retry(const struct cts_device *cts_dev,
				 u32 flash_addr, u32 sram_addr, size_t size,
				 int retry)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret, retries;

	TS_LOG_INFO("Read from 0x%06x to sram 0x%06x size %zu",
		    flash_addr, sram_addr, size);

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL ||
	    efctrl->ops == NULL || efctrl->ops->read_to_sram == NULL) {
		TS_LOG_ERR("Read to sram not supported");
		return -ENOTSUPP;
	}

	if (flash_addr > flash->total_size) {
		TS_LOG_ERR("Read to sram from 0x%06x > flash size 0x%06zx",
			   flash_addr, flash->total_size);
		return -EINVAL;
	}
	size = min(size, flash->total_size - flash_addr);

	TS_LOG_INFO("Read to sram actually from 0x%06x size %zu", flash_addr,
		    size);

	retries = 0;
	do {
		retries++;
		ret =
		    efctrl->ops->read_to_sram(cts_dev, flash_addr, sram_addr,
					      size);
		if (ret) {
			TS_LOG_ERR
			    ("Read from 0x%06x to sram 0x%06x size %zu failed %d retries %d",
			     flash_addr, sram_addr, size, ret, retries);
			continue;
		}
	} while (retries < retry);

	return ret;
}

int cts_read_flash_to_sram_check(const struct cts_device *cts_dev,
	u32 flash_addr, u32 sram_addr,
	size_t size, u32 crc, int retry)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int retries = 0;

	do {
		u32 crc_sram;

		retries++;

		ret =
		    efctrl->ops->read_to_sram(cts_dev, flash_addr, sram_addr,
					      size);
		if (ret) {
			TS_LOG_ERR
			    ("Read from 0x%06x to sram 0x%06x size %zu failed %d retries %d",
			     flash_addr, sram_addr, size, ret, retries);
			continue;
		}

		ret =
		    efctrl->ops->calc_sram_crc(cts_dev, sram_addr, size,
					       &crc_sram);
		if (ret) {
			TS_LOG_ERR("Get crc for read from 0x%06x to sram 0x%06x size %zu failed %d retries %d",
				flash_addr, sram_addr, size, ret, retries);
			continue;
		}

		if (crc == crc_sram)
			return 0;

		TS_LOG_ERR("Check crc for read from 0x%06x to sram 0x%06x size %zu mismatch %x != %x, retries %d",
			flash_addr, sram_addr, size, crc, crc_sram, retries);
		ret = -EFAULT;
	} while (retries < retry);

	return ret;
}
int cts_read_flash_to_sram_check_crc_retry(const struct cts_device *cts_dev,
	u32 flash_addr, u32 sram_addr,
	size_t size, u32 crc, int retry)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret, retries;

	TS_LOG_INFO("Read from 0x%06x to sram 0x%06x size %zu with crc check",
		    flash_addr, sram_addr, size);

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL ||
	    efctrl->ops == NULL ||
	    efctrl->ops->read_to_sram == NULL ||
	    efctrl->ops->calc_sram_crc == NULL) {
		TS_LOG_ERR("Read to sram check crc not supported");
		return -ENOTSUPP;
	}

	if (flash_addr > flash->total_size) {
		TS_LOG_ERR("Read to sram from 0x%06x > flash size 0x%06zx",
			   flash_addr, flash->total_size);
		return -EINVAL;
	}
	size = min(size, flash->total_size - flash_addr);

	TS_LOG_INFO("Read to sram actually from 0x%06x size %zu with crc check",
		    flash_addr, size);

	return ret;
}

int cts_program_flash(const struct cts_device *cts_dev,
		      u32 flash_addr, const void *src, size_t size)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret;

	TS_LOG_INFO("Program to 0x%06x size %zu", flash_addr, size);

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL ||
	    efctrl->ops == NULL || efctrl->ops->program == NULL) {
		TS_LOG_ERR("Program not supported");
		return -ENOTSUPP;
	}

	if (flash_addr >= flash->total_size) {
		TS_LOG_ERR("Program from 0x%06x >= flash size 0x%06zx",
			   flash_addr, flash->total_size);
		return -EINVAL;
	}
	size = min(size, flash->total_size - flash_addr);

	TS_LOG_INFO("Program actually to 0x%06x size %zu", flash_addr, size);

	while (size) {
		size_t l, offset;

		l = min(flash->page_size, size);
		offset = flash_addr & (flash->page_size - 1);

		if (offset)
			l = min(flash->page_size - offset, l);

		ret = efctrl->ops->program(cts_dev, flash_addr, src, l);
		if (ret) {
			TS_LOG_ERR("Program to 0x%06x size %zu failed %d",
				   flash_addr, l, ret);
			return ret;
		}

		src += l;
		size -= l;
		flash_addr += l;
	}

	return 0;
}

int cts_program_flash_from_sram(const struct cts_device *cts_dev,
				u32 flash_addr, u32 sram_addr, size_t size)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret;

	TS_LOG_INFO("Program to 0x%06x from sram 0x%06x size %zu",
		    flash_addr, sram_addr, size);

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL ||
	    efctrl->ops == NULL ||
	    efctrl->ops->page_program_from_sram == NULL ||
	    efctrl->ops->auto_page_program_from_sram == NULL) {
		TS_LOG_ERR("Program from sram not supported");
		return -ENOTSUPP;
	}

	if (flash_addr >= flash->total_size) {
		TS_LOG_ERR("Program from 0x%06x >= flash size 0x%06zx",
			   flash_addr, flash->total_size);
		return -EINVAL;
	}
	size = min(size, flash->total_size - flash_addr);

	TS_LOG_INFO("Program actually to 0x%06x from sram 0x%06x size %zu",
		flash_addr, sram_addr, size);

	ret =
	    efctrl->ops->auto_page_program_from_sram(cts_dev, flash_addr,
						     sram_addr, size);
	if (ret) {
		TS_LOG_ERR("Program to 0x%06x from sram 0x%06x size %zu failed %d",
			flash_addr, sram_addr, size, ret);
		return ret;
	}

	return 0;
}

int cts_erase_flash(const struct cts_device *cts_dev, u32 addr, size_t size)
{
	const struct cts_efctrl *efctrl;
	const struct cts_flash *flash;
	int ret;

	TS_LOG_INFO("Erase from 0x%06x,size %zu", addr, size);

	efctrl = cts_dev->hwdata->efctrl;
	flash = cts_dev->flash;

	if (flash == NULL ||
	    efctrl == NULL || efctrl->ops == NULL ||
	    efctrl->ops->se == NULL || efctrl->ops->be == NULL ||
	    efctrl->ops->ce == NULL || flash == NULL) {
		TS_LOG_ERR("Oops");
		return -EINVAL;
	}

	/* setcor erase ,Addr and size MUST sector aligned */
	addr = rounddown(addr, flash->sector_size);
	size = roundup(size, flash->sector_size);

	if (addr > flash->total_size) {
		TS_LOG_ERR("Erase from 0x%06x > flash size 0x%06zx",
			addr, flash->total_size);
		return -EINVAL;
	}
	size = min(size, flash->total_size - addr);
	TS_LOG_INFO("Erase actually from 0x%06x size %zu", addr, size);

	if (flash->block_size) {
		while (addr != ALIGN(addr, flash->block_size) &&
		       size >= flash->sector_size) {
			ret = erase_sector(cts_dev, addr);
			if (ret) {
				TS_LOG_ERR
				    ("Erase sector 0x%06x size 0x%04zx failed %d",
				     addr, flash->sector_size, ret);
				return ret;
			}
			addr += flash->sector_size;
			size -= flash->sector_size;
		}

		while (size >= flash->block_size) {
			ret = erase_block(cts_dev, addr);
			if (ret) {
				TS_LOG_ERR
				    ("Erase block 0x%06x size 0x%04zx failed %d",
				     addr, flash->block_size, ret);
				return ret;
			}
			addr += flash->block_size;
			size -= flash->block_size;
		}
	}

	while (size >= flash->sector_size) {
		ret = erase_sector(cts_dev, addr);
		if (ret) {
			TS_LOG_ERR("Erase sector 0x%06x size 0x%04zx failed %d",
				addr, flash->sector_size, ret);
			return ret;
		}
		addr += flash->sector_size;
		size -= flash->sector_size;
	}

	return 0;
}
