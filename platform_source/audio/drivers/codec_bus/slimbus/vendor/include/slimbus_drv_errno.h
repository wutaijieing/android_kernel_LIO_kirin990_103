/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_DRV_ERRNO_H__
#define __SLIMBUS_DRV_ERRNO_H__

#ifndef ERRNO_H_NOT_PRESENT
#include <linux/errno.h>
#endif //ERRNO_H_NOT_PRESENT

#ifndef EOK
#define EOK 			0	   /* no error */
#endif //EOK

#ifndef EPERM
#define EPERM			1	   /* Operation not permitted */
#endif //EPERM

#ifndef ENOENT
#define ENOENT			2	   /* No such file or directory */
#endif //ENOENT

#ifndef EIO
#define EIO 			5	   /* I/O error */
#endif //EIO

#ifndef ENOEXEC
#define ENOEXEC 		8	   /* Exec format error */
#endif //ENOEXEC

#ifndef EAGAIN
#define EAGAIN			11		/* Try again */
#endif //EAGAIN

#ifndef ENOMEM
#define ENOMEM			12		/* Out of memory */
#endif //ENOMEM

#ifndef EFAULT
#define EFAULT			14		/* Bad address */
#endif //EFAULT

#ifndef EBUSY
#define EBUSY			16		/* Device or resource busy */
#endif //EBUSY

#ifndef EINVAL
#define EINVAL			22		/* Invalid argument */
#endif //EINVAL

#ifndef ENOSPC
#define ENOSPC			28		/* No space left on device */
#endif //ENOSPC

#ifndef EBADSLT
#define EBADSLT 		57		/* Invalid slot */
#endif //EBADSLT

#ifndef EPROTO
#define EPROTO			71		/* Protocol error */
#endif //EPROTO

#ifndef EOVERFLOW
#define EOVERFLOW		75		/* Value too large for defined data type */
#endif //EOVERFLOW

#ifndef EOPNOTSUPP
#define EOPNOTSUPP		95		/* Operation not supported */
#endif //EOPNOTSUPP

#ifndef EINPROGRESS
#define EINPROGRESS 	115 	/* Operation now in progress */
#endif //EINPROGRESS

#ifndef EDQUOT
#define EDQUOT			122 	/* Quota exceeded */
#endif //EDQUOT

#ifndef ENOTSUP
#define ENOTSUP 		EOPNOTSUPP
#endif //ENOTSUP

#ifndef ECANCELED
#define ECANCELED		126 	 /* Cancelled */
#endif //ECANCELED

#endif /* __SLIMBUS_DRV_ERRNO_H__ */
