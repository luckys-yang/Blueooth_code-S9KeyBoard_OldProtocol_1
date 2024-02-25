/***************************************************************************
 * File: Bsp_Adc.c
 * Author: Yang
 * Date: 2024-02-04 15:16:31
 * description:
 -----------------------------------
adc一次采样同时采样电池电压值和摇杆值
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void vRocker_Up_Handler(void);
static void vRocker_Down_Handler(void);
static void vRocker_Left_Handler(void);
static void vRocker_Right_Handler(void);
static void vRocker_CenterSlide_Handler(void);
/* Public function prototypes=========================================================*/

static void Bsp_Adc_Init(void);
static void Bsp_Adc_Capture_Handler(ADC_HandleTypeDef *hadc);
static uint8_t Startup_Shutdown_Rocker_Check_Handler(void);
static void Shutdown_Rocker_Check_Handler(void);
/* Public variables==========================================================*/
// ADC句柄
ADC_HandleTypeDef hadc;

Bsp_Adc_st Bsp_Adc =
{
    .adc_sampling_count = BATTERY_UPDATE_TIME / ADC_TIME,
    .rocker_voltage = 0,
    .power_voltage = 0,
    .old_power_voltage = 0,
    .rocker_status = rocker_adc_close,
    .Startup_Shutdown_rocker_adc_signal = rocker_adc_start,

    .Bsp_Adc_Init = &Bsp_Adc_Init,
    .Bsp_Adc_Capture_Handler = &Bsp_Adc_Capture_Handler,
    .Startup_Shutdown_Rocker_Check_Handler = &Startup_Shutdown_Rocker_Check_Handler,
    .Shutdown_Rocker_Check_Handler = &Shutdown_Rocker_Check_Handler
};

/**
* @param    None
* @retval   None
* @brief    ADC初始化
**/
static void Bsp_Adc_Init(void)
{
    /*-------------------- GPIO配置 -----------------*/

    /* 摇杆 ADC IO口配置 */
    io_clr_pin(GPIO_PIN_ADC_ROCKER);
    io_cfg_output(GPIO_PIN_ADC_ROCKER);
    pinmux_adc12b_in5_init();   // IO口复用 通道5
    /* 电池 ADC IO配置 */
    io_clr_pin(GPIO_PIN_ADC_POWER);
    io_cfg_output(GPIO_PIN_ADC_POWER);
    pinmux_adc12b_in4_init();   // IO口复用	通道4

    /*-------------------- ADC配置 -----------------*/

    hadc.Instance = LSADC;								// ADC基地址
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;			// 数据右对齐
    hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;			// 使能ADC连续扫描
    hadc.Init.NbrOfConversion = 2;						// 采样通道2
    hadc.Init.DiscontinuousConvMode = DISABLE;			// 失能间断转换
    hadc.Init.NbrOfDiscConversion = 0;					// 间断转换值为0
    hadc.Init.ContinuousConvMode = DISABLE;				// 失能循环模式
    hadc.Init.TrigType = ADC_INJECTED_SOFTWARE_TRIGT;	// 软件触发
    hadc.Init.Vref = ADC_VREF_VCC;						// 系统VCC作为参考电压
    hadc.Init.AdcCkDiv = ADC_CLOCK_DIV32;				// 32分频

    HAL_ADC_Init(&hadc);    // 初始化ADC

    // 摇杆ADC 注入组采样配置
    ADC_InjectionConfTypeDef sConfigInjected = {0};
    sConfigInjected.InjectedChannel = ADC_CHANNEL_5;    // 采样通道5
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1; // 注入组顺序器1(缓冲寄存器1)
    sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_15CYCLES; // 设置的所选通道的采样时间值 - 采样周期15个CLK
    sConfigInjected.InjectedOffset = 0; // 要从原始转换数据中减去的偏移量（仅适用于注入组中设置的通道）
    sConfigInjected.InjectedNbrOfConversion = 2;    // 指定将在注入组顺序器内转换的排数
    sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;    // 指定是否在完整序列/不连续序列（主序列分成连续部分）中执行注入组的转换序列
    sConfigInjected.AutoInjectedConv = DISABLE; // 在常规转换后启用或禁用选定的ADC自动注入组转换
    HAL_ADCEx_InjectedConfigChannel(&hadc, &sConfigInjected);   // 配置ADC的注入组和所选通道

    // 电池ADC 注入组采样配置
    sConfigInjected.InjectedChannel = ADC_CHANNEL_4;		// 采样通道4
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_2;	// 注入组顺序器2(缓冲寄存器2)
    sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_15CYCLES;	// 设置的所选通道的采样时间值 - 采样周期15个CLK
    HAL_ADCEx_InjectedConfigChannel(&hadc, &sConfigInjected);   // 配置ADC的注入组和所选通道

    if(PIN_SET == _ReadPin_USB_POWER_INPUT()) // 如果有USB插入则使能ADC采集
    {
        _SetPin_ADC_POWER_EN();
    }
}

