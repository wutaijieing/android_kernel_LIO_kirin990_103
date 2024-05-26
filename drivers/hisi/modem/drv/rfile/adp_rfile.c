/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/personality.h>
#include <linux/stat.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/rcupdate.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include "product_config.h"

#include "drv_comm.h"
#include "osl_types.h"
#include "osl_sem.h"
#include "osl_list.h"
#include "mdrv_memory.h"
#include "bsp_icc.h"
#include "bsp_rfile.h"
#include <bsp_slice.h>
#include <bsp_print.h>
#include "rfile_balong.h"
#include "mdrv_rfile.h"
#include <securec.h>

#define THIS_MODU mod_rfile


#define RFILE_TIMEOUT_MAX 2000 /* 最长等待2s */
#define DATA_SIZE_1_K ((1024 / sizeof(bsp_rfile_dirent_s)) * sizeof(bsp_rfile_dirent_s))

typedef struct {
    short _flags;
    short _file;
} rfile_file_s;

#define RFILE_INVALID_ERROR_NO 0xa5a5a5a5

unsigned long g_err = RFILE_INVALID_ERROR_NO;



/* 查看输入的路径是否可访问，如不能访问则创建此目录 */
/* 本函数为递归调用函数 */
s32 __access_create(char *pathtmp, s32 mode, u32 count)
{
    char *p = NULL;
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_access(pathtmp, 0);
#else
    ret = sys_access(pathtmp, 0);
#endif
    set_fs(old_fs);

    if (count > RFILE_STACK_MAX) {
        bsp_err("<%s> sys_mkdir %s stack %d over %d.\n", __FUNCTION__, pathtmp, count, RFILE_STACK_MAX);
        return -1;
    }

    if (ret != 0) {
        /* 路径中不包含'/'，返还失败 */
        p = strrchr(pathtmp, '/');
        if (p == NULL) {
            bsp_err("<%s> strrchr %s no '/'.\n", __FUNCTION__, pathtmp);
            return -1;
        }

        /* 已经不是根目录下的文件夹，判断上一级目录是否存在 */
        if (p != pathtmp) {
            /* 查看上一级目录是否存在，如果不存在则创建此目录 */
            *p = '\0';
            // cppcheck-suppress *
            ret = __access_create(pathtmp, mode, count + 1);
            if (ret) {
                return -1;
            }

            /* 创建当前目录 */
            *p = '/';
        }

        old_fs = get_fs();
        set_fs((mm_segment_t)KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        ret = ksys_mkdir(pathtmp, 0775);
#else
        ret = sys_mkdir(pathtmp, 0775);
#endif
        set_fs(old_fs);

        if (ret) {
            bsp_err("<%s> sys_mkdir %s failed ret %d.\n", __FUNCTION__, pathtmp, ret);
            return -1;
        }
    }

    return 0;
}

s32 rfile_access_create(char *pathtmp, s32 mode)
{
    return __access_create(pathtmp, mode, 0);
}



/*
 * 功能描述: bsp_open
 * 返 回 值: 失败返回-1，成功返回文件句柄
 */
s32 bsp_open(const s8 *path, s32 flags, s32 mode)
{
    s32 ret;
    unsigned long old_fs;
    u32 time_stamp[3] = {0};
    char *p = NULL;
    char pathtmp[RFILE_PATHLEN_MAX + 1] = {0};
    time_stamp[0] = bsp_get_slice_value();

    if (strlen(path) > RFILE_PATHLEN_MAX) {
        return -1;
    }

    ret = memcpy_s(pathtmp, sizeof(pathtmp), (char *)path, strlen(path));
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        return -1;
    }

    /* 路径中包含'/'并且不在根目录，则检查当前目录是否存在，不存在则创建目录 */
    p = strrchr(pathtmp, '/');
    if ((p != NULL) && (p != pathtmp)) {
        /* 查看上一级目录是否存在，如果不存在则创建此目录 */
        *p = '\0';
        ret = rfile_access_create((char *)pathtmp, mode);
        if (ret) {
            bsp_err("<%s> access_create failed.\n", __FUNCTION__);
        }
    }

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_open(path, flags, mode);
#else
    ret = sys_open(path, flags, mode);
#endif
    set_fs(old_fs);

    time_stamp[1] = bsp_get_slice_value();
    time_stamp[2] = get_timer_slice_delta(time_stamp[0], time_stamp[1]);
    if (time_stamp[2] > (bsp_get_slice_freq() * DELAY_TIME)) {
        bsp_err("bsp_open file name %s out of time start 0x%x end 0x%x detal 0x%x\n", path, time_stamp[0],
                time_stamp[1], time_stamp[2]);
    }
    return ret;
}

