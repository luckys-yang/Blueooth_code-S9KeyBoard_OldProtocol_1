/***************************************************************************
 * File: Bsp_Power.c
 * Author: Yang
 * Date: 2024-02-07 15:47:59
 * description:
 -----------------------------------
电源部分：
    电源开关GPIO_PIN_POWER_LOCK -> 高电平导通
    ADC采集开关GPIO_PIN_ADC_POWER_EN -> 高电平导通
    USB输入检测GPIO_PIN_USB_POWER_INPUT -> 高电平则表示有USB插入 低电平则表示没有USB插入
    充电锂电池是否完成检测GPIO_PIN_BAT_STDBY -> 充电完成低电平 默认是内部上拉
充电：
    关机状态下插入USB充电则LED会进行闪烁
    开机状态下插入USB充电则不会进行LED闪烁，是正常样子
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_Power_Init(void);
static void Bsp_Power_IOExti_Close_AdcSampling(uint8_t pin);
static uint8_t Bsp_Power_StartingUp_Handler(void);
static uint8_t Bsp_Power_shutdown_Handler(void);
static void Bsp_Power_BatteryUpdate_Handler(void);
static void Bsp_Power_Electric_Check_Handler(void);
/* Public variables==========================================================*/
Bsp_Power_st Bsp_Power =
{
    .Bsp_Power_Init = &Bsp_Power_Init,
    .Bsp_Power_IOExti_Close_AdcSampling = Bsp_Power_IOExti_Close_AdcSampling,
    .Bsp_Power_StartingUp_Handler = &Bsp_Power_StartingUp_Handler,
    .Bsp_Power_BatteryUpdate_Handler = &Bsp_Power_BatteryUpdate_Handler,
    .Bsp_Power_shutdown_Handler = &Bsp_Power_shutdown_Handler,
    .Bsp_Power_Electric_Check_Handler = &Bsp_Power_Electric_Check_Handler
};

/**
 * @param    None
 * @retval   None
 * @brief    电源部分初始化
 **/
static void Bsp_Power_Init(void)
{
    io_cfg_output(GPIO_PIN_POWER_LOCK); // 电源开关 输出模式

    io_cfg_output(GPIO_PIN_ADC_POWER_EN); // ADC采集开关 输出模式

    io_cfg_input(GPIO_PIN_USB_POWER_INPUT);                  // USB插入检测
    io_exti_config(GPIO_PIN_USB_POWER_INPUT, INT_EDGE_BOTH); // 上升沿下降沿都产生中断

    io_cfg_input(GPIO_PIN_BAT_STDBY);              // 锂电池充电是否完成检测引脚 输入模式
    io_pull_write(GPIO_PIN_BAT_STDBY, IO_PULL_UP); // 内部上拉
}

/**
 * @param    pin -> 引脚
 * @retval   None
 * @brief    外部中断下检测关闭ADC采样
 **/
static void Bsp_Power_IOExti_Close_AdcSampling(uint8_t pin)
{
    if (GPIO_PIN_USB_POWER_INPUT == pin)
    {
        // 系统未开机(没有USB插入且没有开机)
        if ((PIN_RESET == _ReadPin_USB_POWER_INPUT()) && (FLAG_false == System_Status.sys_power_switch))
        {
            _ResetPin_ADC_POWER_EN(); // 电源ADC采样失能
            LOG_I_Bsp_Power("USB ON");
        }
    }
}

/**
 * @param    None
 * @retval   0x00 -> 成功 0x01 -> 失败 0x02 -> 关机下推摇杆
 * @brief    开机处理函数
 **/
