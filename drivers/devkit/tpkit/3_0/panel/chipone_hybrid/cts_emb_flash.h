#ifndef CTS_EMB_FLASH_H
#define CTS_EMB_FLASH_H

#define ICN_EMBEDDED_FLASH 0x8918

enum flash_erase_mode {
	SECTOR_EARSE = 0,
	CHIP_EARSE = 1,
};

struct cts_flash {
	const char *name;
	u32 jedec_id; /* Device ID */
	u32 erase_mode; /* erase_mode */
	size_t page_size; /* Page size */
	size_t sector_size; /* Sector size */
	size_t block_size; /* Block size, if 0 means block erase NOT supported */
	size_t  nvr_size; /* nvr size */
	size_t total_size;
};

#define CTS_FLASH_READ_DEFAULT_RETRY        (3)
#define CTS_FLASH_ERASE_DEFAULT_RETRY       (3)

struct cts_device;

int cts_prepare_flash_operation(struct cts_device *cts_dev);

int cts_read_flash_retry(const struct cts_device *cts_dev,
				u32 flash_addr, void *dst, size_t size,
				int retry);
static inline int cts_read_flash(const struct cts_device *cts_dev,
				 u32 flash_addr, void *dst, size_t size)
{
	return cts_read_flash_retry(cts_dev,
				    flash_addr, dst, size,
				    CTS_FLASH_READ_DEFAULT_RETRY);
}

int cts_read_flash_to_sram_retry(const struct cts_device *cts_dev,
					u32 flash_addr, u32 sram_addr,
					size_t size, int retry);
static inline int cts_read_flash_to_sram(const struct cts_device *cts_dev,
					 u32 flash_addr, u32 sram_addr,
					 size_t size)
{
	return cts_read_flash_to_sram_retry(cts_dev,
					    flash_addr, sram_addr, size,
					    CTS_FLASH_READ_DEFAULT_RETRY);
}

int cts_read_flash_to_sram_check_crc_retry(const struct cts_device
						  *cts_dev, u32 flash_addr,
						  u32 sram_addr, size_t size,
						  u32 crc, int retry);
static inline int cts_read_flash_to_sram_check_crc(const struct cts_device
						   *cts_dev, u32 flash_addr,
						   u32 sram_addr, size_t size,
						   u32 crc)
{
	return cts_read_flash_to_sram_check_crc_retry(cts_dev,
					flash_addr, sram_addr, size, crc,
					CTS_FLASH_READ_DEFAULT_RETRY);
}

int cts_erase_flash(const struct cts_device *cts_dev, u32 addr, size_t size);
int cts_program_flash(const struct cts_device *cts_dev, u32 flash_addr,
				const void *src, size_t size);
int cts_program_flash_from_sram(const struct cts_device *cts_dev,
				u32 flash_addr, u32 sram_addr, size_t size);

#endif
