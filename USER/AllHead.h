#ifndef __ALLHEAD_H
#define __ALLHEAD_H

/*蓝牙协议栈相关*/
// 平台头文件
#include "platform.h"
// JLinK调试需要
#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"
#include "log.h"
// 休眠头文件
#include "sleep.h"
#include "ls_ble.h"
// GPIO
#include "ls_soc_gpio.h"
// 软件定时器
#include "builtin_timer.h"
// 蓝牙服务
#include "prf_hid.h"
#include "prf_bass.h"
// ADC
#include "ls_hal_adc.h"
// 硬件定时器
#include "ls_hal_timer.h"
// 串口
#include "ls_hal_uart.h"
// dma
#include "ls_hal_dmac.h"
// 断言相关
#include "ls_dbg.h"
// Flash相关
#include "ls_hal_flash.h"
#include "cmsis_armcc.h"
// 独立看门狗
#include "ls_hal_iwdg.h" 
// HID
#include "prf_hid.h"

/*C库相关*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*硬件外设相关*/
#include "Bsp_SysTimer.h"
#include "Bsp_Led.h"
#include "Bsp_Adc.h"
#include "Bsp_Bluetooth.h"
#include "Bsp_Boot.h"
#include "Bsp_Encoder.h"
#include "Bsp_Key.h"
#include "Bsp_Remote.h"
#include "Bsp_Power.h"
#include "Bsp_Uart.h"
#include "Bsp_Dog.h"
#include "Bsp_Hid.h"
#include "Bsp_OldProtocol.h"
#include "Bsp_NewProtocol.h"

/*用户相关*/
#include "System_Init.h"
#include "Public.h"
#include "CallBack.h"
#endif
