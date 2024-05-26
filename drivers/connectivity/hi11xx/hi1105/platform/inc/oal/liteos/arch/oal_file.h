

#ifndef __OAL_LITEOS_FILE_H__
#define __OAL_LITEOS_FILE_H__

#ifndef _PRE_HI113X_FS_DISABLE
#include <fs/fs.h>
#include <fcntl.h>
#include "oal_mm.h"
#include <oal_types.h>
#include <fs/file.h>
#else
#include "oal_no_fs_adapt.h"
#endif

#ifndef _PRE_HI113X_FS_DISABLE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* ÎÄ¼þÊôÐÔ */
#define OAL_O_ACCMODE           O_ACCMODE
#define OAL_O_RDONLY            O_RDONLY
#define OAL_O_WRONLY            O_WRONLY
#define OAL_O_RDWR              O_RDWR
#define OAL_O_CREAT             O_CREAT
#define OAL_O_TRUNC             O_TRUNC
#define OAL_O_APPEND            O_APPEND
#define OAL_O_EXCL              O_EXCL  /* not fcntl */
#define OAL_O_NOCTTY            O_NOCTTY    /* not fcntl */
#define OAL_O_NONBLOCK          O_NONBLOCK
#define OAL_O_DSYNC             O_DSYNC          /* used to be O_SYNC, see below */
#define OAL_FASYNC              FASYNC            /* fcntl, for BSD compatibility */
#define OAL_O_DIRECT            O_DIRECT        /* direct disk access hint */
#define OAL_O_LARGEFILE         O_LARGEFILE
#define OAL_O_DIRECTORY         O_DIRECTORY      /* must be a directory */
#define OAL_O_NOFOLLOW          O_NOFOLLOW        /* don't follow links */
#define OAL_O_NOATIME           O_NOATIME
#define OAL_O_CLOEXEC           O_CLOEXEC      /* set close_on_exec */


#define OAL_SEEK_SET      SEEK_SET     /* Seek from beginning of file.  */
#define OAL_SEEK_CUR     SEEK_CUR    /* Seek from current position.  */
#define OAL_SEEK_END     SEEK_END    /* Set file pointer to EOF plus "offset" */

#define OAL_FILE_POS(pst_file)      oal_get_file_pos(pst_file)
typedef struct _oal_file_stru_ {
    int fd;
} oal_file_stru;

OAL_STATIC OAL_INLINE oal_file_stru* oal_file_open(const int8_t *pc_path, int32_t flags, int32_t rights)
{
    oal_file_stru   *pst_file = NULL;
    pst_file = calloc(1, sizeof(oal_file_stru));
    if (pst_file == NULL)
        return NULL;
    pst_file->fd = open(pc_path, flags, rights);
    if (pst_file->fd < 0) {
        free(pst_file);
        return NULL;
    }
    return pst_file;
}


OAL_STATIC OAL_INLINE uint32_t oal_file_write(oal_file_stru *pst_file, int8_t *pc_string, uint32_t ul_length)
{
    return (uint32_t)write(pst_file->fd, pc_string, ul_length);
}


OAL_STATIC OAL_INLINE void oal_file_close(oal_file_stru *pst_file)
{
    close(pst_file->fd);
    free(pst_file);
    pst_file = NULL;
}


OAL_STATIC OAL_INLINE int32_t  oal_file_read(oal_file_stru *pst_file,
                                             int8_t *pc_buf,
                                             uint32_t ul_count)
{
    return read(pst_file->fd, pc_buf, ul_count);
}

OAL_STATIC OAL_INLINE int64_t  oal_file_lseek(oal_file_stru *pst_file, int64_t offset, int32_t whence)
{
    return lseek(pst_file->fd, offset, whence);
}


OAL_STATIC OAL_INLINE int32_t  oal_file_size(const int8_t *pc_path, uint32_t   *pul_file_size)
{
    oal_file_stru     *p_file;

    p_file = oal_file_open(pc_path, (OAL_O_RDONLY), 0);
    if (p_file == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    *pul_file_size = oal_file_lseek(p_file, 0, OAL_SEEK_END);
    oal_file_close(p_file);

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_get_file_pos(oal_file_stru *pst_file)
{
    struct file *fp = NULL;
    fp = fs_getfilep(pst_file->fd);
    return (int)fp->f_pos;
}

#else

#define OAL_FILE_POS(pst_file)      oal_get_file_pos(pst_file)


OAL_STATIC OAL_INLINE oal_file_stru* oal_file_open(const int8_t *pc_path, int32_t flags, int32_t rights)
{
return oal_no_fs_file_open(pc_path);
}


OAL_STATIC OAL_INLINE int32_t oal_file_write(oal_file_stru *pst_file, int8_t *pc_string, uint32_t ul_length)
{
    return (int32_t)oal_no_fs_file_write(pst_file, pc_string, ul_length);
}


OAL_STATIC OAL_INLINE void oal_file_close(oal_file_stru *pst_file)
{
    oal_no_fs_file_close(pst_file);
}


OAL_STATIC OAL_INLINE int32_t  oal_file_read(oal_file_stru *pst_file,
                                             int8_t *pc_buf,
                                             uint32_t ul_count)
{
    return oal_no_fs_file_read(pst_file, pc_buf, ul_count);
}

OAL_STATIC OAL_INLINE int64_t  oal_file_lseek(oal_file_stru *pst_file, int64_t offset, int32_t whence)
{
    return oal_no_fs_file_lseek(pst_file, offset, whence);
}


OAL_STATIC OAL_INLINE int32_t  oal_file_size(const int8_t *pc_path, uint32_t   *pul_file_size)
{
    oal_file_stru     *p_file;

    p_file = oal_file_open(pc_path, (OAL_O_RDONLY), 0);
    if (p_file == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    *pul_file_size = oal_file_lseek(p_file, 0, OAL_SEEK_END);
    oal_file_close(p_file);

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_get_file_pos(oal_file_stru *pst_file)
{
    return oal_no_fs_get_file_pos(pst_file);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
#endif /* end of oal_file.h */
