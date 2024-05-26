

#if (defined _PRE_CONFIG_GPIO_TO_SSI_DEBUG) && (defined PLATFORM_SSI_DEBUG_CMD)
/* 头文件包含 */
#include "ssi_common.h"
#include <linux/delay.h>
#include "oal_kernel_file.h"

#define SSI_DEBUG_MSG_LEN 100
struct ssi_sysfs_debug_info {
    char *name;
    char *usage;
    int32_t (*debug)(const char *buf, uint32_t count);
};
static void oal_ssi_print_debug_usages(void);
OAL_STATIC int32_t oal_ssi_print_debug_info(const char *buf, uint32_t count)
{
    oal_ssi_print_debug_usages();
    return OAL_SUCC;
}
OAL_STATIC int32_t oal_ssi_debug_read32(const char *buf, uint32_t count)
{
    uint32_t addr = 0xFFFFFFFF;
    uint32_t value;
    if ((sscanf_s(buf, "0x%x", &addr) != 1)) {
        return -OAL_EINVAL;
    }
    value = ssi_read_value32_test(addr);
    oal_io_print("0x%x\n", value);
    return OAL_SUCC;
}
OAL_STATIC int32_t oal_ssi_debug_read16(const char *buf, uint32_t count)
{
    uint32_t addr = 0xFFFFFFFF;
    uint16_t value;
    if ((sscanf_s(buf, "0x%x", &addr) != 1)) {
        return -OAL_EINVAL;
    }
    value = ssi_read32(addr);
    oal_io_print("0x%x\n", value);
    return OAL_SUCC;
}
OAL_STATIC int32_t oal_ssi_debug_write16(const char *buf, uint32_t count)
{
    uint32_t addr = 0xFFFFFFFF;
    uint32_t value;
    if ((sscanf_s(buf, "0x%x 0x%x", &addr, &value) != 2)) {
        return -OAL_EINVAL;
    }
    ssi_write32(addr, value & 0xFFFF);
    return OAL_SUCC;
}
OAL_STATIC int32_t oal_ssi_debug_write32(const char *buf, uint32_t count)
{
    uint32_t addr = 0xFFFFFFFF;
    uint32_t value;
    if ((sscanf_s(buf, "0x%x 0x%x", &addr, &value) != 2)) {
        return -OAL_EINVAL;
    }
    ssi_write_value32_test(addr, value);
    return OAL_SUCC;
}

void oal_ssi_print_device_mem(uint32_t addr, uint32_t length)
{
    /* echo readmem address length(hex) > debug */
    uint32_t index;
    void *print_buf = NULL;

    if (length == 0) { /* 0 is invalid */
        oal_io_print("input len is 0\n");
        return;
    }
    length = padding_m(length, 4); /* 计算4字节对齐后的长度，默认进位 */
    print_buf = vmalloc(length);
    if (print_buf == NULL) {
        return;
    }
    for (index = 0; index < length; index += sizeof(uint32_t)) { /* 每次偏移4字节 */
        *(uint32_t *)(print_buf + index) = ssi_read_value32_test(addr + index);
    }
    oal_io_print("readmem addr:0x%8x len:%u done\n", addr, index);
    print_hex_dump(KERN_INFO, "readmem: ", DUMP_PREFIX_OFFSET, 16, 4, print_buf, length, true);
    vfree(print_buf);
    return;
}

OAL_STATIC int32_t oal_ssi_debug_readmem(const char *buf, uint32_t count)
{
    /* echo readmem address length(hex) > debug */
    uint32_t addr, length;

    if ((sscanf_s(buf, "0x%x %u", &addr, &length) != 2)) {
        return -OAL_EINVAL;
    }
    oal_ssi_print_device_mem(addr, length);
    return OAL_SUCC;
}

