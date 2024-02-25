/***************************************************************************
 * File: Bsp_Hid.c
 * Author: Yang
 * Date: 2024-02-18 15:53:31
 * description: 
 -----------------------------------
HID人机接口设备
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/
static void Bsp_Hid_DeviceSendData(uint8_t report_idx, uint8_t *report_data, uint8_t len, uint8_t conidx);
static void Bsp_Hid_VolumeBtn_Control_Photo(void);

/* Public variables==========================================================*/
Bsp_Hid_st Bsp_Hid = 
{
    .hid_reported_data = {0},
    .photo_volume_status = FLAG_true,

    .Bsp_Hid_DeviceSendData = &Bsp_Hid_DeviceSendData,
    .Bsp_Hid_VolumeBtn_Control_Photo = &Bsp_Hid_VolumeBtn_Control_Photo
};

/**
* @param    report_idx -> HID报告实例
* @param    *report_data -> 指向要发送的数据地址
* @param    len -> 要发送的数据长度
* @param    conidx -> 连接实例
* @retval   None
* @brief    HID设备发送数据
**/
static void Bsp_Hid_DeviceSendData(uint8_t report_idx, uint8_t *report_data, uint8_t len, uint8_t conidx)
{
    app_hid_send_keyboard_report(report_idx, report_data, len, conidx); // 调用库函数
}

/**
* @param    None
* @retval   None
* @brief    音量键控制拍照
**/
static void Bsp_Hid_VolumeBtn_Control_Photo(void)
{
    /*
    通过来回按下音量+和音量-来控制拍照键，模拟人按下拍照键，目的是为了保持音量不变
    就是如果手机处于拍照下则按下则拍照, 如果是视频录制下则按下则开始录屏/停止录屏
    */

    if (FLAG_true == Bsp_Hid.photo_volume_status) // 拍照时音量状态
    {
        Bsp_Hid.hid_reported_data[0] = HID_EVENT_Volume_Down;
        Bsp_Hid_DeviceSendData(0, Bsp_Hid.hid_reported_data, 1, Bsp_BlueTooth.ble_connect_id);

        Bsp_Hid.hid_reported_data[0] = 0x00;
        Bsp_Hid_DeviceSendData(0, Bsp_Hid.hid_reported_data, 1, Bsp_BlueTooth.ble_connect_id);
        Bsp_Hid.photo_volume_status = FLAG_false;
    }
    else
    {
        Bsp_Hid.hid_reported_data[0] = HID_EVENT_Volume_Up;
        Bsp_Hid_DeviceSendData(0, Bsp_Hid.hid_reported_data, 1, Bsp_BlueTooth.ble_connect_id);

        Bsp_Hid.hid_reported_data[0] = 0x00;
        Bsp_Hid_DeviceSendData(0, Bsp_Hid.hid_reported_data, 1, Bsp_BlueTooth.ble_connect_id);
        Bsp_Hid.photo_volume_status = FLAG_true;
    }
}