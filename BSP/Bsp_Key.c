/***************************************************************************
 * File: Bsp_Key.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
按键检测逻辑实现：
    对于电源键、拍照键、HOME键的单次、多次、长按检测。将按键设置为上升沿、下降沿都中断
    当下降沿中断时，将相应的signal信号置1，在按键定时器中会进入相应按键检测阶段，当上升沿中断时，对应的按键count加1
    在软件定时器时间到达时，进入对应的hanlde函数进行判断
    那么在按键检测的这段时间内：
        若count不为0，则记录当前的count到total_count里
        若count为0，则将这段时间视为长按，如果达到设定的按键长按时间，则响应按键长按，否则继续开启定时器，然后继续进行上述进行判断
        若count为0且按键为高电平，则认为一次按键检测结束
侧边滑动开关：
    T: 变焦--
    W: 变焦--
模式：
    F: 常用模式
    DM: 自动旋转模式(盗梦空间)
    POV: 酷炫模式
    GO: 快速专场模式
无线充：
    高配才有，低配没有，S9没有
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void vMotorCalibrationMode_Press_PhotoKey(void);
static void vRestore_Handle_LongPress_HomeKey(void);
static void vDownSlide_Handle_StirKey(void);
static void vCenterSlide_Handle_StirKey(void);
static void vUpSlide_Handle_StirKey(void);
static void Bsp_KeyTimer_CallBack(void *arg);
static void vPress_Handle_PhotoKey(KeyInfo_st *keyInfo);
static void vPress_Handle_PowerKey(KeyInfo_st *keyInfo);
static void vPress_Handle_HomeKey(KeyInfo_st *keyInfo);
/* Public function prototypes=========================================================*/

static void Bsp_Key_Init(void);
static void Bsp_Key_IOExti_Handler(uint8_t pin, exti_edge_t edge);
static void Bsp_key_Timer_Init(void);
/* Public variables==========================================================*/
struct builtin_timer *key_timer_inst = NULL;   // 按键服务软件定时器句柄
KeyInfo_st KeyInfo_photo = {0};   // 拍照键
KeyInfo_st KeyInfo_power = {0};   // 电源键
KeyInfo_st KeyInfo_home = {0};    // Home键

Bsp_Key_st Bsp_Key =
{
    .Bsp_Key_Init = &Bsp_Key_Init,
    .Bsp_Key_IOExti_Handler = &Bsp_Key_IOExti_Handler,
    .Bsp_key_Timer_Init = &Bsp_key_Timer_Init
};

/**
 * @param    None
 * @retval   None
 * @brief    按键初始化
 **/
static void Bsp_Key_Init(void)
{
    io_cfg_input(GPIO_PIN_KEY_POWER);                  // 硬件上已上拉，所以无需内部上拉
    io_exti_config(GPIO_PIN_KEY_POWER, INT_EDGE_BOTH); // 上升沿下降沿都产生中断

    io_cfg_input(GPIO_PIN_KEY_PHOTO);
    io_pull_write(GPIO_PIN_KEY_PHOTO, IO_PULL_UP);     // 内部上拉
    io_exti_config(GPIO_PIN_KEY_PHOTO, INT_EDGE_BOTH); // 上升沿下降沿都产生中断

    io_cfg_input(GPIO_PIN_KEY_HOME);                  // 硬件上已上拉，所以无需内部上拉
    io_exti_config(GPIO_PIN_KEY_HOME, INT_EDGE_BOTH); // 上升沿下降沿都产生中断

    io_cfg_input(GPIO_PIN_KEY_STIR_UP);
    io_pull_write(GPIO_PIN_KEY_STIR_UP, IO_PULL_UP);     // 内部上拉
    io_exti_config(GPIO_PIN_KEY_STIR_UP, INT_EDGE_BOTH); // 上升沿下降沿都产生中断
    io_cfg_input(GPIO_PIN_KEY_STIR_DOWN);
    io_pull_write(GPIO_PIN_KEY_STIR_DOWN, IO_PULL_UP);     // 内部上拉
    io_exti_config(GPIO_PIN_KEY_STIR_DOWN, INT_EDGE_BOTH); // 上升沿下降沿都产生中断
}

/**
 * @param    None
 * @retval   None
 * @brief    按键软件定时器初始化
 **/
