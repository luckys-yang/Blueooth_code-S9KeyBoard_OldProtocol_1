#ifndef __BSP_BLUETOOTH_H
#define __BSP_BLUETOOTH_H

// 信息打印
#define LOG_I_Bsp_BlueTooth(...) LOG_I(__VA_ARGS__)
#define LOG_HEX_Bsp_BlueTooth(data_pointer, data_length) LOG_HEX(data_pointer, data_length)

/*--------------------------- 通用UUID 宏定义 ---------------------------*/
/*外观UUID值(只能选一个)*/
#define GAP_APPEARANCE_UNKNOWN 0x0000      // 未知外观
#define GAP_APPEARANCE_EARBUD 0x0941       // 耳塞式耳机
#define GAP_APPEARANCE_HEADSET 0x0942      // 耳机
#define GAP_APPEARANCE_HEADPHONES 0x0943   // 头戴式耳机
#define GAP_APPEARANCE_GENERIC_HID 0x03C0  // 通用HID设备(即鼠标和键盘合并)
#define GAP_APPEARANCE_HID_KEYBOARD 0x03C1 // HID键盘
#define GAP_APPEARANCE_HID_MOUSE 0x03C2    // HID鼠标

#define GATT_UUID_HID 0x1812 // HID人机接口设备

/*----------------------------蓝牙数据相关 宏定义 --------------------------*/
#define BLE_SERVER_MAX_MTU 247                       // 蓝牙数据包最大长度(MTU)
#define BLE_SERVER_MTU_DFT 23                        // 蓝牙数据包默认长度(MTU)
#define BLE_SERVER_MAX_DATA_LEN (Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu - 3) // 蓝牙数据包中数据最大长度
#define BLE_SVC_RX_MAX_LEN (BLE_SERVER_MAX_MTU - 3)  // 蓝牙接收的数据包中数据最大长度
#define BLE_SVC_TX_MAX_LEN (BLE_SERVER_MAX_MTU - 3)  // 蓝牙发送数据包中数据最大长度

/*----------------------------蓝牙设备信息相关(文件系统) 宏定义 --------------------------*/
#define DIS_SVC_PNPID_LEN 0x07                              // 设备信息服务PNP ID长度
#define TINYFS_DIR_NAME 7                                   // 目录名称
#define TINYFS_RECORD_KEY1 1                                // 记录关键字1
#define APP_HID_DEV_NAME (ble_adv_name)                     // HID设备名称
#define APP_HID_DEV_NAME_LEN (sizeof(APP_HID_DEV_NAME) - 1) // HID设备名称长度
#define APP_DIS_PNP_ID ("\x02\x5E\x04\x40\x00\x00\x03")     // 设备ID信息
#define APP_DIS_PNP_ID_LEN 7                                // 设备ID信息长度

/*----------------------------蓝牙配对秘钥相关(SEC) 宏定义 --------------------------*/
#define PAIR_OOB_DATA_FLAG 0x0                          // 表示是否支持OOB
#define PAIR_AUTHREQ (AUTH_MITM | AUTH_BOND)            // 设置认证要求 --- 绑定标志|MITM 标志（中间人攻击保护）
#define PAIR_KEY_SIZE 0x10                              // 表示支持的最大LTK大小（范围：7-16）
#define PAIR_INIT_KEY_DIST (KDIST_ENCKEY | KDIST_IDKEY) // 设置初始密钥分发 --- 分发加密和主标识信息|分发身份和地址信息
#define PAIR_RESP_KEY_DIST (KDIST_ENCKEY | KDIST_IDKEY) // 设置响应密钥分发 --- 分发加密和主标识信息|分发身份和地址信息
#define PAIR_PASSKEY_NUMBER {'2','0','0','2','0','8'}   // 配对密码，必须是6字节
/*多种安全模式 --- 多选1*/
#define PAIR_Host_Initiative_PASSW 1 // 主机主动配对(需要配对密码)
#define PAIR_Host_Initiative_None 0  // 主机主动配对(无需配对密码)

#define HID_REPORT_MAP_LEN (sizeof(hid_report_map)) // HID报告映射长度

// 限制蓝牙名称后缀计数的最大范围(0~9)
#define BLE_DEVICE_NAME_MAX_LEN 10

// 蓝牙名称(包含最后的\0)，固定格式不改否则配套APP会检测不到
#define BLE_NAME "SMART_XE"

// 蓝牙没有连接时ID
#define BLE_DISCONNECTED_ID 0xFF

