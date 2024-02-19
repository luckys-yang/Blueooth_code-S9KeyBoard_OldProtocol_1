#ifndef __BSP_BOOT_H
#define __BSP_BOOT_H

#define DATA0_ADDR_BASE 0x18034000  // 数据区0基地址(默认)
#define DATA1_ADDR_BASE 0x18057F00  // 数据区1基地址

#define OTA_SETTING_INFO_ADDR	0x1807BE00  // ota固件信息地址
#define BOOT_LOADER_ADDR 0x00000000  // (本工程无bootloader)
#define Ota_PackageData_Size 128    // 固件升级数据包大小

#define Flash_Page_Size FLASH_PAGE_SIZE // Flash页大小
#define SingleDataArea_Page_Count 575  // 单个数据区页数量

#endif