/***************************************************************************
 * File: main.c
 * Author: Yang
 * Date: 2024-02-25 11:49:23
 * description: 
 -----------------------------------
工程采用面向对象思想 模块化编程
 -----------------------------------
****************************************************************************/
#include "AllHead.h" 

int main(void)
{
	System_Init.Hardware_Init();	// 硬件初始化
	ble_loop();	// 主循环调度
}
