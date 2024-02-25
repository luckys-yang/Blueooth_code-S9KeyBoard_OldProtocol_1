/***************************************************************************
 * File: Bsp_Uart.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
串口：
    uart1为云台串口，uart3为上位机串口，串口2为2.4g接收(遥控器+AI)
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public variables==========================================================*/

static void Bsp_Uart_Init(void);
static void Bsp_Uart_SerialPort1_SendData(uint8_t *data, uint16_t length);
static void Bsp_Uart_SerialPort2_SendData(uint8_t *data, uint16_t length);
static void Bsp_Uart_SerialPort3_SendData(uint8_t *data, uint16_t length);
static void Bsp_Uart_Ble_SendData(uint8_t *data, uint16_t length);
static void Bsp_Uart_SendFinish_Handler(UART_HandleTypeDef *huart);
static void Bsp_Uart_RecData_Handler(UART_HandleTypeDef *huart);
static void Bsp_Uart_RecData_AddPosition(uint16_t *count, uint16_t add_num);
static void Bsp_Uart_ParameterInit(void);
/* Public variables==========================================================*/
uint8_t uart1_rx_buffer[RX_BUFFER_SIZE]; // uart1接收缓存数组
uint8_t uart1_tx_buffer[TX_BUFFER_SIZE]; // uart1发送缓存数组
UART_HandleTypeDef UART1_Config;         // uart1句柄

uint8_t uart2_rx_buffer[RX_BUFFER_SIZE]; // uart2接收缓存数组
uint8_t uart2_tx_buffer[TX_BUFFER_SIZE]; // uart2发送缓存数组
UART_HandleTypeDef UART2_Config;         // uart2句柄

uint8_t uart3_rx_buffer[RX_BUFFER_SIZE]; // uart3接收缓存数组
uint8_t uart3_tx_buffer[TX_BUFFER_SIZE]; // uart3发送缓存数组
UART_HandleTypeDef UART3_Config;         // uart3句柄

uint8_t ble_rx_buffer[BLE_SVC_BUFFER_SIZE]; // ble接收缓存数组
uint8_t ble_tx_buffer[BLE_SVC_BUFFER_SIZE]; // ble发送缓存数组

Bsp_Uart_st Bsp_Uart =
{
    .UartInnfo =
    {
        {
            .uart_rx_buffer_ptr = uart1_rx_buffer,  // 只在使能串口中断时用
            .uart_tx_buffer_ptr = uart1_tx_buffer,  // 发送中断会用
            .uart_rx_index = 0,
            .uart_tx_index = 0,
            .uart_tx_busy_Flag = FLAG_true,
            .uart_rx_busy_Flag = FLAG_true
        },  // 串口1
        {
            .uart_rx_buffer_ptr = uart2_rx_buffer,
            .uart_tx_buffer_ptr = uart2_tx_buffer,
            .uart_rx_index = 0,
            .uart_tx_index = 0,
            .uart_tx_busy_Flag = FLAG_true,
            .uart_rx_busy_Flag = FLAG_true
        },  // 串口2
        {
            .uart_rx_buffer_ptr = uart3_rx_buffer,
            .uart_tx_buffer_ptr = uart3_tx_buffer,
            .uart_rx_index = 0,
            .uart_tx_index = 0,
            .uart_tx_busy_Flag = FLAG_true,
            .uart_rx_busy_Flag = FLAG_true
        },   // 串口3
        {
            .uart_rx_buffer_ptr = ble_rx_buffer,
            .uart_tx_buffer_ptr = ble_tx_buffer,
            .uart_rx_index = 0,
            .uart_tx_index = 0,
            .uart_tx_busy_Flag = FLAG_true,
            .uart_rx_busy_Flag = FLAG_true
        },     // 蓝牙串口
    },

    .Bsp_Uart_Init = &Bsp_Uart_Init,
    .Bsp_Uart_SerialPort1_SendData = &Bsp_Uart_SerialPort1_SendData,
    .Bsp_Uart_SerialPort2_SendData = &Bsp_Uart_SerialPort2_SendData,
    .Bsp_Uart_SerialPort3_SendData = &Bsp_Uart_SerialPort3_SendData,
    .Bsp_Uart_Ble_SendData = &Bsp_Uart_Ble_SendData,
    .Bsp_Uart_SendFinish_Handler = &Bsp_Uart_SendFinish_Handler,
    .Bsp_Uart_RecData_Handler = &Bsp_Uart_RecData_Handler,
    .Bsp_Uart_RecData_AddPosition = &Bsp_Uart_RecData_AddPosition,
    .Bsp_Uart_ParameterInit = &Bsp_Uart_ParameterInit
};
/*--------------------串口队列解析实例------------------------*/
// 串口1队列数据解析结构体变量
Uart_QueueParse_st Uart_QueueParse_Uart1 =
{
    .deal_queue = {0},
    .deal_queue_index = 0,
    .Rec_Buffer_ptr = uart1_rx_buffer
};
// 串口2队列数据解析结构体变量
Uart_QueueParse_st Uart_QueueParse_Uart2 =
{
    .deal_queue = {0},
    .deal_queue_index = 0,
    .Rec_Buffer_ptr = uart2_rx_buffer
};
// 串口3队列数据解析结构体变量
Uart_QueueParse_st Uart_QueueParse_Uart3 =
{
    .deal_queue = {0},
    .deal_queue_index = 0,
    .Rec_Buffer_ptr = uart3_rx_buffer
};
// 蓝牙队列数据解析结构体变量
Uart_QueueParse_st Uart_QueueParse_Ble =
{
    .deal_queue = {0},
    .deal_queue_index = 0,
    .Rec_Buffer_ptr = ble_rx_buffer
};

