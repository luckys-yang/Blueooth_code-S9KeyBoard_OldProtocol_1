#include "AllHead.h" 

int main(void)
{
	System_Init.Hardware_Init();	// 硬件初始化
	
	ble_loop();	// 主循环调度
}