/**
* @param    None
* @retval   None
* @brief    ADC采集处理 1. 主要对采样的电池电压值和摇杆电压值进行转换 2. 并且对电池更新事件和摇杆按下事件进行判断
**/
static void Bsp_Adc_Capture_Handler(ADC_HandleTypeDef *hadc)
{
    uint16_t adc_value = 0; // ADC值

    adc_value = HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);  // 获取ADC缓冲寄存器1
    Bsp_Adc.rocker_voltage = (uint32_t)(1000 * 3.3 * adc_value / 4095.0);  // 摇杆ADC值转换

    adc_value = HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_2);  // 获取ADC缓冲寄存器2
    Bsp_Adc.power_voltage = (uint32_t)(1000 * 3.3 * adc_value / 4095.0);    // 电池ADC值转换

    LOG_I_Bsp_Adc("ADC: %d", Bsp_Adc.power_voltage);
    // 开机状态下（防止关机时在充电触发到关机函数） + 电量小于自动关机电量时
    if((FLAG_true == System_Status.sys_power_switch) && (Bsp_Adc.power_voltage < shutdown_electric))
    {
        if(FLAG_false == System_Status.lowpower_shutdown)
        {
            System_Status.lowpower_shutdown = FLAG_true;    // 低电量关机信号使能
            Bsp_SysTimerCount.lowpower_shutdown_count = 0;
        }
    }
    else
    {
        System_Status.lowpower_shutdown = FLAG_false;
    }

    if(Bsp_Adc.Startup_Shutdown_rocker_adc_signal != rocker_adc_close) // 若是开机时的ADC采样
    {
        if(Bsp_Adc.rocker_voltage < 500)    // 若摇杆上推
        {
            Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_up; // 对开机时的摇杆状态赋值
        }
        else if((Bsp_Adc.rocker_voltage < 1800) && (Bsp_Adc.rocker_voltage > 500))  // 若摇杆下推
        {
            Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_down;
        }
        else
        {
            Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_finish; // 开机时完成一次ADC采样
        }
    }

    if(FLAG_true == System_Status.sys_power_switch) // 若已经开机
    {
        if(Bsp_Adc.rocker_voltage < 500)    // 上推
        {
            vRocker_Up_Handler();   // 上推处理
            LOG_I_Bsp_Adc("Rocker Up");
        }
        else if((Bsp_Adc.rocker_voltage < 1800) && (Bsp_Adc.rocker_voltage > 500))  // 下推
        {
            vRocker_Down_Handler();   // 下推处理
            LOG_I_Bsp_Adc("Rocker Down");
        }
        else if((Bsp_Adc.rocker_voltage < 2300) && (Bsp_Adc.rocker_voltage > 1800)) // 左推
        {
            vRocker_Left_Handler(); // 左推处理
            LOG_I_Bsp_Adc("Rocker Left");
        }
        else if((Bsp_Adc.rocker_voltage < 2800) && (Bsp_Adc.rocker_voltage > 2300)) // 右推
        {
            vRocker_Right_Handler();    // 右推处理
            LOG_I_Bsp_Adc("Rocker Right");
        }
        else if(Bsp_Adc.rocker_status != rocker_adc_close)    // 回中
        {
            vRocker_CenterSlide_Handler();    // 回中处理
            LOG_I_Bsp_Adc("Rocker Center");
        }
    }
    // 电量更新处理
    Bsp_Power.Bsp_Power_BatteryUpdate_Handler();
}

