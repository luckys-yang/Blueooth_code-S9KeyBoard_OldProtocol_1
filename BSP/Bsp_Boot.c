/***************************************************************************
 * File: Bsp_Boot.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description: 
 -----------------------------------
固件升级（操作Flash）
    注意：
        若当前按键板镜像在数据区0，则要发送数据区1固件。若当前按键板镜像在数据区1，则要发送数据区0固件。
        不同数据区链接起始地址不同，若混乱，程序无法正常运行
        若升级过程中用户断电，则重启后继续运行旧程序，APP可重新开启升级流程。
        若APP始终收不到某个包回复，可直接软件复位，复位后继续运行旧程序，APP可重新开启升级流程。
升级流程：
    ----------下面开始使用旧协议----------
    1.APP发送切换协议，功能码为0xC0, 具体值为：0x55 0x04 0x0B 0xC0 0x00 0x00 0xCF
    按键回复, 功能码为0x00, 具体值为：0x55 0x0B 0x04 0x00 0x02 0x02 0xC0 0x00 0xD3

    ----------下面开始使用新协议----------
    1.APP发送按键板查询按键板固件版本
    按键板回复 软件版本 硬件版本

    2.APP根据软件版本和硬件版本判断是否进行升级

    3.APP发送查询固件镜像地址
    按键板回复当前固件所在地址 （0x18034000为数据区0 | 0x18057F00为数据区1）

    4.APP根据软件版本与当前程序所在数据区决定升级固件
    按键板回复收到的包

    5.APP发送完所有包并且成功接收按键板包回复，发送所有包发送成功
    按键板回复收到所有包发送成功

    6.APP软件复位
    按键板回复软件复位

    7.升级成功

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