static void Bsp_key_Timer_Init(void)
{
    key_timer_inst = builtin_timer_create(Bsp_KeyTimer_CallBack);
    builtin_timer_start(key_timer_inst, KEY_DETECTION_TIME, NULL);
}

/**
 * @param    pin -> 引脚
 * @param    edge -> 中断沿
 * @retval   None
 * @brief    按键外部中断处理函数
 **/
static void Bsp_Key_IOExti_Handler(uint8_t pin, exti_edge_t edge)
{
    if ((FLAG_true == System_Status.ble_loop_switch) && (FLAG_false == System_Status.PTZ_update_mode)) // 系统开机才运行按键检测
    {
        switch (pin)
        {
        case GPIO_PIN_KEY_PHOTO:
        {
            // motrocal模式
            if ((FLAG_true == System_Status.motorcal_mode) && (FLAG_false == System_Status.start_motorcal_signal))
            {
                vMotorCalibrationMode_Press_PhotoKey(); // 电机校准模式下按下拍照键
            }

            if (FLAG_false == System_Status.sys_power_switch) // 没开机则退出
            {
                break;
            }

            // 按下开启软件定时器
            if ((FLAG_false == KeyInfo_photo.start_signal) && (PIN_RESET == _ReadPin_KEY_PHOTO()))
            {
                KeyInfo_photo.start_signal = FLAG_true;
            }
            // 按上次数加1
            else if ((FLAG_true == KeyInfo_photo.start_signal) && (PIN_SET == _ReadPin_KEY_PHOTO()))
            {
                KeyInfo_photo.press_count++;
            }
            break;
        }
        case GPIO_PIN_KEY_POWER:
        {
            if ((FLAG_false == KeyInfo_power.start_signal) && (PIN_RESET == _ReadPin_KEY_POWER())) // 按下低电平 - 按下则表示开始计数
            {
                KeyInfo_power.start_signal = FLAG_true;
            }
            else if ((FLAG_true == KeyInfo_power.start_signal) && (PIN_SET == _ReadPin_KEY_POWER())) // 已开始计数且松开则表示按下了一次
            {
                KeyInfo_power.press_count++;
            }
            break;
        }
        case GPIO_PIN_KEY_HOME:
        {
            if (FLAG_false == System_Status.sys_power_switch) // 没开机则退出
            {
                break;
            }

            if ((FLAG_false == KeyInfo_home.start_signal) && (PIN_RESET == _ReadPin_KEY_HOME()))
            {
                KeyInfo_home.start_signal = FLAG_true;
            }
            else if ((FLAG_true == KeyInfo_home.start_signal) && (PIN_SET == _ReadPin_KEY_HOME()))
            {
                KeyInfo_home.press_count++;
            }
            else if (MODE_GO == System_Status.console_mode)
            {
                vRestore_Handle_LongPress_HomeKey(); // HOME键长按恢复处理(退出FG模式)
            }
            break;
        }
        case GPIO_PIN_KEY_STIR_DOWN:
        {
            if (FLAG_false == System_Status.sys_power_switch) // 没开机则退出
            {
                break;
            }

            if (PIN_RESET == _ReadPin_KEY_STIR_DOWN())
            {
                vDownSlide_Handle_StirKey(); // 拨动开关下滑处理
            }
            else
            {
                vCenterSlide_Handle_StirKey(); // 拨动开关回中处理
            }
            break;
        }
        case GPIO_PIN_KEY_STIR_UP:
        {
            if (FLAG_false == System_Status.sys_power_switch) // 没开机则退出
            {
                break;
            }

            if (PIN_RESET == _ReadPin_KEY_STIR_UP())
            {
                vUpSlide_Handle_StirKey(); // 拨动开关上滑处理
            }
            else
            {
                vCenterSlide_Handle_StirKey(); // 拨动开关回中处理
            }
            break;
        }
        }
    }
}

/**
 * @param    *arg -> 无需管
 * @retval   None
 * @brief    按键软件定时器回调函数
 **/
