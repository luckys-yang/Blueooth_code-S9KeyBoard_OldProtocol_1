/***************************************************************************
 * File: Bsp_Led.c
 * Author: Yang
 * Date: 2024-02-04 19:42:42
 * description:
 -----------------------------------
LED的不同状态表示意思：
    1. 初始化完状态是全灭
    2. 开机--长按电源键：步骤间隔时间@START_UP_LED_TIME ms【1】:LED1亮 【2】: 接着LED2亮 【3】打开采样电源+打开电源+摇杆上推检测 接着LED3亮 【4】LED4打开 完成步骤后全灭LED1亮
    3. 蓝牙状态打开则LED4亮  DM模式下--LED3常亮 F模式下--LED1常亮 POV模式下--LED2常亮 GO模式下--LED1 LED2 LED3 常亮
    4. 进入电机校准模式下 LED1常亮 LED3 500ms慢闪
    5. 电机校准过程中 LED1常亮 LED3 100ms快闪 校准完成成功则LED1 LED3 常亮 校准完成失败则LED1常亮 LED3灭
    6. 充电时：0格电量(LED1慢闪) 1格电量(LED1常亮 LED2慢闪) 2格电量(LED1 LED2常亮 LED3慢闪) 3格电量(LED1 LED2 LED3常亮 LED4慢闪) 4格电量(LED1 LED2 LED3 LED4常亮)
    7. 电量检测时(持续时间@CHECK_ELECTRIC_TIME ms)： 低电量(LED1 500ms慢闪) 1格电量(LED1常亮) 2格电量(LED1 LED2常亮) 3格电量(LED1 LED2 LED3常亮) 4格电量(LED1 LED2 LED3 LED4常亮)
    8. 低电量模式时：dm_mode下LED3 间隔@LOW_POWER_PTZ_MODE_TIME ms MODE_F模式下LED1 间隔@LOW_POWER_PTZ_MODE_TIME ms MODE_POV模式下LED2 间隔@LOW_POWER_PTZ_MODE_TIME ms
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_Led_Init(void);
static void Bsp_Led_BlinkConrol_Handler(Bsp_LedNum_st led_numbe, Bssp_LedStatus_et led_status, uint16_t timeout);
static void Bsp_Led_All_Close(void);
static void Bsp_Led_Console_Mode_Display(void);
static void Bsp_Led_StatusUpdate_Handler(void);
static void Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler(void);
/* Public variables==========================================================*/
Bsp_Led_st Bsp_Led =
{
    .LedInfo =
    {
        {
            .led_status = no_blink,
            .led_flip_timeout = 0
        },
        {
            .led_status = no_blink,
            .led_flip_timeout = 0
        },
        {
            .led_status = no_blink,
            .led_flip_timeout = 0
        },
        {
            .led_status = no_blink,
            .led_flip_timeout = 0
        }
    },
    .Bsp_Led_Init = &Bsp_Led_Init,
    .Bsp_Led_BlinkConrol_Handler = &Bsp_Led_BlinkConrol_Handler,
    .Bsp_Led_All_Close = &Bsp_Led_All_Close,
    .Bsp_Led_Console_Mode_Display = &Bsp_Led_Console_Mode_Display,
    .Bsp_Led_StatusUpdate_Handler = &Bsp_Led_StatusUpdate_Handler,
    .Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler = &Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler
};

/**
 * @param    None
 * @retval   None
 * @brief    LED初始化
 **/
static void Bsp_Led_Init(void)
{
    io_cfg_output(GPIO_PIN_LED1); // 输出模式
    io_cfg_output(GPIO_PIN_LED2); // 输出模式
    io_cfg_output(GPIO_PIN_LED3); // 输出模式
    io_cfg_output(GPIO_PIN_LED4); // 输出模式
    _Led1_Conrol(PIN_RESET);      // 默认低电平
    _Led2_Conrol(PIN_RESET);      // 默认低电平
    _Led3_Conrol(PIN_RESET);      // 默认低电平
    _Led4_Conrol(PIN_RESET);      // 默认低电平
}

/**
 * @param    led_number - LED编号 @Bsp_LedNum_st
 * @param    led_status - LED状态(闪烁/不闪烁) @Bssp_LedStatus_et
 * @param    timeout - 间隔时间
 * @retval   None
 * @brief    LED闪烁控制处理函数
 **/
static void Bsp_Led_BlinkConrol_Handler(Bsp_LedNum_st led_numbe, Bssp_LedStatus_et led_status, uint16_t timeout)
{
    if (blink == led_status) // 若LED闪烁
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = blink;
            Bsp_Led.LedInfo[LED_1].led_flip_timeout = timeout;
            Bsp_SysTimerCount.led1_count = 0; // 将系统软件定时器LED1闪烁计数器置0
            break;
        }
        case LED_2:
        {
            Bsp_Led.LedInfo[LED_2].led_status = blink;
            Bsp_Led.LedInfo[LED_2].led_flip_timeout = timeout;
            Bsp_SysTimerCount.led2_count = 0; // 将系统软件定时器LED2闪烁计数器置0
            break;
        }
        case LED_3:
        {
            Bsp_Led.LedInfo[LED_3].led_status = blink;
            Bsp_Led.LedInfo[LED_3].led_flip_timeout = timeout;
            Bsp_SysTimerCount.led3_count = 0; // 将系统软件定时器LED3闪烁计数器置0
            break;
        }
        case LED_4:
        {
            Bsp_Led.LedInfo[LED_4].led_status = blink;
            Bsp_Led.LedInfo[LED_4].led_flip_timeout = timeout;
            Bsp_SysTimerCount.led4_count = 0; // 将系统软件定时器LED4闪烁计数器置0
            break;
        }
        default:
            break;
        }
    }
    else // 若不闪烁
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = no_blink; // 状态改变为：不闪烁
            _Led1_Conrol(PIN_RESET);        // 灭
            break;
        }
        case LED_2:
        {
            Bsp_Led.LedInfo[LED_2].led_status = no_blink; // 状态改变为：不闪烁
            _Led2_Conrol(PIN_RESET);        // 灭
            break;
        }
        case LED_3:
        {
            Bsp_Led.LedInfo[LED_3].led_status = no_blink; // 状态改变为：不闪烁
            _Led3_Conrol(PIN_RESET);        // 灭
            break;
        }
        case LED_4:
        {
            Bsp_Led.LedInfo[LED_4].led_status = no_blink; // 状态改变为：不闪烁
            _Led4_Conrol(PIN_RESET);        // 灭
            break;
        }
        default:
            break;
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    全灭
 **/
