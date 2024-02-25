/***************************************************************************
 * File: Bsp_NewProtocol.c
 * Author: Yang
 * Date: 2024-02-18 21:08:44
 * description: 
 -----------------------------------
新协议解析
    新协议(NewProtoocol)：
    【格式】: 
            帧头--2Byte 高字节：0xA5 低字节：0x5A
            源地址--1Byte	按键板是0x04
            目标地址--1Byte
            指令识别码--2Byte CMD_H + CMD_L	
            用户数据长度--2Byte LEN_H + LEN_L
            用户数据(UD)--NByte	用户数据没有则为空
            校验--2Byte CRC_H + CRC_L 和校验：暂时为数据内容部分
    【传输】：
        协议的双字节都是高位在前低位在后，发送的数据内容都是先发高再发低
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void Bsp_NewProtocol_ClearParsedPackage(Uart_QueueParse_st *deal_param, uint8_t clear_offset);
static uint8_t Bsp_NewProtocol_GetPackageInfo(uint8_t *Rec_Buffer, uint16_t *RecBuffer_count);
static void Bsp_NewProtocol_ParsedSuccess_Handler(uint8_t *Rec_Buffer);
static uint16_t Bsp_NewProtocol_CRC_Calculate(uint8_t *data, uint16_t len);
static void Bsp_NewProtocol_SyntheticData_Handler(uint8_t *Rec_Buffer);

/* Public function prototypes=========================================================*/

static uint8_t Bsp_NewProtocol_RxDataParse_Handler(Uart_QueueParse_st *deal_param, uint16_t value_index, uint16_t Rec_len);
static void Bsp_NewProtocol_SendPackage(uint8_t des, uint16_t cmd, uint16_t datalen, uint8_t *data);

/* Public variables==========================================================*/
// 旧协议包解析信息结构体变量
NewProtocol_Package_Info_st NewProtocol_Package_Info = {0};

