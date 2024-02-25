#ifndef __BSP_BOOT_H
#define __BSP_BOOT_H

#define DATA0_ADDR_BASE 0x18034000  // 数据区0基地址(默认)
#define DATA1_ADDR_BASE 0x18057F00  // 数据区1基地址

#define OTA_SETTING_INFO_ADDR	0x1807BE00  // ota固件信息首地址(1页 -- 256 Byte)

#define BOOT_LOADER_ADDR 0x00000000  // (本工程无bootloader)
#define Ota_PackageData_Size 128    // 固件升级数据包大小
#define Have_NewFirmware_Flag	0x00000000  // 有新固件标志位
#define None_NewFirmware_Flag 0xFFFFFFFF   // 无新固件标志位

#define Own_Flash_Page_Size FLASH_PAGE_SIZE // 当前芯片的Flash页大小
#define SingleDataArea_Page_Count 575  // 单个数据区页数量(147200 Byte)

#define SYS_INFO_ADDR (CURRENT_FIRMWARE_ADDR + ((SingleDataArea_Page_Count - 1) * Own_Flash_Page_Size))   // 存储系统信息的首地址

/* 固件升级信息结构体(需要用volatile修饰防止被优化) */
typedef struct
{
    volatile uint32_t Soft_Version;          // 当前软件版本号
    volatile uint32_t Hard_Version;          // 当前硬件版本号
    volatile uint32_t Image_Base;            // 当前映像地址
    volatile uint32_t NewFirmware_Flag;      // 是否有新固件标志位
    volatile uint32_t NewFirmware_AddrStart; // 新固件开始地址
    volatile uint32_t NewFirmware_Version;   // 新固件版本号
    volatile uint32_t Bootloadr_addr;        // bootloader地址
    volatile uint32_t Update_Src;            // 固件更新源地址(即谁发起的升级,到时候回传数据包会根据这个来回传)
} BootInfo_st;

/*系统信息结构体*/
typedef struct
{
    volatile uint32_t ble_name_count;   // 蓝牙名字计数
    volatile uint32_t phone_system; // 手机系统(暂无用到)
} SysInfo_st;

typedef struct
{
    uint8_t *flash_page_manage_ptr; // Flash页面管理数组指针

    uint8_t (*Bsp_Boot_Cmd_R_start_update_Handler)(uint8_t *); // 【开始固件升级】命令 处理函数
    void (*Bsp_Boot_InfoGet)(volatile BootInfo_st **);   // 固件升级开始
    uint8_t (*Bsp_Boot_Cmd_R_update_file_Handler)(uint8_t *); // 【发送升级手柄的128字节】命令 处理函数
    uint8_t (*Bsp_Boot_Cmd_R_all_file_finish_Handler)(uint8_t *);   // 【发送升级手柄文件结束】命令 处理函数
    uint8_t (*Bsp_Boot_Cmd_R_change_firmware_Handler)(uint8_t *); // 【软件复位/变换协议】命令 处理函数
    uint8_t (*Bsp_Boot_Cmd_R_check_keyboard_version_Handler)(uint8_t *); // 【查询按键版固件版本】命令 处理函数
    uint8_t (*Bsp_Boot_Cmd_R_check_image_base_Handler)(uint8_t *); // 【查询固件镜像地址】命令 处理函数
    void (*Bsp_Boot_JumpAppRun)(uint32_t);  // 程序跳转函数，跳到指定地址运行
    void (*Bsp_Boot_SysInfoGet)(volatile SysInfo_st **); // 系统信息结构体获取函数
} Bsp_Boot_st;

extern Bsp_Boot_st Bsp_Boot;
extern volatile BootInfo_st *Current_BootInfo;
extern volatile SysInfo_st *SysInfo;

#endif