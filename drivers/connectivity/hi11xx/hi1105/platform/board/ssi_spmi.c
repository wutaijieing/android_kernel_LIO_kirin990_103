
#include "oal_types.h"
#include "ssi_spmi.h"
#include "ssi_common.h"

enum spmi_cmd_opc {
    SPMI_CMD_REG_ZERO_WRITE = 0,
    SPMI_CMD_REG_WRITE = 1,
    SPMI_CMD_REG_READ = 2,
    SPMI_CMD_EXT_REG_WRITE = 3,
    SPMI_CMD_EXT_REG_READ = 4,
    SPMI_CMD_EXT_REG_WRITE_L = 5,
    SPMI_CMD_EXT_REG_READ_L = 6,
    SPMI_CMD_RESET = 7,
    SPMI_CMD_SLEEP = 8,
    SPMI_CMD_SHUTDOWN = 9,
    SPMI_CMD_WAKEUP = 10,
    SPMI_CMD_OPC_MAX
};

/* SPMI cmd register */
#define SPMI_APB_SPMI_CMD_EN             (1U << 31)
#define SPMI_APB_SPMI_CMD_TYPE_OFFSET    24
#define SPMI_APB_SPMI_CMD_LENGTH_OFFSET  20
#define SPMI_APB_SPMI_CMD_SLAVEID_OFFSET 16
#define SPMI_APB_SPMI_CMD_ADDR_OFFSET    0

/* spmi_base_addr默认值，不同项目可以通过调用reinit_spmi_base_addr进行配置 */
OAL_STATIC uint32_t g_spmi_base_addr = 0x40014000;

#define SPMI_CHANNEL_OFFSET              0x0300
#define SPMI_SLAVE_OFFSET                0x20
#define SPMI_BYTE_CNT                    0x1

#define WRITE_READ_FIRST_BYTE_OFFSET     24

#define spmi_cmd_addr(channel_id)    (g_spmi_base_addr + 0x100 + SPMI_CHANNEL_OFFSET * (channel_id))
#define spmi_wdata0_addr(channel_id) (g_spmi_base_addr + 0x104 + SPMI_CHANNEL_OFFSET * (channel_id))
#define spmi_wdata1_addr(channel_id) (g_spmi_base_addr + 0x108 + SPMI_CHANNEL_OFFSET * (channel_id))
#define spmi_wdata2_addr(channel_id) (g_spmi_base_addr + 0x10C + SPMI_CHANNEL_OFFSET * (channel_id))
#define spmi_wdata3_addr(channel_id) (g_spmi_base_addr + 0x110 + SPMI_CHANNEL_OFFSET * (channel_id))

#define spmi_status_addr(channel_id, slave_id) \
    (g_spmi_base_addr + 0x200 + SPMI_CHANNEL_OFFSET * (channel_id) + SPMI_SLAVE_OFFSET * (slave_id))
#define spmi_rdata0_addr(channel_id, slave_id) \
    (g_spmi_base_addr + 0x204 + SPMI_CHANNEL_OFFSET * (channel_id) + SPMI_SLAVE_OFFSET * (slave_id))
#define spmi_rdata1_addr(channel_id, slave_id) \
    (g_spmi_base_addr + 0x208 + SPMI_CHANNEL_OFFSET * (channel_id) + SPMI_SLAVE_OFFSET * (slave_id))
#define spmi_rdata2_addr(channel_id, slave_id) \
    (g_spmi_base_addr + 0x20C + SPMI_CHANNEL_OFFSET * (channel_id) + SPMI_SLAVE_OFFSET * (slave_id))
#define spmi_rdata3_addr(channel_id, slave_id) \
    (g_spmi_base_addr + 0x210 + SPMI_CHANNEL_OFFSET * (channel_id) + SPMI_SLAVE_OFFSET * (slave_id))

/* 调用该函数设置spmi_base_addr */
void reinit_spmi_base_addr(uint32_t spmi_base_addr)
{
    g_spmi_base_addr = spmi_base_addr;
}