static uint8_t Bsp_Power_StartingUp_Handler(void)
{
    if (PIN_SET == _ReadPin_KEY_POWER())
    {
        Bsp_Led.Bsp_Led_All_Close(); // LED全灭
        Public.Public_Delay_Ms(20);
        return 0x01;
    }
    _Led1_Conrol(PIN_SET);
    Public.Public_Delay_Ms(START_UP_LED_TIME);

    if (PIN_SET == _ReadPin_KEY_POWER())
    {
        Bsp_Led.Bsp_Led_All_Close(); // LED全灭
        Public.Public_Delay_Ms(20);
        return 0x01;
    }
    _Led2_Conrol(PIN_SET);
    Public.Public_Delay_Ms(START_UP_LED_TIME);

    if (PIN_SET == _ReadPin_KEY_POWER())
    {
        Bsp_Led.Bsp_Led_All_Close(); // LED全灭
        Public.Public_Delay_Ms(20);
        return 0x01;
    }
    _SetPin_ADC_POWER_EN(); // 打开采样电源
    _SetPin_POWER_LOCK();   // 打开电源
    // 开关机时摇杆按键检测
    if (!Bsp_Adc.Startup_Shutdown_Rocker_Check_Handler())
    {
        Bsp_Led.Bsp_Led_All_Close();
        return 0x02;
    }

    _Led3_Conrol(PIN_SET);
    Public.Public_Delay_Ms(START_UP_LED_TIME);

    System_Status.sys_power_switch = FLAG_true;                    // 系统电源打开置1
    Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_close; // 开机摇杆状态

    _Led4_Conrol(PIN_SET);
    Public.Public_Delay_Ms(START_UP_LED_TIME);

    Bsp_Led.Bsp_Led_All_Close(); // LED全灭
    _Led1_Conrol(PIN_SET);       // 打开LED1

    if (FLAG_true == System_Status.bluetooth) // 蓝牙状态连接
    {
        _Led4_Conrol(PIN_SET);
    }

    /*发送默认状态给云台*/
    uint8_t data[6];
    data[0] = 0x00; // 对于开关机功能码：开机0x00 关机0x01  对于运动控制功能码：0x00回中
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_StartupShutdown, 1, data);
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ExerciseContorl, 1, data);
    data[0] = NORMAL_MODE_DATA;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);

    if (FLAG_false == System_Status.bluetooth) // 蓝牙状态断开(不加则有电情况下开机无法连接蓝牙)
    {
        // 广播
        Bsp_BlueTooth.Bsp_BlueTooth_Start_adv();
    }
    return 0x00;
}

/**
 * @param    None
 * @retval   0x00 -> 成功
 * @brief    关机处理函数
 **/
static uint8_t Bsp_Power_shutdown_Handler(void)
{
    /*发送关机命令*/
    uint8_t data[1];
    data[0] = 0x01; // 0x01关机
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_StartupShutdown, 1, data);

    _set_bit_value(System_Status.sys_timer_signal, EVENT_Shutdown); // 关机事件
    System_Status.shutdown_signal = shutdown_Step_first;            // 关机步骤1

    Bsp_Led.Bsp_Led_All_Close(); // 全部关闭
    _Led4_Conrol(PIN_SET);

    Bsp_Adc.Startup_Shutdown_Rocker_Check_Handler(); // 摇杆检测(暂时无用)

    Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_start; // 开关机信号复位
    System_Status.sys_power_switch = FLAG_false;                   // 开机信号置0
    System_Status.motorcal_mode = FLAG_false;                      // 电机校准信号置0

    return 0x00;
}

/**
 * @param    None
 * @retval   None
 * @brief    电池图标更新处理函数
 **/
