/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: format boot_ctrl.img source file.
 * Author: wudajun
 * Create: 2020-05-12
 */

#ifndef __BOOTCTRL_MAKE_H__
#define __BOOTCTRL_MAKE_H__

#define BOOTCTRL_MAKE_PARA_COUNT       2
#define BOOTCTRL_MAKE_OK               0
#define BOOTCTRL_MAKE_ERR              -1

#define BOOTCTRL_WRITE_COUNT       1
#define BOOT_BIT_PER_BYPE          8
#define BOOT_SLOT_NAME_LEN         8
#define BOOT_SLOT_SUFFIX_LEN       8
#define BOOT_SLOT_RETRY_COUNT      10
#define BOOT_SLOT_RESERVED_LEN     64

#define BOOT_SLOT_ACTIVE          (1 << 0)
#define BOOT_SLOT_NONBOOTABLE     (1 << 1)
#define BOOT_SLOT_SUCCESSFUL      (1 << 2)

#define BOOT_SLOT_A_NAME          "a"
#define BOOT_SLOT_B_NAME          "b"
#define BOOT_SLOT_A_SUFFIX        "_a"
#define BOOT_SLOT_B_SUFFIX        "_b"

enum boot_slot_type {
	BOOT_SLOT_A      = 0x0,
	BOOT_SLOT_B,
	BOOT_BIOS_SLOT_A,
	BOOT_BIOS_SLOT_B,
	BOOT_SLOT_MAX
};

enum boot_select_type {
	BOOT_SELECT_COMMON    = 0x0,
	BOOT_SELECT_BIOS      = 0x1,
	BOOT_SELECT_MAX
};

#pragma pack(1)

struct bootctrl_slot_info {
	char slot_name[BOOT_SLOT_NAME_LEN];
	char slot_suffix[BOOT_SLOT_SUFFIX_LEN];
	unsigned int flags;
	int retry_remain;
	char reserved[BOOT_SLOT_RESERVED_LEN];
};

struct bootctrl_info {
	unsigned int slot_count;
	unsigned int crc32;
	struct bootctrl_slot_info slot_info[BOOT_SLOT_MAX];
};

#pragma pack()

unsigned int calculate_crc32(unsigned char *data, unsigned int size);
enum boot_slot_type get_inactive_slot(enum boot_slot_type boot_slot);
int check_slot_match(enum boot_select_type select_type, enum boot_slot_type slot);
void set_slot_active(struct bootctrl_info *boot_info, enum boot_slot_type boot_slot);
enum boot_slot_type get_active_slot(struct bootctrl_info *boot_info, enum boot_select_type select_type);
enum boot_slot_type get_available_slot(struct bootctrl_info *boot_info, enum boot_select_type select_type);

#endif
