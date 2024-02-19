#ifndef __SYSTEM_INIT_H
#define __SYSTEM_INIT_H

/* Public define==========================================================*/
// 信息打印
#define LOG_I_System_Init(...) LOG_I(__VA_ARGS__)

/* -----------------引脚定义---------------- */
/*LED引脚 (高电平亮)*/
#define GPIO_PIN_LED1 PB08 // LED1
#define GPIO_PIN_LED2 PB09 // LED2
#define GPIO_PIN_LED3 PB10 // LED3
#define GPIO_PIN_LED4 PB11 // LED4

/*按键引脚*/
#define GPIO_PIN_KEY_POWER PC01     // 电源键
#define GPIO_PIN_KEY_PHOTO PA08     // 拍照键
#define GPIO_PIN_KEY_HOME PA06      // home键
#define GPIO_PIN_KEY_STIR_UP PA04   // 拨动开关向上
#define GPIO_PIN_KEY_STIR_DOWN PA03 // 拨动开关向下

/*电源引脚*/
#define GPIO_PIN_POWER_LOCK PB15      // 电源开关
#define GPIO_PIN_ADC_POWER_EN PA02    // 电压采样开关
#define GPIO_PIN_USB_POWER_INPUT PC00 // USB充电电源插入检测
#define GPIO_PIN_BAT_STDBY PA05       // USB充电是否完成检测引脚

/*ADC引脚*/
#define GPIO_PIN_ADC_POWER PA00  // 电池电压采样IO
#define GPIO_PIN_ADC_ROCKER PA01 // 摇杆键采样IO

// 定时器编码器引脚
#define GPIO_PIN_ENCODERA PA11
#define GPIO_PIN_ENCODERB PA12

// 串口引脚
#define GPIO_PIN_UART1_TX PA14 // 串口1TX引脚，串口1用于与云台通信
#define GPIO_PIN_UART1_RX PA13 // 串口1RX引脚
#define GPIO_PIN_UART2_TX PB02 // 串口2TX引脚，串口2用于与2.4g通信
#define GPIO_PIN_UART2_RX PA15 // 串口2RX引脚
#define GPIO_PIN_UART3_TX PB00 // 串口3TX引脚，串口3用于与上位机通信
#define GPIO_PIN_UART3_RX PB01 // 串口3RX引脚

/* -----------------响应时间配置---------------- */
#define SYS_TIME 1                     // 切换固件后系统复位时间(ms) 关机后掉电时间(ms) (建议50ms以上，否则切换成功的回复包未发出系统就复位了)
#define KEY_DETECTION_TIME 50          // 按键查询时间(ms)
#define PHOTO_TIME 200                 // 拍照键相邻两次按键检测时间间隔(ms)
#define PHOTO_LONGPRESS_TIME 1000      // 拍照键长按时间(ms)
#define POWER_TIME 200                 // 电源键相邻两次按键检测时间间隔(ms)
#define POWER_LONGPRESS_TIME 1000      // 电源键长按时间(ms)
#define HOME_TIME 200                  // HOME键相邻两次按键检测时间间隔(ms)
#define HOME_LONGPRESS_TIME 200        // HOME键长按时间(ms)
#define START_UP_LED_TIME 500          // 开机时LED亮的间隔时间(ms)
#define ADC_TIME 50                    // ADC采样时间(ms)
#define BATTERY_UPDATE_TIME 1000       // 电池电量更新时间(ms)
#define BATTERY_CHARGE_UPDATE_TIME 500 // 电池充电时更新时间(ms)
#define CHECK_ELECTRIC_TIME 2000       // 检查电量显示时间(ms)
#define DM_FINISH_TIME 500             // DM模式完成后停留时间
#define CHECK_PTZ_STATUS_TIME 50       // 检查云台状态时间
#define MOTOR_CALIBRATION_UNDERWAY_BLINK_TIME 100 // LED闪烁速度(ms)--电机校准过程中...
#define ENTTER_MOTOR_CALIBRATION_BLINK_TIME 500   // LED闪烁速度(ms)--进入电机校准模式下
#define LOW_POWER_SHUTDOWN_TIME 5000              // 低电量多久关机(ms)
#define CHECK_LOW_POWER_TIME 1000                 // 低电量检查时间(ms)
#define LOW_POWER_PTZ_MODE_TIME 500                 // 低电量下模式的LED闪烁时间(ms)

/* -----------------电池电量对应电压配置(对应电压*1000)----------------- */
#define four_electric 2780     // 四格电对应电压(2.78V*1000)
#define three_electric 2613    // 三格电对应电压(2.61V*1000)
#define two_electric 2526      // 两格电对应电压(2.52V*1000)
#define one_electric 2300      // 一格电对应电压(2.30V*1000)
#define low_electric 2250      // 零格电对应电压(2.25V*1000)
#define shutdown_electric 2200 // 自动关机电量
#define electric_offset 120    // 补偿值

/* -----------------功能码【模式数据】的模式----------------- */
#define F_DATA 0x00   // F模式数据
#define POV_DATA 0xC0 // POV模式数据
#define AI_DATA 0x40

/* -----------------功能码【过程运动控制】的命令码----------------- */
#define START_DM_MODE 0x01 // 盗梦空间1

/* -----------------功能码【模式设置】的模式----------------- */
#define START_GO_MODE 0x02 // (GO模式)