/*
 * 功能描述: bsp_close
 * 返 回 值: 成功返回0，失败返回-1
 */
s32 bsp_close(u32 fp)
{
    s32 ret;
    unsigned long old_fs;
    u32 time_stamp[3] = {0};
    struct file *file_p = NULL;

    time_stamp[0] = bsp_get_slice_value();

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

    file_p = fget(fp);
    if (file_p != NULL && file_p->f_inode) {
        invalidate_mapping_pages(file_p->f_inode->i_mapping, 0, -1);
    }
    if (file_p != NULL) {
        fput(file_p);
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_close(fp);
#else
    ret = sys_close(fp);
#endif
    set_fs(old_fs);
    time_stamp[1] = bsp_get_slice_value();
    time_stamp[2] = get_timer_slice_delta(time_stamp[0], time_stamp[1]);
    if (time_stamp[2] > (bsp_get_slice_freq() * DELAY_TIME)) {
        bsp_err("bsp_close %d out of time start 0x%x end 0x%x detal 0x%x\n", fp, time_stamp[0], time_stamp[1],
                time_stamp[2]);
    }

    return ret;
}

/*
 * 功能描述: bsp_write
 * 返 回 值: 返回写入的数据长度
 */
s32 bsp_write(u32 fd, const s8 *ptr, u32 size)
{
    s32 ret;
    unsigned long old_fs;
    u32 time_stamp[3] = {0};
    struct file *file_p = NULL;
    file_p = fget(fd);
    time_stamp[0] = bsp_get_slice_value();

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_write(fd, ptr, (s32)size);
#else
    ret = sys_write(fd, ptr, (s32)size);
#endif

    set_fs(old_fs);

    time_stamp[1] = bsp_get_slice_value();
    time_stamp[2] = get_timer_slice_delta(time_stamp[0], time_stamp[1]);
    if (time_stamp[2] > (bsp_get_slice_freq() * DELAY_TIME)) {
        if (file_p != NULL) {
            bsp_err("bsp_write file name %s out of time start 0x%x end 0x%x detal 0x%x\n",
                    file_p->f_path.dentry->d_iname, time_stamp[0], time_stamp[1], time_stamp[2]);

        } else {
            bsp_err("bsp_write file fd %d out of time start 0x%x end 0x%x detal 0x%x\n", fd, time_stamp[0],
                    time_stamp[1], time_stamp[2]);
        }
    }
    if (file_p != NULL) {
        fput(file_p);
    }
    return ret;
}

/*
 * 功能描述: bsp_write_sync
 * 返 回 值: 返回写入的数据长度
 */
s32 bsp_write_sync(u32 fd, const s8 *ptr, u32 size)
{
    s32 ret;
    unsigned long old_fs;
    u32 time_stamp[3] = {0};
    struct file *file_p = NULL;
    file_p = fget(fd);
    time_stamp[0] = bsp_get_slice_value();

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_write(fd, ptr, (s32)size);
    (void)ksys_fsync(fd);
#else
    ret = sys_write(fd, ptr, (s32)size);
    (void)sys_fsync(fd);
#endif

    set_fs(old_fs);
    time_stamp[1] = bsp_get_slice_value();
    time_stamp[2] = get_timer_slice_delta(time_stamp[0], time_stamp[1]);
    if (time_stamp[2] > (bsp_get_slice_freq() * DELAY_TIME)) {
        if (file_p != NULL) {
            bsp_err("bsp_write_sync file name %s out of time start 0x%x end 0x%x detal 0x%x\n",
                    file_p->f_path.dentry->d_iname, time_stamp[0], time_stamp[1], time_stamp[2]);

        } else {
            bsp_err("bsp_write_sync file fd %d out of time start 0x%x end 0x%x detal 0x%x\n", fd, time_stamp[0],
                    time_stamp[1], time_stamp[2]);
        }
    }
    if (file_p != NULL) {
        fput(file_p);
    }
    return ret;
}

/*
 * 功能描述: bsp_read
 * 返 回 值: 返回读取的数据长度
 */
s32 bsp_read(u32 fd, s8 *ptr, u32 size)
{
    s32 ret;
    unsigned long old_fs;
    u32 time_stamp[3] = {0};
    struct file *file_p = NULL;
    file_p = fget(fd);
    time_stamp[0] = bsp_get_slice_value();

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_read(fd, ptr, (s32)size);
#else
    ret = sys_read(fd, ptr, (s32)size);
#endif
    set_fs(old_fs);
    time_stamp[1] = bsp_get_slice_value();
    time_stamp[2] = get_timer_slice_delta(time_stamp[0], time_stamp[1]);
    if (time_stamp[2] > (bsp_get_slice_freq() * DELAY_TIME)) {
        if (file_p != NULL) {
            bsp_err("bsp_read file name %s out of time start 0x%x end 0x%x detal 0x%x\n",
                    file_p->f_path.dentry->d_iname, time_stamp[0], time_stamp[1], time_stamp[2]);

        } else {
            bsp_err("bsp_read file fd %d out of time start 0x%x end 0x%x detal 0x%x\n", fd, time_stamp[0],
                    time_stamp[1], time_stamp[2]);
        }
    }
    if (file_p != NULL) {
        fput(file_p);
    }
    return ret;
}

/*
 * 功能描述: bsp_lseek
 * 返 回 值: 成功返回0，失败返回-1
 */
s32 bsp_lseek(u32 fd, long offset, s32 whence)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_lseek(fd, offset, (u32)whence);
#else
    ret = sys_lseek(fd, offset, (u32)whence);
#endif
    set_fs(old_fs);

    return ret;
}

