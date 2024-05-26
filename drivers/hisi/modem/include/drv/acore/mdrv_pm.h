/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 */

/**
 *  @brief   pmom mdrv interface for ps
 *  @file    mdrv_pm.h
 *  @author  
 *  @version v1.0
 *  @date    2019.12.19
 *  @note    
 *  <ul><li>v1.0|2012.11.29|create file</li></ul>
 *  <ul><li>v2.0|2019.12.17|modify for interface automation</li></ul>
 *  @since   
 */

#ifndef __MDRV_ACORE_DPM_H__
#define __MDRV_ACORE_DPM_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "mdrv_public.h"
/*
 *@brief   dynamic wakesrc
 */
typedef enum pm_wake_src
{
    PM_WAKE_SRC_UART0 = 0, 
    PM_WAKE_SRC_SOCP ,    
    PM_WAKE_SRC_DRX_TIMER, 
    PM_WAKE_SRC_BUT,
}pm_wake_src_e;

/**
* @brief PWC_SLEEP_FUNCPTR 
*/
typedef void (*PWC_SLEEP_FUNCPTR)(void);
/*
 *  DFS profile
 */
typedef enum
{
    DFS_PROFILE_0 = 0,            
    DFS_PROFILE_1 = 1,    
    DFS_PROFILE_2 = 2,         
    DFS_PROFILE_3 = 3,           
    DFS_PROFILE_4 = 4,     
    DFS_PROFILE_BUTT
} PWC_DFS_PROFILE_E;

/*
 *  DFS Device QoS
 */
typedef enum tagdfs_qos_id_e
{
    DFS_QOS_ID_ACPU_MINFREQ_E = 0,  /**<(0)*/
    DFS_QOS_ID_ACPU_MAXFREQ_E,      /**<(1)*/
    DFS_QOS_ID_CCPU_MINFREQ_E,      /**<(2)*/
    DFS_QOS_ID_CCPU_MAXFREQ_E,      /**<(3)*/
    DFS_QOS_ID_DDR_MINFREQ_E,       /**<(4)*/
    DFS_QOS_ID_DDR_MAXFREQ_E,       /**<(5)*/
    DFS_QOS_ID_GPU_MINFREQ_E,       /**<(6)*/
    DFS_QOS_ID_GPU_MAXFREQ_E,       /**<(7)*/
    DFS_QOS_ID_BUS_MINFREQ_E,       /**<(8)*/
    DFS_QOS_ID_BUS_MAXFREQ_E        /**<(9)*/
}dfs_qos_id_e;

/**
* @brief PWRCTRLFUNCPTR 
*/
typedef unsigned int (*PWRCTRLFUNCPTR)(unsigned int arg);

/**
 * @brief 投票调频；并获取一个投票者的req_id号，此req_id号用于后续更新调频
 *
 * @par 描述:
 * 根据调频类型、投票值，投票频率
 *
 * @param[in]  qos_id，调频类型，dfs_qos_id_e类型值。
 * @param[in]  req_value，投票值，单位kHz。
 * @param[out]  req_id，输出投票者的req_id号，只需获取一次。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @attention
 * <ul><li>此接口只需在第一次投票时使用</li></ul>
 *
 * @par 依赖:
 * <ul><li>mdrv_pm.h：该接口声明所在的头文件。</li></ul>
 *
 * @see dfs_qos_id_e
 */
int mdrv_pm_dfs_qos_get(int qos_id, unsigned int req_value, int* req_id);

/**
 * @brief 释放投票
 *
 * @par 描述:
 * 释放对应调频类型的投票
 *
 * @param[in]  qos_id，调频类型，dfs_qos_id_e类型值。
 * @param[out]  req_id，投票者的req_id。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @attention
 * <ul><li>释放后req_id变为无效值，不能再次用于投票</li></ul>
 *
 * @par 依赖:
 * <ul><li>mdrv_pm.h：该接口声明所在的头文件。</li></ul>
 *
 * @see dfs_qos_id_e
 */
int mdrv_pm_dfs_qos_put(int qos_id, int* req_id);

/**
 * @brief 更新投票值
 *
 * @par 描述:
 * 更新对应调频类型的投票值
 *
 * @param[in]  qos_id，调频类型，dfs_qos_id_e类型值。
 * @param[in]  req_id，投票者的id号。
 * @param[in]  req_value，投票值，单位kHz。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @attention
 * <ul><li>根据业务需要投票；不做业务时把投票值改为0，以降低功耗</li></ul>
 *
 * @par 依赖:
 * <ul><li>mdrv_pm.h：该接口声明所在的头文件。</li></ul>
 *
 * @see dfs_qos_id_e
 */
int mdrv_pm_dfs_qos_update(int qos_id, int req_id, unsigned int req_value);

/**
 * @brief 建议使用投票接口；获取cpu profile
 *
 * @retval ,profile值，非频率值。
 *
 * @attention
 * <ul><li>返回值为profile，非频率值</li></ul>
 */
int mdrv_pm_get_dfs_profile(void);

/**
 * @brief 建议使用投票接口；设置cpu的profile
 *
 * @par 描述:
 * 设置的profile
 *
 * @param[in]  profile，非频率值。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @attention
 * <ul><li>profile，非频率值</li></ul>
 *
 */
int mdrv_pm_set_dfs_profile(int profile);

/**
 * @brief 建议使用投票接口；设置cpu最低profile
 *
 * @par 描述:
 * 最低profile
 *
 * @param[in]  profile，最低profile，非频率值。
 *
 * @retval 0,表示操作成功。
 * @retval -1,表示操作失败。
 *
 * @attention
 * <ul><li>profile，非频率值</li></ul>
 *
 */
int mdrv_pm_set_dfs_baseprofile(int profile);

/**
 * @brief唤醒源设置接口
 *
 * @par 描述:
 * 设置唤醒源
 *
 * @attention
 * <ul><li>na</li></ul>
 *
 * @param[in]  pm_wake_src_e wake_src需要设置的唤醒源
 * @retval 0
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_pm_set_wakesrc(pm_wake_src_e wake_src);

/**
 * @brief唤醒源取消接口
 *
 * @par 描述:
 * 取消设置唤醒源
 *
 * @attention
 * <ul><li>na</li></ul>
 *
 * @param[in]  pm_wake_src_e wake_src需要取消设置的唤醒源
 * @retval 0
 * @par 依赖:
 * <ul><li>NA</li></ul>
 *
 * @see
 */
void mdrv_pm_clear_wakesrc(pm_wake_src_e wake_src);

#ifdef __cplusplus
}
#endif
#endif
