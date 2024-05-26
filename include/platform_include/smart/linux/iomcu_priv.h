/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub internal interface
 * Create: 2021-03-09
 */

#ifdef __cplusplus
extern "C" {
#endif

extern struct notifier_block g_iomcu_boot_nb;
extern rproc_id_t ipc_iom_to_ap_mbx;
extern rproc_id_t ipc_ap_to_iom_mbx;
extern rproc_id_t ipc_ap_to_lpm_mbx;
extern atomic_t iom3_rec_state;

extern uint32_t need_reset_io_power;
extern uint32_t need_set_3v_io_power;
extern uint32_t need_set_3_1v_io_power;
extern uint32_t need_set_3_2v_io_power;

#ifdef __cplusplus
}
#endif