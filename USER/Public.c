/***************************************************************************
 * File: xxx.c
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

/* Public variables==========================================================*/
Public_st Public = 
{
    .Public_Delay_Ms = &Public_Delay_Ms
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