/***************************************************************************
 * File: System_Init.c
 * Author: Yang
 * Date: 2024-02-04 14:56:54
 * description: 
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private variables=========================================================*/
// 存储按键板编译时间数组(年 月 日 时 分 秒 秒)
static uint8_t device_compile_time[16] = "20240220195151";

/* Private function prototypes===============================================*/

static void System_Info_Init(void);

/* Public function prototypes=========================================================*/

static void Hardware_Init(void);

/* Public variables==========================================================*/
System_Status_st System_Status = 
{
    .update_mode = FLAG_false,										
	.motorcal_mode = FLAG_false,									
	.key_test_mode = FLAG_false,										
	.console_mode = FLAG_false,									
	.console_old_mode = FLAG_false,							
	.wireless_charing = FLAG_false,							
	.bluetooth = FLAG_false,									
	.battery = FLAG_false,										
	.sys_power_switch = FLAG_false,										
	.ble_loop_switch = FLAG_false,					
	.start_motorcal_signal = FLAG_false,				
	.horizontal_vertical_signal = FLAG_false,			
	.sys_timer_signal = FLAG_false,						
	.console_mode_data = FLAG_false,						
	.check_electric = FLAG_false,									
	.low_power_mode = FLAG_false,									
	.shutdown_signal = FLAG_false,								
	.key_dm_mode = FLAG_false,										
	.dm_mode = FLAG_false,												
	.lowpower_shutdown = FLAG_false,							
	.PTZ_update_mode = FLAG_false,
	.remote_key_type = FLAG_false,
	.remote_press_type = FLAG_false
};

System_KeyBoardInfo_st System_KeyBoardInfo = 
{
    .device_compile_time_ptr = device_compile_time
};

System_Init_st System_Init = 
{
    .Hardware_Init = &Hardware_Init
};

/**
 * @param    None
 * @retval   None
 * @brief    硬件初始化
 **/
static void Hardware_Init(void)
{
    sys_init_app();                 // 【协议栈FUNC】系统初始化
    SEGGER_RTT_Init();              // 初始化RTT控制块(用于J-Link调试打印，生产时去掉即可)
    ble_init();                     // BLE 初始化
    Bsp_Led.Bsp_Led_Init();         // LED初始化
    Bsp_Power.Bsp_Power_Init();     // 电源部分初始化
    Bsp_Key.Bsp_Key_Init();         // 按键初始化
    Bsp_Adc.Bsp_Adc_Init();         // ADC初始化
    Bsp_Encoder.Bsp_Encoder_Init(); // 正交编码器初始化
    Bsp_Uart.Bsp_Uart_Init();       // 串口初始化
    System_Info_Init();             // 系统信息初始化
	
    Bsp_BlueTooth.Bsp_BlueTooth_Init();       // 蓝牙相关初始化
    Bsp_Power.Bsp_Power_StartingUp_Handler(); // 开机处理函数(用于没插着USB线时响应开机屏蔽则不会响应)
	Bsp_Dog.Bsp_Dog_Init();                  // 看门狗初始化

    System_Status.ble_loop_switch = FLAG_true; // 标志位置1
}

/**
* @param    None
* @retval   None
* @brief    系统信息初始化
**/
static void System_Info_Init(void)
{
    Bsp_Boot.Bsp_Boot_SysInfoGet(&SysInfo); // 获取系统信息

    Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count = (uint8_t)SysInfo->ble_name_count; // 获取名称计数

    if (Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count >= BLE_DEVICE_NAME_MAX_LEN) // 限制范围
    {
        Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count = 0;
    }

    if (Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count != 0)
    {
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_name_ptr[sizeof(BLE_NAME) - 1] = '(';  // 把原有的字符串的\0覆盖
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_name_ptr[sizeof(BLE_NAME)] = '0';
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_name_ptr[sizeof(BLE_NAME) + 1] = ')';
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_name_ptr[sizeof(BLE_NAME)] += Bsp_BlueTooth.ble_adv_info_Instance->ble_name_count; // 名称计数值
        // 数组最后一位是保存\0的...
    }

}