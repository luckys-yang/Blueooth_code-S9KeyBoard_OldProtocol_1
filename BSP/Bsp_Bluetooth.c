/***************************************************************************
 * File: Bsp_Bluetooth.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description: 
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_BlueTooth_Init(void);

/* Private function prototypes===============================================*/

static void dev_manager_callback(enum dev_evt_type type, union dev_evt_u *evt);

/* Public variables==========================================================*/
Bsp_BlueTooth_st Bsp_BlueTooth = 
{
    .ble_connect_id = 0xFF,

    .Bsp_BlueTooth_Init = &Bsp_BlueTooth_Init
};

/**
* @param    None
* @retval   None
* @brief    蓝牙协议栈/服务初始化
**/
static void Bsp_BlueTooth_Init(void)
{
    dev_manager_init(dev_manager_callback); // 初始化设备管理器
}

/**
* @param    type -> 设备管理器中的事件类型
* @param    *evt -> 设备事件联合体
* @retval   None
* @brief    处理所有dev_manager消息的回调函数
**/
/*
【事件类型】
STACK_INIT --- 栈初始化事件
STACK_READY --- 栈就绪事件
PROFILE_ADDED --- 添加配置文件事件
SERVICE_ADDED --- 添加服务事件
ADV_OBJ_CREATED --- 创建广播对象事件
SCAN_OBJ_CREATED --- 创建扫描对象事件
INIT_OBJ_CREATED --- 创建初始化对象事件
ADV_STARTED --- 广播开始事件
ADV_STOPPED --- 广播停止事件
ADV_UPDATED --- 广播数据更新事件
SCAN_STARTED --- 扫描开始事件
SCAN_STOPPED --- 扫描停止事件
INIT_STARTED --- 初始化开始事件
INIT_STOPPED --- 初始化停止事件
OBJ_DELETED --- 对象删除事件
ADV_REPORT --- 接收广播报告事件
SCAN_REQ_IND --- 表示接收到扫描请求事件
*/
static void dev_manager_callback(enum dev_evt_type type, union dev_evt_u *evt)
{
    LOG_I_Bsp_BlueTooth("device evt:%d", type);

    switch (type)
    {
    case STACK_INIT:    // 栈初始化事件
    {
        struct ble_stack_cfg cfg =
        {
            .private_addr = false,
            .controller_privacy = false,
        };
        dev_manager_stack_init(&cfg);
        break;
    }
    case STACK_READY:   // 栈就绪事件
    {
        Bsp_SysTimer.Bsp_SysTimer_Init();  // 系统软件定时器初始化
        Bsp_Key.Bsp_key_Timer_Init();   // 按键软件定时器初始化
        bas_batt_lvl_update(0, 100);    // 更新某种基于电池级别的信息
        if(FLAG_true == System_Status.motorcal_mode)    // 进入电机校准模式下
        {
            // LED状态
            Bsp_Led.Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler();
        }
        break;
    }
    case SERVICE_ADDED: // 添加服务事件
    {
        break;
    }
    case PROFILE_ADDED: // 添加配置文件事件
    {
        break;
    }
    case ADV_OBJ_CREATED:   // 创建广播对象事件
    {
        break;
    }
    case ADV_STOPPED:   // 广播停止事件
    {
        break;
    }
    case SCAN_STOPPED:  // 扫描停止事件
    {
        break;
    }
    default:
        break;
    }
}