OAL_STATIC OAL_INLINE uint32_t ssi_spmi_cmd(uint32_t opc, uint32_t len, uint32_t slave_id, uint32_t addr)
{
    return (SPMI_APB_SPMI_CMD_EN | \
            ((uint32_t)(opc) << SPMI_APB_SPMI_CMD_TYPE_OFFSET) |  \
            (((len) - 1) << SPMI_APB_SPMI_CMD_LENGTH_OFFSET) | \
            (((slave_id) & 0xF) << SPMI_APB_SPMI_CMD_SLAVEID_OFFSET) | \
            (((addr) & 0xFFFF) << SPMI_APB_SPMI_CMD_ADDR_OFFSET));
}

/* 组合spmi_cmd，增加入参判断 */
OAL_STATIC int32_t ssi_spmi_cmd_prepare(uint32_t opc, uint32_t len, uint32_t slave_id,
                                        uint32_t addr, uint32_t *spmi_cmd)
{
    if (spmi_cmd == NULL) {
        ps_print_err("spmi_cmd ptr is NULL\n");
        return -OAL_EFAIL;
    }

    if ((opc >= SPMI_CMD_OPC_MAX) || (slave_id >= SPMI_SLAVEID_MAX)) {
        ps_print_err("spmi cmd opc error\n");
        return -OAL_EFAIL;
    }

    *spmi_cmd = ssi_spmi_cmd(opc, len, slave_id, addr);

    return OAL_SUCC;
}

OAL_STATIC int32_t ssi_spmi_wait_done(uint32_t channel, uint32_t slave_id)
{
    uint32_t timeout = 1000; // when trying more than 1000 times, it is considered timed out
    while (timeout--) {
        if (ssi_read_value32_test(spmi_status_addr(channel, slave_id)) == 1) {
            return OAL_SUCC;
        }
    }

    ps_print_err("host spmi trans fail!\n");
    return -OAL_EFAIL;
}

int32_t ssi_spmi_read(uint32_t addr, uint32_t channel, uint32_t slave_id, uint32_t *value)
{
    int32_t ret;
    uint32_t cmd;
    uint32_t data;

    if (value == NULL) {
        ps_print_err("The input parameter is a null pointer!\n");
        return -OAL_EFAIL;
    }

    /* combination command */
    ret = ssi_spmi_cmd_prepare(SPMI_CMD_EXT_REG_READ_L, SPMI_BYTE_CNT, slave_id, addr, &cmd);
    if (ret != OAL_SUCC) {
        ps_print_err("Host spmi combination command failed!, ret = %d\n", ret);
        return ret;
    }

    /* send read commands to slave through ssi */
    ssi_write_value32_test(spmi_cmd_addr(channel), cmd);
    if (ssi_spmi_wait_done(channel, slave_id) != OAL_SUCC) {
        ps_print_err("Ssi_spmi_read fail\n");
        return -OAL_EFAIL;
    }

    data = ssi_read_value32_test(spmi_rdata0_addr(channel, slave_id));
    *value = (data >> WRITE_READ_FIRST_BYTE_OFFSET);

    return OAL_SUCC;
}

int32_t ssi_spmi_write(uint32_t addr, uint32_t channel, uint32_t slave_id, uint32_t data)
{
    int32_t ret;
    uint32_t cmd;

    /* prepare the data to be written */
    ssi_write_value32_test(spmi_wdata0_addr(channel), data << WRITE_READ_FIRST_BYTE_OFFSET);
    if (ssi_spmi_wait_done(channel, slave_id) != OAL_SUCC) {
        return -OAL_EFAIL;
    }

    /* combination command */
    ret = ssi_spmi_cmd_prepare(SPMI_CMD_EXT_REG_WRITE_L, SPMI_BYTE_CNT, slave_id, addr, &cmd);
    if (ret != OAL_SUCC) {
        ps_print_err("Host spmi combination command failed!, ret = %d\n", ret);
        return ret;
    }

    /* send write commands to slave through ssi */
    ssi_write_value32_test(spmi_cmd_addr(channel), cmd);
    if (ssi_spmi_wait_done(channel, slave_id) != OAL_SUCC) {
        ps_print_err("Ssi_spmi_write fail\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
