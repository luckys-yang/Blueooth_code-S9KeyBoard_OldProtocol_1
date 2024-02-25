#ifndef __NEWPROTOCOL_H
#define __NEWPROTOCOL_H

/* Public define==========================================================*/
#define NewProtocol_Package_Head_H 0xA5 // 新协议包头高字节
#define NewProtocol_Package_Head_L 0x5A // 新协议包头低字节

#define NewProtocol_Exclude_Data_PackageLen 10   // 新协议除数据外包的字节大小(单位Byte)

/*新协议的地址枚举*/
typedef enum
{
    NEW_ADDR_PTZ = 0x01,      // 俯仰(云台)地址 横滚地址 航向地址
    NEW_ADDR_KEY_BOARD = 0x04, // 按键板地址
    NEW_ADDR_APP = 0x03,       // APP地址
    NEW_ADDR_PC = 0x05,        // PC端(上位机)地址
} NewProtocol_Addr_et;

#define NEW_OWN_ADDR (NewProtocol_Addr_et) NEW_ADDR_KEY_BOARD // 当前地址

/*新协议功能码类型(请按照命令大小升序排序) 大写R开头为接收，大写S开头为发送*/
typedef enum
{
    R_start_update = 0x4027,    // 【请求开始升级】-- 请求
    S_start_update = 0x4028,   // 【请求开始升级】-- 应答

    R_update_file = 0x4029, // 【发送升级手柄的128字节】-- 请求
    S_update_file = 0x402A, // 【发送升级手柄的128字节】-- 应答

    R_all_file_finish = 0x402B, // 【发送升级手柄文件结束】-- 请求
    S_all_file_finish = 0x402C, // 【发送升级手柄文件结束】-- 应答

    R_change_firmware = 0x402D, // 【软件复位/变换协议】-- 请求
    S_change_firmware = 0x402E, // 【软件复位/变换协议】-- 应答

    R_check_keyboard_version = 0x8025,  // 【查询按键版固件版本】-- 请求
    S_check_keyboard_version = 0x8026,  // 【查询按键版固件版本】-- 应答

    R_check_image_base = 0x8027,    // 【查询固件镜像地址】-- 请求
    S_check_image_base = 0x8028,    // 【查询固件镜像地址】-- 应答
} NewProtocol_CmdType_et;

/* 新协议当个包位置*/
typedef enum
{
    Position_Head_H = 1,    // 包头高字节
    Position_Head_L = 2,    // 包头低字节
    Position_Src = 3,       // 源地址
    Position_Des = 4,       // 目标地址
    Position_Cmd_H = 5,     // 功能码高字节
    Position_Cmd_L = 6,     // 功能码低字节
    Position_DataLen_H = 7, // 数据长度高字节
    Position_DataLen_L = 8, // 数据长度低字节
    Position_Crc_H = 9,     // CRC高字节
    Position_Crc_L = 10,    // CRC低字节
} NewProtocol_SignPosition_et;

/* 新协议解析包信息 */
typedef struct
{
    uint8_t src;                  // 包源地址
    uint8_t des;                  // 包目标地址
    uint16_t cmd;                 // 功能码
    uint16_t data_len;            // 数据长度
    uint16_t crc;                 // crc校验
    uint8_t package_state;        // 解析是否成功信号(FLAG_false为解析失败，FLAG_true为解析成功)
    uint16_t head_position;       // 包头所处位置
    uint16_t first_data_position; // 第一个数据所处位置
    uint16_t package_number;      // 包编号(在固件升级时赋值)
} NewProtocol_Package_Info_st;

typedef struct
{
    uint8_t (*Bsp_NewProtocol_RxDataParse_Handler)(Uart_QueueParse_st *, uint16_t, uint16_t); // 新协议接收数据解析处理
    void (*Bsp_NewProtocol_SendPackage)(uint8_t, uint16_t, uint16_t, uint8_t *);    // 新协议发送包函数
} Bsp_NewProtocol_st;


extern Bsp_NewProtocol_st Bsp_NewProtocol;
extern NewProtocol_Package_Info_st NewProtocol_Package_Info;

#endif
