/*
 * This file define interfaces for ivp dsm
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _IVP_DSM_STUB_H_
#define _IVP_DSM_STUB_H_

#include <linux/types.h>
#include <linux/stddef.h>

#define CLIENT_NAME_LEN          32
#define DSM_IVP_SMMU_ERROR_NO    927005000
#define DSM_IVP_WATCH_ERROR_NO   927005001
#define DSM_IVP_DWAXI_ERROR_NO   927005002
#define DSM_IVP_OPEN_ERROR_NO    927005003
#define DSM_IVP_PANIC_ERROR_NO   927005005

struct dsm_client_ops {
	int (*poll_state)(void);
	int (*dump_func)(int type, void *buff, int size);
};

struct dsm_dev {
	const char *name;
	const char *device_name;
	const char *ic_name;
	const char *module_name;
	struct dsm_client_ops *fops;
	size_t buff_size;
};

struct dsm_client {
	char client_name[CLIENT_NAME_LEN];
};

static inline struct dsm_client *dsm_register_client(
	struct dsm_dev *dev __attribute__((unused)))
{
	return NULL;
}

static inline int dsm_client_ocuppy(
	struct dsm_client *client __attribute__((unused)))
{
	return 0;
}

static inline int dsm_client_record(
	struct dsm_client *client __attribute__((unused)),
	const char *fmt __attribute__((unused)), ...)
{
	return 0;
}

static inline void dsm_client_notify(
	struct dsm_client *client __attribute__((unused)),
	int error_no __attribute__((unused))) {}
#endif /* _IVP_DSM_STUB_H_ */
