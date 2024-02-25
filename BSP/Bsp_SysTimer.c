/***************************************************************************
 * File: Bsp_SysTimer.c
 * Author: Yang
 * Date: 2024-02-04 20:44:21
 * description:
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_SysTimer_Init(void);
static void Bsp_SysTimer_CallBack(void *arg);

/* Private function prototypes===============================================*/

static void vSysTimerEvent_ShutdownStep_Handler(void);
static void vSysTimerEvent_CheckElectric_Handler(void);
static void vSysTimerEvent_AdcEnabledCheck_Handler(void);
static void vSysTimerEvent_SerialPort_SendData_Handler(void);
static void vSysTimerEvent_DM_Done_Handler(void);
static void vSysTimerEvent_SendCheckPtzStatusPackage_Handler(void);
static void vSysTimerEvent_LowPowerShutdown_Handler(void);
static void vSysTimerEvent_LowPowerMode_Handler(void);
static void vSysTimerEvent_SoftwareReset_Handler(void);
/* Public variables==========================================================*/
struct builtin_timer *sys_timer_inst = NULL;    // 系统软件定时器句柄
// 模块超时时间结构体
Bsp_SysTimerCount_st Bsp_SysTimerCount =
{
    .check_adc_count = 0,
    .check_electric_count = 0,
    .check_low_power_count = 0,
    .check_PTZ_count = 0,
    .dm_finish_count = 0,
    .led1_count = 0,
    .led2_count = 0,
    .led3_count = 0,
    .led4_count = 0,
    .lowpower_shutdown_count = 0,
    .remote_contorl_count = 0,
    .reset_count = 0,
    .shut_down_count = 0
};

Bsp_SysTimer_st Bsp_SysTimer =
{
    .Bsp_SysTimer_Init = &Bsp_SysTimer_Init
};

/**
 * @param    None
 * @retval   None
 * @brief    系统软件定时器初始化
 **/
static void Bsp_SysTimer_Init(void)
{
    sys_timer_inst = builtin_timer_create(Bsp_SysTimer_CallBack); // 创建软件定时器 参数-回调函数指针
    builtin_timer_start(sys_timer_inst, SYS_TIME, NULL);          // 启动定时器 参数1-指向定时器的指针 参数2-定时器的超时时间 参数3-传递给定时器回调函数的参数
}

/**
 * @param    None
 * @retval   None
 * @brief    系统软件定时器回调函数
 **/
