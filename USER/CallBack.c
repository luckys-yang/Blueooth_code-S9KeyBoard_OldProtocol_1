/***************************************************************************
 * File: xxx.c
 * Author: Yang
 * Date: 2024-02-07 10:14:46
 * description: 
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/**
 * @param    pin -> 发送中断的引脚
 * @param    edge -> 发送中断的沿
 * @retval   None
 * @brief    IO 中断回调函数
 **/
void io_exti_callback(uint8_t pin,exti_edge_t edge)
{
    Bsp_Key.Bsp_Key_IOExti_Handler(pin, edge);  // 按键外部中断处理函数
    Bsp_Power.Bsp_Power_IOExti_Close_AdcSampling(pin);  // 外部中断下检测关闭ADC采样
}

/**
 * @param    hadc -> ADC句柄
 * @retval   None
 * @brief    adc采样完成回调函数
 **/
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    Bsp_Adc.Bsp_Adc_Capture_Handler(hadc);  // ADC采集处理
}

/**
 * @param    huart -> 串口句柄
 * @retval   None
 * @brief    串口发送完成回调函数
 **/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    Bsp_Uart.Bsp_Uart_SendFinish_Handler(huart);    // 串口发送完成处理
}

/**
 * @param    huart -> 串口句柄
 * @retval   None
 * @brief    串口接收回调函数
 **/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    Bsp_Uart.Bsp_Uart_RecData_Handler(huart);   // 串口接收处理
}