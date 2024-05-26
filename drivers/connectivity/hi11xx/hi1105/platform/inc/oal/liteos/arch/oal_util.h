

#ifndef __OAL_LITEOS_UTIL_H__
#define __OAL_LITEOS_UTIL_H__

#include "stdio.h"
#include "los_sys.h"
#include "oal_time.h"
#include "oal_atomic.h"
#include "oal_mm.h"
#include <asm/delay.h>
#include <string.h>
#include <ctype.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef current
#define current 0
#endif
#define __attribute_const__

/* 32字节序大小端转换 */
#define OAL_SWAP_BYTEORDER_32(_val)        \
    ((((_val) & 0x000000FF) << 24) +     \
     (((_val) & 0x0000FF00) << 8) +       \
     (((_val) & 0x00FF0000) >> 8) +       \
     (((_val) & 0xFF000000) >> 24))

/* 用作oal_print_hex_dump()的第三个参数，因为函数并没有用到该参数，所以定义成固定值 */
#define HEX_DUMP_GROUP_SIZE 32

/* 获取CORE ID */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define oal_get_core_id()    (0)
#else
#define oal_get_core_id()    smp_processor_id()
#endif

#define oal_likely(_expr)       likely(_expr)
#define oal_unlikely(_expr)     unlikely(_expr)
#define OAL_FUNC_NAME           __func__
#define OAL_RET_ADDR            ((uintptr_t)__builtin_return_address(0))

#define OAL_PAGE_SIZE         PAGE_SIZE

/* 内存读屏障 */
#define OAL_RMB()               rmb()

/* 内存写屏障 */
#define OAL_WMB()               wmb()

/* 内存屏障 */
#define OAL_MB()                mb()

#define OAL_OFFSET_OF          offsetof

#define __OAL_DECLARE_PACKED    __attribute__((__packed__))

#ifndef HISI_LOG_TAG
#define HISI_LOG_TAG
#endif

#ifndef HW_LITEOS_OPEN_VERSION_NUM
#define HW_LITEOS_OPEN_VERSION_NUM  KERNEL_VERSION(1, 3, 2)
#endif

#define oal_io_print                dprintf
#define oal_io_print_ERR            dprintf
#define local_clock()               0
#define NEWLINE "\n"

#define OAL_BUG_ON(_con)            BUG_ON(_con)
#define oal_warn_on(condition)      (condition)  //lint !e522
typedef ssize_t     oal_ssize_t;    /* hi1102-cb for sys interface  51/02 */
typedef size_t      oal_size_t;
typedef struct device_attribute     oal_device_attribute_stru;
typedef struct device               oal_device_stru;

#define OAL_DEVICE_ATTR
#define OAL_S_IRUGO                 S_IRUGO
#define OAL_S_IWGRP                 S_IWGRP
#define OAL_S_IWUSR                 S_IWUSR
typedef struct kobject              oal_kobject;
#define OAL_STRLEN                                  strlen
#define OAL_STRSTR                                  strstr
#define OAL_STRCMP                                  strcmp
#define OAL_STRCHR                                  strchr
#define OAL_STRNCMP                                 strncmp
#define OAL_SIZEOF                                  sizeof
#define oal_simple_strtoul                          strtoul
#define PLATFORM_NAME_SIZE  20
#define GPIOF_DIR_OUT   (0 << 0)
#define GPIOF_DIR_IN    (1 << 0)

#define GPIOF_INIT_LOW  (0 << 1)
#define GPIOF_INIT_HIGH (1 << 1)

#define GPIOF_IN        (GPIOF_DIR_IN)
#define GPIOF_OUT_INIT_LOW  (GPIOF_DIR_OUT | GPIOF_INIT_LOW)
typedef unsigned long kernel_ulong_t;
typedef void *pm_message_t;

#define OAL_UNUSED_PARAM(x)                (void)(x)

/* 虚拟地址转物理地址 */
#define OAL_VIRT_TO_PHY_ADDR(_virt_addr)            ((uint32_t)(_virt_addr))

/* 物理地址转虚拟地址 */
#define oal_phy_to_virt_addr(_phy_addr)             ((uint32_t *)(_phy_addr))


ktime_t g_ret_time;


struct attribute {
    const char *name;
    unsigned short mode;
};

struct attribute_group {
    const char      *name;
    unsigned short          (*is_visible)(struct kobject *,
                                          struct attribute *, int);
    struct attribute    **attrs;
};

struct device_attribute {
    struct attribute    attr;
    ssize_t (*show)(struct device *dev, struct device_attribute *attr,
                    char *buf);
    ssize_t (*store)(struct device *dev, struct device_attribute *attr,
                     const char *buf, size_t count);
};

struct kobj_attribute {
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,
                    char *buf);
    ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,
                     const char *buf, size_t count);
};

struct platform_device_id {
    char name[PLATFORM_NAME_SIZE];
    kernel_ulong_t driver_data;
};

struct pdev_archdata {
};

