#ifndef __BSP_KEY_H
#define __BSP_KEY_H

/* Public define==========================================================*/
// 信息打印(不需要调试时把宏后面去掉即可需要则把右边的部分加到下面) LOG_I(__VA_ARGS__)
#define LOG_I_Bsp_Key(...)

/*读取按键电平状态*/
#define _ReadPin_KEY_POWER() io_read_pin(GPIO_PIN_KEY_POWER)         // 读取电源键电平状态
#define _ReadPin_KEY_PHOTO() io_read_pin(GPIO_PIN_KEY_PHOTO)         // 读取拍照键电平状态
#define _ReadPin_KEY_HOME() io_read_pin(GPIO_PIN_KEY_HOME)           // 读取home键电平状态
#define _ReadPin_KEY_STIR_UP() io_read_pin(GPIO_PIN_KEY_STIR_UP)     // 读取拨动开关键电平状态
#define _ReadPin_KEY_STIR_DOWN() io_read_pin(GPIO_PIN_KEY_STIR_DOWN) // 读取拨动开关键电平状态

/*按键信息*/
typedef struct
{
    uint8_t start_signal;      // 开始检测信号
    uint8_t press_count;       // 按下个数
    uint16_t total_press_count; // 按下总数
    uint16_t long_press_time;   // 长按时间
    uint16_t check_time;        // 检测时间
} KeyInfo_st;

/*按键结构体*/
typedef struct
{
    void (*Bsp_Key_Init)(void);                           // 按键初始化
    void (*Bsp_Key_IOExti_Handler)(uint8_t, exti_edge_t); // 按键外部中断处理函数
    void (*Bsp_key_Timer_Init)(void);                     // 按键软件定时器初始化

} Bsp_Key_st;

extern Bsp_Key_st Bsp_Key;

#endif