static void Bsp_Power_BatteryUpdate_Handler(void)
{
    // 已开机 或 有USB插入
    if ((FLAG_true == System_Status.sys_power_switch) || (PIN_SET == _ReadPin_USB_POWER_INPUT()))
    {
        // 两次ADC采样的时间间隔很短，故通过@adc_sampling_count变量来延长电池图标更新时间
        if ((Bsp_Adc.adc_sampling_count * ADC_TIME) >= BATTERY_UPDATE_TIME)
        {
            // 当电压值变化大于0.005V时候才进行电池信息更新
            if ((Bsp_Adc.power_voltage > Bsp_Adc.old_power_voltage ? Bsp_Adc.power_voltage - Bsp_Adc.old_power_voltage : Bsp_Adc.old_power_voltage - Bsp_Adc.power_voltage) > 10)
            {
                if (PIN_RESET == _ReadPin_USB_POWER_INPUT())    // 没有USB插入
                {
                    if (Bsp_Adc.power_voltage >= four_electric) // 4格电
                    {
                        System_Status.battery = battery_four;
                        Bsp_Adc.old_power_voltage = four_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= three_electric) // 3格电
                    {
                        System_Status.battery = battery_three;
                        Bsp_Adc.old_power_voltage = three_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= two_electric) // 两格电
                    {
                        System_Status.battery = battery_two;
                        Bsp_Adc.old_power_voltage = two_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= one_electric) // 1格电
                    {
                        System_Status.battery = battery_one;
                        Bsp_Adc.old_power_voltage = one_electric;
                    }
                    else if (Bsp_Adc.power_voltage > shutdown_electric - 50) // 自动关机
                    {
                        System_Status.battery = battery_low;
                        Bsp_Adc.old_power_voltage = low_electric;
                    }
                }
                else     // 有USB插入
                {
                    if ((Bsp_Adc.power_voltage >= four_electric) && (PIN_RESET == _ResetPin_BAT_STDBY())) // 大于4格电且充电完成
                    {
                        System_Status.battery = battery_four;
                        Bsp_Adc.old_power_voltage = four_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= (three_electric + electric_offset)) // 大于3格电
                    {
                        System_Status.battery = battery_three;
                        Bsp_Adc.old_power_voltage = three_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= (two_electric + electric_offset)) // 大于2格电
                    {
                        System_Status.battery = battery_two;
                        Bsp_Adc.old_power_voltage = two_electric;
                    }
                    else if (Bsp_Adc.power_voltage >= (one_electric + electric_offset)) // 大于1格电
                    {
                        System_Status.battery = battery_one;
                        Bsp_Adc.old_power_voltage = one_electric;
                    }
                    else if (Bsp_Adc.power_voltage > (shutdown_electric - 50 + electric_offset)) // 低电量
                    {
                        System_Status.battery = battery_low;
                        Bsp_Adc.old_power_voltage = low_electric;
                    }
                }

                if (System_Status.battery != battery_low)   // 不是0格电
                {
                    if (FLAG_true == System_Status.low_power_mode) // 如果处于低电量模式则退出低电量模式
                    {
                        Bsp_Led.Bsp_Led_All_Close(); // 关闭所有指示灯
                        Bsp_Led.Bsp_Led_Console_Mode_Display();
                    }
                    System_Status.low_power_mode = FLAG_false;
                }
                else
                {
                    System_Status.low_power_mode = FLAG_true;
                }
            }

            Bsp_Adc.adc_sampling_count = 0; // 采样次数计数清0

            // 若充电开启(有USB插入, 关机状态, 检查电量信号为关闭, 等待芯片掉电完成则EVENT_Shutdown为0)
            if ((PIN_SET == _ReadPin_USB_POWER_INPUT()) && (FLAG_false == System_Status.sys_power_switch)
                    && (FLAG_false == System_Status.check_electric)
                    && (_get_bit_value(System_Status.sys_timer_signal, EVENT_Shutdown) != 1))
            {
                switch (System_Status.battery)
                {
                case battery_low: // 0格电量(LED1慢闪)
                {
                    if (Bsp_Led.LedInfo[LED_1].led_status != blink)
                    {
                        Bsp_Led.Bsp_Led_All_Close();
                        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, BATTERY_CHARGE_UPDATE_TIME);
                    }
                    break;
                }
                case battery_one: // 1格电量(LED1常亮 LED2慢闪)
                {
                    if (Bsp_Led.LedInfo[LED_2].led_status != blink)
                    {
                        Bsp_Led.Bsp_Led_All_Close();
                        _Led1_Conrol(PIN_SET);
                        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_2, blink, BATTERY_CHARGE_UPDATE_TIME);
                    }
                    break;
                }
                case battery_two: // 2格电量(LED1 LED2常亮 LED3慢闪)
                {
                    if (Bsp_Led.LedInfo[LED_3].led_status != blink)
                    {
                        Bsp_Led.Bsp_Led_All_Close();
                        _Led1_Conrol(PIN_SET);
                        _Led2_Conrol(PIN_SET);
                        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_3, blink, BATTERY_CHARGE_UPDATE_TIME);
                    }
                    break;
                }
                case battery_three: // 3格电量(LED1 LED2 LED3常亮 LED4慢闪)
                {
                    if (Bsp_Led.LedInfo[LED_4].led_status != blink)
                    {
                        Bsp_Led.Bsp_Led_All_Close();
                        _Led1_Conrol(PIN_SET);
                        _Led2_Conrol(PIN_SET);
                        _Led3_Conrol(PIN_SET);
                        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_4, blink, BATTERY_CHARGE_UPDATE_TIME);
                    }
                    break;
                }
                case battery_four: // 4格电量(LED1 LED2 LED3 LED4常亮)
                {
                    Bsp_Led.Bsp_Led_All_Close();
                    _Led1_Conrol(PIN_SET);
                    _Led2_Conrol(PIN_SET);
                    _Led3_Conrol(PIN_SET);
                    _Led4_Conrol(PIN_SET);
                    break;
                }
                default:
                    break;
                }
            }
        }
        else
        {
            Bsp_Adc.adc_sampling_count++; // 采样次数计数++
        }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    电量检测处理
 **/
static void Bsp_Power_Electric_Check_Handler(void)
{
    Bsp_Led.Bsp_Led_All_Close();

    switch (System_Status.battery)
    {
    case battery_low: // 低电量
    {
        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, 500);
        break;
    }
    case battery_one:
    {
        _Led1_Conrol(PIN_SET);
        break;
    }
    case battery_two:
    {
        _Led1_Conrol(PIN_SET);
        _Led2_Conrol(PIN_SET);
        break;
    }
    case battery_three:
    {
        _Led1_Conrol(PIN_SET);
        _Led2_Conrol(PIN_SET);
        _Led3_Conrol(PIN_SET);
        break;
    }
    case battery_four:
    {
        _Led1_Conrol(PIN_SET);
        _Led2_Conrol(PIN_SET);
        _Led3_Conrol(PIN_SET);
        _Led4_Conrol(PIN_SET);
        break;
    }
    default:
        break;
    }
}
