#ifndef __BSP_UART_H
#define __BSP_UART_H

#define Rec_Buffer_size 1024 // 接收包缓存大小
#define Send_Buffer_size 128 // 发送包临时缓存大小

#define RX_BUFFER_SIZE Rec_Buffer_size // 接收数组大小
#define TX_BUFFER_SIZE 2048            // 发送数组大小
#define BLE_SVC_BUFFER_SIZE Rec_Buffer_size // 蓝牙发送接收缓存长度

#define single_rec_len 1 // 接收数量触发中断

#define QUEUE_PARSE_MAX_SIZE 20 // 队列解析数组大小(一次性可以同时解析多少条)

#define UART1_BaudRate UART_BAUDRATE_115200 // 串口1波特率(用于与云台通信)
#define UART2_BaudRate UART_BAUDRATE_9600   // 串口2波特率(用于与2.4g通信)
#define UART3_BaudRate UART_BAUDRATE_9600   // 串口3波特率(用于与上位机通信)

/*串口数量枚举*/
typedef enum
{
    UART_1 = 0,
    UART_2,
    UART_3,
    UART_BLE,
    UART_MAX   // 最大数量(用作后面定义数组大小)
} Bsp_UartNum_et;

/*单个队列的参数*/
typedef struct
{
    uint16_t data_len;   // 用于存储包的数据长度
    uint16_t count;     // 用于计数(也是包头的位置)
    uint16_t deal_sign; // 用于记录处理接收进度
} SingleQueueInfo_st;

/*串口队列数据解析结构体*/
typedef struct
{
    SingleQueueInfo_st deal_queue[QUEUE_PARSE_MAX_SIZE]; //  一个包含 QUEUE_PARSE_MAX_SIZE 个 DEAL_INFO 结构体的数组，用于存储协议解析队列
    uint16_t deal_queue_index;   // 协议解析索引
    uint8_t *Rec_Buffer_ptr;     // 存储接收缓冲区数组的指针
} Uart_QueueParse_st;

/*串口信息结构体*/
typedef struct
{
    uint8_t *uart_rx_buffer_ptr; // 串口接收缓存数组指针
    uint8_t *uart_tx_buffer_ptr; // 串口发送缓存数组指针
    uint16_t uart_rx_index;      // uart接收缓存索引
    uint16_t uart_tx_index;      // uart发送索引
    bool uart_tx_busy_Flag;      // 串口发送忙标志位(FLAG_true--空闲 FLAG_false--忙)
    bool uart_rx_busy_Flag;      // 串口接收忙标志位(FLAG_true--空闲 FLAG_false--忙)
} UartInfo_st;

typedef struct
{
    UartInfo_st UartInnfo[UART_MAX];    // 定义串口信息实例数组

    void (*Bsp_Uart_Init)(void);                                                                    // 串口初始化
    void (*Bsp_Uart_SerialPort1_SendData)(uint8_t *, uint16_t);                                     // 串口1发送数据函数
    void (*Bsp_Uart_SerialPort2_SendData)(uint8_t *, uint16_t);                                     // 串口2发送数据函数
    void (*Bsp_Uart_SerialPort3_SendData)(uint8_t *, uint16_t);                                     // 串口3发送数据函数
    void (*Bsp_Uart_Ble_SendData)(uint8_t *, uint16_t);                                             // 蓝牙发送数据(通知)函数
    void (*Bsp_Uart_SendFinish_Handler)(UART_HandleTypeDef *);                                      // 串口发送完成处理函数
    void (*Bsp_Uart_RecData_Handler)(UART_HandleTypeDef *);                                         // 串口接收处理函数
    void (*Bsp_Uart_RecData_AddPosition)(uint16_t *, uint16_t);                                     // 对接收缓冲协议解析索引值进行增加
    void (*Bsp_Uart_ParameterInit)(void);   // 串口参数初始化
} Bsp_Uart_st;

extern Bsp_Uart_st Bsp_Uart;
extern Uart_QueueParse_st Uart_QueueParse_Ble;
extern UART_HandleTypeDef UART1_Config;                      // uart1句柄
extern UART_HandleTypeDef UART2_Config;                      // uart2句柄
extern UART_HandleTypeDef UART3_Config;                      // uart3句柄
#endif