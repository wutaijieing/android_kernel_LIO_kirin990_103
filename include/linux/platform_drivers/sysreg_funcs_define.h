/*
 * sysreg_funcs_define.h
 *
 * access systemreg funcs
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SYSREG_FUNCS_DEFINE_H__
#define __SYSREG_FUNCS_DEFINE_H__

#define _DEFINE_SYSREG_READ_FUNC(_name, _reg_name)              \
u64 read_ ## _name(void)				\
{								\
	return read_sysreg(_reg_name);				\
}

#define _DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)		\
void write_ ## _name(u64 v)			\
{								\
	write_sysreg(v, _reg_name);				\
}

/* Define read & write function for renamed system register */
#define DEFINE_RENAME_SYSREG_RW_FUNCS(_name, _reg_name)		\
	_DEFINE_SYSREG_READ_FUNC(_name, _reg_name)		\
	_DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

/* Define read function for renamed system register */
#define DEFINE_RENAME_SYSREG_READ_FUNC(_name, _reg_name)	\
	_DEFINE_SYSREG_READ_FUNC(_name, _reg_name)

/* Define write function for renamed system register */
#define DEFINE_RENAME_SYSREG_WRITE_FUNC(_name, _reg_name)	\
	_DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

#endif /* __SYSREG_FUNCS_DEFINE_H__ */