/**
* @param    None
* @retval   None
* @brief    串口参数初始化
**/
static void Bsp_Uart_ParameterInit(void)
{
    /*
    注意不能在这里进行数组或者别的耗时初始化否则会导致蓝牙断开！！！
    不初始化发送索引，否则会导致切换协议触发不会回复确认包
    */
    /*串口信息结构体初始化*/
    for (uint8_t i = UART_1; i < UART_MAX; i++)
    {
        /*另外两个数组指针不初始化了，暂时没问题*/
        Bsp_Uart.UartInnfo[i].uart_rx_index = 0;
        Bsp_Uart.UartInnfo[i].uart_tx_busy_Flag = FLAG_true;
        Bsp_Uart.UartInnfo[i].uart_rx_busy_Flag = FLAG_true;
    }
    /*串口队列数据解析结构体初始化*/
    // 不能初始化Uart_QueueParse_Uart1的否则也会卡死断开连接
}

/**
 * @param    None
 * @retval   None
 * @brief    串口初始化
 **/
static void Bsp_Uart_Init(void)
{
    /* UART1初始化 */
    pinmux_uart1_init(GPIO_PIN_UART1_TX, GPIO_PIN_UART1_RX); // 复用
    io_pull_write(GPIO_PIN_UART1_RX, IO_PULL_UP);            // 上拉
    UART1_Config.UARTX = UART1;                              // UART寄存器的基址
    UART1_Config.Init.BaudRate = UART1_BaudRate;             // 波特率
    UART1_Config.Init.MSBEN = 0;                             // 设置数据位的最高位优先为禁用
    UART1_Config.Init.Parity = UART_NOPARITY;                // 设置奇偶校验为无校验位
    UART1_Config.Init.StopBits = UART_STOPBITS1;             // 设置停止位为1个停止位
    UART1_Config.Init.WordLength = UART_BYTESIZE8;           // 设置数据位长度为8位
    HAL_UART_Init(&UART1_Config);                            // UART1 的初始化

    /* UART2初始化 */
    pinmux_uart2_init(GPIO_PIN_UART2_TX, GPIO_PIN_UART2_RX);
    io_pull_write(GPIO_PIN_UART2_RX, IO_PULL_UP);
    UART2_Config.UARTX = UART2;
    UART2_Config.Init.BaudRate = UART2_BaudRate;
    UART2_Config.Init.MSBEN = 0;
    UART2_Config.Init.Parity = UART_NOPARITY;
    UART2_Config.Init.StopBits = UART_STOPBITS1;
    UART2_Config.Init.WordLength = UART_BYTESIZE8;
    HAL_UART_Init(&UART2_Config);

    /* UART3初始化 */
    pinmux_uart3_init(GPIO_PIN_UART3_TX, GPIO_PIN_UART3_RX);
    io_pull_write(GPIO_PIN_UART3_RX, IO_PULL_UP);
    UART3_Config.UARTX = UART3;
    UART3_Config.Init.BaudRate = UART3_BaudRate;
    UART3_Config.Init.MSBEN = 0;
    UART3_Config.Init.Parity = UART_NOPARITY;
    UART3_Config.Init.StopBits = UART_STOPBITS1;
    UART3_Config.Init.WordLength = UART_BYTESIZE8;
    HAL_UART_Init(&UART3_Config);

    HAL_UART_Receive_IT(&UART1_Config, Bsp_Uart.UartInnfo[UART_1].uart_rx_buffer_ptr, single_rec_len); // 以非阻塞模式接收
    HAL_UART_Receive_IT(&UART2_Config, Bsp_Uart.UartInnfo[UART_2].uart_rx_buffer_ptr, single_rec_len);
    HAL_UART_Receive_IT(&UART3_Config, Bsp_Uart.UartInnfo[UART_3].uart_rx_buffer_ptr, single_rec_len);
}

