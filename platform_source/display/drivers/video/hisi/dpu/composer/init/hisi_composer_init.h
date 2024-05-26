/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef HISI_COMPOSER_INIT_H
#define HISI_COMPOSER_INIT_H

int hisi_offline_reg_modules_init(struct hisi_composer_data *data);
int hisi_offline_module_default(struct hisi_composer_data *ov_data);
void hisi_offline_init_param_init(struct hisi_composer_data *data);
#endif
