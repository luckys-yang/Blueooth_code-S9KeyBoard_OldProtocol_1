#ifndef __BSP_ADC_H
#define __BSP_ADC_H

/* Public define==========================================================*/
// 信息打印 LOG_I(__VA_ARGS__)
#define LOG_I_Bsp_Adc(...)

#define _SetPin_ADC_POWER_EN() io_set_pin(GPIO_PIN_ADC_POWER_EN);   // ADC采样IO使能
#define _ResetPin_ADC_POWER_EN() io_clr_pin(GPIO_PIN_ADC_POWER_EN); // ADC采样IO失能

/*摇杆数据采集状态*/
typedef enum
{
    rocker_adc_finish = 0xEF, // 完成
    rocker_adc_close = 0xFF,  // 关闭
    rocker_adc_start = 0x00,  // 开始
    rocker_up = (1 << 3),     // 向上
    rocker_down = (1 << 4),   // 向下
    rocker_left = (1 << 5),   // 向左
    rocker_right = (1 << 6)   // 向右
} Bsp_Adc_RockerStatus_et;

typedef struct
{
    uint16_t adc_sampling_count;                // ADC采样次数
    uint16_t rocker_voltage;                    // 转换的摇杆电压值
    uint16_t power_voltage;                     // 转换的电池电压值
    uint16_t old_power_voltage;                 // 上一次显示的电池电压值
    uint8_t rocker_status;                      // 摇杆状态
    uint8_t Startup_Shutdown_rocker_adc_signal; // 开关机摇杆采样ADC信号(要初始化为0)

    void (*Bsp_Adc_Init)(void);                             // ADC初始化
    void (*Bsp_Adc_Capture_Handler)(ADC_HandleTypeDef *);   // ADC采集处理
    uint8_t (*Startup_Shutdown_Rocker_Check_Handler)(void); // 开关机时摇杆按键检测
    void (*Shutdown_Rocker_Check_Handler)(void);            // 关机时摇杆下推处理函数
} Bsp_Adc_st;

extern ADC_HandleTypeDef hadc;
extern Bsp_Adc_st Bsp_Adc;

#endif