/* -----------------功能码【AI手势】的命令码----------------- */
#define AI_START_SOP_VIDEO 0x02    // 开始/结束录像  （按键版响应）
#define AI_PHOTO_VIDEO_SWITCH 0x03 // 拍照/录像切换命令（按键版响应）

/* -----------------功能码【模式数据】的id----------------- */
#define NORMAL_MODE_DATA 0x00 // 缺省

/* ----------------云台系统状态->DM模式状态值----------------- */
/*￥应用模式￥*/
#define dm_status_bit 7        // 在数据里的位置是第7个字节(在第一个数据上进行偏移7个Byte: 0,1,2,3,4,5,6,7)
#define dm_status_bit_offset 0 // 无需偏移
#define dm_status_bit_len 8    // 应用模式占8位
/*￥应用模式下的运行数据-步骤￥*/
#define dm_posture_bit 9        // 在数据里的位置是第9个字节
#define dm_posture_bit_offset 0 // 无需偏移
#define dm_posture_bit_len 8

#define DEAL_DM_READY 0x01  // 扩展状态下应用模式下的运行数据下的步骤--准备完成
#define DEAL_DM_FINISH 0x04 // 扩展状态下应用模式下的运行数据下的步骤--执行完成
#define DEAL_DM_MODE 0x04   // 扩展状态下应用模式--盗梦空间1模式

/* ----------------云台系统状态->电机校准状态值----------------- */
/*￥校准状态￥*/
#define motor_status_bit 2        // 在数据里的位置是第2个字节
#define motor_status_bit_offset 0 // 无需偏移
#define motor_status_bit_len 3    // 占3位
/*￥校准步骤￥*/
#define motorcal_result_bit 2        // 在数据里的位置是第2个字节
#define motorcal_result_bit_offset 3 // 偏移3位
#define motorcal_result_bit_len 4    // 占4位
/*￥竖屏模式￥*/
#define horizontal_vertical_bit 5        // 在数据里的位置是第5个字节
#define horizontal_vertical_bit_offset 3 // 偏移3位
#define horizontal_vertical_bit_len 1    // 占1位

#define DEAL_MOTORCAL_SUCCESS 10 // 系统状态下校准步骤--校准完成
#define DEAL_MOTORCAL_TIMEOUT 11 // 系统状态下校准步骤--校准超时
#define DEAL_MOTORCAL_ERROR1 12  // 系统状态下校准步骤--校准错误1
#define DEAL_MOTORCAL_ERROR2 13  // 系统状态下校准步骤--校准错误2
#define DEAL_MOTORCAL_ERROR3 14  // 系统状态下校准步骤--校准错误3
#define DEAL_MOTORCAL_ERROR4 15  // 系统状态下校准步骤--校准错误4

/* -----------------位操作---------------- */
#define _get_deal_value(data, offset, len) ((data >> offset) & ((1 << len) - 1)) // 获取数据中指定位段的值（从右往左数）data--数据  offset--偏移多少位  len--生成一个指定长度为len的二进制数全为 1 的值
#define _get_bit_value(data, offset) ((data >> offset) & 0x01)                   // 获取数据中指定位置的位值（从右往左数）
#define _set_bit_value(data, offset) (data |= (1 << offset))                     // 将数据中指定位置的位值设置为1（从右往左数）
#define _clear_bit_value(data, offset) (data &= (~(1 << offset)))                // 将数据中指定位置的位值清零（设置为0）（从右往左数）

/* 云台当前模式枚举 */
enum console_type
{
    MODE_F = 0, // F模式(常用模式)
    MODE_POV,   // POV模式(酷炫模式)
    MODE_GO,    // GO模式(快速专场模式)
    MODE_DM     // DM模式(自动旋转模式)(暂时没用上)
};

/* 系统状态结构体 */
typedef struct
{
    uint8_t update_mode;                // 升级模式
    uint8_t motorcal_mode;              // 电机校准模式
    uint8_t key_test_mode;                  // 按键测试模式
    uint8_t console_mode;               // 云台现在所处模式
    uint8_t console_old_mode;           // 云台之前所处模式,为了在退出DM模式和GO模式时恢复之前的模式
    uint8_t wireless_charing;           // 云台无线充状态
    uint8_t bluetooth;                  // 蓝牙状态
    uint8_t battery;                    // 电池状态
    uint8_t sys_power_switch;           // 系统电源开关信号
    uint8_t ble_loop_switch;            // 蓝牙调度开始信号
    uint8_t start_motorcal_signal;      // 电机校准开始信号
    uint8_t horizontal_vertical_signal; // 横竖屏切换信号
    uint8_t sys_timer_signal;           // 系统定时器信号
    uint8_t console_mode_data;          // 云台模式数据
    uint8_t check_electric;             // 检查电量信号
    uint8_t low_power_mode;             // 低电量模式
    uint8_t shutdown_signal;            // 关机步骤信号
    uint8_t key_dm_mode;                // 按键开启DM模式
    uint8_t dm_mode;                    // DM模式
    uint8_t lowpower_shutdown;          // 低电量关机信号
    uint8_t PTZ_update_mode;            // 云台固件升级模式
    uint8_t remote_key_type;
    uint8_t remote_press_type;
} System_Status_st;

typedef struct
{
    void (*Hardware_Init)(void); // 硬件初始化
} System_Init_st;

extern System_Init_st System_Init;
extern System_Status_st System_Status;

#endif
