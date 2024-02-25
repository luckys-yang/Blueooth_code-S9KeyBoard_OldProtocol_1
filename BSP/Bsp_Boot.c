/***************************************************************************
 * File: Bsp_Boot.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description: 
 -----------------------------------
固件升级（操作Flash,使用新协议）
    注意：
        若当前按键板镜像在数据区0，则要发送数据区1固件。若当前按键板镜像在数据区1，则要发送数据区0固件。
        不同数据区链接起始地址不同，若混乱，程序无法正常运行
        若升级过程中用户断电，则重启后继续运行旧程序，APP可重新开启升级流程。
        若APP始终收不到某个包回复，可直接软件复位，复位后继续运行旧程序，APP可重新开启升级流程。
升级流程(上位机示范APP同理)：
    ----------下面开始使用旧协议----------
    1.APP发送切换协议，功能码为0xC0, 具体值为：0x55 0x04 0x0B 0xC0 0x00 0x00 0xCF
    按键回复, 功能码为0x00, 具体值为：0x55 0x0B 0x04 0x00 0x02 0x02 0xC0 0x00 0xD3

    1.上位机发送切换协议，功能码为0xC0, 具体值为：0x55 0x04 0x0C 0xC0 0x00 0x00 0xD0
    按键回复, 功能码为0x00, 具体值为：55 0C 04 00 02 02 C0 00 D4

    ----------下面开始使用新协议----------
    1.上位机发送按键板查询按键板固件版本 具体值为：A5 5A 05 04 80 25 00 00 00 00
    按键板回复 软件版本 硬件版本

    2.上位机根据软件版本和硬件版本判断是否进行升级

    3.上位机发送查询固件镜像地址 具体值为：A5 5A 05 04 80 27 00 00 00 00
    按键板回复当前固件所在地址 （0x18034000为数据区0 | 0x18057F00为数据区1）

    4.上位机根据软件版本与当前程序所在数据区决定升级固件，发送请求升级命令(设置新版本号) 示例值为：A5 5A 05 04 40 27 00 04 00 00 00 03 00 03
    按键板回复 设置成功 具体值为：A5 5A 04 05 40 28 00 01 00 00 00  | 设置失败

    5. 上位机开始发送固件升级包，每一个包128字节 示例值为：
A5 5A 05 04 40 29 00 82 00 00 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F 50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F 70 71 72 73 74 75 76 77 78 79 7A 7B 7C 7D 7E 7F 80 81 82 21 40
    按键板回复 收到的包编号表示接收成功

    5.上位机发送完所有包并且成功接收按键板包回复，发送所有包发送成功  具体值为：A5 5A 05 04 40 2B 00 00 00 00
    按键板回复收到所有包发送成功 具体值为：A5 5A 04 05 40 2C 00 01 00 00 00  | 设置失败

    6.上位机软件复位 具体值为：A5 5A 04 05 40 2D 00 01 00 00 00
    按键板回复软件复位 具体值为：A5 5A 04 05 40 2E 00 01 00 00 00
    然后设备会进行复位用户重新开机即可

    7.升级成功
固件信息页存放内容：
    1. 当前软件版本号
    2. 当前硬件版本号
    3. 当前映像地址
    4. 是否有新固件标志位
    5. 新固件开始地址
    6. 新固件版本号
    7. bootloader地址
    8. 固件更新源地址(即谁发起的升级,到时候回传数据包会根据这个来回传)

系统信息页存放内容(当前程序数据区的最后一页)：
    1. 蓝牙名称计数
    2. 手机系统

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void Bsp_Boot_StartUpdate(uint8_t *Rec_Buffer,uint32_t Src);
static void Bsp_boot_UpdateSuccess_Handler(void);
static uint8_t Bsp_Boot_UpdateData_WriteFlash(uint8_t *Rec_Buffer);

/* Public function prototypes=========================================================*/