static void Bsp_KeyTimer_CallBack(void *arg)
{
    /* -----------------photo按键---------------- */
    if (FLAG_true == KeyInfo_photo.start_signal) // 若开启按键检测
    {
        if (KeyInfo_photo.check_time >= PHOTO_TIME) // 到达检测间隔时间
        {
            KeyInfo_photo.check_time = 0; // 检测时间计数器置0

            if (KeyInfo_photo.press_count != 0) // 若按下次数不为0
            {
                KeyInfo_photo.total_press_count += KeyInfo_photo.press_count; // 记录当前count值
                KeyInfo_photo.long_press_time = 0;                            // 长按时间清0
                KeyInfo_photo.press_count = 0;                                // 按下个数清0
            }
            else if (PIN_RESET == _ReadPin_KEY_PHOTO()) // 若count为0且按键未松开
            {
                KeyInfo_photo.long_press_time += PHOTO_TIME;               // 视这段软件定时器时间为长按
                KeyInfo_photo.total_press_count = 0;                       // 按下总数清0
                if (KeyInfo_photo.long_press_time >= PHOTO_LONGPRESS_TIME) // 判断时间是否大于等于长按事件
                {
                    KeyInfo_photo.start_signal = FLAG_false; // 停止按键检测
                    vPress_Handle_PhotoKey(&KeyInfo_photo);                // 响应长按
                }
            }
            else
            {
                KeyInfo_photo.start_signal = FLAG_false; // 停止按键检测
                vPress_Handle_PhotoKey(&KeyInfo_photo);                // 响应单次或多次按下
            }
        }
        else
        {
            KeyInfo_photo.check_time += KEY_DETECTION_TIME;
        }
    }
    /* -----------------power按键----------------- */
    if (FLAG_true == KeyInfo_power.start_signal)
    {
        if (KeyInfo_power.check_time >= POWER_TIME)
        {
            KeyInfo_power.check_time = 0; // 电源键间隔检测时间清0

            if (KeyInfo_power.press_count != 0)
            {
                KeyInfo_power.total_press_count += KeyInfo_power.press_count; // 按下总数
                KeyInfo_power.long_press_time = 0;                            // 长按时间清0
                KeyInfo_power.press_count = 0;                                // 电源键按下个数清0
            }
            else if (PIN_RESET == _ReadPin_KEY_POWER()) // 若count为0且按键未松开
            {
                KeyInfo_power.long_press_time += POWER_TIME; // 视这段软件定时器时间为长按
                KeyInfo_power.total_press_count = 0;
                if (KeyInfo_power.long_press_time >= POWER_LONGPRESS_TIME)
                {
                    KeyInfo_power.start_signal = FLAG_false;
                    vPress_Handle_PowerKey(&KeyInfo_power); // 响应长按
                }
            }
            else
            {
                KeyInfo_power.start_signal = FLAG_false;
                vPress_Handle_PowerKey(&KeyInfo_power); // 响应单次或多次按下
            }
        }
        else
        {
            KeyInfo_power.check_time += KEY_DETECTION_TIME;
        }
    }
    /* -----------------home按键---------------- */
    if (FLAG_true == KeyInfo_home.start_signal)
    {
        if (KeyInfo_home.check_time >= HOME_TIME)
        {
            KeyInfo_home.check_time = 0; // Home键间隔检测时间清0

            if (KeyInfo_home.press_count != 0)
            {
                KeyInfo_home.total_press_count += KeyInfo_home.press_count; // 按下总数
                KeyInfo_home.long_press_time = 0;                           // 长按时间清0
                KeyInfo_home.press_count = 0;                               // Home键按下个数清0
            }
            else if (PIN_RESET == _ReadPin_KEY_HOME())
            {
                KeyInfo_home.long_press_time += HOME_TIME; // 视这段软件定时器时间为长按
                KeyInfo_home.total_press_count = 0;        // 总按下个数清0
                if (KeyInfo_home.long_press_time >= HOME_LONGPRESS_TIME)
                {
                    KeyInfo_home.start_signal = FLAG_false;
                    vPress_Handle_HomeKey(&KeyInfo_home); // 响应长按
                }
            }
            else
            {
                KeyInfo_home.start_signal = FLAG_false;
                vPress_Handle_HomeKey(&KeyInfo_home); // 响应单次或多次按下
            }
        }
        else
        {
            KeyInfo_home.check_time += KEY_DETECTION_TIME;
        }
    }
    // 正交编码器控制
	Bsp_Encoder.Bsp_Encoder_Control();

    builtin_timer_start(key_timer_inst, KEY_DETECTION_TIME, NULL);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★按键处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/**
 * @param    None
 * @retval   None
 * @brief    电机校准模式按下拍照键
 **/
static void vMotorCalibrationMode_Press_PhotoKey(void)
{
   Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_3, no_blink, NULL); // 停止闪烁
   Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_3, blink, MOTOR_CALIBRATION_UNDERWAY_BLINK_TIME);    // 快速闪烁
   System_Status.start_motorcal_signal = FLAG_true;         // 电机校准开始信号使能
   Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_MotorAdjust, 0, 0);  // 电机校准功能码发送
}

