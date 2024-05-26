

#ifndef __WIRELESS_PATCH_H__
#define __WIRELESS_PATCH_H__

#include "hw_bfg_ps.h"
#include "plat_debug.h"
#include "plat_firmware.h"

typedef struct uart_firmware_cmd_st {
    uint8_t *key;
    int32_t (*cmd_exec)(struct ps_core_s *ps_core_d, uint8_t *key, uint8_t *value);
} uart_firmware_cmd;

#define os_init_waitqueue_head(wq)                               init_waitqueue_head(wq)
#define os_wait_event_interruptible(wq, condition)               wait_event_interruptible(wq, condition)
#define os_wait_event_interruptible_timeout(wq, condition, time) wait_event_interruptible_timeout(wq, condition, time)
#define os_waitqueue_active(wq)                                  waitqueue_active(wq)
#define os_wake_up_interruptible(wq)                             wake_up_interruptible(wq)

#define SOH 0x01 /* 开始字符 */
#define EOT 0x04 /* 发送完成 */
#define ACK 0x06 /* 正确接收应答 */
#define NAK 0x15 /* 校验错误重新发送，通讯开始时用于接收方协商累加校验 */
#define CAN 0x18 /* 结束下载 */

#define PATCH_INTEROP_TIMEOUT HZ

#define CRC_TABLE_SIZE 256

#define DATA_BUF_LEN 128

#define CFG_PACH_LEN 64

#define UART_CFG_FILE_FPGA_HI1105  "/vendor/firmware/hi1105/fpga/uart_cfg"
#define UART_CFG_FILE_PILOT_HI1105 "/vendor/firmware/hi1105/pilot/uart_cfg"

#define UART_CFG_FILE_FPGA_SHENKUO  "/vendor/firmware/hi1106/fpga/uart_cfg"
#define UART_CFG_FILE_PILOT_SHENKUO "/vendor/firmware/hi1106/pilot/uart_cfg"

#define BUART_CFG_FILE_FPGA_BISHENG  "/vendor/firmware/bisheng/fpga/bfgx_uart_cfg"
#define BUART_CFG_FILE_PILOT_BISHENG "/vendor/firmware/bisheng/pilot/bfgx_uart_cfg"

#define GUART_CFG_FILE_FPGA_BISHENG  "/vendor/firmware/bisheng/fpga/gle_uart_cfg"
#define GUART_CFG_FILE_PILOT_BISHENG "/vendor/firmware/bisheng/pilot/gle_uart_cfg"


#define BRT_CMD_KEYWORD "BAUDRATE"

#define CMD_LEN  32
#define PARA_LEN 128


#define MSG_FORM_DRV_G 'G'
#define MSG_FORM_DRV_C 'C'
#define MSG_FORM_DRV_A 'a'
#define MSG_FORM_DRV_N 'n'

#define READ_PATCH_BUF_LEN        (1024 * 32)
#define READ_DATA_BUF_LEN         (1024 * 50)
#define READ_DATA_REALLOC_BUF_LEN (1024 * 4)

#define XMODE_DATA_LEN 1024

#define XMODEM_PAK_LEN (XMODE_DATA_LEN + 5)

/* Enum Type Definition */
enum PATCH_WAIT_RESPONSE_ENUM {
    NO_RESPONSE = 0, /* 不等待device响应 */
    WAIT_RESPONSE    /* 等待device响应 */
};

typedef wait_queue_head_t os_wait_queue_head;

typedef struct {
    uint8_t cfg_path[CFG_PACH_LEN];
    uint8_t cfg_version[VERSION_LEN];
    uint8_t dev_version[VERSION_LEN];

    uint8_t resv_buf1[RECV_BUF_LEN];
    int32_t resv_buf1_len;

    uint8_t resv_buf2[RECV_BUF_LEN];
    int32_t resv_buf2_len;

    int32_t count;
    struct cmd_type_st *cmd;

    os_wait_queue_head *wait;
} patch_globals;

/* xmodem每包数据的结构，CRC校验 */
typedef struct {
    char head;      /* 开始字符 */
    char packet_num; /* 包序号 */
    char packet_ant; /* 包序号补码 */
} xmodem_head_pkt;

typedef struct {
    uint8_t *pbufstart;
    uint8_t *pbufend;
    uint8_t *phead;
    uint8_t *ptail;
} ring_buf;

/* Global Variable Declaring */
extern ring_buf g_stringbuf[UART_BUTT];
extern uint8_t *g_data_buf[UART_BUTT];
extern patch_globals g_patch_ctrl[UART_BUTT];

/* Function Declare */
extern int bfg_patch_download_function(struct ps_core_s *ps_core_d);
extern int32_t patch_send_char(struct ps_core_s *ps_core_d, char num, int32_t wait);
extern int32_t bfg_patch_recv(struct ps_core_s *ps_core_d, const uint8_t *data, int32_t count);
extern int32_t ps_patch_write(struct ps_core_s *ps_core_d, uint8_t *data, int32_t count);
extern int32_t ps_recv_patch(void *disc_data, const uint8_t *data, int32_t count);

#endif /* end of oam_kernel.h */
