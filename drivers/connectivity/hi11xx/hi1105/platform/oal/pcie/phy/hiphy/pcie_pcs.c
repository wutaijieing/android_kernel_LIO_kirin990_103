

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_pcs.h"
#include "oal_util.h"

/* 拆分打印，printk 一次打印buff不能过长, LOG_LINE_MAX(from kernel printk.c < 1024) */
#define PCS_LOG_LINE_MAX 512
/* large buff, more than printk's log buff(4KB) */
void pcie_pcs_print_split_log_info(char *str)
{
    char c;
    int32_t pos;

    int32_t size = PCS_LOG_LINE_MAX;
    char *src = str;
    char dst[PCS_LOG_LINE_MAX];
    dst[0] = '\0';

    pos = 0;
    forever_loop() {
        c = *src;
        if (c == '\0') {
            *(dst + pos) = '\0';
            oal_io_print("%s", dst);
            break;
        }
        *(dst + pos) = c;
        src++;
        pos++;
        if ((pos >= (size - 1)) || (c == '\n')) {
            *(dst + pos) = '\0';
            pos = 0; // 打印下一行
            oal_io_print("%s", dst);
            continue;
        }
    }
}
#endif