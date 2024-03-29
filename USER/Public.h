#ifndef __PUBLIC_H
#define __PUBLIC_H

typedef enum
{
    PIN_RESET = 0x00, // 清除引脚
    PIN_SET = 0x01,   // 设置引脚
} PinState_et;

typedef enum
{
    FLAG_false = 0,
    FLAG_true = 1
} FlagState_et;

typedef struct
{
    void (*Public_Delay_Ms)(uint16_t);  // ms延时
    void (*Public_BufferInit)(uint8_t *, uint16_t, uint8_t);    // 字符串/数组初始化为指定值
} Public_st;

extern Public_st Public;

#endif