// 常见码枚举
typedef enum
{
    HCI_SUCCESS = 0x00,                                      // 成功
    HCI_NO_CONNECTION = 0x02,                                // 无连接
    HCI_HW_FAILURE = 0x03,                                   // 硬件故障
    HCI_PAGE_TIMEOUT = 0x04,                                 // 页面超时
    HCI_AUTHENTICATION_FAILURE = 0x05,                       // 认证失败
    HCI_KEY_MISSING = 0x06,                                  // 缺少密钥
    HCI_CONN_TIMEOUT = 0x08,                                 // 连接超时
    HCI_MAX_NUMBER_OF_SCO_CONNECTIONS_TO_DEVICE = 0x0A,      // 设备的SCO连接数达到最大
    HCI_HOST_TIMEOUT = 0x10,                                 // 主机超时
    HCI_OTHER_END_TERMINATED_CONN_USER_ENDED = 0x13,         // 对方终止连接-用户结束
    HCI_OTHER_END_TERMINATED_CONN_LOW_RESOURCES = 0x14,      // 对方终止连接-资源不足
    HCI_OTHER_END_TERMINATED_CONN_ABOUT_TO_POWER_OFF = 0x15, // 对方终止连接-即将关闭电源
    HCI_CONN_TERMINATED_BY_LOCAL_HOST = 0x16,                // 由本地主机终止连接
    HCI_REPETED_ATTEMPTS = 0x17,                             // 重复尝试
    HCI_PAIRING_NOT_ALLOWED = 0x18,                          // 不允许配对
    HCI_INSTANT_PASSED = 0x28,                               // 已经过期
    HCI_PAIRING_UNIT_KEY_NOT_SUPPORTED = 0x29                // 不支持的配对单元密钥
} hci_result_et;

/*定义 UART 服务的属性索引枚举*/
typedef enum
{
    BLE_SVC_IDX_RX_CHAR,    // UART 接收特征句柄
    BLE_SVC_IDX_RX_VAL,     // UART 接收值句柄
    BLE_SVC_IDX_TX_CHAR,    // UART 发送特征句柄
    BLE_SVC_IDX_TX_VAL,     // UART 发送值句柄
    BLE_SVC_IDX_TX_NTF_CFG, // UART 发送通知配置句柄
    BLE_SVC_ATT_NUM         // UART 服务属性数量
} ble_svc_att_db_handles_et;

/*定义 设备信息服务（DIS） 枚举*/
typedef enum
{
    DIS_SVC_IDX_PNP_ID_CHAR, // 表示设备信息服务（DIS）的 PnP ID 特征的句柄
    DIS_SVC_IDX_PNP_ID_VAL,  // 表示设备信息服务（DIS）的 PnP ID 值的句柄
    DIS_SVC_ATT_NUM,         // 表示设备信息服务（DIS）的属性数量
} dis_svc_att_db_hdlles_et;

/*蓝牙广播信息结构体*/
typedef struct
{
    uint8_t ble_name_count;    // 蓝牙名称计数
    char *ble_adv_name_ptr;    // 存储蓝牙广播名字存储数组指针(+3是为了后面修改名字时加后缀)
    uint8_t *ble_mac_addr_ptr; // 存储蓝牙MAC地址数组指针
    uint8_t ble_adv_handle;    // 广播句柄
    uint8_t *ble_adv_data_ptr; // 广播数据数组指针
} BLE_AdvInfo_st;

/*蓝牙串口信息结构体*/
typedef struct
{
    uint16_t reply_data;    // 服务器回复数据
    uint16_t ble_server_mtu;    // 蓝牙发送服务包大小
} BLE_UartInfo_st;

typedef struct
{
    BLE_AdvInfo_st *ble_adv_info_Instance; // 创建蓝牙广播信息结构体变量指针
    BLE_UartInfo_st *ble_uart_info_Instance;    // 创建蓝牙串口信息结构体变量指针

    uint8_t ble_connect_id;                // 设备连接ID 未连接时为0XFF
    void (*Bsp_BlueTooth_Init)(void);      // 蓝牙协议栈/服务初始化
    void (*Bsp_BlueTooth_Start_adv)(void); // 蓝牙广播开始
    void (*Bsp_BlueTooth_Send_Notification)(void); // 蓝牙发送通知(数据)给客户端
} Bsp_BlueTooth_st;

extern Bsp_BlueTooth_st Bsp_BlueTooth;
#endif