static void Bsp_Boot_InfoGet(volatile BootInfo_st **boot_info);
static uint8_t Bsp_Boot_Cmd_R_start_update_Handler(uint8_t *Rec_Buffer);
static uint8_t Bsp_Boot_Cmd_R_update_file_Handler(uint8_t *Rec_Buffer);
static uint8_t Bsp_Boot_Cmd_R_all_file_finish_Handler(uint8_t *Rec_Buffer);
static uint8_t Bsp_Boot_Cmd_R_change_firmware_Handler(uint8_t *Rec_Buffer);
static uint8_t Bsp_Boot_Cmd_R_check_keyboard_version_Handler(uint8_t *Rec_Buffer);
static uint8_t Bsp_Boot_Cmd_R_check_image_base_Handler(uint8_t *Rec_Buffer);
static void Bsp_Boot_JumpAppRun(uint32_t base);
static void Bsp_Boot_SysInfoGet(volatile SysInfo_st **info);

/* Public variables==========================================================*/
// Flash页面管理数组，单次固件升级包为128字节，而一页有256字节，而Nor Flash 需要按页擦除，通过该数组管理一个页面只擦除一次
uint8_t flash_page_manage[SingleDataArea_Page_Count] = {FLAG_false};
volatile BootInfo_st *Current_BootInfo = {0};  // 存储当前程序里的固件升级信息 结构体指针变量
volatile BootInfo_st New_BootInfo = {0};  // 存储最新的固件升级信息结构体变量
volatile SysInfo_st *SysInfo = {0}; // 存储系统信息 结构体指针变量