/*
 * 功能描述: bsp_tell
 * 返 回 值: 成功返回当前读写位置，失败返回-1
 */
long bsp_tell(u32 fd)
{
    s32 ret;
    loff_t offset = 0;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_lseek(fd, 0, SEEK_CUR);
    if (ret < 0) {
        bsp_err("<%s> ksys_lseek err. ret =  %d.\n", __FUNCTION__, ret);
    }
    offset = (loff_t)ret;
#else
    ret = sys_llseek(fd, 0, 0, &offset, SEEK_CUR);
    if (ret) {
        bsp_err("<%s> sys_llseek err. ret =  %d.\n", __FUNCTION__, ret);
    }
#endif

    set_fs(old_fs);

    return (long)offset;
}

/*
 * 功能描述: bsp_remove
 * 返 回 值: 成功返回0，失败返回-1
 */
s32 bsp_remove(const s8 *pathname)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_unlink(pathname);
#else
    ret = sys_unlink(pathname);
#endif

    set_fs(old_fs);

    return ret;
}

/*
 * 功能描述: bsp_mkdir
 */
s32 bsp_mkdir(s8 *dir_name, s32 mode)
{
    s32 ret;
    char pathtmp[RFILE_PATHLEN_MAX + 1] = {0};

    if (strlen(dir_name) > RFILE_PATHLEN_MAX) {
        return -1;
    }

    ret = memcpy_s(pathtmp, sizeof(pathtmp), (char *)dir_name, strlen(dir_name));
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        return -1;
    }

    ret = rfile_access_create((char *)pathtmp, mode);
    if (ret) {
        bsp_err("<%s> access_create failed.\n", __FUNCTION__);
    }

    return ret;
}

/*
 * 功能描述: bsp_rmdir
 * 返 回 值: 返回0
 */
s32 bsp_rmdir(s8 *path)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_rmdir(path);
#else
    ret = sys_rmdir(path);
#endif
    set_fs(old_fs);

    return ret;
}

/*
 * 功能描述: bsp_opendir
 * 返 回 值: 目录句柄
 */
s32 bsp_opendir(s8 *dir_name)
{
    s32 handle;

    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    handle = ksys_open(dir_name, RFILE_RDONLY | RFILE_DIRECTORY, 0);
#else
    handle = sys_open(dir_name, RFILE_RDONLY | RFILE_DIRECTORY, 0);
#endif
    set_fs(old_fs);

    return handle;
}

