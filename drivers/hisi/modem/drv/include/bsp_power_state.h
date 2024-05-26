#ifndef __BSP_POWER_STATE__
#define  __BSP_POWER_STATE__

#include <linux/version.h>

enum modem_state_node
{
    modem_state_e,
    modem_reset0_e,
    modem_reset1_e,
    modem_reset2_e,
};

enum modem_state_index {
    MODEM_NOT_READY = 0,
    MODEM_READY,
    MODEM_INVALID,
};

static const char* const modem_state_str[] = {
    "MODEM_STATE_OFF\n",
    "MODEM_STATE_READY\n",
    "MODEM_STATE_INVALID\n",
};

int bsp_set_modem_state(enum modem_state_node node, unsigned int state);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
typedef int (*state_cb)(const char* buf, int len);
#else
typedef int (*state_cb)(char* buf, int len);
#endif

int bsp_modem_state_cb_register(state_cb);

#endif
