#ifndef __BSP_SYSTIMER_H
#define __BSP_SYSTIMER_H

/*系统软件定时器特殊事件枚举*/
typedef enum
{
    EVENT_Reset = 0,     // 复位事件
    EVENT_Shutdown,      // 关机事件
    EVENT_CheckElectric, // 检测电量事件
    EVENT_DM_Done,       // DM模式完成事件(盗梦空间1模式)
} Bsp_Timer_Event_et;

/*系统软件定时器计数结构体*/
typedef struct
{
    uint16_t led1_count;              // LED1闪烁系统软件定时器计数器
    uint16_t led2_count;              // LED2闪烁系统软件定时器计数器
    uint16_t led3_count;              // LED3闪烁系统软件定时器计数器
    uint16_t led4_count;              // LED4闪烁系统软件定时器计数器
    uint16_t check_electric_count;    // 检查电量系统软件定时器计数器
    uint16_t dm_finish_count;         // DM完成延时系统软件定时器计数器
    uint16_t shut_down_count;         // 关机系统软件定时器计数器
    uint16_t check_adc_count;         // ADC系统软件定时器计数器
    uint16_t check_PTZ_count;         // 发送查询云台状态包系统软件定时器计数器
    uint16_t check_low_power_count;   // 低电量检查系统软件定时器计数器
    uint16_t lowpower_shutdown_count; // 低电量关机软件定时器计数器
    uint16_t reset_count;             // 复位软件定时器计数器
    uint16_t remote_contorl_count;    // 遥控器控制软件定时器计数器
} Bsp_SysTimerCount_st;

typedef struct
{
    void (*Bsp_SysTimer_Init)(void);
} Bsp_SysTimer_st;

extern Bsp_SysTimerCount_st Bsp_SysTimerCount;
extern Bsp_SysTimer_st Bsp_SysTimer;

#endif