#if (HW_LITEOS_OPEN_VERSION_NUM < KERNEL_VERSION(3, 1, 3))
struct platform_device {
    const char  *name;
    int     id;
    bool        id_auto;
    struct device   dev;
    uint32_t num_resources;
    struct resource *resource;

    const struct platform_device_id *id_entry;

    /* MFD cell pointer */
    struct mfd_cell *mfd_cell;

    /* arch specific additions */
    struct pdev_archdata    archdata;
};

struct platform_driver {
    int (*probe)(struct platform_device *);
    int (*remove)(struct platform_device *);
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;
    const struct platform_device_id *id_table;
};
#endif
OAL_STATIC  OAL_INLINE uint64_t tickcount_get(void)
{
    return LOS_TickCountGet();
}

OAL_STATIC  OAL_INLINE ktime_t ktime_get(void)
{
    g_ret_time.tv64 = LOS_TickCountGet();

    return g_ret_time;
}

OAL_STATIC  OAL_INLINE ktime_t ktime_sub(ktime_t time_1st, ktime_t time_2nd)
{
    ktime_t ret_time;
    ret_time.tv64 = time_2nd.tv64 - time_1st.tv64;
    return ret_time;
}

OAL_STATIC  OAL_INLINE uint64_t ktime_to_us(ktime_t in_time)
{
    return((in_time.tv64) * 10 * 1000);
}

OAL_STATIC  OAL_INLINE uint64_t ktime_to_ms(ktime_t in_time)
{
    return((in_time.tv64) * 10);
}

OAL_STATIC  OAL_INLINE void oal_random_ether_addr(uint8_t *puc_macaddr)
{
    struct timeval st_tv_1;
    struct timeval st_tv_2;

    if (puc_macaddr == NULL) {
        return;
    }

    /* 获取随机种子 */
    gettimeofday(&st_tv_1, NULL);

    /* 防止秒级种子为0 */
    st_tv_1.tv_sec += 2;

    st_tv_2.tv_sec = (uint32_t)st_tv_1.tv_sec * st_tv_1.tv_sec * st_tv_1.tv_usec;
    st_tv_2.tv_usec = (uint32_t)st_tv_1.tv_sec * st_tv_1.tv_usec * st_tv_1.tv_usec;

    /* 生成随机的mac地址 */
    puc_macaddr[0] = (st_tv_2.tv_sec & 0xff) & 0xfe;
    puc_macaddr[1] = st_tv_2.tv_usec & 0xff;
    puc_macaddr[2] = (st_tv_2.tv_sec & 0xff0) >> 4;
    puc_macaddr[3] = (st_tv_2.tv_usec & 0xff0) >> 4;
    puc_macaddr[4] = (st_tv_2.tv_sec & 0xff00) >> 8;
    puc_macaddr[5] = (st_tv_2.tv_usec & 0xff00) >> 8;
}



OAL_STATIC OAL_INLINE __attribute_const__ uint16_t  oal_byteorder_host_to_net_uint16(uint16_t us_byte)
{
    us_byte = ((us_byte & 0x00FF) << 8) + ((us_byte & 0xFF00) >> 8);

    return us_byte;
}


OAL_STATIC OAL_INLINE __attribute_const__ uint16_t  oal_byteorder_net_to_host_uint16(uint16_t us_byte)
{
    us_byte = ((us_byte & 0x00FF) << 8) + ((us_byte & 0xFF00) >> 8);

    return us_byte;
}


OAL_STATIC OAL_INLINE __attribute_const__ uint32_t  oal_byteorder_host_to_net_uint32(uint32_t ul_byte)
{
    ul_byte = OAL_SWAP_BYTEORDER_32(ul_byte);

    return ul_byte;
}


OAL_STATIC OAL_INLINE __attribute_const__ uint32_t  oal_byteorder_net_to_host_uint32(uint32_t ul_byte)
{
    ul_byte = OAL_SWAP_BYTEORDER_32(ul_byte);

    return ul_byte;
}


OAL_STATIC OAL_INLINE int32_t  oal_atoi(const char *c_string)
{
    int32_t l_ret = 0;
    int32_t flag = 0;

    for (;; c_string++) {
        switch (*c_string) {
            case '0' ... '9':
                l_ret = 10 * l_ret + (*c_string - '0');
                break;

            case '-':
                flag = 1;
                break;

            case ' ':
                continue;

            default:
                return ((flag == 0) ? l_ret : (-l_ret));;
        }
    }
}

OAL_STATIC OAL_INLINE void  oal_itoa(int32_t l_val, int8_t *c_string, uint8_t uc_strlen)
{
    if (snprintf_s((char *)c_string, uc_strlen, uc_strlen - 1, "%d", l_val) < EOK) {
        printk("oal_itoa::snprintf_s failed!");
        return;
    }
}


