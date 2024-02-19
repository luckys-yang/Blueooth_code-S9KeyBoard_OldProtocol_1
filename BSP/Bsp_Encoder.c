/***************************************************************************
 * File: Bsp_Encoder.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description: 
 -----------------------------------
编码器：
    两个编码器IO口都是 TIM_ENCODERMODE_TI1 模式
    跟焦: 调虚实
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static int16_t Bsp_Encoder_GetEncoderCount(void);
static void Bsp_Encoder_Postive_Hanlder(void);
static void Bsp_Encoder_Negative_Hanlder(void);

/* Public function prototypes=========================================================*/

static void Bsp_Encoder_Init(void);
static void Bsp_Encoder_Control(void);
/* Public variables==========================================================*/
TIM_HandleTypeDef TIM_EncoderHandle;    // 定时器句柄

Bsp_Encoder_st Bsp_Encoder = 
{
    .encoder_count = 0,

    .Bsp_Encoder_Init = &Bsp_Encoder_Init,
    .Bsp_Encoder_Control = &Bsp_Encoder_Control
};

/**
* @param    None
* @retval   None
* @brief    编码器初始化
**/
static void Bsp_Encoder_Init(void)
{
    pinmux_gptima1_ch1_init(GPIO_PIN_ENCODERA, false, PIN_RESET); // 复用功能 输入模式
    pinmux_gptima1_ch2_init(GPIO_PIN_ENCODERB, false, PIN_RESET);

    /* 定时器基本配置 */
    TIM_Encoder_InitTypeDef Encoder_ConfigStructure;

    TIM_EncoderHandle.Instance = LSGPTIMA;                                     // 寄存器基地址--选择高级定时器A
    TIM_EncoderHandle.Init.Prescaler = 0;                                      // 预分频系数--时基单元时钟预分频值为0
    TIM_EncoderHandle.Init.CounterMode = TIM_COUNTERMODE_UP;                   // 指定计数器模式--向上计数模式
    TIM_EncoderHandle.Init.Period = ENCODER_TIM_PERIOD - 1;                    // 自动重装载值
    TIM_EncoderHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             // 时钟分频因子--不分频
    TIM_EncoderHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // 失能自动重装载功能

    Encoder_ConfigStructure.EncoderMode = ENCODER_MODE; // 编码器模式--TI1

    Encoder_ConfigStructure.IC1Polarity = TIM_ICPOLARITY_RISING;     // 输入信号极性--上升沿
    Encoder_ConfigStructure.IC1Selection = TIM_ICSELECTION_DIRECTTI; // 输入信号选择
    Encoder_ConfigStructure.IC1Prescaler = TIM_ICPSC_DIV1;           // 输入捕获预分频器--每次在捕获输入上检测到边缘时执行捕获
    Encoder_ConfigStructure.IC1Filter = 0xF;                         // 输入捕获滤波器

    Encoder_ConfigStructure.IC2Polarity = TIM_ICPOLARITY_RISING;
    Encoder_ConfigStructure.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    Encoder_ConfigStructure.IC2Prescaler = TIM_ICPSC_DIV1;
    Encoder_ConfigStructure.IC2Filter = 0xF;

    HAL_TIM_Encoder_Init(&TIM_EncoderHandle, &Encoder_ConfigStructure); // 初始化TIM编码器接口并初始化关联句柄
    HAL_TIM_Encoder_Start(&TIM_EncoderHandle, TIM_CHANNEL_ALL);         // 启动TIM编码器接口
}

/**
 * @param    None
 * @retval   None
 * @brief    获取编码器值
 **/
static int16_t Bsp_Encoder_GetEncoderCount(void)
{
    int16_t ret = 0;

    ret = __HAL_TIM_GET_COUNTER(&TIM_EncoderHandle); // 获取编码值
    __HAL_TIM_SET_COUNTER(&TIM_EncoderHandle, 0);    // 将编码值置0

    if (ret < 250) // 定时器转载值为499，则0~249为正转，250~499为反转
    {
        return ret;
    }
    else
    {
        return ret - ENCODER_TIM_PERIOD;
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    编码器正转处理
 **/
static void Bsp_Encoder_Postive_Hanlder(void)
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = 0xFD;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_FocusSet, 2, data);
}

/**
 * @param    None
 * @retval   None
 * @brief    编码器反转处理
 **/
static void Bsp_Encoder_Negative_Hanlder(void)
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = 0x03;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_FocusSet, 2, data);
}

/**
* @param    None
* @retval   None
* @brief    正交编码器控制
**/
static void Bsp_Encoder_Control(void)
{
    if (FLAG_true == System_Status.sys_power_switch) // 若已经开机
    {
        Bsp_Encoder.encoder_count = Bsp_Encoder_GetEncoderCount();

        if (Bsp_Encoder.encoder_count > 0)
        {
            // 正转处理
            Bsp_Encoder_Postive_Hanlder();
        }
        else if (Bsp_Encoder.encoder_count < 0)
        {
            // 反转处理
            Bsp_Encoder_Negative_Hanlder();
        }
    }
}