static void Bsp_Led_All_Close(void)
{
    Bsp_Led.LedInfo[LED_1].led_status = no_blink;
    Bsp_Led.LedInfo[LED_2].led_status = no_blink;
    Bsp_Led.LedInfo[LED_3].led_status = no_blink;
    Bsp_Led.LedInfo[LED_4].led_status = no_blink;
    _Led1_Conrol(PIN_RESET);
    _Led2_Conrol(PIN_RESET);
    _Led3_Conrol(PIN_RESET);
    _Led4_Conrol(PIN_RESET);
}

/**
 * @param    None
 * @retval   None
 * @brief    云台模式下LED刷新显示
 **/
static void Bsp_Led_Console_Mode_Display(void)
{
    if (System_Status.check_electric != FLAG_true) // 非电量查询模式下
    {
        if (FLAG_true == System_Status.dm_mode) // DM模式
        {
            Bsp_Led_All_Close(); // LED全灭
            _Led3_Conrol(PIN_SET);
        }
        else
        {
            switch (System_Status.console_mode) // 云台当前模式
            {
            case MODE_F:
            {
                Bsp_Led_All_Close(); // LED全灭
                _Led1_Conrol(PIN_SET);
                break;
            }
            case MODE_POV:
            {
                Bsp_Led_All_Close(); // LED全灭
                _Led2_Conrol(PIN_SET);
                break;
            }
            case MODE_GO:
            {
                Bsp_Led_All_Close(); // LED全灭
                _Led1_Conrol(PIN_SET);
                _Led2_Conrol(PIN_SET);
                _Led3_Conrol(PIN_SET);
                break;
            }
            default:
                break;
            }
        }
        if (FLAG_true == System_Status.bluetooth)
        {
            _Led4_Conrol(PIN_SET);
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    LED状态刷新处理
 **/
static void Bsp_Led_StatusUpdate_Handler(void)
{
    if (blink == Bsp_Led.LedInfo[LED_1].led_status) // 若LED状态为闪烁
    {
        if ((Bsp_SysTimerCount.led1_count * SYS_TIME) > Bsp_Led.LedInfo[LED_1].led_flip_timeout) // 到达闪烁间隔
        {
            io_toggle_pin(GPIO_PIN_LED1);  // 翻转LED
            Bsp_SysTimerCount.led1_count = 0; // 系统软件定时器LED闪烁计数器清0
        }
        else
        {
            Bsp_SysTimerCount.led1_count++; // 若没有到达闪烁时间，系统软件定时器LED闪烁计数器加1
        }
    }

    if (blink == Bsp_Led.LedInfo[LED_2].led_status)
    {
        if ((Bsp_SysTimerCount.led2_count * SYS_TIME) > Bsp_Led.LedInfo[LED_2].led_flip_timeout)
        {
            io_toggle_pin(GPIO_PIN_LED2);  // 翻转LED
            Bsp_SysTimerCount.led2_count = 0; // 系统软件定时器LED闪烁计数器清0
        }
        else
        {
            Bsp_SysTimerCount.led2_count++; // 若没有到达闪烁时间，系统软件定时器LED闪烁计数器加1
        }
    }

    if (blink == Bsp_Led.LedInfo[LED_3].led_status)
    {
        if ((Bsp_SysTimerCount.led3_count * SYS_TIME) > Bsp_Led.LedInfo[LED_3].led_flip_timeout)
        {
            io_toggle_pin(GPIO_PIN_LED3);  // 翻转LED
            Bsp_SysTimerCount.led3_count = 0; // 系统软件定时器LED闪烁计数器清0
        }
        else
        {
            Bsp_SysTimerCount.led3_count++; // 若没有到达闪烁时间，系统软件定时器LED闪烁计数器加1
        }
    }

    if (blink == Bsp_Led.LedInfo[LED_4].led_status)
    {
        if ((Bsp_SysTimerCount.led4_count * SYS_TIME) > Bsp_Led.LedInfo[LED_4].led_flip_timeout)
        {
            io_toggle_pin(GPIO_PIN_LED4);  // 翻转LED
            Bsp_SysTimerCount.led4_count = 0; // 系统软件定时器LED闪烁计数器清0
        }
        else
        {
            Bsp_SysTimerCount.led4_count++; // 若没有到达闪烁时间，系统软件定时器LED闪烁计数器加1
        }
    }
}

/**
* @param    None
* @retval   None
* @brief    进入电机校准的LED状态刷新处理
**/
static void Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler(void)
{
    Bsp_Led_All_Close();
    // 1 亮 3闪烁
    _Led1_Conrol(PIN_SET);
    _Led2_Conrol(PIN_RESET);
    _Led3_Conrol(PIN_SET);
    _Led4_Conrol(PIN_RESET);
    Bsp_Led_BlinkConrol_Handler(LED_3, blink, ENTTER_MOTOR_CALIBRATION_BLINK_TIME);
}