/**
 * @param    data -> 需要发送数据的地址
 * @param    length -> 需要发送数据长度
 * @retval   None
 * @brief    串口1发送数据函数(主要将需要发送的函数压入串口1发送缓存，在串口软件定时器中会定时发送缓存内数据)
 **/
static void Bsp_Uart_SerialPort1_SendData(uint8_t *data, uint16_t length)
{
    LS_ASSERT((length + Bsp_Uart.UartInnfo[UART_1].uart_tx_index) <= TX_BUFFER_SIZE);            // 使用 LS_ASSERT 宏进行断言检查，确保发送的数据长度不超过 TX_BUFFER_SIZE
    memcpy(&uart1_tx_buffer[Bsp_Uart.UartInnfo[UART_1].uart_tx_index], (uint8_t *)data, length); // 将 data 指向的数据拷贝到 uart1_tx_buffer 中
    Bsp_Uart.UartInnfo[UART_1].uart_tx_index += length;                                          // 更新发送索引，指向下一个可用位置
}

/**
 * @param    data -> 需要发送数据的地址
 * @param    length -> 需要发送数据长度
 * @retval   None
 * @brief    串口2发送数据函数
 **/
static void Bsp_Uart_SerialPort2_SendData(uint8_t *data, uint16_t length)
{
    LS_ASSERT((length + Bsp_Uart.UartInnfo[UART_2].uart_tx_index) <= TX_BUFFER_SIZE);            // 使用 LS_ASSERT 宏进行断言检查，确保发送的数据长度不超过 TX_BUFFER_SIZE
    memcpy(&uart2_tx_buffer[Bsp_Uart.UartInnfo[UART_2].uart_tx_index], (uint8_t *)data, length); // 将 data 指向的数据拷贝到 uart2_tx_buffer 中
    Bsp_Uart.UartInnfo[UART_2].uart_tx_index += length;                                          // 更新发送索引，指向下一个可用位置
}

/**
 * @param    data -> 需要发送数据的地址
 * @param    length -> 需要发送数据长度
 * @retval   None
 * @brief    串口3发送数据函数
 **/
static void Bsp_Uart_SerialPort3_SendData(uint8_t *data, uint16_t length)
{
    LS_ASSERT((length + Bsp_Uart.UartInnfo[UART_3].uart_tx_index) <= TX_BUFFER_SIZE);            // 使用 LS_ASSERT 宏进行断言检查，确保发送的数据长度不超过 TX_BUFFER_SIZE
    memcpy(&uart3_tx_buffer[Bsp_Uart.UartInnfo[UART_3].uart_tx_index], (uint8_t *)data, length); // 将 data 指向的数据拷贝到 uart3_tx_buffer 中
    Bsp_Uart.UartInnfo[UART_3].uart_tx_index += length;                                          // 更新发送索引，指向下一个可用位置
}

/**
 * @param    data -> 需要发送数据的地址
 * @param    length -> 需要发送数据长度
 * @retval   None
 * @brief    蓝牙发送数据函数
 **/
static void Bsp_Uart_Ble_SendData(uint8_t *data, uint16_t length)
{
    LS_ASSERT((length + Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index) <= BLE_SVC_BUFFER_SIZE);            // 使用 LS_ASSERT 宏进行断言检查，确保发送的数据长度不超过 BLE_SVC_BUFFER_SIZE
    memcpy(&ble_tx_buffer[Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index], (uint8_t *)data, length); // 将 data 指向的数据拷贝到 ble_tx_buffer 中
    Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index += length;                                          // 更新发送索引，指向下一个可用位置
}

/**
 * @param    *huart -> 串口对象指针@UART_HandleTypeDef
 * @retval   None
 * @brief    串口发送完成处理函数
 **/
static void Bsp_Uart_SendFinish_Handler(UART_HandleTypeDef *huart)
{
    if (huart == &UART1_Config)
    {
        Bsp_Uart.UartInnfo[UART_1].uart_tx_busy_Flag = FLAG_true; // 改变uart1串口状态为空闲
    }
    if (huart == &UART2_Config)
    {
        Bsp_Uart.UartInnfo[UART_2].uart_tx_busy_Flag = FLAG_true; // 改变uart3串口状态为空闲
    }
    if (huart == &UART3_Config)
    {
        Bsp_Uart.UartInnfo[UART_3].uart_tx_busy_Flag = FLAG_true; // 改变uart3串口状态为空闲
    }
}