/**
 * @param    None
 * @retval   None
 * @brief    HOME键长按恢复处理
 **/
static void vRestore_Handle_LongPress_HomeKey(void)
{
    Bsp_SysTimerCount.check_low_power_count = 0; // 低电量检测清0

    uint8_t data[2];
    data[0] = 0x00; // 0x00:普通模式 0x02:FG模式
    data[1] = 0x00; // 模式参数--缺省
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeSet, 2, data);

    System_Status.console_mode = System_Status.console_old_mode;    // 获取FG模式的上一次状态
    switch (System_Status.console_mode)
    {
    case MODE_F:
    {
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;
        data[1] = F_DATA;
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        break;
    }
    case MODE_POV:
    {
        uint8_t data[6];
        data[0] = NORMAL_MODE_DATA;
        data[1] = POV_DATA;
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);
        break;
    }
    default: break;
    }
    Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
}

/**
 * @param    None
 * @retval   None
 * @brief    拨动开关下滑处理
 **/
static void vDownSlide_Handle_StirKey(void)
{
    /*变焦*/
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = 0x03;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_FocusSet, 2, data);
}

/**
 * @param    None
 * @retval   None
 * @brief    拨动开关回中处理
 **/
static void vCenterSlide_Handle_StirKey(void)
{
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = 0x00;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_FocusSet, 2, data);
}

/**
 * @param    None
 * @retval   None
 * @brief    拨动开关上滑处理
 **/
static void vUpSlide_Handle_StirKey(void)
{
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = 0xFD;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_FocusSet, 2, data);    
}

/**
 * @param    *keyInfo -> 按键信息结构体地址
 * @retval   None
 * @brief    拍照键按下处理
 **/
static void vPress_Handle_PhotoKey(KeyInfo_st *keyInfo)
{
    if (1 == keyInfo->total_press_count) // 单击
    {
        uint8_t data[2];
        data[0] = 0x04; // 按钮事件->按钮ID-下按钮
        data[1] = 0x01; // 按钮事件->事件类型-1:单击 2:双击
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_AppKeyPress, 2, data);
        Bsp_Hid.Bsp_Hid_VolumeBtn_Control_Photo();  // 音量键控制拍照
    }
    else if (2 == keyInfo->total_press_count)   // 双击
    {
        /*控制横竖屏切换*/
        uint8_t data[1];
        if (FLAG_false == System_Status.horizontal_vertical_signal)
        {
            data[0] = 0x01; // 1：切换到竖屏
        }
        else
        {
            data[0] = 0x00; // 0：切换到横屏
        }
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_HorizontalVerticalSwitch, 1, data);
    }
    else if (3 == keyInfo->total_press_count)   // 三击
    {
        uint8_t data[2];
        data[0] = 0x04;
        data[1] = 0x03;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_AppKeyPress, 2, data);
    }
    else if (5 == keyInfo->total_press_count) // 五击
    {
        if (FLAG_false == System_Status.motorcal_mode)
        {
            if (FLAG_false == System_Status.key_test_mode)// 进入按键测试模式
            {
                System_Status.key_test_mode = FLAG_true; 
                Bsp_Led.Bsp_Led_All_Close();
            }
            else // 退出按键测试模式
            {
                System_Status.key_test_mode = FLAG_false;
                Bsp_Led.Bsp_Led_Console_Mode_Display();
            }
        }
    }
    else if (keyInfo->long_press_time >= PHOTO_LONGPRESS_TIME) // 长按
    {
        // 无操作
    }
    keyInfo->press_count = 0;
    keyInfo->long_press_time = 0;
    keyInfo->total_press_count = 0;
}

/**
 * @param    *keyInfo -> 按键信息结构体地址
 * @retval   None
 * @brief    电源键按下处理
 **/
