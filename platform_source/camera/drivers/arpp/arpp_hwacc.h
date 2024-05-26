/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _ARPP_HWACC_H_
#define _ARPP_HWACC_H_

#include "arpp_smmu.h"
#include "arpp_isr.h"
#include "platform.h"

struct arpp_hwacc {
	struct mapped_buffer cmdlist_buf;
	struct lba_buffer_info lba_buf_info;
	struct arpp_device *arpp_dev;
	struct device *dev;
};

int hwacc_init(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info);
int hwacc_deinit(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info);
int do_lba_calculation(struct arpp_hwacc *hwacc_info,
	struct arpp_ahc *ahc_info, struct arpp_lba_info *lba_info);
int hwacc_wait_for_power_down(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info);

#endif /* _ARPP_HWACC_H_ */