Bsp_NewProtocol_st Bsp_NewProtocol = 
{
    .Bsp_NewProtocol_RxDataParse_Handler = &Bsp_NewProtocol_RxDataParse_Handler,
    .Bsp_NewProtocol_SendPackage = &Bsp_NewProtocol_SendPackage
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★新协议应用层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    deal_param -> 串口协议解析参数结构体指针
 * @param    value_index -> 接收缓冲当前数据索引（接收当前数据前）
 * @param    Rec_len -> 接收到数据的大小
 * @retval   None
 * @brief    新协议接收数据解析处理
 **/
static uint8_t Bsp_NewProtocol_RxDataParse_Handler(Uart_QueueParse_st *deal_param, uint16_t value_index, uint16_t Rec_len)
{
    while (Rec_len != 0)
    {
        for (uint16_t i = 0; i < deal_param->deal_queue_index; i++)
        {
            deal_param->deal_queue[i].deal_sign++;

            switch (deal_param->deal_queue[i].deal_sign)
            {
            case Position_Head_L:   // 包头低字节
            {
                if (deal_param->Rec_Buffer_ptr[value_index] != NewProtocol_Package_Head_L)  // 不等于新协议包头低字节则清除包
                {
                    Bsp_NewProtocol_ClearParsedPackage(deal_param, i);
                    i--;
                }
                break;
            }
            case Position_DataLen_H:    // 用户数据长度高字节
            {
                deal_param->deal_queue[i].data_len = deal_param->Rec_Buffer_ptr[value_index] << 8;  // 移到高位
            }
            case Position_DataLen_L:    // 用户数据长度低字节
            {
                deal_param->deal_queue[i].data_len += deal_param->Rec_Buffer_ptr[value_index];
            }
            default: break;
            }

            if (deal_param->deal_queue[i].deal_sign == (deal_param->deal_queue[i].data_len + NewProtocol_Exclude_Data_PackageLen))
            {
                if (!Bsp_NewProtocol_GetPackageInfo(deal_param->Rec_Buffer_ptr, &deal_param->deal_queue[i].count))
                {
                    Bsp_NewProtocol_ParsedSuccess_Handler(deal_param->Rec_Buffer_ptr);
                }
                Bsp_NewProtocol_ClearParsedPackage(deal_param, i);
                i--;
            }
        }
        if (NewProtocol_Package_Head_H == deal_param->Rec_Buffer_ptr[value_index])  // 判断包头高字节
        {
            deal_param->deal_queue[deal_param->deal_queue_index].deal_sign++;
            ;
            deal_param->deal_queue[deal_param->deal_queue_index].count = value_index;
            deal_param->deal_queue_index++;
        }
        /*兼容传入的数据长度不是1时需要++*/
        value_index++;
        if (value_index == Rec_Buffer_size)
        {
            value_index = 0;
        }        
        Rec_len--;
    }
    return 0x00;
}

/**
* @param    des -> 【发送目标地址】
* @param    cmd -> 【发送功能码】
* @param    datalen -> 发送数据长度
* @param    *data -> 发送数据地址
* @retval   None
* @brief    新协议发送包函数
**/
static void Bsp_NewProtocol_SendPackage(uint8_t des, uint16_t cmd, uint16_t datalen, uint8_t *data)
{
    uint8_t i;
    uint16_t crc;   // 存储CRC校验和
    uint16_t first_data_position_temp;    // 数据内容中第一个数据在包中的位置临时变量
    uint8_t Rep_Buffer[Send_Buffer_size];    // 发送数据临时缓冲数组
    uint16_t RepBuffer_Index = 0;   // 发送数据临时缓冲数组索引

    Rep_Buffer[RepBuffer_Index] = NewProtocol_Package_Head_H; // 【高字节包头】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = NewProtocol_Package_Head_L; // 【低字节包头】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = NEW_OWN_ADDR; // 【源地址】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = des; // 【目标地址】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = (uint8_t)(cmd >> 8); // 功能码高字节
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = (uint8_t)(cmd); // 功能码低字节
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = (uint8_t)(datalen >> 8);  // 【数据长度高字节】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = (uint8_t)(datalen);  // 【数据长度低字节】
    RepBuffer_Index++;
    first_data_position_temp = RepBuffer_Index;

    // 填充数据
    for (i = 0; i < datalen; i++)
    {
        Rep_Buffer[RepBuffer_Index] = *(data + i);
        RepBuffer_Index++;
    }
    crc = Bsp_NewProtocol_CRC_Calculate(&Rep_Buffer[first_data_position_temp], datalen);    // CRC计算
    Rep_Buffer[RepBuffer_Index] = (uint8_t)(crc >> 8);  // 【crc高字节】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = (uint8_t)crc;  // 【crc高字节】
    RepBuffer_Index++;

    switch (des)    // 根据目的地发送
    {
    case NEW_ADDR_APP:
    {
        Bsp_Uart.Bsp_Uart_Ble_SendData(Rep_Buffer, RepBuffer_Index);
        break;
    }
    case NEW_ADDR_PTZ:
    {
        Bsp_Uart.Bsp_Uart_SerialPort1_SendData(Rep_Buffer, RepBuffer_Index);
        break;
    }
    case NEW_ADDR_PC:
    {
        Bsp_Uart.Bsp_Uart_SerialPort3_SendData(Rep_Buffer, RepBuffer_Index);
        break;
    }
    default: break;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★新协议中间层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    *deal_param -> 串口协议解析参数结构体指针
 * @param    clear_offset -> 清理包的偏移
 * @retval   None
 * @brief    清除解析过的包，避免被重复解析
 **/
static void Bsp_NewProtocol_ClearParsedPackage(Uart_QueueParse_st *deal_param, uint8_t clear_offset)
{
    // 先判断需要清除的元素是否是队列中最后一个元素,如果不是，则
    if (deal_param->deal_queue_index > (clear_offset + 1))
    {
        // 将它后面的元素向前移动，以覆盖当前元素
        memcpy((void *)&deal_param->deal_queue[clear_offset], (uint8_t *)&deal_param->deal_queue[clear_offset + 1], (deal_param->deal_queue_index - clear_offset - 1) * sizeof(SingleQueueInfo_st));
        deal_param->deal_queue[deal_param->deal_queue_index - 1].count = 0;     // 计数清0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].data_len = 0;  // 数据长度清0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].deal_sign = 0; // 处理接收进度
    }
    else // 如果已经是最后的元素则直接清除对应的队列参数即可
    {
        deal_param->deal_queue[clear_offset].count = 0;
        deal_param->deal_queue[clear_offset].data_len = 0;
        deal_param->deal_queue[clear_offset].deal_sign = 0;
    }
    deal_param->deal_queue_index--; // 索引减1    
}

/**
 * @param    Rec_Buffer -> 接收数组地址
 * @param    RecBuffer_count -> 接收数组索引地址
 * @retval   0x00->成功 0x01->包数据大小大于接收缓冲大小 0x02->包数据crc校验错误
 * @brief    新协议获取数据包信息 通过协议获取对应位置信息
 **/
static uint8_t Bsp_NewProtocol_GetPackageInfo(uint8_t *Rec_Buffer, uint16_t *RecBuffer_count)
{
    uint8_t temp_h = 0, temp_l = 0; // 存储 命令的高低字节,数据长度的高低字节,CRC的高低字节 临时缓存
    uint16_t temp_crc = 0;  // 存储校验和临时缓存

    NewProtocol_Package_Info.package_state = FLAG_true;          // 解析成功
    NewProtocol_Package_Info.head_position = (*RecBuffer_count); // 记录存储包头所处位置

    /*获取包源地址--偏移2字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 2);
    NewProtocol_Package_Info.src = Rec_Buffer[(*RecBuffer_count)];
    /*获取目标地址--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    NewProtocol_Package_Info.des = Rec_Buffer[(*RecBuffer_count)];

    /*获取命令高字节--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    temp_h = Rec_Buffer[(*RecBuffer_count)];
    /*获取命令低字节位--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    temp_l = Rec_Buffer[(*RecBuffer_count)];
    NewProtocol_Package_Info.cmd = (temp_h << 8) + temp_l;  // 合并为16位数据

    /*获取数据长度高字节位--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    temp_h = Rec_Buffer[(*RecBuffer_count)];
    /*获取数据长度低字节位--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    temp_l = Rec_Buffer[(*RecBuffer_count)];
    NewProtocol_Package_Info.data_len = (temp_h << 8) + temp_l;  // 合并为16位数据

    if (NewProtocol_Package_Info.data_len > Rec_Buffer_size)    // 数据长度超出最大长度则解析失败
    {
        NewProtocol_Package_Info.package_state = 0;
        return 0x01;
    }

    /*记录第一个数据所处位置*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    NewProtocol_Package_Info.first_data_position = (*RecBuffer_count);  

    /*获取CRC高字节位--偏移数据长度个字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, NewProtocol_Package_Info.data_len);
    temp_h = Rec_Buffer[(*RecBuffer_count)];
    /*获取CRC低字节位--偏移1个字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    temp_l = Rec_Buffer[(*RecBuffer_count)];
    NewProtocol_Package_Info.crc = (temp_h << 8) + temp_l;  // 合并为16位数据

    // 第一个数据所处位置+数据长度
    if (NewProtocol_Package_Info.first_data_position + NewProtocol_Package_Info.data_len > Rec_Buffer_size)
    {
        uint16_t offset_temp = Rec_Buffer_size - NewProtocol_Package_Info.first_data_position;  // 得到超过缓冲区大小的差值
        temp_crc += Bsp_NewProtocol_CRC_Calculate(&Rec_Buffer[NewProtocol_Package_Info.first_data_position], offset_temp);    // 先计算未超出部分
        temp_crc += Bsp_NewProtocol_CRC_Calculate(&Rec_Buffer[0], NewProtocol_Package_Info.data_len - offset_temp); // 计算超出的部分
    }
    else
    {
        temp_crc = Bsp_NewProtocol_CRC_Calculate(&Rec_Buffer[NewProtocol_Package_Info.first_data_position], NewProtocol_Package_Info.data_len);
    }

    if (NewProtocol_Package_Info.crc != temp_crc)
    {
        NewProtocol_Package_Info.package_state = FLAG_false;
        return 0x02;
    }
     /* 偏移一个字节习惯(可加可不加反正退出函数后会进行清0) */
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    return 0x00;
}

/**
 * @param    data -> 数据地址
 * @param    len -> 数据长度
 * @retval   CRC结果
 * @brief    新协议CRC计算
 **/
static uint16_t Bsp_NewProtocol_CRC_Calculate(uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t crc = 0;

    for (i = 0; i < len; i++)
    {
        crc += *(data + i);
    }
    return crc;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★新协议解析成功处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    Rec_Buffer -> 接收缓冲地址
 * @retval   None
 * @brief    新协议解析成功处理函数
 **/
static void Bsp_NewProtocol_ParsedSuccess_Handler(uint8_t *Rec_Buffer)
{
    Bsp_NewProtocol_SyntheticData_Handler(Rec_Buffer);
}

/**
 * @param    Rec_Buffer -> 接收缓冲地址
 * @retval   None
 * @brief    新协议处理综合数据
 **/
static void Bsp_NewProtocol_SyntheticData_Handler(uint8_t *Rec_Buffer)
{
    uint16_t total_len = 0;                                                              // 包的总长度
    total_len = NewProtocol_Package_Info.data_len + NewProtocol_Exclude_Data_PackageLen; // 若包需要转发，需要转发的数据长度

    switch (NewProtocol_Package_Info.des) // 目标地址
    {
    case NEW_OWN_ADDR:
    {
        switch (NewProtocol_Package_Info.cmd)
        {
        case R_start_update: // 【请求开始升级】-- 请求
        {
            Bsp_Boot.Bsp_Boot_Cmd_R_start_update_Handler(Rec_Buffer);
            break;
        }
        case R_update_file: // 【发送升级手柄的128字节】-- 请求
        {
            Bsp_Boot.Bsp_Boot_Cmd_R_update_file_Handler(Rec_Buffer);
            break;
        }
        case R_all_file_finish: // 【发送升级手柄文件结束】-- 请求
        {
            // 到这步就无法回头！！！升级失败就变砖！！
            Bsp_Boot.Bsp_Boot_Cmd_R_all_file_finish_Handler(Rec_Buffer);
            break;
        }
        case R_change_firmware: // 【软件复位/变换协议】-- 请求
        {
            Bsp_Boot.Bsp_Boot_Cmd_R_change_firmware_Handler(Rec_Buffer);
            break;
        }
        case R_check_keyboard_version: // 【查询按键版固件版本】-- 请求
        {
            Bsp_Boot.Bsp_Boot_Cmd_R_check_keyboard_version_Handler(Rec_Buffer);
            break;
        }
        case R_check_image_base: // 【查询固件镜像地址】-- 请求
        {
            Bsp_Boot.Bsp_Boot_Cmd_R_check_image_base_Handler(Rec_Buffer);
            break;
        }
        default:
            break;
        }
        break;
    }
    case NEW_ADDR_PC: // 若目的地是别的地址，进行转发
    {
        // 索引是0开始的，所以当==时数组大小是比索引多1
        if (OldProtocol_Package_Info.head_position + total_len > RX_BUFFER_SIZE) // 判断需要转发的数据是否连续
        {
            // 若不连续，先发缓冲数组底部数据（即发送超出前的）
            Bsp_Uart.Bsp_Uart_SerialPort3_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position);
            // 再发缓冲数组顶部数据（即发送超出后的）
            Bsp_Uart.Bsp_Uart_SerialPort3_SendData(&Rec_Buffer[0], total_len - (RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position));
        }
        else
        {
            // 若连续，直接发
            Bsp_Uart.Bsp_Uart_SerialPort3_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], OldProtocol_Package_Info.data_len + NewProtocol_Exclude_Data_PackageLen);
        }
        break;
    }
    case NEW_ADDR_PTZ:
    {
        if (OldProtocol_Package_Info.head_position + total_len > RX_BUFFER_SIZE)
        {
            // 若不连续，先发缓冲数组底部数据
            Bsp_Uart.Bsp_Uart_SerialPort1_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position);
            // 再发缓冲数组顶部数据
            Bsp_Uart.Bsp_Uart_SerialPort1_SendData(&Rec_Buffer[0], total_len - (RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position));
        }
        else
        {
            // 若连续，直接发
            Bsp_Uart.Bsp_Uart_SerialPort1_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], OldProtocol_Package_Info.data_len + NewProtocol_Exclude_Data_PackageLen);
        }
        break;
    }
    case NEW_ADDR_APP:
    {
        if (OldProtocol_Package_Info.head_position + total_len > RX_BUFFER_SIZE)
        {
            // 若不连续，先发缓冲数组底部数据
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position);
            // 再发缓冲数组顶部数据
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[0], total_len - (RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position));            
        }
        else
        {
            // 若连续，直接发
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], OldProtocol_Package_Info.data_len + NewProtocol_Exclude_Data_PackageLen);
        }
        break;
    }
    default: break;
    }
}