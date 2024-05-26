

#ifndef __OAL_LITEOS_KERNEL_FILE_H__
#define __OAL_LITEOS_KERNEL_FILE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define OAL_KERNEL_DS           KERNEL_DS
#define OAL_PRINT_FORMAT_LENGTH     512                     /* 打印格式字符串的最大长度 */
typedef struct file             oal_file;
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
extern oal_kobject* oal_get_sysfs_root_object(void);
extern oal_kobject* oal_get_sysfs_root_boot_object(void);
extern oal_kobject* oal_conn_sysfs_root_obj_init(void);
extern void oal_conn_sysfs_root_obj_exit(void);
extern void oal_conn_sysfs_root_boot_obj_exit(void);
#endif
extern oal_file* oal_kernel_file_open(uint8_t *path, int32_t ul_attribute);
extern loff_t oal_kernel_file_size(oal_file *pst_file);
extern void *oal_kernel_file_read(oal_file *pst_file, loff_t ul_fsize);
extern int32_t oal_kernel_file_write(oal_file *pst_file, uint8_t *pst_buf, loff_t fsize);
extern int32_t oal_kernel_file_print(oal_file *pst_file, const int8_t *pc_fmt, ...);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_main */