/*
 * 功能描述: bsp_readdir
 * 返 回 值: s32
 */
s32 bsp_readdir(u32 fd, void *dirent, u32 count)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_getdents64(fd, dirent, count);
#else
    ret = sys_getdents64(fd, dirent, count);
#endif
    set_fs(old_fs);

    return ret;
}
/*
 * 功能描述: bsp_closedir
 */
s32 bsp_closedir(s32 dir)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_close((u32)dir);
#else
    ret = sys_close((u32)dir);
#endif
    set_fs(old_fs);

    return ret;
}

s32 bsp_access(s8 *path, s32 mode)
{
    s32 ret;
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_access(path, mode);
#else
    ret = sys_access(path, mode);
#endif
    set_fs(old_fs);

    return ret;
}

void rfile_trans_stat(struct rfile_stat_stru *rfile_stat, struct kstat *rfile_kstat)
{
    /* 兼容 32位和64位 */
    rfile_stat->ino = (u64)rfile_kstat->ino;
    rfile_stat->dev = (u32)rfile_kstat->dev;
    rfile_stat->mode = (u16)rfile_kstat->mode;
    rfile_stat->nlink = (u32)rfile_kstat->nlink;
    rfile_stat->uid = *(u32 *)&rfile_kstat->uid;
    rfile_stat->gid = *(u32 *)&rfile_kstat->gid;
    rfile_stat->rdev = (u32)rfile_kstat->rdev;
    rfile_stat->size = (u64)rfile_kstat->size;
    rfile_stat->atime.tv_sec = (u64)rfile_kstat->atime.tv_sec;
    rfile_stat->atime.tv_nsec = (u64)rfile_kstat->atime.tv_nsec;
    rfile_stat->ctime.tv_sec = (u64)rfile_kstat->ctime.tv_sec;
    rfile_stat->mtime.tv_nsec = (u64)rfile_kstat->mtime.tv_nsec;
    rfile_stat->mtime.tv_sec = (u64)rfile_kstat->mtime.tv_sec;
    rfile_stat->ctime.tv_nsec = (u64)rfile_kstat->ctime.tv_nsec;
    rfile_stat->blksize = (u32)rfile_kstat->blksize;
    rfile_stat->blocks = (u64)rfile_kstat->blocks;
}

/*
 * 功能描述: bsp_stat
 * 返 回 值: 成功返回0,失败返回-1
 */
s32 bsp_stat(s8 *name, void *stat)
{
    s32 ret;
    struct kstat kstattmp = {0};
    unsigned long old_fs;

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

    ret = vfs_stat(name, &kstattmp);
    if (ret == 0) {
        rfile_trans_stat(stat, &kstattmp);
    }

    set_fs(old_fs);

    return ret;
}

/*
 * 功能描述: bsp_rename
 * 返 回 值: 成功返回0,失败返回-1
 */
