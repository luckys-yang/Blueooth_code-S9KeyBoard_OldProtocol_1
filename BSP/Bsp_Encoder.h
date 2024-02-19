#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#define ENCODER_TIM_PERIOD 500           // 自动重装载值
#define ENCODER_MODE TIM_ENCODERMODE_TI1 // 模式

typedef struct
{
    int16_t encoder_count; // 编码器编码值

    void (*Bsp_Encoder_Init)(void);               // 编码器初始化
    void (*Bsp_Encoder_Control)(void);               // 正交编码器控制
} Bsp_Encoder_st;

extern Bsp_Encoder_st Bsp_Encoder;

#endif