OAL_STATIC int32_t oal_ssi_loadfile(char *file_name, uint32_t addr)
{
    /* echo loadfile address filename > debug */
    /* echo loadfile 0x600000 /tmp/readmem.bin */
    struct file *fp = NULL;
    int32_t rlen;
    int32_t total_len = 0;
    void *pst_buf = NULL;
    mm_segment_t fs;
    int i;

    pst_buf = oal_memalloc(PAGE_SIZE);
    if (pst_buf == NULL) {
        oal_io_print("pst_buf is null\n");
        return -OAL_ENOMEM;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(file_name, O_RDWR, 0664);
    set_fs(fs);
    if (oal_is_err_or_null(fp)) {
        oal_io_print("open file error,fp = 0x%p, filename is [%s]\n", fp, file_name);
        oal_free(pst_buf);
        return -OAL_EINVAL;
    }

    oal_io_print("loadfile cpu:0x%8x loadpath:%s\n", addr, file_name);
    forever_loop() {
        rlen = oal_file_read_ext(fp, fp->f_pos, (void *)pst_buf, PAGE_SIZE);
        if (rlen <= 0) {
            break;
        }
        fp->f_pos += rlen;
        for (i = 0; i < rlen; i += sizeof(uint32_t)) {
            ssi_write_value32_test(addr + i, *(uint32_t *)(pst_buf + i));
        }
        total_len += rlen;
        addr += rlen;
        msleep(1);
        oal_io_print("###");
    }

    if (total_len) {
        oal_io_print("loadfile cpu:0x%8x len:%u loadpath:%s done\n", addr, total_len, file_name);
    } else {
        oal_io_print("loadfile cpu:0x%8x len:%u loadpath:%s failed\n", addr, total_len, file_name);
    }
    oal_free(pst_buf);
    oal_file_close(fp);
    return OAL_SUCC;
}

OAL_STATIC int32_t oal_ssi_debug_loadfile(const char *buf, uint32_t count)
{
    char file_name[SSI_DEBUG_MSG_LEN];
    uint32_t addr;

    if (strlen(buf) >= sizeof(file_name)) {
        oal_io_print("input illegal!%s\n", __FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf_s(buf, "0x%x %s", &addr, file_name, sizeof(file_name)) != 2)) {
        oal_io_print("loadfile argument invalid,[%s], should be [echo writemem address  filename > debug]\n", buf);
        return -OAL_EINVAL;
    }
    return oal_ssi_loadfile(file_name, addr);
}

OAL_STATIC struct ssi_sysfs_debug_info g_ssi_debug[] = {
    { "help", "", oal_ssi_print_debug_info },
    { "read16", "address(hex)", oal_ssi_debug_read16 },
    { "write16", "address(hex) value(hex)", oal_ssi_debug_write16 },
    { "read32", "address(hex)", oal_ssi_debug_read32 },
    { "write32", "address(hex) value(hex)", oal_ssi_debug_write32 },
    { "readmem", "address(hex) length(decimal)", oal_ssi_debug_readmem },
    { "loadfile", "address(hex) filename", oal_ssi_debug_loadfile },
};

OAL_STATIC void oal_ssi_print_debug_usage(uint32_t i)
{
    int32_t ret;
    void *buf = oal_memalloc(PAGE_SIZE);
    if (buf == NULL) {
        return;
    }
    ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "echo %s %s > /sys/hisys/boot/ssi_cmd\n",
                     g_ssi_debug[i].name ? : "", g_ssi_debug[i].usage ? : "");
    if (ret < 0) {
        oal_free(buf);
        oal_io_print("log str format err line[%d]\n", __LINE__);
        return;
    }
    printk("%s", (char *)buf);
    oal_free(buf);
}
static void oal_ssi_print_debug_usages(void)
{
    int32_t i;
    int32_t ret;
    void *buf = oal_memalloc(PAGE_SIZE);
    if (buf == NULL) {
        return;
    }
    for (i = 0; i < oal_array_size(g_ssi_debug); i++) {
        ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "echo %s %s > /sys/hisys/boot/ssi_cmd\n",
                         g_ssi_debug[i].name ? : "", g_ssi_debug[i].usage ? : "");
        if (ret < 0) {
            oal_free(buf);
            oal_io_print("log str format err line[%d]\n", __LINE__);
            return;
        }
        printk("%s", (char *)buf);
    }
    oal_free(buf);
}
OAL_STATIC ssize_t oal_ssi_get_debug_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret;
    int32_t i;
    int32_t count = 0;
    ret = snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1, "pci debug cmds:\n");
    if (ret < 0) {
        oal_io_print("log str format err line[%d]\n", __LINE__);
        return count;
    }
    count += ret;
    for (i = 0; i < oal_array_size(g_ssi_debug); i++) {
        ret = snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1, "%s\n", g_ssi_debug[i].name);
        if (ret < 0) {
            oal_io_print("log str format err line[%d]\n", __LINE__);
            return count;
        }
        count += ret;
    }
    return count;
}
OAL_STATIC ssize_t oal_ssi_set_debug_info(struct kobject *dev, struct kobj_attribute *attr, const char *buf,
                                          size_t count)
{
    int32_t i;
    if (buf[count] != '\0') { /* 确保传进来的buf是一个字符串, count不包含结束符 */
        oal_io_print("invalid pci cmd\n");
        return 0;
    }
    for (i = 0; i < oal_array_size(g_ssi_debug); i++) {
        if (g_ssi_debug[i].name) {
            int32_t len = OAL_STRLEN(g_ssi_debug[i].name);
            char last_c = *(buf + OAL_STRLEN(g_ssi_debug[i].name));
            if ((count >= len) && !oal_memcmp(g_ssi_debug[i].name, buf, OAL_STRLEN(g_ssi_debug[i].name)) &&
                (last_c == '\n' || last_c == ' ' || last_c == '\0')) {
                break;
            }
        }
    }
    if (i == oal_array_size(g_ssi_debug)) {
        oal_io_print("invalid ssi cmd:%s\n", buf);
        oal_ssi_print_debug_usages();
        return count;
    }
    buf += OAL_STRLEN(g_ssi_debug[i].name);
    if (*buf != '\0') { // count > OAL_STRLEN(g_ssi_debug[i].name)
        buf += 1;       /* EOF */
    }
    for (; *buf == ' '; buf++) {
    }
    if (g_ssi_debug[i].debug((const char *)buf, count) == -OAL_EINVAL) {
        oal_ssi_print_debug_usage(i);
    }
    return count;
}
STATIC struct kobj_attribute g_dev_attr_ssi_cmd =
    __ATTR(ssi_cmd, S_IRUGO | S_IWUSR, oal_ssi_get_debug_info, oal_ssi_set_debug_info);
OAL_STATIC struct attribute *g_ssi_cmd_init_sysfs_entries[] = { &g_dev_attr_ssi_cmd.attr, NULL };
OAL_STATIC struct attribute_group g_ssi_cmd_attribute_group = {
    .attrs = g_ssi_cmd_init_sysfs_entries,
};

void ssi_cmd_create_dev_node(void)
{
    int ret;
    oal_kobject *pst_root_boot_object = oal_get_sysfs_root_boot_object();
    ret = sysfs_create_group(pst_root_boot_object, &g_ssi_cmd_attribute_group);
    if (ret) {
        oal_io_print("sysfs create ssi cmd node fail.ret=%d\n", ret);
        return;
    }
    oal_io_print("create ssi cmd node succ\r\n");
}
#endif
