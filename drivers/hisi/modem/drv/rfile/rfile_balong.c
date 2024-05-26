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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include "product_config.h"

#include "mdrv.h"
#include <osl_thread.h>
#include "bsp_pm_om.h"
#include "bsp_slice.h"
#include "rfile_balong.h"
#include <securec.h>
#include <bsp_print.h>
#define THIS_MODU mod_rfile


#define RFILE_PM_SIZE 257

struct bsp_rfile_main_stru g_bsp_rfile_main = {
    EN_RFILE_INIT_INVALID,
};

struct rfile_mntn_stru g_rfile_mntn_info;

s8 *g_rfileerror = (s8 *)"error";
int g_rfile_while = 1;

s32 bsp_fs_ok(void)
{
    if (g_bsp_rfile_main.init_flag == EN_RFILE_INIT_INVALID) {
        return -1;
    }
    return 0;
}

s32 rfile_lpmcallback(int x)
{
    g_bsp_rfile_main.lpmstate = 1;
    return 0;
}

s8 *rfile_getdirpath(s32 fp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct dir_list *tmp = NULL;

    list_for_each_safe(p, n, &(g_bsp_rfile_main.dplist)) {
        tmp = list_entry(p, struct dir_list, stlist);
        if (tmp->dp == fp) {
            return tmp->name;
        }
    }

    return g_rfileerror;
}

s8 *rfile_getfilepath(s32 fp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct fp_list *tmp = NULL;

    list_for_each_safe(p, n, &(g_bsp_rfile_main.fplist)) {
        tmp = list_entry(p, struct fp_list, stlist);
        if (tmp->fp == fp) {
            return tmp->name;
        }
    }

    return g_rfileerror;
}
s32 rfile_lpm_print_path(u32 op, BSP_CHAR *path)
{
    s32 ret;
    if (g_bsp_rfile_main.lpmstate) {
        char rfile_pm[RFILE_PM_SIZE] = {0};
        ret = snprintf_s(rfile_pm, RFILE_PM_SIZE, RFILE_PM_SIZE - 1, "op %d,path %s.\n", op, path);
        if (ret < 0) {
            bsp_err("<%s> snprintf_s err. ret =  %d.\n", __FUNCTION__, ret);
            return ret;
        }
        bsp_pm_log(PM_OM_ARFILE, strlen(rfile_pm), rfile_pm);
        g_bsp_rfile_main.lpmstate = 0;
        bsp_info("[C SR] rfile op %d, path %s.\n", op, path);
    }
    return BSP_OK;
}

s32 rfile_lpm_print_dirpath(u32 op, u32 fd)
{
    s32 ret;
    if (g_bsp_rfile_main.lpmstate) {
        char rfile_pm[RFILE_PM_SIZE] = {0};
        ret = snprintf_s(rfile_pm, RFILE_PM_SIZE, RFILE_PM_SIZE - 1, "op %d,path %s.\n", op, rfile_getdirpath((s32)fd));
        if (ret < 0) {
            bsp_err("<%s> snprintf_s err. ret =  %d.\n", __FUNCTION__, ret);
            return ret;
        }
        bsp_pm_log(PM_OM_ARFILE, strlen(rfile_pm), rfile_pm);
        g_bsp_rfile_main.lpmstate = 0;
        bsp_info("[C SR] rfile op %d, path %s.\n", op, rfile_getdirpath((s32)fd));
    }
    return BSP_OK;
}

s32 rfile_lpm_print_filepath(u32 op, u32 fd)
{
    s32 ret;
    if (g_bsp_rfile_main.lpmstate) {
        char rfile_pm[RFILE_PM_SIZE] = {0};
        ret = snprintf_s(rfile_pm, RFILE_PM_SIZE, RFILE_PM_SIZE - 1, "op %d,path %s.\n", op,
                         rfile_getfilepath((s32)fd));
        if (ret < 0) {
            bsp_err("<%s> snprintf_s err. ret =  %d.\n", __FUNCTION__, ret);
            return ret;
        }
        bsp_pm_log(PM_OM_ARFILE, strlen(rfile_pm), rfile_pm);
        g_bsp_rfile_main.lpmstate = 0;
        bsp_info("[C SR] rfile op %d, path %s.\n", op, rfile_getfilepath((s32)fd));
    }
    return BSP_OK;
}

int rfile_check_path(const char *path)
{
    unsigned int i, f_num;
    const char *d_str = "../";
    const char *f_str[] = {
        MODEM_LOG_ROOT, MODEM_DATA_ROOT, "/mnvm2:0", "/modem_secure/", "/modem_fw/","/mnt/modem/mnvm2:0", "/mnt/modem/modem_secure/", "/vendor/modem/modem_fw/", "/patch_hw/modem_fw/", "/mnt/modem/patch_hw/"
    };

    if (path == NULL) {
        bsp_err("<%s> path is NULL.\n", __FUNCTION__);
        return -1;
    }

    if (strstr(path, d_str)) {
        bsp_err("<%s> %s is not allow %s.\n", __FUNCTION__, path, d_str);
        return -1;
    }

    f_num = sizeof(f_str) / sizeof(f_str[0]);

    for (i = 0; i < f_num; i++) {
        if (0 == strncmp(path, f_str[i], strlen(f_str[i]))) {
            return 0;
        }
    }

    bsp_err("<%s> %s is not in list.\n", __FUNCTION__, path);

    return -1;
}