/**
 * @param    *huart -> 串口对象指针@UART_HandleTypeDef
 * @retval   None
 * @brief    串口接收处理函数(放回调函数里)
 **/
static void Bsp_Uart_RecData_Handler(UART_HandleTypeDef *huart)
{
    if (huart == &UART1_Config) /*--------------串口1----------------*/
    {
        // 开机 或 电机校准模式
        if ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode))
        {
            if (FLAG_true == System_Status.update_mode) // 升级模式
            {
                // 新协议数据解析处理
                Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart1, Bsp_Uart.UartInnfo[UART_1].uart_rx_index, single_rec_len);
            }
            else
            {
                // 旧协议数据解析处理
                Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart1, Bsp_Uart.UartInnfo[UART_1].uart_rx_index, single_rec_len);
            }
        }
        Bsp_Uart.UartInnfo[UART_1].uart_rx_index++; // uart1接收缓存索引加1
        if (Bsp_Uart.UartInnfo[UART_1].uart_rx_index == Rec_Buffer_size)
        {
            Bsp_Uart.UartInnfo[UART_1].uart_rx_index = 0; // 如果到达缓存底部，重新偏移索引到缓存头部
        }
        HAL_UART_Receive_IT(&UART1_Config, &uart1_rx_buffer[Bsp_Uart.UartInnfo[UART_1].uart_rx_index], single_rec_len); // 再次开启中断接收
    }
    else if (huart == &UART2_Config) /*--------------串口2----------------*/
    {
        if ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode))
        {
            if (FLAG_true == System_Status.update_mode) // 升级模式
            {
                // 新协议数据解析处理
                Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart2, Bsp_Uart.UartInnfo[UART_1].uart_rx_index, single_rec_len);
            }
            else
            {
                // 旧协议数据解析处理
                Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart2, Bsp_Uart.UartInnfo[UART_2].uart_rx_index, single_rec_len);
            }
        }
        Bsp_Uart.UartInnfo[UART_2].uart_rx_index++; // uart2接收缓存索引加1

        if (Bsp_Uart.UartInnfo[UART_2].uart_rx_index == Rec_Buffer_size)
        {
            Bsp_Uart.UartInnfo[UART_2].uart_rx_index = 0; // 如果到达缓存底部，重新偏移索引到缓存头部
        }
        HAL_UART_Receive_IT(&UART2_Config, &uart2_rx_buffer[Bsp_Uart.UartInnfo[UART_2].uart_rx_index], single_rec_len); // 再次开启中断接收
    }
    else if (huart == &UART3_Config) /*--------------串口3----------------*/
    {
        if ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode))
        {
            if (FLAG_true == System_Status.update_mode) // 升级模式
            {
                // 新协议数据解析处理
                Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart3, Bsp_Uart.UartInnfo[UART_1].uart_rx_index, single_rec_len);
            }
            else
            {
                // 旧协议数据解析处理
                Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Uart3, Bsp_Uart.UartInnfo[UART_3].uart_rx_index, single_rec_len);
            }
        }
        Bsp_Uart.UartInnfo[UART_3].uart_rx_index++; // uart3接收缓存索引加1
        if (Bsp_Uart.UartInnfo[UART_3].uart_rx_index == Rec_Buffer_size)
        {
            Bsp_Uart.UartInnfo[UART_3].uart_rx_index = 0; // 如果到达缓存底部，重新偏移索引到缓存头部
        }
        HAL_UART_Receive_IT(&UART3_Config, &uart3_rx_buffer[Bsp_Uart.UartInnfo[UART_3].uart_rx_index], single_rec_len); // 再次开启中断接收
    }
}

/**
 * @param    count -> 接收缓冲协议解析索引
 * @param    add_num -> 增加大小
 * @retval   CRC结果
 * @brief    对接收缓冲协议解析索引值进行增加
 **/
static void Bsp_Uart_RecData_AddPosition(uint16_t *count, uint16_t add_num)
{
    if (((*count) + add_num) >= Rec_Buffer_size) // 超出最大值(合理范围正常是: 0 ~ Rec_Buffer_size - 1)
    {
        /*
        得到超过缓冲区大小的差值 目的是实现一个循环缓冲区的效果
        当计数器超过缓冲区大小时，将其调整为剩余的位置，从而实现数据在缓冲区中的循环存储
        */
        (*count) = (*count) + add_num - Rec_Buffer_size;
    }
    else
    {
        (*count) += add_num;
    }
}