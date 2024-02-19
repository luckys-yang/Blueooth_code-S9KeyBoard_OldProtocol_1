#ifndef __BSP_POWER_H
#define __BSP_POWER_H

/* Public define==========================================================*/
// 信息打印
#define LOG_I_Bsp_Power(...) LOG_I(__VA_ARGS__)

#define _ReadPin_USB_POWER_INPUT() io_read_pin(GPIO_PIN_USB_POWER_INPUT) // 读取USB输入检测 电平状态
#define _SetPin_POWER_LOCK() io_set_pin(GPIO_PIN_POWER_LOCK)             // 电源开关使能
#define _ResetPin_POWER_LOCK() io_clr_pin(GPIO_PIN_POWER_LOCK)           // 电源开关失能
#define _ResetPin_BAT_STDBY() io_read_pin(GPIO_PIN_BAT_STDBY)            // 查询是否充电完成

/* 电池电量枚举*/
typedef enum
{
    battery_low,   // 0格电量
    battery_one,   // 1格电量
    battery_two,   // 2格电量
    battery_three, // 3格电量
    battery_four   // 4格电量
} Bsp_Power_BatteryElectric_et;

/* 关机步骤枚举*/
typedef enum
{
    shutdown_Step_done = 0, // 完成
    shutdown_Step_first,    // 第一步
    shutdown_Step_second,   // 第二步
    shutdown_Step_third,    // 第三步
    shutdown_Step_fourth    // 第四步
} Shutdown_StepType_et;

typedef struct
{
    void (*Bsp_Power_Init)(void);                        // 电源部分初始化
    void (*Bsp_Power_IOExti_Close_AdcSampling)(uint8_t); // 外部中断下检测关闭ADC采样
    uint8_t (*Bsp_Power_StartingUp_Handler)(void);       // 开机处理函数
    void (*Bsp_Power_BatteryUpdate_Handler)(void);       // 电池图标更新处理函数
    uint8_t (*Bsp_Power_shutdown_Handler)(void);         // 关机处理函数
    void (*Bsp_Power_Electric_Check_Handler)(void);      // 电量检测处理
} Bsp_Power_st;

extern Bsp_Power_st Bsp_Power;

#endif