int rfile_check_fp(u32 fp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct fp_list *tmp = NULL;

    list_for_each_safe(p, n, &(g_bsp_rfile_main.fplist)) {
        tmp = list_entry(p, struct fp_list, stlist);
        if (tmp->fp == (s32)fp) {
            return 0;
        }
    }

    bsp_err("<%s> fp %d is not in list.\n", __FUNCTION__, fp);
    return -1;
}

int rfile_check_dp(u32 dp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct dir_list *tmp = NULL;

    list_for_each_safe(p, n, &(g_bsp_rfile_main.dplist)) {
        tmp = list_entry(p, struct dir_list, stlist);
        if (tmp->dp == (s32)dp) {
            return 0;
        }
    }

    bsp_err("<%s> dp %d is not in list.\n", __FUNCTION__, dp);
    return -1;
}

void rfile_mntn_dot_record(u32 line)
{
    u32 ptr = g_rfile_mntn_info.stdot.ptr;
    g_rfile_mntn_info.stdot.line[ptr] = line;
    g_rfile_mntn_info.stdot.slice[ptr] = bsp_get_slice_value();
    g_rfile_mntn_info.stdot.ptr = (g_rfile_mntn_info.stdot.ptr + 1) % RFILE_MNTN_DOT_NUM;
}

void rfile_fplist_del(s32 fp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct fp_list *tmp = NULL;

    rfile_mntn_dot_record(__LINE__);

    list_for_each_safe(p, n, &(g_bsp_rfile_main.fplist)) {
        tmp = list_entry(p, struct fp_list, stlist);
        if (tmp->fp == fp) {
            list_del(&tmp->stlist);
            kfree(tmp);
        }
    }
}

s32 rfile_fplist_add(s32 fp, s8 *name)
{
    struct fp_list *fp_elemt = NULL;
    u32 len;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    fp_elemt = (struct fp_list *)kzalloc(sizeof(struct fp_list), GFP_KERNEL);
    if (fp_elemt == NULL) {
        bsp_err("<%s> malloc fp_elemt failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    fp_elemt->fp = fp;

    len = (u32)strlen(name);
    if (len > RFILE_NAME_MAX) {
        len = RFILE_NAME_MAX;
    }

    ret = memcpy_s(fp_elemt->name, RFILE_NAME_MAX, name, (s32)len);
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(fp_elemt);
        return BSP_ERROR;
    }
    fp_elemt->name[len] = '\0';

    list_add(&(fp_elemt->stlist), &(g_bsp_rfile_main.fplist));

    return BSP_OK;
}

void rfile_dp_list_del(s32 dp)
{
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct dir_list *tmp = NULL;

    rfile_mntn_dot_record(__LINE__);

    list_for_each_safe(p, n, &(g_bsp_rfile_main.dplist)) {
        tmp = list_entry(p, struct dir_list, stlist);
        if (tmp->dp == dp) {
            list_del(&tmp->stlist);
            kfree(tmp);
        }
    }
}

s32 rfile_dp_list_add(s32 dp, s8 *name)
{
    struct dir_list *dp_elemt = NULL;
    u32 len;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    dp_elemt = (struct dir_list *)kzalloc(sizeof(struct dir_list), GFP_KERNEL);
    if (dp_elemt == NULL) {
        bsp_err("<%s> malloc dp_elemt failed.\n", __FUNCTION__);

        return BSP_ERROR;
    }

    dp_elemt->dp = dp;

    len = (u32)strlen(name);
    if (len > RFILE_NAME_MAX) {
        len = RFILE_NAME_MAX;
    }

    ret = memcpy_s(dp_elemt->name, RFILE_NAME_MAX, name, (s32)len);
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(dp_elemt);
        return BSP_ERROR;
    }
    dp_elemt->name[len] = '\0';

    list_add(&(dp_elemt->stlist), &(g_bsp_rfile_main.dplist));

    return BSP_OK;
}

/*
 * 功能描述: 向ICC通道发送数据，如果通道满，则重复尝试多次
 * 返 回 值: void
 */
