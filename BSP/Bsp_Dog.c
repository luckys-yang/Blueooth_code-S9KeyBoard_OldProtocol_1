/***************************************************************************
 * File: Bsp_Dog.c
 * Author: Yang
 * Date: 2024-02-17 14:23:05
 * description: 
 -----------------------------------
独立看门狗：

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_Dog_Init(void);
static void Bsp_Dog_FeedDog(void);

/* Public variables==========================================================*/
Bsp_Dog_st Bsp_Dog = 
{
    .Bsp_Dog_Init = &Bsp_Dog_Init,
    .Bsp_Dog_FeedDog = &Bsp_Dog_FeedDog
};

/**
* @param    None
* @retval   None
* @brief    看门狗初始化
**/
static void Bsp_Dog_Init(void)
{
    HAL_IWDG_Init(IWDG_LOAD_VALUE); // 初始化
}

/**
* @param    None
* @retval   None
* @brief    喂狗
**/
static void Bsp_Dog_FeedDog(void)
{
    HAL_IWDG_Refresh();
}