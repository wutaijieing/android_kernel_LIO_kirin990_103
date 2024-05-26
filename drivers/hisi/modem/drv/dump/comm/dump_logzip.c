#include <product_config.h>
#include <osl_types.h>
#include <osl_list.h>
#include <osl_sem.h>
#include <osl_thread.h>
#include <osl_malloc.h>
#include <osl_irq.h>
#include <securec.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/atomic.h>
#include <linux/miscdevice.h>
#include <linux/suspend.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/err.h>
#include <linux/syscalls.h>
#include <bsp_slice.h>
#include "dump_area.h"
#include "dump_core.h"
#include "dump_debug.h"
#include "dump_config.h"
#include "dump_logzip.h"

#undef THIS_MODU
#define THIS_MODU mod_dump

s32 dump_trigger_compress(const char *logpath, int pathlen, struct dump_file_save_info_s *datainfo)
{
    return BSP_ERROR;
}

void dump_wait_compress_done(const char *log_path)
{
}

void dump_logzip_init(void)
{
}