static void vPress_Handle_PowerKey(KeyInfo_st *keyInfo)
{
    if (keyInfo->long_press_time >= POWER_LONGPRESS_TIME) // 长按
    {
        // 关机状态下且不是电机校准模式下
        if ((FLAG_false == System_Status.sys_power_switch) && (FLAG_false == System_Status.motorcal_mode))
        {
            // 开机处理(用于插着USB线时响应开机屏蔽则不会响应)
            Bsp_Power.Bsp_Power_StartingUp_Handler();
        }
        else
        {
            // 关机处理
            Bsp_Power.Bsp_Power_shutdown_Handler();
        }
    }
    else if (1 == keyInfo->total_press_count) // 单击
    {
        // 无需处理...
    }
    else if (2 == keyInfo->total_press_count) // 双击
    {
        if (FLAG_false == System_Status.key_test_mode) // 非按键测试模式
        {
            System_Status.check_electric = FLAG_true; // 检查电量信号置1
            // 电量检查处理
            Bsp_Power.Bsp_Power_Electric_Check_Handler();
            _set_bit_value(System_Status.sys_timer_signal, EVENT_CheckElectric);
        }
    }
    // 清除相关参数
    keyInfo->press_count = 0;
    keyInfo->long_press_time = 0;
    keyInfo->total_press_count = 0;
}

/**
 * @param    *keyInfo -> 按键信息结构体地址
 * @retval   None
 * @brief    Home键按下处理
 **/
static void vPress_Handle_HomeKey(KeyInfo_st *keyInfo)
{
    if (1 == keyInfo->total_press_count) // 单击
    {
        if (System_Status.dm_mode != FLAG_true) // 若不是在DM模式
        {
            Bsp_SysTimerCount.check_low_power_count = 0;

            switch (System_Status.console_mode)
            {
            case MODE_F:
            {
                /*模式数据*/
                uint8_t data[6];
                data[0] = NORMAL_MODE_DATA;
                data[1] = POV_DATA;
                data[2] = 0x00;
                data[3] = 0x00;
                data[4] = 0x00;
                data[5] = 0x00;
                Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);

                data[2] = 0x03;
                Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_ModeData, 6, data);

                System_Status.console_mode_data = POV_DATA;
                System_Status.console_mode = MODE_POV;
                break;
            }
            case MODE_POV:
            {
                /*模式数据*/
                uint8_t data[6];
                data[0] = NORMAL_MODE_DATA;
                data[1] = F_DATA;
                data[2] = 0x00;
                data[3] = 0x00;
                data[4] = 0x00;
                data[5] = 0x00;
                Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeData, 6, data);

                data[2] = 0x00;
                Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_APP, Old_Protocol_CMD_ModeData, 6, data);

                System_Status.console_mode_data = F_DATA;
                System_Status.console_mode = MODE_F;
                break;
            }
            default: break;
            }
			Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
        }
    }
    else if (2 == keyInfo->total_press_count)   // 双击
    {
        /*回中*/
        uint8_t data[1];
        data[0] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ExerciseContorl, 1, data);

        if (FLAG_true == System_Status.dm_mode) // 此时如果是DM模式下则
        {
            System_Status.dm_mode = FLAG_false; // DM模式退出
            System_Status.key_dm_mode = FLAG_false;
        }
		Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
    }
    else if ((3 == keyInfo->total_press_count) && (System_Status.console_mode != MODE_POV) 
            && (System_Status.dm_mode != FLAG_true)) // 三击且不在POV模式
    {
        Bsp_SysTimerCount.check_low_power_count = 0;
        /*DM1启动*/
        uint8_t data[3];
        data[0] = START_DM_MODE; // 0x01:盗梦空间1模式
        data[1] = 0x00;
        data[2] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ProcessExerciseContorl, 3, data);
        System_Status.key_dm_mode = FLAG_true;
        System_Status.dm_mode = FLAG_true;
		Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
    }
    else if ((keyInfo->long_press_time >= HOME_LONGPRESS_TIME) && (System_Status.console_mode != MODE_GO)) // 长按且不能在GO模式
    {
        /*GO模式*/
        uint8_t data[2];
        data[0] = START_GO_MODE;
        data[1] = 0x00;
        Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_PITCH, Old_Protocol_CMD_ModeSet, 2, data);
        System_Status.console_old_mode = System_Status.console_mode; // 记录旧模式，以便在退出GO模式时恢复
        System_Status.console_mode = MODE_GO;
		Bsp_Led.Bsp_Led_Console_Mode_Display(); // 云台模式下LED刷新显示
    }

    keyInfo->press_count = 0;
    keyInfo->long_press_time = 0;
    keyInfo->total_press_count = 0;
}