static void Bsp_SysTimer_CallBack(void *arg)
{
    vSysTimerEvent_SoftwareReset_Handler();       // 定时器事件--软件复位处理
    vSysTimerEvent_ShutdownStep_Handler();        // 定时器事件--关机步骤事件处理
    vSysTimerEvent_CheckElectric_Handler();       // 定时器事件--电量检测事件处理
    vSysTimerEvent_DM_Done_Handler();             // 定时器事件--DM模式完成事件处理
    vSysTimerEvent_SerialPort_SendData_Handler(); // 定时器事件--检测串口发送缓冲区是否有数据有则发送事件处理

    vSysTimerEvent_AdcEnabledCheck_Handler();           // 定时器事件--ADC是否需要打开检测事件处理
    vSysTimerEvent_SendCheckPtzStatusPackage_Handler(); // 定时器事件--发送查询云台状态包事件处理
    vSysTimerEvent_LowPowerMode_Handler();              // 定时器事件--低电量模式处理
    vSysTimerEvent_LowPowerShutdown_Handler();          // 定时器事件--低电量关机信号处理

    Bsp_Led.Bsp_Led_StatusUpdate_Handler();              // LED状态刷新
    Bsp_Dog.Bsp_Dog_FeedDog();                           // 喂狗
    builtin_timer_start(sys_timer_inst, SYS_TIME, NULL); // 重新启动定时器
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--关机步骤事件处理
 **/
static void vSysTimerEvent_ShutdownStep_Handler(void)
{
    if (1 == _get_bit_value(System_Status.sys_timer_signal, EVENT_Shutdown))
    {
        if ((Bsp_SysTimerCount.shut_down_count * SYS_TIME) >= 500)
        {
            Bsp_SysTimerCount.shut_down_count = 0;

            switch (System_Status.shutdown_signal)
            {
            case shutdown_Step_first:
            {
                System_Status.shutdown_signal = shutdown_Step_second;
                if (rocker_down == Bsp_Adc.Startup_Shutdown_rocker_adc_signal) // 关机时摇杆下推
                {
                    Bsp_Adc.Shutdown_Rocker_Check_Handler();
                }
                _Led3_Conrol(PIN_SET);
                break;
            }
            case shutdown_Step_second:
            {
                System_Status.shutdown_signal = shutdown_Step_third;
                _Led2_Conrol(PIN_SET);
                break;
            }
            case shutdown_Step_third:
            {
                System_Status.shutdown_signal = shutdown_Step_fourth;
                _Led1_Conrol(PIN_SET);
                break;
            }
            case shutdown_Step_fourth:
            {
                System_Status.shutdown_signal = shutdown_Step_done;
                _ResetPin_POWER_LOCK();      // 电源开关失能
                Bsp_Led.Bsp_Led_All_Close(); // LED全灭

                if (PIN_RESET == _ReadPin_USB_POWER_INPUT()) // 若此时UBS没插
                {
                    _ResetPin_ADC_POWER_EN(); // ADC采集失能
                }

                Public.Public_Delay_Ms(100); 									  // 等待芯片掉电
                _clear_bit_value(System_Status.sys_timer_signal, EVENT_Shutdown); // 清除关机事件位
                break;
            }
            default:
                break;
            }
        }
        else
        {
            Bsp_SysTimerCount.shut_down_count++;
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--电量检测事件处理
 **/
static void vSysTimerEvent_CheckElectric_Handler(void)
{
    // 计时时间到前显示的是电量，计时完成后恢复到云台模式LED显示状态
    if (1 == _get_bit_value(System_Status.sys_timer_signal, EVENT_CheckElectric))
    {
        if ((Bsp_SysTimerCount.check_electric_count * SYS_TIME) >= CHECK_ELECTRIC_TIME)
        {
            System_Status.check_electric = FLAG_false;
            Bsp_SysTimerCount.check_electric_count = 0;

            if (FLAG_true == System_Status.sys_power_switch) // 若已经开机
            {
                // 恢复到云台模式LED显示
                Bsp_Led.Bsp_Led_Console_Mode_Display();
            }
            _clear_bit_value(System_Status.sys_timer_signal, EVENT_CheckElectric); // 清除检测电量事件位
        }
        else
        {
            Bsp_SysTimerCount.check_electric_count++;
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--ADC是否需要打开检测事件处理
 **/
static void vSysTimerEvent_AdcEnabledCheck_Handler(void)
{
    // 50ms检测一次且非云台更新模式下
    if ((Bsp_SysTimerCount.check_adc_count * SYS_TIME >= ADC_TIME) && (System_Status.PTZ_update_mode != FLAG_true))
    {
        Bsp_SysTimerCount.check_adc_count = 0; // 计数清0
        // 开机 或者 有USB插入
        if ((FLAG_true == System_Status.sys_power_switch) || (PIN_SET == _ReadPin_USB_POWER_INPUT()))
        {
            HAL_ADCEx_InjectedStart_IT(&hadc);
        }
    }
    else
    {
        Bsp_SysTimerCount.check_adc_count++;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--检测串口发送缓冲区是否有数据有则发送事件处理
 **/
static void vSysTimerEvent_SerialPort_SendData_Handler(void)
{
    if ((Bsp_Uart.UartInnfo[UART_1].uart_tx_index > 0) && (FLAG_true == Bsp_Uart.UartInnfo[UART_1].uart_tx_busy_Flag)) // 若串口发送缓存不为空，且串口空闲
    {
        Bsp_Uart.UartInnfo[UART_1].uart_tx_busy_Flag = FLAG_false;                                                   // 改变串口状态为发送忙
        HAL_UART_Transmit_IT(&UART1_Config, Bsp_Uart.UartInnfo[UART_1].uart_tx_buffer_ptr, Bsp_Uart.UartInnfo[UART_1].uart_tx_index); // 开启串口发送
        Bsp_Uart.UartInnfo[UART_1].uart_tx_index = 0;
    }

    if ((Bsp_Uart.UartInnfo[UART_2].uart_tx_index > 0) && (FLAG_true == Bsp_Uart.UartInnfo[UART_2].uart_tx_busy_Flag)) // 若串口发送缓存不为空，且串口空闲
    {
        Bsp_Uart.UartInnfo[UART_2].uart_tx_busy_Flag = FLAG_false;                                                   // 改变串口状态为发送忙
        HAL_UART_Transmit_IT(&UART2_Config, Bsp_Uart.UartInnfo[UART_2].uart_tx_buffer_ptr, Bsp_Uart.UartInnfo[UART_2].uart_tx_index); // 开启串口发送
        Bsp_Uart.UartInnfo[UART_2].uart_tx_index = 0;
    }

    if ((Bsp_Uart.UartInnfo[UART_3].uart_tx_index > 0) && (FLAG_true == Bsp_Uart.UartInnfo[UART_3].uart_tx_busy_Flag)) // 若串口发送缓存不为空，且串口空闲
    {
        Bsp_Uart.UartInnfo[UART_3].uart_tx_busy_Flag = FLAG_false;                                                   // 改变串口状态为发送忙
        HAL_UART_Transmit_IT(&UART3_Config, Bsp_Uart.UartInnfo[UART_3].uart_tx_buffer_ptr, Bsp_Uart.UartInnfo[UART_3].uart_tx_index); // 开启串口发送
        Bsp_Uart.UartInnfo[UART_3].uart_tx_index = 0;
    }

    if (Bsp_BlueTooth.ble_connect_id != BLE_DISCONNECTED_ID)   // 蓝牙发送通知
    {
        Bsp_BlueTooth.Bsp_BlueTooth_Send_Notification();
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--DM模式完成事件处理
 **/
static void vSysTimerEvent_DM_Done_Handler(void)
{
    if (1 == _get_bit_value(System_Status.sys_timer_signal, EVENT_DM_Done))
    {
        if ((Bsp_SysTimerCount.dm_finish_count * SYS_TIME) >= DM_FINISH_TIME)
        {
            Bsp_SysTimerCount.dm_finish_count = 0;
            // 处于 DM模式 + 按键开启DM模式
            if ((FLAG_true == System_Status.dm_mode) && (FLAG_true == System_Status.key_dm_mode))
            {
                uint8_t data[1];
                data[0] = 0x00; // 回中
                Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ExerciseContorl, 1, data);
                System_Status.key_dm_mode = FLAG_false;
                System_Status.dm_mode = FLAG_false;
                Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
            }
            _clear_bit_value(System_Status.sys_timer_signal, EVENT_DM_Done);
        }
        else
        {
            Bsp_SysTimerCount.dm_finish_count++;
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--发送查询云台状态包事件处理
 **/
static void vSysTimerEvent_SendCheckPtzStatusPackage_Handler(void)
{
    // 已开机 || 电机校准模式 && 云台不处于固件升级模式
    if (((Bsp_SysTimerCount.check_PTZ_count * SYS_TIME) >= CHECK_PTZ_STATUS_TIME) && ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode)) && (System_Status.PTZ_update_mode != FLAG_true))
    {
        Bsp_SysTimerCount.check_PTZ_count = 0;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_PtzStatusCheck, 0, 0);
    }
    else
    {
        Bsp_SysTimerCount.check_PTZ_count++;
    }
}

/**
* @param    None
* @retval   None
* @brief    定时器事件--低电量关机信号处理
**/
static void vSysTimerEvent_LowPowerShutdown_Handler(void)
{
    if (FLAG_true == System_Status.lowpower_shutdown)
    {
        if ((Bsp_SysTimerCount.lowpower_shutdown_count * SYS_TIME) >= LOW_POWER_SHUTDOWN_TIME)
        {
            Bsp_SysTimerCount.lowpower_shutdown_count = 0;
            Bsp_Power.Bsp_Power_shutdown_Handler(); // 关机处理
        }
        else
        {
            Bsp_SysTimerCount.lowpower_shutdown_count++;
        }
    }
}

/**
* @param    None
* @retval   None
* @brief    定时器事件--低电量模式处理
**/
static void vSysTimerEvent_LowPowerMode_Handler(void)
{
    // 开机模式下
    if ((Bsp_SysTimerCount.check_low_power_count * SYS_TIME) >= CHECK_LOW_POWER_TIME && (FLAG_true == System_Status.sys_power_switch))
    {
        Bsp_SysTimerCount.check_low_power_count = 0;
        // 低电量模式+非GO模式
        if ((FLAG_true == System_Status.low_power_mode) && (System_Status.console_mode != MODE_GO))
        {
            Bsp_Led.Bsp_Led_All_Close();
            if (FLAG_true == System_Status.bluetooth)   // 蓝牙模式下
            {
                _Led4_Conrol(PIN_SET);
            }
            if (FLAG_true == System_Status.dm_mode)
            {
                Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_3, blink, LOW_POWER_PTZ_MODE_TIME);
            }
            else if (MODE_F == System_Status.console_mode)
            {
                Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, LOW_POWER_PTZ_MODE_TIME);
            }
            else if (MODE_POV == System_Status.console_mode)
            {
                Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_2, blink, LOW_POWER_PTZ_MODE_TIME);
            }
        }
    }
    else
    {
        Bsp_SysTimerCount.check_low_power_count++;
    }
}

/**
* @param    None
* @retval   None
* @brief    定时器事件--软件复位处理
**/
static void vSysTimerEvent_SoftwareReset_Handler(void)
{
    if (1 == _get_bit_value(System_Status.sys_timer_signal, EVENT_SoftwareReset))
    {
        if (Bsp_SysTimerCount.reset_count * SYS_TIME >= SOFTWARE_RESET_TIME)
        {
            Bsp_SysTimerCount.reset_count = 0;
            Bsp_Boot.Bsp_Boot_InfoGet(&Current_BootInfo);    // 获取映像信息页
            ota_boot_addr_set(Current_BootInfo->Image_Base); // 设置当前映像地址
            platform_reset(0);                               // 芯片复位

            _clear_bit_value(System_Status.sys_timer_signal, EVENT_SoftwareReset);
        }
        else
        {
            Bsp_SysTimerCount.reset_count++;
        }
    }
}