Bsp_Boot_st Bsp_Boot = 
{
    .flash_page_manage_ptr = flash_page_manage,

    .Bsp_Boot_Cmd_R_start_update_Handler = &Bsp_Boot_Cmd_R_start_update_Handler,
    .Bsp_Boot_InfoGet = &Bsp_Boot_InfoGet,
    .Bsp_Boot_Cmd_R_update_file_Handler = &Bsp_Boot_Cmd_R_update_file_Handler,
    .Bsp_Boot_Cmd_R_all_file_finish_Handler = &Bsp_Boot_Cmd_R_all_file_finish_Handler,
    .Bsp_Boot_Cmd_R_change_firmware_Handler = &Bsp_Boot_Cmd_R_change_firmware_Handler,
    .Bsp_Boot_Cmd_R_check_keyboard_version_Handler = &Bsp_Boot_Cmd_R_check_keyboard_version_Handler,
    .Bsp_Boot_Cmd_R_check_image_base_Handler = &Bsp_Boot_Cmd_R_check_image_base_Handler,
    .Bsp_Boot_JumpAppRun = &Bsp_Boot_JumpAppRun,
    .Bsp_Boot_SysInfoGet = &Bsp_Boot_SysInfoGet

};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★Boot应用层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【请求开始升级】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_start_update_Handler(uint8_t *Rec_Buffer)
{
    Bsp_Boot_StartUpdate(Rec_Buffer, (uint32_t)NewProtocol_Package_Info.src);

    /*【请求开始升级】应答*/
    uint8_t rep_data[1];
    rep_data[0] = 0x00;
    // 参数1：目标地址(即收到包的源地址)
    Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_start_update, 1, rep_data);

    return 0x00;
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【发送升级手柄的128字节】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_update_file_Handler(uint8_t *Rec_Buffer)
{
    /*升级文件的前2个字节是表示第几包！！！剩下的128字节是升级文件数据！*/

    // 获取第几包的高字节(第一个包是0，第二个是1...)
    NewProtocol_Package_Info.package_number = (uint16_t)(Rec_Buffer[NewProtocol_Package_Info.first_data_position] << 8);
    /* 偏移一个字节 获取第几包的低字节 */
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(&NewProtocol_Package_Info.first_data_position, 1);
    NewProtocol_Package_Info.package_number += Rec_Buffer[NewProtocol_Package_Info.first_data_position];    // 合并为16位数据

    Bsp_Uart.Bsp_Uart_RecData_AddPosition(&NewProtocol_Package_Info.first_data_position, 1);    // 偏移到升级文件数据的位置
    NewProtocol_Package_Info.data_len -= 2; // 数据长度减2

    if (!Bsp_Boot_UpdateData_WriteFlash(Rec_Buffer))    // 写入成功则
    {
        /*【发送升级手柄的128字节】应答*/
        uint8_t rep_data[2];
        rep_data[0] = NewProtocol_Package_Info.package_number >> 8;
        rep_data[1] = NewProtocol_Package_Info.package_number;
        Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_update_file, 2, rep_data);
    }
    return 0x00;
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【发送升级手柄文件结束】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_all_file_finish_Handler(uint8_t *Rec_Buffer)
{
    Bsp_boot_UpdateSuccess_Handler();
    /*【发送升级手柄文件结束】应答*/
    uint8_t rep_data[1];
    rep_data[0] = 0x00;
    Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_all_file_finish, 1, rep_data);
    return 0x00;
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【软件复位/变换协议】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_change_firmware_Handler(uint8_t *Rec_Buffer)
{
    /*【软件复位/变换协议】应答*/
    uint8_t rep_data[1];
    rep_data[0] = 0x00;
    Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_change_firmware, 1, rep_data);

    _set_bit_value(System_Status.sys_timer_signal, EVENT_SoftwareReset);    // 软件复位事件
    return 0x00;
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【查询按键版固件版本】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_check_keyboard_version_Handler(uint8_t *Rec_Buffer)
{
    Bsp_Boot_InfoGet(&Current_BootInfo);    // 获取当前程序的固件信息
    /*【查询按键版固件版本】 应答 */
    uint8_t rep_data[8];
    rep_data[0] = (uint8_t)(Current_BootInfo->Soft_Version >> 24);  // 32位的软件版本信息
    rep_data[1] = (uint8_t)(Current_BootInfo->Soft_Version >> 16);
    rep_data[2] = (uint8_t)(Current_BootInfo->Soft_Version >> 8);
    rep_data[3] = (uint8_t)(Current_BootInfo->Soft_Version);

    rep_data[4] = (uint8_t)(Current_BootInfo->Hard_Version >> 24);  // 32位的硬件版本信息
    rep_data[5] = (uint8_t)(Current_BootInfo->Hard_Version >> 16);
    rep_data[6] = (uint8_t)(Current_BootInfo->Hard_Version >> 8);
    rep_data[7] = (uint8_t)(Current_BootInfo->Hard_Version);

    Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_check_keyboard_version, 8, rep_data);

    return 0x00;
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:成功
* @brief   【查询固件镜像地址】命令 处理函数
**/
static uint8_t Bsp_Boot_Cmd_R_check_image_base_Handler(uint8_t *Rec_Buffer)
{
    Bsp_Boot_InfoGet(&Current_BootInfo);    // 获取当前程序的固件信息
    /*【查询固件镜像地址】 应答 */
    uint8_t rep_data[4];
    rep_data[0] = (uint8_t)(Current_BootInfo->Image_Base >> 24);
    rep_data[1] = (uint8_t)(Current_BootInfo->Image_Base >> 16);
    rep_data[2] = (uint8_t)(Current_BootInfo->Image_Base >> 8);
    rep_data[3] = (uint8_t)(Current_BootInfo->Image_Base);

    Bsp_NewProtocol.Bsp_NewProtocol_SendPackage(NewProtocol_Package_Info.src, S_check_image_base, 4, rep_data);

    return 0x00;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★Boot中间层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @param    Src -> 固件升级包源地址
* @retval   None
* @brief    固件升级开始
**/
static void Bsp_Boot_StartUpdate(uint8_t *Rec_Buffer,uint32_t Src)
{
    Bsp_Boot_InfoGet(&Current_BootInfo);    // 获取当前程序的固件信息

    /*获取程序固件信息存储到最新固件信息结构体里*/
    New_BootInfo.Soft_Version = Current_BootInfo->Soft_Version; // 保留当前程序的软件版本信息
    New_BootInfo.Hard_Version = Current_BootInfo->Hard_Version; // 保留当前程序的硬件版本信息
    New_BootInfo.Image_Base = Current_BootInfo->Image_Base; // 保留当前程序的映像地址信息
    New_BootInfo.NewFirmware_Flag = Have_NewFirmware_Flag;  // 写入固件升级标志位为有

    if (DATA0_ADDR_BASE == Current_BootInfo->Image_Base)        // 当前为数据0区则
    {
        New_BootInfo.NewFirmware_AddrStart = DATA1_ADDR_BASE;   // 若当前映像地址在数据区0，则新固件开始地址为数据区1
    }
    else if (DATA1_ADDR_BASE == Current_BootInfo->Image_Base)   // 当前为数据1区则
    {
        New_BootInfo.NewFirmware_AddrStart = DATA0_ADDR_BASE;   // 若当前映像地址在数据区1，则新固件开始地址为数据区0
    }

    /*获取版本号(32位的)，这里first_data_position直接++即可反正执行完会清0*/
    New_BootInfo.NewFirmware_Version = (Rec_Buffer[NewProtocol_Package_Info.first_data_position] << 24);
    New_BootInfo.NewFirmware_Version += (Rec_Buffer[NewProtocol_Package_Info.first_data_position + 1] << 16);
    New_BootInfo.NewFirmware_Version += (Rec_Buffer[NewProtocol_Package_Info.first_data_position + 2] << 8);
    New_BootInfo.NewFirmware_Version += (Rec_Buffer[NewProtocol_Package_Info.first_data_position + 3]);

    New_BootInfo.Bootloadr_addr = BOOT_LOADER_ADDR; // 本工程无bootloader,走流程
    New_BootInfo.Update_Src = Src;  // 更新源地址
}

/**
* @param    Rec_Buffer -> 协议解析接收缓冲地址
* @retval   0x00:写入成功 0x01:写入失败
* @brief    固件升级数据写入Flash
**/
static uint8_t Bsp_Boot_UpdateData_WriteFlash(uint8_t *Rec_Buffer)
{
    /*一页可以写2个包，必须先擦一次再写入2个包*/

    uint16_t i;
    uint16_t crc = 0;   // 存储CRC校验和
    uint16_t page_offset;   // 表示当前所在的 Flash 页数
    uint16_t page_location; // 页内偏移(单位128字节)(0,1,0,1...这样来进行页内偏移)
    uint32_t addrstart; // 写入的FLASH偏移(指向每一页的首地址)

    page_offset = NewProtocol_Package_Info.package_number / (Own_Flash_Page_Size / Ota_PackageData_Size);
    page_location = NewProtocol_Package_Info.package_number % (Own_Flash_Page_Size / Ota_PackageData_Size);   
    addrstart = New_BootInfo.NewFirmware_AddrStart + page_offset * Own_Flash_Page_Size;

    if (FLAG_false == Bsp_Boot.flash_page_manage_ptr[page_offset])  // 若该页未被擦除
    {
        hal_flash_page_erase(addrstart);    // 擦除该页
        Bsp_Boot.flash_page_manage_ptr[page_offset] = FLAG_true;   // 标志该页已经被擦除
    }
    // 若数据一部分在缓存底部，一部分在缓存顶部，即地址不连续
    if (NewProtocol_Package_Info.first_data_position + NewProtocol_Package_Info.data_len >= Rec_Buffer_size)
    {
        uint16_t temp_offset = Rec_Buffer_size - NewProtocol_Package_Info.first_data_position;
        // 先写入底部数据(参数1：偏移地址 参数2：要写入数据的buffer 指针 参数3：写入数据长度)
        hal_flash_quad_page_program(addrstart + page_location * Ota_PackageData_Size, &Rec_Buffer[NewProtocol_Package_Info.first_data_position], temp_offset);
        // 再写入顶部数据
        hal_flash_quad_page_program(addrstart + page_location * Ota_PackageData_Size + temp_offset, &Rec_Buffer[0], NewProtocol_Package_Info.data_len - temp_offset);
    }
    else
    {
        // 若数据连续，整体写入
        hal_flash_quad_page_program(addrstart + page_location * Ota_PackageData_Size, &Rec_Buffer[NewProtocol_Package_Info.first_data_position], NewProtocol_Package_Info.data_len);
    }

    /* 对写入数据进行一次crc校验 */
    for (i = 0; i < NewProtocol_Package_Info.data_len; i++)
    {
        crc += *(uint8_t *)(addrstart + page_location * Ota_PackageData_Size + i); 
    }
    /* 把包编号也加到CRC里 */
    crc += (uint8_t)(NewProtocol_Package_Info.package_number >> 8); 
    crc += (uint8_t)(NewProtocol_Package_Info.package_number);

    if (NewProtocol_Package_Info.crc != crc)    // 若CRC不成功
    {
        return 0x01;
    }
    return 0x00;
}

/**
* @param    None
* @retval   None
* @brief    固件升级成功处理函数(此步完成后无法回头，固件升级失败则变砖)
**/
static void Bsp_boot_UpdateSuccess_Handler(void)
{
    New_BootInfo.Soft_Version = New_BootInfo.NewFirmware_Version;                                        // 更新软件版本号
    New_BootInfo.Image_Base = New_BootInfo.NewFirmware_AddrStart;                                        // 更新映像地址
    New_BootInfo.NewFirmware_Flag = None_NewFirmware_Flag;                                               // 清除更新标志位
    New_BootInfo.NewFirmware_AddrStart = None_NewFirmware_Flag;                                          // 清除新固件开始地址
    New_BootInfo.NewFirmware_Version = None_NewFirmware_Flag;                                            // 清除新固件版本号
    New_BootInfo.Update_Src = None_NewFirmware_Flag;                                                     // 清除更新源地址
    hal_flash_page_erase(OTA_SETTING_INFO_ADDR);                                                         // 擦数固件信息页
    hal_flash_quad_page_program(OTA_SETTING_INFO_ADDR, (uint8_t *)(&New_BootInfo), sizeof(BootInfo_st)); // 写入固件升级信息
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★Boot底层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    boot_info -> 固件升级结构体指针地址
* @retval   None
* @brief    固件升级信息结构体获取函数
**/
static void Bsp_Boot_InfoGet(volatile BootInfo_st **boot_info)
{
    // 获取映像信息地址(也就是把这个地址强制转换为指向这个结构体类型的指针然后赋给这个二级指针)
    *boot_info = (volatile BootInfo_st *)(OTA_SETTING_INFO_ADDR); 
}

/**
* @param    base -> 跳转地址
* @retval   None
* @brief    程序跳转函数，跳到指定地址运行
**/
static void Bsp_Boot_JumpAppRun(uint32_t base)
{
    uint32_t *msp = (void *)base;   // 获取主程序栈指针地址
    void (**reset_handler)(void) = (void *)(base + 4);  // 获取镜像Reset_Handler地址
    __set_MSP(*msp);    // 设置主程序堆栈指针
    __enable_irq(); // 打开中断
    (*reset_handler)();
}

/**
* @param    info -> 存放系统信息指针地址
* @retval   None
* @brief    系统信息结构体获取函数
**/
static void Bsp_Boot_SysInfoGet(volatile SysInfo_st **info)
{
    // 获取映像信息地址(也就是把这个地址强制转换为指向这个结构体类型的指针然后赋给这个二级指针)
    *info = (volatile SysInfo_st *)(SYS_INFO_ADDR); 
}