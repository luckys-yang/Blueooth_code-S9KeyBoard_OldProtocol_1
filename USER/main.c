#include "AllHead.h" 

int main(void)
{
	System_Init.Hardware_Init();	// Ӳ����ʼ��
	
	ble_loop();	// ��ѭ������
}
