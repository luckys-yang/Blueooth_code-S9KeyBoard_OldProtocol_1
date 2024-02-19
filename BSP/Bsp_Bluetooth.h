#ifndef __BSP_BLUETOOTH_H
#define __BSP_BLUETOOTH_H

// 信息打印
#define LOG_I_Bsp_BlueTooth(...) LOG_I(__VA_ARGS__)

typedef struct
{
    uint8_t ble_connect_id;  // 设备连接ID 未连接时为0XFF
    void (*Bsp_BlueTooth_Init)(void);   // 蓝牙协议栈/服务初始化
} Bsp_BlueTooth_st;

extern Bsp_BlueTooth_st Bsp_BlueTooth;
#endif
