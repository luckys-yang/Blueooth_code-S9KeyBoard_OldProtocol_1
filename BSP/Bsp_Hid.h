#ifndef __BSP_HID_H
#define __BSP_HID_H

/* Public define==========================================================*/
/* -----------------HID按键服务事件自定义--------------- */
#define HID_EVENT_Volume_Up (1 << 0)   // HID按键服务事件--音量加
#define HID_EVENT_Volume_Down (1 << 1) // HID按键服务事件--音量减
#define HID_EVENT_Mode_Step (1 << 2)   // HID按键服务事件--前后摄像头切换
#define HID_EVENT_VCR_Plus (1 << 3)    // HID按键服务事件--拍照录像切换
#define HID_EVENT_AC_Zoom_Up (1 << 4)  // HID按键服务事件--焦距加
#define HID_EVENT_AC_Zoom_Down (1 << 5) // HID按键服务事件--焦距减

typedef struct
{
    uint8_t hid_reported_data[1];   // HID上报值
    uint8_t photo_volume_status;    // 拍照时音量状态

    void (*Bsp_Hid_DeviceSendData)(uint8_t, uint8_t *, uint8_t, uint8_t);   // HID设备发送数据
    void (*Bsp_Hid_VolumeBtn_Control_Photo)(void); // 音量键控制拍照
} Bsp_Hid_st;

extern Bsp_Hid_st Bsp_Hid;

#endif
