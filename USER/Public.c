/***************************************************************************
 * File: Public.c
 * Author: Yang
 * Date: 2024-02-04 14:56:32
 * description: 
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Public_Delay_Ms(uint16_t nms);
static void Public_BufferInit(uint8_t *data, uint16_t len, uint8_t assign_value);

/* Public variables==========================================================*/
Public_st Public = 
{
    .Public_Delay_Ms = &Public_Delay_Ms,
    .Public_BufferInit = &Public_BufferInit
};


/**
 * @param    nms -> 需要延时的时间
 * @retval   None
 * @brief    ms延时
 **/
static void Public_Delay_Ms(uint16_t nms)
{
    // 调用us库函数
	DELAY_US(nms * 1000);
}

/**
* @param    data -> 数组地址
* @param    len -> 数组数据长度
* @param    assign_value -> 要初始化为什么值
* @retval   None
* @brief    字符串/数组初始化为指定值
**/
static void Public_BufferInit(uint8_t *data, uint16_t len, uint8_t assign_value)
{
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        *(data + i) = assign_value;
    }
}