s32 bsp_rename(const char *oldname, const char *newname)
{
    s32 ret;
    unsigned long old_fs;

    if ((oldname == NULL) || (newname == NULL)) {
        return -1;
    }

    old_fs = get_fs();
    set_fs((mm_segment_t)KERNEL_DS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ret = ksys_rename(oldname, newname);
#else
    ret = sys_rename(oldname, newname);
#endif
    set_fs(old_fs);

    return ret;
}

EXPORT_SYMBOL(bsp_open);
EXPORT_SYMBOL(bsp_access);
EXPORT_SYMBOL(bsp_close);
EXPORT_SYMBOL(bsp_write);
EXPORT_SYMBOL(bsp_read);
EXPORT_SYMBOL(bsp_lseek);
EXPORT_SYMBOL(bsp_tell);
EXPORT_SYMBOL(bsp_remove);
EXPORT_SYMBOL(bsp_mkdir);
EXPORT_SYMBOL(bsp_rmdir);
EXPORT_SYMBOL(bsp_opendir);
EXPORT_SYMBOL(bsp_readdir);
EXPORT_SYMBOL(bsp_closedir);
EXPORT_SYMBOL(bsp_stat);

unsigned long mdrv_file_get_errno()
{
    if (RFILE_INVALID_ERROR_NO == g_err) {
        return 1;
    } else {
        return g_err;
    }
}

int rfile_getmode_check(const char *mode, int ret)
{
    /* check for garbage in second character */
    if ((*mode != '+') && (*mode != 'b') && (*mode != '\0')) {
        bsp_err("[%s]:1. mode:%c.\n", __FUNCTION__, *mode);
        return (0);
    }

    /* check for garbage in third character */
    if (*mode++ == '\0') {
        return (ret); /* no third char */
    }

    if ((*mode != '+') && (*mode != 'b') && (*mode != '\0')) {
        bsp_err("[%s]:3. mode:%c.\n", __FUNCTION__, *mode);
        return (0);
    }

    /* check for garbage in fourth character */
    if (*mode++ == '\0') {
        return (ret); /* no fourth char */
    }

    if (*mode != '\0') {
        bsp_err("[%s]:5. mode:%c.\n", __FUNCTION__, *mode);
        return (0);
    } else {
        return (ret);
    }
}

int rfile_getmode(const char *mode, int *flag)
{
    int ret, ret_temp;
    unsigned int m;
    unsigned int o;

    switch (*mode++) {
        case 'r': /* open for reading */
            ret_temp = 0x0004;
            m = RFILE_RDONLY;
            o = 0;
            break;

        case 'w': /* open for writing */
            ret_temp = 0x0008;
            m = RFILE_WRONLY;
            o = RFILE_CREAT | RFILE_TRUNC;
            break;

        case 'a': /* open for appending */
            ret_temp = 0x0008;
            m = RFILE_WRONLY;
            o = RFILE_CREAT | RFILE_APPEND;
            break;

        default: /* illegal mode */
            g_err = 22;
            return (0);
    }

    /* [rwa]\+ or [rwa]b\+ means read and write */
    if ((*mode == '+') || (*mode == 'b' && mode[1] == '+')) {
        ret_temp = 0x0010;
        m = RFILE_RDWR;
    }

    *flag = (int)(m | o);

    ret = rfile_getmode_check(mode, ret_temp);

    return ret;
}

rfile_file_s *rfile_stdio_fp_create(void)
{
    rfile_file_s *fp = NULL;

    fp = (rfile_file_s *)kzalloc(sizeof(rfile_file_s), GFP_KERNEL);
    if (fp != NULL) {
        fp->_flags = 1; /* caller sets real flags */
        fp->_file = -1; /* no file */
    }

    return (fp);
}

int rfile_stdio_fp_destroy(rfile_file_s *fp)
{
    if (fp == NULL) {
        return -1;
    }

    /* fclose() deallocates any buffers associated with the file pointer */
    kfree((char *)fp);

    return 0;
}

/* 文件系统接口 */
void *mdrv_file_open(const char *path, const char *mode)
{
    int ret;
    int oflags;
    int flags;
    rfile_file_s *fp = NULL;

    if ((path == NULL) || (mode == NULL)) {
        g_err = 1;
        return 0;
    }

    /* 将字符串参数转换成整数 */
    flags = rfile_getmode(mode, &oflags);
    if (flags == 0) {
        bsp_err("[%s] rfile_getmode failed. ret = %d.\n", __FUNCTION__, flags);
        return 0;
    }

    fp = rfile_stdio_fp_create();
    if (fp == NULL) {
        g_err = 1;
        return (NULL);
    }

    g_err = RFILE_INVALID_ERROR_NO;
    ret = bsp_open((const s8 *)path, oflags, 0664);
    if (ret < 0) {
        bsp_err("[%s] bsp_open failed,path=%s, ret = %x.\n", __FUNCTION__, path, ret);
        ret = rfile_stdio_fp_destroy(fp); /* destroy file pointer */
        g_err = (u32)ret;
        return 0;
    }

    fp->_file = (short)ret;
    fp->_flags = (short)flags;

    return (void *)fp;
}

int mdrv_file_close(void *fp)
{
    int ret;
    if (fp == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    ret = bsp_close(((rfile_file_s *)fp)->_file);
    if (ret == 0) {
        ret = rfile_stdio_fp_destroy(fp);
    }

    return ret;
}

int mdrv_file_read(void *ptr, unsigned int size, unsigned int number, const void *stream)
{
    int cnt;

    if ((ptr == NULL) || (stream == NULL) || (size == 0)) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    cnt = bsp_read(((rfile_file_s *)stream)->_file, ptr, (size * number));

    return cnt / ((int)size);
}

int mdrv_file_write(const void *ptr, unsigned int size, unsigned int number, const void *stream)
{
    int cnt;

    if ((ptr == NULL) || (stream == NULL) || (size == 0)) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    cnt = bsp_write(((rfile_file_s *)stream)->_file, ptr, (size * number));

    return cnt / ((int)size);
}

int mdrv_file_write_sync(const void *ptr, unsigned int size, unsigned int number, const void *stream)
{
    int cnt;

    if ((ptr == NULL) || (stream == NULL) || (size == 0)) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    cnt = bsp_write_sync(((rfile_file_s *)stream)->_file, ptr, (size * number));

    return cnt / ((int)size);
}

int mdrv_file_seek(const void *stream, long offset, int whence)
{
    int ret;

    if (stream == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    ret = bsp_lseek(((rfile_file_s *)stream)->_file, offset, whence);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

long mdrv_file_tell(const void *stream)
{
    if (stream == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    return bsp_tell(((rfile_file_s *)stream)->_file);
}

int mdrv_file_remove(const char *pathname)
{
    if (pathname == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    return bsp_remove((const s8 *)pathname);
}

int mdrv_file_mkdir(const char *dir_name)
{
    if (dir_name == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    return bsp_mkdir((s8 *)dir_name, 0775);
}

int mdrv_file_rmdir(const char *path)
{
    if (path == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    return bsp_rmdir((s8 *)path);
}

rfile_dir_s *rfile_stdio_dir_create(void)
{
    rfile_dir_s *dir = NULL;

    dir = (rfile_dir_s *)kzalloc(sizeof(rfile_dir_s), GFP_KERNEL);
    if (dir != NULL) {
        dir->dd_fd = -1;
        dir->dd_cookie = 0;
        dir->dd_eof = 0;
    }

    return (dir);
}

int rfile_stdio_dir_destroy(rfile_dir_s *dir)
{
    int ret;
    if (dir == NULL) {
        return -1;
    }
    dir->dd_fd = -1;
    ret = memset_s(&dir->dd_dirent, sizeof(rfile_dirent_s), 0, sizeof(rfile_dirent_s));
    if (ret != EOK) {
        bsp_err("[%s] memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return -1;
    }

    kfree((char *)dir);

    return 0;
}

rfile_dir_s *mdrv_file_opendir(const char *dir_name)
{
    int ret;
    rfile_dir_s *dir = NULL;
    size_t min_length;

    if (dir_name == NULL) {
        g_err = 1;
        return NULL;
    }

    ret = bsp_opendir((s8 *)dir_name);
    if (ret < 0) {
        g_err = 2;
        return 0;
    }

    dir = rfile_stdio_dir_create();
    if (dir == NULL) {
        g_err = 3;
        (void)bsp_closedir(ret);
        return NULL;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    dir->dd_fd = ret;
    min_length = min_t(size_t, strlen(dir_name), DRV_NAME_MAX);
    ret = memcpy_s(dir->dd_dirent.d_name, DRV_NAME_MAX, dir_name, min_length);
    if (ret != EOK) {
        bsp_err("[%s] memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(dir);
        return NULL;
    }

    return dir;
}

struct rfile_dirent_info {
    rfile_dir_s *phandle;
    bsp_rfile_dirent_s *pdirent;
    int len;                 /* 总长度 */
    int ptr;                 /* 当前偏移 */
    struct list_head stlist; /* 链表节点 */
    rfile_dirent_s stdirent;
};

struct rfile_adp_ctrl {
    struct list_head rfile_listhead;
    rfile_sem_id sem_list;
};

struct rfile_adp_ctrl g_adp_rfile;

void adp_rfile_init()
{
    INIT_LIST_HEAD(&g_adp_rfile.rfile_listhead);

#ifdef __KERNEL__
    sema_init(&g_adp_rfile.sem_list, 1);
#else  /* __VXWORKS__ */
    g_adp_rfile.sem_list = semBCreate(SEM_Q_FIFO, (SEM_B_STATE)SEM_FULL);
#endif /* end of __KERNEL__ */
}

struct rfile_dirent_info *adp_get_node(rfile_dir_s *dirp)
{
    int ret;
    struct list_head *me = NULL;
    struct rfile_dirent_info *dirent = NULL;

    if (dirp == NULL) {
        return NULL;
    }

    ret = osl_sem_downtimeout(&g_adp_rfile.sem_list, (long)msecs_to_jiffies(RFILE_TIMEOUT_MAX));
    if (ret) {
        return NULL;
    }

    list_for_each(me, &g_adp_rfile.rfile_listhead)
    {
        dirent = list_entry(me, struct rfile_dirent_info, stlist);

        if ((dirent != NULL) && (dirp == dirent->phandle)) {
            break;
        } else {
            dirent = NULL;
        }
    }

    osl_sem_up(&g_adp_rfile.sem_list);

    return dirent;
}

int adp_add_node(struct rfile_dirent_info *pdirent_list)
{
    int ret;

    ret = osl_sem_downtimeout(&g_adp_rfile.sem_list, (long)msecs_to_jiffies(RFILE_TIMEOUT_MAX));
    if (ret) {
        return -1;
    }

    list_add(&pdirent_list->stlist, &g_adp_rfile.rfile_listhead);

    osl_sem_up(&g_adp_rfile.sem_list);

    return 0;
}

void adp_del_node(rfile_dir_s *dirp)
{
    int ret;
    struct list_head *me = NULL;
    struct list_head *n = NULL;
    struct rfile_dirent_info *dirent = NULL;

    if (dirp == NULL) {
        return;
    }

    ret = osl_sem_downtimeout(&g_adp_rfile.sem_list, (long)msecs_to_jiffies(RFILE_TIMEOUT_MAX));
    if (ret) {
        return;
    }

    list_for_each_safe(me, n, &g_adp_rfile.rfile_listhead)
    {
        dirent = list_entry(me, struct rfile_dirent_info, stlist);
        if ((dirent != NULL) && (dirp == dirent->phandle)) {
            list_del(&dirent->stlist);
            kfree(dirent->pdirent);
            kfree(dirent);
            break;
        }
    }

    osl_sem_up(&g_adp_rfile.sem_list);

    return;
}
struct rfile_dirent_info *rfile_adp_readdir(rfile_dir_s *dirp)
{
    int len;
    int ret;
    char data[DATA_SIZE_1_K] = {0};

    struct rfile_dirent_info *dirent = NULL;
    bsp_rfile_dirent_s *pdirent = NULL;

    len = bsp_readdir((unsigned int)dirp->dd_fd, data, DATA_SIZE_1_K);
    if (len <= 0) {
        return NULL;
    }

    pdirent = (bsp_rfile_dirent_s *)kzalloc(DATA_SIZE_1_K, GFP_KERNEL);
    if (pdirent == NULL) {
        return NULL;
    }

    dirent = kzalloc(sizeof(struct rfile_dirent_info), GFP_KERNEL);
    if (dirent == NULL) {
        kfree(pdirent);
        return NULL;
    }
    ret = memcpy_s((void *)pdirent, DATA_SIZE_1_K, data, (unsigned int)len);
    if (ret != EOK) {
        bsp_err("[%s] memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(dirent);
        kfree(pdirent);
        return NULL;
    }
    dirent->phandle = dirp;
    
    dirent->pdirent = pdirent;
    dirent->len = len;
    dirent->ptr = 0;
    
    if (adp_add_node(dirent) != 0) {
        kfree(dirent);
        kfree(pdirent);
        return NULL;
    }
    return dirent;
}

rfile_dirent_s *mdrv_file_readdir(rfile_dir_s *dirp)
{
    int ret;
    struct rfile_dirent_info *dirent = NULL;
    bsp_rfile_dirent_s *pdirentcur = NULL;

    if (dirp == NULL) {
        g_err = 1;
        return 0;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    dirent = adp_get_node(dirp);

    if (dirent == NULL) {
        dirent = rfile_adp_readdir(dirp);
        if (dirent == NULL) {
            return NULL;
        }
    }

    if (dirent->ptr >= dirent->len) {
        bsp_err("[%s] ptr %d, len %d.\n", __FUNCTION__, dirent->ptr, dirent->len);
        goto err;
    }

    pdirentcur = (bsp_rfile_dirent_s *)((u8 *)(dirent->pdirent) + dirent->ptr);
    dirent->stdirent.d_ino = pdirentcur->d_ino;

    ret = memset_s((void *)dirent->stdirent.d_name, (DRV_NAME_MAX + 1), 0, (DRV_NAME_MAX + 1));
    if (ret != EOK) {
        bsp_err("[%s] memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        goto err;
    }
    if (strlen((char *)pdirentcur->d_name) > DRV_NAME_MAX) {
        ret = memcpy_s(dirent->stdirent.d_name, DRV_NAME_MAX + 1, pdirentcur->d_name, DRV_NAME_MAX);
        if (ret != EOK) {
            bsp_err("[%s] memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
            goto err;
        }
    } else {
        ret = strncpy_s(dirent->stdirent.d_name, DRV_NAME_MAX + 1, (char *)pdirentcur->d_name,
                        (unsigned long)DRV_NAME_MAX);
        if (ret != EOK) {
            bsp_err("[%s] strncpy_s err. ret =  %d.\n", __FUNCTION__, ret);
            goto err;
        }
    }

    dirent->ptr += pdirentcur->d_reclen;

    return &(dirent->stdirent);
err:
    if (dirent->pdirent != NULL) {
        kfree(dirent->pdirent);
    }
    if (dirent != NULL) {
        kfree(dirent);
    }
    return NULL;
}

int mdrv_file_closedir(rfile_dir_s *dirp)
{
    int ret;

    if (dirp == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    adp_del_node(dirp);

    ret = bsp_closedir(dirp->dd_fd);
    if (ret == 0) {
        ret = rfile_stdio_dir_destroy(dirp);
    }

    return ret;
}

int mdrv_file_get_stat(const char *path, rfile_stat_s *buf)
{
    int ret;
    struct rfile_stat_stru ststat;

    if (buf == NULL) {
        g_err = 1;
        return -1;
    }
    g_err = RFILE_INVALID_ERROR_NO;

    ret = bsp_stat((s8 *)path, &ststat);
    if (ret == 0) {
        buf->st_dev = (unsigned long)ststat.dev;
        buf->st_ino = (unsigned long)ststat.ino;
        buf->st_mode = (int)ststat.mode;
        buf->st_nlink = (unsigned long)ststat.nlink;
        buf->st_uid = (unsigned short)ststat.uid;
        buf->st_gid = (unsigned short)ststat.gid;
        buf->st_rdev = (unsigned long)ststat.rdev;
        buf->st_size = (signed long long)ststat.size;
        buf->ul_atime = (unsigned long)ststat.atime.tv_sec;
        buf->ul_mtime = (unsigned long)ststat.mtime.tv_sec;
        buf->ul_ctime = (unsigned long)ststat.ctime.tv_sec;
        buf->st_blksize = (long)ststat.blksize;
        buf->st_blocks = (unsigned long)ststat.blocks;
        buf->st_attrib = 0;
    }

    return ret;
}

int mdrv_file_access(const char *path, int amode)
{
    return bsp_access((s8 *)path, amode);
}

int mdrv_file_rename(const char *oldname, const char *newname)
{
    return bsp_rename(oldname, newname);
}

EXPORT_SYMBOL(mdrv_file_read);
EXPORT_SYMBOL(mdrv_file_opendir);
EXPORT_SYMBOL(mdrv_file_seek);
EXPORT_SYMBOL(mdrv_file_get_stat);
EXPORT_SYMBOL(mdrv_file_close);
EXPORT_SYMBOL(mdrv_file_closedir);
EXPORT_SYMBOL(mdrv_file_open);
EXPORT_SYMBOL(mdrv_file_rmdir);
EXPORT_SYMBOL(mdrv_file_write);
EXPORT_SYMBOL(mdrv_file_mkdir);
EXPORT_SYMBOL(mdrv_file_tell);
EXPORT_SYMBOL(mdrv_file_readdir);
EXPORT_SYMBOL(mdrv_file_remove);

EXPORT_SYMBOL(mdrv_file_write_sync);
EXPORT_SYMBOL(mdrv_file_rename);
EXPORT_SYMBOL(mdrv_file_access);
EXPORT_SYMBOL(mdrv_file_get_errno);