/**
 * @param    None
 * @retval   None
 * @brief    上推处理
 **/
static void vRocker_Up_Handler(void)
{
    if ((System_Status.console_mode != MODE_GO) && (System_Status.dm_mode != FLAG_true))
    {
        /*模式数据*/
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;                // id
        data[1] = System_Status.console_mode_data; // mode
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x3F; // pitch
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        Bsp_Adc.rocker_status = rocker_up;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    下推处理
 **/
static void vRocker_Down_Handler(void)
{
    if ((System_Status.console_mode != MODE_GO) && (System_Status.dm_mode != FLAG_true))
    {
        /*模式数据*/
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;                // id
        data[1] = System_Status.console_mode_data; // mode
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0xC1; // pitch
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        Bsp_Adc.rocker_status = rocker_down;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    左推处理
 **/
static void vRocker_Left_Handler(void)
{
    if ((System_Status.console_mode != MODE_GO) && (System_Status.dm_mode != FLAG_true))
    {
        /*模式数据*/
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;                // id
        data[1] = System_Status.console_mode_data; // mode
        data[2] = 0x00;
        data[3] = 0xC1; // Yaw
        data[4] = 0x00;
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        Bsp_Adc.rocker_status = rocker_left;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    右推处理
 **/
static void vRocker_Right_Handler(void)
{
    if ((System_Status.console_mode != MODE_GO) && (System_Status.dm_mode != FLAG_true))
    {
        /*模式数据*/
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;                // id
        data[1] = System_Status.console_mode_data; // mode
        data[2] = 0x00;
        data[3] = 0x3F; // Yaw
        data[4] = 0x00;
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        Bsp_Adc.rocker_status = rocker_right;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    回中处理
 **/
static void vRocker_CenterSlide_Handler(void)
{
    if ((System_Status.console_mode != MODE_GO) && (System_Status.dm_mode != FLAG_true))
    {
        /*模式数据*/
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;                // id
        data[1] = System_Status.console_mode_data; // mode
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        Bsp_Adc.rocker_status = rocker_adc_close;
    }
}

/**
* @param    None
* @retval   上推：0x00 无上推：0x01
* @brief    开关机时摇杆按键检测
**/
static uint8_t Startup_Shutdown_Rocker_Check_Handler(void)
{
    if(PIN_RESET == _ReadPin_USB_POWER_INPUT())  // 没有USB插入才能执行下面
    {
        HAL_ADCEx_InjectedStart_IT(&hadc);  // 启动ADC采集
        while(rocker_adc_start == Bsp_Adc.Startup_Shutdown_rocker_adc_signal);  // 等待上推操作完成

        if(rocker_up == Bsp_Adc.Startup_Shutdown_rocker_adc_signal) // 上推
        {
            Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_close;
            System_Status.motorcal_mode = FLAG_true;    // 电机校准模式置位
            return 0x00;
        }
        else
        {
            Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_start;
        }
    }
    return 0x01;
}

/**
* @param    None
* @retval   None
* @brief    关机时摇杆下推处理函数
**/
static void Shutdown_Rocker_Check_Handler(void)
{
    Bsp_Adc.Startup_Shutdown_rocker_adc_signal = rocker_adc_close;

    Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count = *(uint8_t *)(SYS_INFO_ADDR);
    Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count++;

    if (Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count >= BLE_DEVICE_NAME_MAX_LEN)
    {
        Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count = 0;
    }

    hal_flash_page_erase(SYS_INFO_ADDR);
    hal_flash_quad_page_program(SYS_INFO_ADDR, &Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count, 1);
    Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr[5]++;
    dev_manager_set_mac_addr(Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr);   // 重新设置mac地址
}