OAL_STATIC OAL_INLINE int8_t *oal_strtok(int8_t *pc_token, const int8_t *pc_delemit, int8_t **ppc_context)
{
    int8_t *pc_str = NULL;
    const int8_t *pc_ctrl = pc_delemit;

    uint8_t uc_map[32];
    int32_t l_count;

    static int8_t *pc_nextoken;

    /* Clear control map */
    for (l_count = 0; l_count < 32; l_count++) {
        uc_map[l_count] = 0;
    }

    /* Set bits in delimiter table */
    do {
        uc_map[*pc_ctrl >> 3] |= (1 << (*pc_ctrl & 7));
    } while (*pc_ctrl++);

    /* Initialize str. If string is NULL, set str to the saved
    * pointer (i.e., continue breaking tokens out of the string
    * from the last strtok call) */
    if (pc_token != NULL) {
        pc_str = pc_token;
    } else {
        pc_str = pc_nextoken;
    }

    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token iff this loop sets str to point to the terminal
    * null (*str == '\0') */
    while ((uc_map[*pc_str >> 3] & (1 << (*pc_str & 7))) && *pc_str) {
        pc_str++;
    }

    pc_token = pc_str;

    /* Find the end of the token. If it is not the end of the string,
    * put a null there. */
    for (; *pc_str; pc_str++) {
        if (uc_map[*pc_str >> 3] & (1 << (*pc_str & 7))) {
            *pc_str++ = '\0';
            break;
        }
    }

    /* Update nextoken (or the corresponding field in the per-thread data
    * structure */
    pc_nextoken = pc_str;

    /* Determine if a token has been found. */
    if (pc_token == pc_str) {
        return NULL;
    } else {
        return pc_token;
    }
}

/* Works only for digits and letters, but small and fast */
#define TOLOWER(x) ((x) | 0x20)
#define OAL_ROUND_UP(x, y)   OAL_ROUNDUP(x, y)
/* 计算为字节对齐后填充后的长度 */
#define OAL_ROUNDUP(_old_len, _align)  ((((_old_len) + ((_align) - 1)) / (_align)) * (_align))
#define OAL_ROUND_DOWN(value, boundary)  ((value) & (~((boundary)-1)))

OAL_STATIC OAL_INLINE unsigned int simple_guess_base(const char *cp)
{
    if (cp[0] == '0') {
        if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2])) {
            return 16;
        } else {
            return 8;
}
    } else {
        return 10;
    }
}

OAL_STATIC OAL_INLINE unsigned long long oal_simple_strtoull(const int8_t *cp, int8_t **endp, unsigned int base)
{
    unsigned long long result = 0;

    if (!base)
        base = simple_guess_base(cp);

    if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
        cp += 2;

    while (isxdigit(*cp)) {
        unsigned int value;

        value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
        if (value >= base)
            break;
        result = result * base + value;
        cp++;
    }
    if (endp != NULL)
        *endp = (int8_t *)cp;

    return result;
}

OAL_STATIC OAL_INLINE int32_t  oal_strtol(const int8_t *pc_nptr, int8_t **ppc_endptr, int32_t l_base)
{
    /* 跳过空格 */
    while ((*pc_nptr) == ' ') {
        pc_nptr++;
    }

    if (*pc_nptr == '-')
        return -oal_simple_strtoull(pc_nptr + 1, ppc_endptr, l_base);

    return oal_simple_strtoull(pc_nptr, ppc_endptr, l_base);
}

OAL_STATIC OAL_INLINE void  oal_udelay(uint32_t u_loops)
{
    udelay(u_loops);
}

OAL_STATIC OAL_INLINE void oal_usleep(unsigned long us)
{
    oal_udelay(us);
}

OAL_STATIC OAL_INLINE void  oal_mdelay(uint32_t u_loops)
{
    mdelay(u_loops);
}

OAL_STATIC OAL_INLINE uint32_t  oal_kallsyms_lookup_name(const uint8_t *uc_var_name)
{
    return 1;
}

OAL_STATIC OAL_INLINE void oal_dump_stack(void)
{
}

OAL_STATIC OAL_INLINE void  oal_msleep(uint32_t ul_usecs)
{
    msleep(ul_usecs);
}

OAL_STATIC OAL_INLINE size_t oal_strlen(const char *str)
{
    return strlen(str);
}
OAL_STATIC OAL_INLINE void oal_print_hex_dump(uint8_t *addr, int32_t len, int32_t groupsize,
                                              int8_t *pre_str)
{
#ifdef CONFIG_PRINTK
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_REFERENCE(groupsize);
    printk(KERN_DEBUG"buf %p,len:%d\n",
           addr,
           len);
    print_hex_dump(KERN_DEBUG, pre_str, DUMP_PREFIX_ADDRESS, 16, 1,
                   addr, len, true);
    printk(KERN_DEBUG"\n");
#endif
#endif
    oal_io_print("%s\n", pre_str);
    int i;
    for (i = 0; i < len; i++) {
        oal_io_print("netbuf[%d]=%02x\n", i, addr[i]);
    }
    oal_io_print("---end---\n");
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_util.h */
