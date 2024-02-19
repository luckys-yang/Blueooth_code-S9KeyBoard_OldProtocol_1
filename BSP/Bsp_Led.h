#ifndef __BSP_LED_H
#define __BSP_LED_H

/* Public define==========================================================*/
/*LED控制亮灭 亮：PIN_SET 灭：PIN_RESET*/
#define _Led1_Conrol(x) io_write_pin(GPIO_PIN_LED1, x)
#define _Led2_Conrol(x) io_write_pin(GPIO_PIN_LED2, x)
#define _Led3_Conrol(x) io_write_pin(GPIO_PIN_LED3, x)
#define _Led4_Conrol(x) io_write_pin(GPIO_PIN_LED4, x)

/*LED编号枚举*/
typedef enum
{
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_4,
    LED_MAX = LED_4 + 1 // 最大led数(用作后面定义数组大小)
} Bsp_LedNum_st;

/*LED状态枚举*/
typedef enum
{
    no_blink = 0x00, // 不闪烁
    blink = 0x01,    // 闪烁
} Bssp_LedStatus_et;

/*LED信息结构体*/
typedef struct
{
    uint8_t led_status; // led状态
    uint16_t led_flip_timeout;    // led翻转时间间隔
} LedInfo_st;

/*LED结构体*/
typedef struct
{
    LedInfo_st LedInfo[LED_MAX];    // 定义led信息结构体变量

    void (*Bsp_Led_Init)(void);                                                   // LED初始化
    void (*Bsp_Led_BlinkConrol_Handler)(Bsp_LedNum_st, Bssp_LedStatus_et, uint16_t); // LED闪烁控制处理函数
    void (*Bsp_Led_All_Close)(void);                                              // LED全灭
    void (*Bsp_Led_Console_Mode_Display)(void);                                   // 云台模式下LED刷新显示
    void (*Bsp_Led_StatusUpdate_Handler)(void);                                   // LED状态刷新处理
    void (*Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler)(void);  // 进入电机校准的LED状态刷新处理
} Bsp_Led_st;

extern Bsp_Led_st Bsp_Led;

#endif
