#ifndef __BSP_OLDPROTOCOL_H
#define __BSP_OLDPROTOCOL_H

// 信息打印
#define LOG_I_Bsp_OldProtocol(...) LOG_I(__VA_ARGS__)

/*旧协议相关*/
#define OldProtocol_DataLen_Offset 5          // 旧协议数据长度所在协议的偏移地址(即表示数据长度Byte在协议的位置)(单位Byte)
#define OldProtocol_Exclude_Data_PackageLen 7 // 旧协议除数据外包的字节大小(单位Byte)
#define OldProtocol_Package_Head 0x55         // 旧协议包头

/*旧协议的地址枚举*/
typedef enum
{
    OLD_ADDR_ROLL = 0x01,      // 横滚地址
    OLD_ADDR_PITCH = 0x02,     // 俯仰(云台)地址
    OLD_ADDR_YAW = 0x03,       // 航向地址
    OLD_ADDR_KEY_BOARD = 0x04, // 按键板地址
    OLD_ADDR_APP = 0x0B,       // APP地址
    OLD_ADDR_PC = 0x0C,        // PC端(上位机)地址
    OLD_ADDR_AI_CAMERA = 0x0D, // AI摄像头地址
    OLD_ADDR_REMOTE = 0x0E,    // 遥控器地址
} OldProtocol_Addr_et;

#define OLD_OWN_ADDR (OldProtocol_Addr_et) OLD_ADDR_KEY_BOARD // 当前地址
#define OLD_UPDATE_PC_ADDR 0x00                           // 旧协议上位机更新电机板时地址会变为0x00

/*旧协议功能码类型(请按照命令大小升序排序)*/
typedef enum
{
    Old_Protocol_CMD_Confirm = 0x00,                  // 【确认】
    Old_Protocol_CMD_PostureCheck = 0x10,             // 【姿态查询】
    Old_Protocol_CMD_PostureSet = 0x11,               // 【姿态确认】
    Old_Protocol_CMD_UniformExerciseSpeedSet = 0x13,  // 【匀速运动速度设置】
    Old_Protocol_CMD_FollowSet = 0x14,                // 【跟随设置】
    Old_Protocol_CMD_ModeSet = 0x18,                  // 【模式设置】
    Old_Protocol_CMD_PostureMove = 0x21,              // 【姿态位移】
    Old_Protocol_CMD_FollowCenter = 0x23,             // 【中心跟随】
    Old_Protocol_CMD_HorizontalVerticalSwitch = 0x25, // 【横竖排切换】
    Old_Protocol_CMD_AiGesture = 0x26,                // 【AI手势】
    Old_Protocol_CMD_ExerciseContorl = 0x30,          // 【运动控制】
    Old_Protocol_CMD_ProcessExerciseContorl = 0x31,   // 【过程运动控制】
    Old_Protocol_CMD_ExendKey = 0x32,                 // 【扩展按键】
    Old_Protocol_CMD_MotorTest = 0x40,                // 【电机测试】
    Old_Protocol_CMD_VersionCheck = 0x50,             // 【版本查询】
    Old_Protocol_CMD_PtzStatusCheck = 0x51,           // 【云台系统状态查询】
    Old_Protocol_CMD_StartupShutdown = 0x52,          // 【开关机】
    Old_Protocol_CMD_MotorAdjust = 0x53,              // 【电机校准】
    Old_Protocol_CMD_ModeData = 0x61,                 // 【模式数据】
    Old_Protocol_CMD_PtzUpdateReady = 0x71,           // 【云台固件升级准备】
    Old_Protocol_CMD_PtzUpdateFinish = 0x74,          // 【云台固件升级完成】
    Old_Protocol_CMD_AppKeyPress = 0x84,              // 【APP按下】
    Old_Protocol_CMD_FocusSet = 0x86,                 // 【焦距设置】
    Old_Protocol_CMD_FaceFollow = 0x88,               // 【脸部跟随】
    Old_Protocol_CMD_ProtocolSwich = 0xC0,            // 【协议切换】
} OldProtocol_CmdType_et;

/* 旧协议解析包信息 */
typedef struct
{
    uint8_t src;                  // 包源地址
    uint8_t des;                  // 包目标地址
    uint8_t cmd;                  // 功能码
    uint8_t crc;                  // crc校验
    uint8_t data_len;             // 数据长度(不分1,2)
    uint8_t package_state;        // 解析是否成功信号(FLAG_false为解析失败，FLAG_true为解析成功)
    uint16_t head_position;       // 包头所处位置
    uint16_t first_data_position; // 第一个数据所处位置
    uint16_t package_number;      // 包编号(在固件升级时赋值)
} OldProtocol_Package_Info_st;

typedef struct
{
    uint8_t (*Bsp_OldProtocol_RxDataParse_Handler)(Uart_QueueParse_st *, uint16_t, uint16_t); // 旧协议接收数据解析处理
    void (*Bsp_OldProtocol_SendPackage)(uint8_t, uint8_t, uint8_t, uint8_t *);                 // 旧协议发送包函数
} Bsp_OldProtocol_st;

extern Bsp_OldProtocol_st Bsp_OldProtocol;

extern OldProtocol_Package_Info_st OldProtocol_Package_Info; // 旧协议包解析信息结构体变量

#endif
