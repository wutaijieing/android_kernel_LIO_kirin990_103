/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 */

/**
 *  @brief   pmom mdrv interface for ps
 *  @file    mdrv_sync.h
 *  @author  lixiaojie
 *  @version v1.0
 *  @date    2019.12.19
 *  @note    
 *  <ul><li>v1.0|2012.11.29|create file</li></ul>
 *  <ul><li>v2.0|2019.12.17|modify for interface automation</li></ul>
 *  @since   
 */

#ifndef __MDRV_ACORE_SYNC_H__
#define __MDRV_ACORE_SYNC_H__
#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief BSP_ERR_SYNC_BASE 
*/
#define BSP_ERR_SYNC_BASE                (BSP_S32)(0x80000000 | (BSP_DEF_ERR(BSP_MODU_SYNC, 0)))
#define BSP_ERR_SYNC_TIMEOUT             (BSP_ERR_SYNC_BASE + 0x0)
/**
* @brief tagdrv_sync_module_e for all
*/
typedef enum tagdrv_sync_module_e
{
    SYNC_MODULE_MEM     = 0,
    SYNC_MODULE_NANDC,
    SYNC_MODULE_PMU,
    SYNC_MODULE_SOCP,
    SYNC_MODULE_DMA,
    SYNC_MODULE_IPF,
    SYNC_MODULE_NV,
    SYNC_MODULE_YAFFS,
    SYNC_MODULE_MSP,
    SYNC_MODULE_GPIO,
    SYNC_MODULE_CSHELL,
    SYNC_MODULE_MNTN,
	SYNC_MODULE_MSPNV,
	SYNC_MODULE_ONLINE,
	SYNC_MODULE_CHG,
	SYNC_MODULE_TFUPDATE,
	SYNC_MODULE_RESET,
	SYNC_MODULE_TEST,
    SYNC_MODULE_BUTT,
}drv_sync_module_e;



/**
 * @brief mdrv_sync_lock interface
 *
 * @par 描述:
 * 锁定并查询模块的初始化状态
 *
 * @attention
 * <ul><li>NA</li></ul>
 * @param[in]  u32Module, drv_sync_module_e u32Module 模块ID
 * @param[out] *pState         模块的初始化状态
 * @param[in] u32TimeOut      超时值，0为永远等待
 * @retval 0,表示函数执行成功。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_sync_lock(drv_sync_module_e u32Module, unsigned int *pState, unsigned int u32TimeOut);

/**
 * @brief mdrv_sync_unlock interface
 * @par 描述:
 *  解锁并更新模块初始化状态
 * @attention
 * <ul><li>NA</li></ul>
 * @param[in]  u32Module, drv_sync_module_e u32Module 模块ID
 * @param[in]  u32State        模块的初始化状态
 * @retval 0,表示函数执行成功。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_sync_unlock(drv_sync_module_e u32Module, unsigned int u32State);

/**
 * @brief mdrv_sync_wait interface
 * @par 描述:
 *  从核等待主核操作完成函数
 * @attention
 * <ul><li>NA</li></ul>
 * @param[in]  u32Module, drv_sync_module_e u32Module 模块ID
 * @param[in]  u32TimeOut       超时值，0为永远等待
 * @retval 0,表示函数执行成功。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_sync_wait(drv_sync_module_e u32Module, unsigned int u32TimeOut);

/**
 * @brief mdrv_sync_give interface
 * @par 描述:
 * 功能描述  : 主核通知从核操作已经完成
 * @attention
 * <ul><li>NA</li></ul>
 * @param[in]  u32Module, drv_sync_module_e u32Module 模块ID
 * @retval 0,表示函数执行成功。
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
int mdrv_sync_give(drv_sync_module_e u32Module);
#ifdef __cplusplus
}
#endif
#endif