void rfile_icc_send(void *pdata, u32 len, u32 id)
{
    s32 ret, i;

    rfile_mntn_dot_record(__LINE__);

    for (i = 0; i < RFILE_MAX_SEND_TIMES; i++) {
        if (g_bsp_rfile_main.init_flag != EN_RFILE_INIT_FINISH) {
            return;
        }

        ret = bsp_icc_send(((RFILE_CCORE_ICC_WR_CHAN == id) ? ICC_CPU_MODEM : ICC_CPU_MCU), id, (u8 *)pdata, len);
        if (ret == ICC_INVALID_NO_FIFO_SPACE) {
            /* buffer满，延时后重发 */
            msleep(50);
            continue;
        } else if (ret == BSP_ERR_ICC_CCORE_RESETTING) {
            bsp_err("<%s> icc  cannot use.\n", __FUNCTION__);
        } else if (len != (u32)ret) {
            bsp_err("<%s> icc_send failed.\n", __FUNCTION__);
            return;
        } else {
            return;
        }
    }
}
/*
 * 功能描述: fopen的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_open_req(struct bsp_rfile_open_req *req, u32 id)
{
    u32 name_len;
    s32 ret;
    char *name = NULL;
    struct bsp_rfile_open_cnf cnf = {0};
    char *p = NULL;
    char pathtmp[RFILE_PATHLEN_MAX + 1] = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = -1;

    name_len = (u32)(req->namelen);

    if (name_len > RFILE_PATHLEN_MAX) {
        bsp_err("<%s> name_len too large , name_len =  %d.\n", __FUNCTION__, name_len);
        goto err;
    }

    name = kzalloc(name_len, GFP_KERNEL);
    if (name == NULL) {
        bsp_err("<%s> kmalloc fail.\n", __FUNCTION__);
        goto err;
    }

    name[0] = '\0';
    ret = strncat_s(name, name_len, (char *)req->data, (unsigned long)(req->namelen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(name);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)name)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, name);
        kfree(name);
        return BSP_ERROR;
    }
    ret = memcpy_s(pathtmp, sizeof(pathtmp), name, name_len);
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(name);
        return BSP_ERROR;
    }

    /* 路径中包含'/'并且不在根目录，则检查当前目录是否存在，不存在则创建目录 */
    p = strrchr(pathtmp, '/');
    if ((p != NULL) && (p != pathtmp)) {
        /* 查看上一级目录是否存在，如果不存在则创建此目录 */
        *p = '\0';
        ret = rfile_access_create((char *)pathtmp, req->mode);
        if (ret) {
            bsp_err("<%s> access_create failed.\n", __FUNCTION__);
        }
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_OPEN, name) != BSP_OK) {
        kfree(name);
        return BSP_ERROR;
    }

    cnf.ret = bsp_open(name, req->flags, req->mode);

    if (cnf.ret >= 0) {
        if (rfile_fplist_add(cnf.ret, name)) {
            bsp_err("<%s> add fplist fail.\n", __FUNCTION__);
            kfree(name);
            return BSP_ERROR;
        }
    }

    kfree(name);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: fclose的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_close_req(struct bsp_rfile_close_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }

    if (rfile_lpm_print_filepath(EN_RFILE_OP_CLOSE, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = bsp_close(req->fd);

    rfile_fplist_del((s32)req->fd);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: fwrite的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_write_req(struct bsp_rfile_write_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }
    if (req->size > RFILE_WR_LEN_MAX) {
        bsp_err("<%s> write size too large , size =  %d.\n", __FUNCTION__, req->size);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_filepath(EN_RFILE_OP_WRITE, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = bsp_write(req->fd, (s8 *)req->data, req->size);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    ret = memset_s((void *)req, RFILE_LEN_MAX, 0, (req->size + sizeof(struct bsp_rfile_write_req)));
    if (ret != EOK) {
        bsp_err("<%s> memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return BSP_ERROR;
    }

    return BSP_OK;
}

/*
 * 功能描述: fwrite的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_writesync_req(struct bsp_rfile_write_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }
    if (req->size > RFILE_WR_LEN_MAX) {
        bsp_err("<%s> write size too large , size =  %d.\n", __FUNCTION__, req->size);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_filepath(EN_RFILE_OP_WRITE_SYNC, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = bsp_write_sync(req->fd, (s8 *)req->data, req->size);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    ret = memset_s((void *)req, RFILE_LEN_MAX, 0, (req->size + sizeof(struct bsp_rfile_write_req)));
    if (ret != EOK) {
        bsp_err("<%s> memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return BSP_ERROR;
    }

    return BSP_OK;
}

/*
 * 功能描述: fopen的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_read_req(struct bsp_rfile_read_req *req, u32 id)
{
    u32 len;
    s32 ret;

    struct bsp_rfile_read_cnf *cnf = NULL;

    rfile_mntn_dot_record(__LINE__);
    if ((u32)req->size > RFILE_RD_LEN_MAX) {
        bsp_err("<%s> cnf->size %d > RFILE_RD_LEN_MAX.\n", __FUNCTION__, req->size);

        return BSP_ERROR;
    }
    len = sizeof(struct bsp_rfile_read_cnf) + req->size;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }

    cnf = (struct bsp_rfile_read_cnf *)g_bsp_rfile_main.data_send;
    if (cnf == NULL) {
        bsp_err("<%s> g_bsp_rfile_main.data_send is null.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    ret = memset_s((void *)cnf, len, 0, len);
    if (ret != EOK) {
        bsp_err("<%s> memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return BSP_ERROR;
    }

    cnf->op_type = req->op_type;
    cnf->pstlist = req->pstlist;

    if (rfile_lpm_print_filepath(EN_RFILE_OP_READ, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf->size = bsp_read(req->fd, (s8 *)cnf->data, req->size);

    /* 由C核请求的地方保证读取的数据长度不超过ICC最大长度限制 */
    rfile_icc_send(cnf, len, id);

    ret = memset_s((void *)cnf, len, 0, len);
    if (ret != EOK) {
        bsp_err("<%s> memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return BSP_ERROR;
    }


    return BSP_OK;
}

/*
 * 功能描述: fseek的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_seek_req(struct bsp_rfile_seek_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }

    if (rfile_lpm_print_filepath(EN_RFILE_OP_SEEK, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = bsp_lseek(req->fd, (long)req->offset, req->whence);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: ftell的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_tell_req(struct bsp_rfile_tell_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;

    if (rfile_check_fp(req->fd)) {
        bsp_err("<%s> fd %d rfile_check_fp failed.\n", __FUNCTION__, req->fd);
        return BSP_ERROR;
    }

    if (rfile_lpm_print_filepath(EN_RFILE_OP_TELL, req->fd) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = (s32)bsp_tell(req->fd);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: remove的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_remove_req(struct bsp_rfile_remove_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    char *path = NULL;
    u32 pathlen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;

    pathlen = (u32)(req->pathlen);

    if (pathlen > RFILE_PATHLEN_MAX) {
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->pathlen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_REMOVE, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }

    cnf.ret = bsp_remove((s8 *)path);

    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: mkdir的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_mkdir_req(struct bsp_rfile_mkdir_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    char *path = NULL;
    u32 pathlen;
    s32 ret;

    char pathtmp[RFILE_PATHLEN_MAX + 1] = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    pathlen = (u32)(req->pathlen);

    if (pathlen > RFILE_PATHLEN_MAX) {
        bsp_err("<%s> pathlen too large , pathlen =  %d.\n", __FUNCTION__, pathlen);
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        bsp_err("<%s> kmalloc fail.\n", __FUNCTION__);
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->pathlen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_MKDIR, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }


    ret = memcpy_s(pathtmp, sizeof(pathtmp), path, pathlen);
    if (ret != EOK) {
        bsp_err("<%s> memcpy_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }

    cnf.ret = rfile_access_create((char *)pathtmp, req->mode);
    if (cnf.ret) {
        bsp_err("<%s> access_create failed.\n", __FUNCTION__);
    }
    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: rmdir的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_rmdir_req(struct bsp_rfile_rmdir_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    char *path = NULL;
    u32 pathlen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    pathlen = (u32)(req->pathlen);

    if (pathlen > RFILE_PATHLEN_MAX) {
        bsp_err("<%s> pathlen too large , pathlen =  %d.\n", __FUNCTION__, pathlen);
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        bsp_err("<%s> kmalloc fail.\n", __FUNCTION__);
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->pathlen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_RMDIR, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }

    cnf.ret = bsp_rmdir(path);

    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: opendir的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_opendir_req(struct bsp_rfile_opendir_req *req, u32 id)
{
    struct bsp_rfile_opendir_cnf cnf = {0};
    char *path = NULL;
    u32 pathlen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.dirhandle = BSP_ERROR;

    pathlen = (u32)(req->namelen);

    if (pathlen > RFILE_PATHLEN_MAX) {
        bsp_err("<%s> pathlen too large , pathlen =  %d.\n", __FUNCTION__, pathlen);
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        bsp_err("<%s> kmalloc fail.\n", __FUNCTION__);
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->namelen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_OPENDIR, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }

    cnf.dirhandle = bsp_opendir(path);

    if (cnf.dirhandle >= 0) {
        if (rfile_dp_list_add(cnf.dirhandle, path)) {
            kfree(path);
            return BSP_ERROR;
        }
    }

    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: readdir的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_readdir_req(struct bsp_rfile_readdir_req *req, u32 id)
{
    u32 len;
    struct bsp_rfile_readdir_cnf *cnf = NULL;

    rfile_mntn_dot_record(__LINE__);

    if (req->count > RFILE_RD_LEN_MAX) {
        bsp_err("<%s> req->count too large , req->count =  %d.\n", __FUNCTION__, req->count);
        return BSP_ERROR;
    }

    if (rfile_check_dp(req->dir)) {
        bsp_err("<%s> dir %d rfile_check_dp failed.\n", __FUNCTION__, req->dir);
        return BSP_ERROR;
    }

    len = sizeof(struct bsp_rfile_readdir_cnf) + req->count;

    cnf = kzalloc(len, GFP_KERNEL);
    if (cnf == NULL) {
        bsp_err("<%s> kmalloc failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    cnf->op_type = req->op_type;
    cnf->pstlist = req->pstlist;

    if (rfile_lpm_print_dirpath(EN_RFILE_OP_READDIR, req->dir) != BSP_OK) {
        kfree(cnf);
        return BSP_ERROR;
    }

    cnf->size = bsp_readdir(req->dir, (struct linux_dirent *)(cnf->data), req->count);

    rfile_icc_send(cnf, len, id);

    kfree(cnf);

    return BSP_OK;
}

/*
 * 功能描述: closedir的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_closedir_req(struct bsp_rfile_closedir_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;

    if (rfile_check_dp(req->dir)) {
        bsp_err("<%s> dir %d rfile_check_dp failed.\n", __FUNCTION__, req->dir);
        return BSP_ERROR;
    }

    if (rfile_lpm_print_dirpath(EN_RFILE_OP_CLOSEDIR, req->dir) != BSP_OK) {
        return BSP_ERROR;
    }

    cnf.ret = bsp_closedir(req->dir);

    rfile_dp_list_del(req->dir);

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: stat的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_stat_req(struct bsp_rfile_stat_req *req, u32 id)
{
    struct bsp_rfile_stat_cnf cnf = {0};
    BSP_CHAR *path = NULL;
    u32 pathlen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    pathlen = req->size;

    if (pathlen > RFILE_PATHLEN_MAX) {
        bsp_err("<%s> pathlen too large , pathlen =  %d.\n", __FUNCTION__, pathlen);
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        bsp_err("<%s> kmalloc failed.\n", __FUNCTION__);
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->size));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_STAT, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }

    cnf.ret = bsp_stat(path, &(cnf.ststat));

    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: rename的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_rename_req(struct bsp_rfile_rename_req *req, u32 id)
{
    struct bsp_rfile_rename_cnf cnf = {0};
    char *oldname = NULL;
    char *newname = NULL;
    u32 uloldnamelen, ulnewnamelen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    uloldnamelen = (u32)(strlen((char *)(req->data)) + 1);
    if (uloldnamelen >= req->size) {
        bsp_err("<%s> uloldnamelen too large , uloldnamelen =  %d, req->size = %d.\n", __FUNCTION__, uloldnamelen, req->size);
        goto err;
    }

    ulnewnamelen = (u32)(strlen((char *)(req->data + uloldnamelen)) + 1);

    if ((uloldnamelen + ulnewnamelen) != req->size) {
        bsp_err("<%s> name_len error , uloldnamelen =  %d,ulnewnamelen = %d,req->size = %d.\n", __FUNCTION__, uloldnamelen, ulnewnamelen,req->size);
        goto err;
    }

    oldname = kzalloc(uloldnamelen, GFP_KERNEL);
    if (oldname == NULL) {
        bsp_err("<%s> kmalloc oldname failed.\n", __FUNCTION__);
        goto err;
    }

    newname = kzalloc(ulnewnamelen, GFP_KERNEL);
    if (newname == NULL) {
        kfree(oldname);
        bsp_err("<%s> kmalloc newname failed.\n", __FUNCTION__);
        goto err;
    }

    oldname[0] = '\0';
    ret = strncat_s(oldname, uloldnamelen, (char *)req->data,
                    (unsigned long)(strlen((char *)(req->data)) + 1));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(oldname);
        kfree(newname);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)oldname)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, oldname);
        kfree(oldname);
        kfree(newname);
        return BSP_ERROR;
    }
    newname[0] = '\0';
    ret = strncat_s(newname, ulnewnamelen, (char *)(req->data + uloldnamelen),
                    (unsigned long)(strlen((char *)(req->data + uloldnamelen)) + 1));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(oldname);
        kfree(newname);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)newname)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, newname);
        kfree(oldname);
        kfree(newname);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_RENAME, newname) != BSP_OK) {
        kfree(oldname);
        kfree(newname);
        return BSP_ERROR;
    }

    cnf.ret = bsp_rename(oldname, newname);

    kfree(oldname);
    kfree(newname);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: stat的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_access_req(struct bsp_rfile_access_req *req, u32 id)
{
    struct bsp_rfile_common_cnf cnf = {0};
    BSP_CHAR *path = NULL;
    u32 pathlen;
    s32 ret;

    rfile_mntn_dot_record(__LINE__);

    cnf.op_type = req->op_type;
    cnf.pstlist = req->pstlist;
    cnf.ret = BSP_ERROR;

    pathlen = (u32)req->pathlen;

    if (pathlen > RFILE_PATHLEN_MAX) {
        goto err;
    }

    path = kzalloc(pathlen, GFP_KERNEL);
    if (path == NULL) {
        bsp_err("<%s> kmalloc failed.\n", __FUNCTION__);
        goto err;
    }

    path[0] = '\0';
    ret = strncat_s(path, pathlen, (char *)req->data, (unsigned long)(req->pathlen));
    if (ret != EOK) {
        bsp_err("<%s> strncat_s err. ret =  %d.\n", __FUNCTION__, ret);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_check_path((const char *)path)) {
        bsp_err("<%s> path %s rfile_check_path failed.\n", __FUNCTION__, path);
        kfree(path);
        return BSP_ERROR;
    }
    if (rfile_lpm_print_path(EN_RFILE_OP_ACCESS, path) != BSP_OK) {
        kfree(path);
        return BSP_ERROR;
    }

    cnf.ret = bsp_access(path, req->mode);

    kfree(path);

err:

    rfile_icc_send(&cnf, sizeof(cnf), id);

    return BSP_OK;
}

/*
 * 功能描述: massread的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_massrd_req(struct bsp_rfile_massread_req *req, u32 id)
{
    return BSP_ERROR;
}

/*
 * 功能描述: massread的请求处理
 * 返 回 值: s32
 */
s32 rfile_acore_masswr_req(struct bsp_rfile_masswrite_req *req, u32 id)
{
    return BSP_ERROR;
}
/*
 * 功能描述: icc回调处理函数
 * 返 回 值: s32
 */
s32 bsp_rfile_icc_callback(u32 channel_id, u32 len, void *context)
{
    rfile_mntn_dot_record(__LINE__);

    if (channel_id != RFILE_CCORE_ICC_RD_CHAN) {
        bsp_err("<%s> channel_id %d error.\n", __FUNCTION__, channel_id);

        return BSP_ERROR;
    }

    /* 如果rfile未初始化则利用icc的缓存机制保存数据 */
    if (g_bsp_rfile_main.init_flag != EN_RFILE_INIT_FINISH) {
        bsp_wrn("<%s> initflag %d.\n", __FUNCTION__, g_bsp_rfile_main.init_flag);

        return BSP_OK;
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_stay_awake(g_bsp_rfile_main.wake_lock);
#else
    __pm_stay_awake(&g_bsp_rfile_main.wake_lock);
#endif
    osl_sem_up(&g_bsp_rfile_main.sem_task);

    return BSP_OK;
}

void rfile_reset_proc(void)
{
    int ret;
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    struct fp_list *tmpfp = NULL;
    struct dir_list *tmpdir = NULL;

    /* close dir or files */
    rfile_mntn_dot_record(__LINE__);

    list_for_each_safe(p, n, &(g_bsp_rfile_main.fplist)) {
        tmpfp = list_entry(p, struct fp_list, stlist);

        ret = bsp_close(tmpfp->fp);
        if (ret != BSP_OK) {
            bsp_err("[reset]: <%s> bsp_close fp  failed.\n", __FUNCTION__);
        }

        list_del(&tmpfp->stlist);
        kfree(tmpfp);
    }

    list_for_each_safe(p, n, &(g_bsp_rfile_main.dplist)) {
        tmpdir = list_entry(p, struct dir_list, stlist);
        ret = bsp_closedir(tmpdir->dp);
        if (ret != BSP_OK) {
            bsp_err("[reset]: <%s> bsp_close dir failed.\n", __FUNCTION__);
        }

        list_del(&tmpdir->stlist);
        kfree(tmpdir);
    }
}
s32 rfile_acore_reqfunc_part2(u32 op_type, u32 channel_id)
{
    switch (op_type) {
        case EN_RFILE_OP_OPENDIR: {
            return rfile_acore_opendir_req((struct bsp_rfile_opendir_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_READDIR: {
            return rfile_acore_readdir_req((struct bsp_rfile_readdir_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_CLOSEDIR: {
            return rfile_acore_closedir_req((struct bsp_rfile_closedir_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_STAT: {
            return rfile_acore_stat_req((struct bsp_rfile_stat_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_ACCESS: {
            return rfile_acore_access_req((struct bsp_rfile_access_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_MASSRD: {
            return rfile_acore_massrd_req((struct bsp_rfile_massread_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_MASSWR: {
            return rfile_acore_masswr_req((struct bsp_rfile_masswrite_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_RENAME: {
            return rfile_acore_rename_req((struct bsp_rfile_rename_req *)g_bsp_rfile_main.data, channel_id);
        }

        default: {
            bsp_fatal("<%s> op_type %d  is bigger than EN_RFILE_OP_BUTT.\n", __FUNCTION__, op_type);
            return BSP_ERROR;
        }
    }
}

s32 rfile_acore_reqfunc_part1(u32 op_type, u32 channel_id)
{
    switch (op_type) {
        case EN_RFILE_OP_OPEN: {
            return rfile_acore_open_req((struct bsp_rfile_open_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_CLOSE: {
            return rfile_acore_close_req((struct bsp_rfile_close_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_WRITE: {
            return rfile_acore_write_req((struct bsp_rfile_write_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_WRITE_SYNC: {
            return rfile_acore_writesync_req((struct bsp_rfile_write_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_READ: {
            return rfile_acore_read_req((struct bsp_rfile_read_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_SEEK: {
            return rfile_acore_seek_req((struct bsp_rfile_seek_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_TELL: {
            return rfile_acore_tell_req((struct bsp_rfile_tell_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_REMOVE: {
            return rfile_acore_remove_req((struct bsp_rfile_remove_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_MKDIR: {
            return rfile_acore_mkdir_req((struct bsp_rfile_mkdir_req *)g_bsp_rfile_main.data, channel_id);
        }
        case EN_RFILE_OP_RMDIR: {
            return rfile_acore_rmdir_req((struct bsp_rfile_rmdir_req *)g_bsp_rfile_main.data, channel_id);
        }
        default: {
            return rfile_acore_reqfunc_part2(op_type, channel_id);
        }
    }
}

/*
 * 功能描述: 自处理任务
 * 返 回 值: s32
 */
s32 rfile_task_proc(void *obj)
{
    s32 ret;
    u32 op_type;
    u32 channel_id;

    bsp_debug("<%s> entry.\n", __FUNCTION__);

    while (g_rfile_while) {
        osl_sem_down(&g_bsp_rfile_main.sem_task);

        if (g_bsp_rfile_main.init_flag == EN_RFILE_INIT_SUSPEND) {
            /* 置为idle，不再处理新请求 */
            g_bsp_rfile_main.op_state = EN_RFILE_IDLE;

            /* 关闭文件，并阻塞单独复位任务完成 */
            rfile_reset_proc();
            g_bsp_rfile_main.init_flag = EN_RFILE_INIT_SUSPEND_WAIT;
            osl_sem_up(&g_bsp_rfile_main.sem_closefp);

            continue;
        }

        channel_id = RFILE_CCORE_ICC_RD_CHAN;
        /* 未初始化完成或者处于睡眠状态则利用icc缓冲请求数据 */
        if (g_bsp_rfile_main.init_flag != EN_RFILE_INIT_FINISH) {
            continue;
        }

        g_bsp_rfile_main.op_state = EN_RFILE_DOING;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        __pm_stay_awake(g_bsp_rfile_main.wake_lock);
#else
        __pm_stay_awake(&g_bsp_rfile_main.wake_lock);
#endif
        if (g_bsp_rfile_main.pm_state == EN_RFILE_SLEEP_STATE) {
            bsp_err("%s cur state in sleeping,wait for resume end!\n", __func__);
            continue;
        }

        /* 读取ICC-C通道，输入的长度是buff的size，返回值是实际读取的数据长度 */
        ret = bsp_icc_read(channel_id, g_bsp_rfile_main.data, RFILE_LEN_MAX);
        if (((u32)ret > RFILE_LEN_MAX) || (ret <= 0)) {
            bsp_debug("<%s> icc_read %d.\n", __FUNCTION__, ret);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
            __pm_relax(g_bsp_rfile_main.wake_lock);
#else
            __pm_relax(&g_bsp_rfile_main.wake_lock);
#endif
            g_bsp_rfile_main.op_state = EN_RFILE_IDLE;
            continue; /* A-C通道没读到数据 */
        }

        /* 请求的第一个四字节对应的是 op type */
        op_type = *(u32 *)(g_bsp_rfile_main.data);

        if (op_type >= EN_RFILE_OP_BUTT) {
            bsp_fatal("<%s> op_type %d  is bigger than EN_RFILE_OP_BUTT.\n", __FUNCTION__, op_type);
        } else {
            ret = rfile_acore_reqfunc_part1(op_type, channel_id);
            if (ret != BSP_OK) {
                bsp_err("<%s> cnf_func failed %d.\n", __FUNCTION__, op_type);
            }
        }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        __pm_relax(g_bsp_rfile_main.wake_lock);
#else
        __pm_relax(&g_bsp_rfile_main.wake_lock);
#endif
        /* 处理结束后避免ICC通道中有缓存，再次启动读取 */
        osl_sem_up(&g_bsp_rfile_main.sem_task);
    }
    return 0;
}

s32 bsp_rfile_reset_cb(drv_reset_cb_moment_e eparam, s32 userdata)
{
    if (g_bsp_rfile_main.init_flag == EN_RFILE_INIT_INVALID) {
        bsp_err("[reset]<%s> cnf_func failed  rfile states: EN_RFILE_INIT_INVALID \n", __FUNCTION__);
        return -1;
    }

    if (eparam == MDRV_RESET_CB_BEFORE) {
        /* 设置为suspend状态，待close打开的文件、目录后恢复为FINISH状态 */
        g_bsp_rfile_main.init_flag = EN_RFILE_INIT_SUSPEND;

        /* 启动任务中的close处理 */
        osl_sem_up(&g_bsp_rfile_main.sem_task);

        osl_sem_down(&g_bsp_rfile_main.sem_closefp);
    }

    if (eparam == MDRV_RESET_RESETTING) {
        /* 此时icc缓存已清空，可以接收复位后的C核rfile请求，设置为finish状态 */
        g_bsp_rfile_main.init_flag = EN_RFILE_INIT_FINISH;
        /* 并主动唤醒任务，避免C核启动过程中请求丢失 */
        osl_sem_up(&g_bsp_rfile_main.sem_task);
    }

    return 0;
}

/*
 * 功能描述: rfile模块初始化
 * 返 回 值: void
 */
s32 bsp_rfile_init(void)
{
    s32 ret;
    struct sched_param sch_para;
    sch_para.sched_priority = 15;

    bsp_err("[init]start.\n");

    osl_sem_init(0, &(g_bsp_rfile_main.sem_task));
    osl_sem_init(0, &(g_bsp_rfile_main.sem_closefp));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    g_bsp_rfile_main.wake_lock = wakeup_source_register(NULL, "rfile_wakelock");
    if (g_bsp_rfile_main.wake_lock == NULL) {
        bsp_err("[init]: g_bsp_rfile_main.wake_lock init error.\n");
        return BSP_ERROR;
    }
#else
    wakeup_source_init(&g_bsp_rfile_main.wake_lock, "rfile_wakelock");
#endif
    g_bsp_rfile_main.taskid = kthread_run(rfile_task_proc, BSP_NULL, "rfile");
    if (IS_ERR(g_bsp_rfile_main.taskid)) {
        bsp_err("[init]: <%s> kthread_run failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    if (sched_setscheduler(g_bsp_rfile_main.taskid, SCHED_FIFO, &sch_para) != BSP_OK) {
        bsp_err("[init]: <%s> sched_setscheduler failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    g_bsp_rfile_main.data = kzalloc(RFILE_LEN_MAX, GFP_KERNEL);
    if (g_bsp_rfile_main.data == NULL) {
        bsp_err("[init]: <%s> g_bsp_rfile_main.data alloc failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    g_bsp_rfile_main.data_send = kzalloc(RFILE_LEN_MAX, GFP_KERNEL);
    if (g_bsp_rfile_main.data_send == NULL) {
        bsp_err("[init]: <%s> g_bsp_rfile_main.data_send alloc failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    INIT_LIST_HEAD(&g_bsp_rfile_main.fplist);
    INIT_LIST_HEAD(&g_bsp_rfile_main.dplist);

    ret = memset_s((void *)&g_rfile_mntn_info, sizeof(g_rfile_mntn_info), 0, sizeof(g_rfile_mntn_info));
    if (ret != EOK) {
        bsp_err("<%s> memset_s err. ret =  %d.\n", __FUNCTION__, ret);
        return BSP_ERROR;
    }

    g_bsp_rfile_main.init_flag = EN_RFILE_INIT_FINISH;

    bsp_icc_debug_register(RFILE_CCORE_ICC_RD_CHAN, rfile_lpmcallback, 0);

    ret = bsp_icc_event_register(RFILE_CCORE_ICC_RD_CHAN, bsp_rfile_icc_callback, NULL, NULL, NULL);
    if (ret) {
        bsp_err("[init]: <%s> bsp_icc_event_register failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    adp_rfile_init();

    bsp_err("[init]ok.\n");

    return BSP_OK;
}

/*
 * 功能描述: rfile模块去初始化
 */
s32 bsp_rfile_release(void)
{
    s32 ret;

    bsp_debug("<%s> entry.\n", __FUNCTION__);

    g_bsp_rfile_main.init_flag = EN_RFILE_INIT_INVALID;

    osl_sema_delete(&g_bsp_rfile_main.sem_task);

    kthread_stop(g_bsp_rfile_main.taskid);

    ret = bsp_icc_event_unregister(RFILE_CCORE_ICC_RD_CHAN);
    if (ret) {
        bsp_err("<%s> bsp_icc_event_unregister failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    return BSP_OK;
}



#if (FEATURE_DELAY_MODEM_INIT == FEATURE_ON)

static int modem_rfile_probe(struct platform_device *dev)
{
    int ret;

    g_bsp_rfile_main.pm_state = EN_RFILE_WAKEUP_STATE;
    g_bsp_rfile_main.op_state = EN_RFILE_IDLE;

    ret = bsp_rfile_init();

    return ret;
}

static void modem_rfile_shutdown(struct platform_device *dev)
{
    bsp_info("%s shutdown start \n", __func__);

    g_bsp_rfile_main.init_flag = EN_RFILE_INIT_INVALID;
}
#ifdef CONFIG_PM
static s32 modem_rfile_suspend(struct device *dev)
{
    static s32 count = 0;
    if (g_bsp_rfile_main.op_state != EN_RFILE_IDLE) {
        bsp_err("[SR] %s modem rfile is in doing!\n", __func__);
        return -1;
    }
    g_bsp_rfile_main.pm_state = EN_RFILE_SLEEP_STATE;
    bsp_info("[SR]modem rfile enter suspend! %d times \n", ++count);
    return 0;
}
static s32 modem_rfile_resume(struct device *dev)
{
    static s32 count = 0;
    g_bsp_rfile_main.pm_state = EN_RFILE_WAKEUP_STATE;
    if (g_bsp_rfile_main.op_state == EN_RFILE_DOING) {
        bsp_err("[SR]%s need to enter task proc!\n", __func__);
        osl_sem_up(&g_bsp_rfile_main.sem_task);
    }
    bsp_info("[SR]modem rfile enter resume! %d times \n", ++count);
    return 0;
}
static const struct dev_pm_ops modem_rfile_pm_ops = {
    .suspend = modem_rfile_suspend,
    .resume = modem_rfile_resume,
};

#define BALONG_RFILE_PM_OPS (&modem_rfile_pm_ops)
#else
#define BALONG_RFILE_PM_OPS NULL
#endif
static struct platform_driver g_modem_rfile_drv = {
    .probe      = modem_rfile_probe,
    .shutdown   = modem_rfile_shutdown,
    .driver     = {
        .name     = "modem_rfile",
        .owner    = THIS_MODULE,
        .pm       = BALONG_RFILE_PM_OPS,
        .probe_type = PROBE_FORCE_SYNCHRONOUS,
    },
};

static struct platform_device g_modem_rfile_device = {
    .name = "modem_rfile",
    .id = 0,
    .dev = {
    .init_name = "modem_rfile",
    },
};

int modem_rfile_init(void)
{
    int ret;

    ret = platform_device_register(&g_modem_rfile_device);
    if (ret) {
        bsp_err("[init]platform_device_register g_modem_rfile_device fail !\n");
        return -1;
    }

    ret = platform_driver_register(&g_modem_rfile_drv);
    if (ret) {
        bsp_err("[init]platform_device_register g_modem_rfile_drv fail !\n");
        platform_device_unregister(&g_modem_rfile_device);
        return -1;
    }

    return 0;
}

void modem_rfile_exit(void)
{
    platform_device_unregister(&g_modem_rfile_device);
    platform_driver_unregister(&g_modem_rfile_drv);
}

#else
module_init(bsp_rfile_init);
#endif

