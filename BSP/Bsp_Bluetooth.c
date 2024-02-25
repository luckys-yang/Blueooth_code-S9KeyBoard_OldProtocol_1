/***************************************************************************
 * File: Bsp_Bluetooth.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
蓝牙名字：
    默认是格式是: "xxxx"
    当关机时下推摇杆则进入修改名字，在原有基础上增加后缀变成: "xxxx(y)",y为名称计数值每次进入此函数则进行+1,在范围内循环:0~BLE_DEVICE_NAME_MAX_LEN
    修改完需要断电重新启动才会生效，手机上则如果之前有配对过需要重新取消配对才能重新进行连接
APP_DIS_PNP_ID（设备ID）:
    Byte1       ------  PNP_ID来源
    Byte2~3     ------  供应商
    Byte4~5     ------  制造商管理标示符
    Byte6~7     ------  产品版本

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void Bsp_BlueTooth_dev_manager_CallBack(enum dev_evt_type type, union dev_evt_u *evt);
static void Bsp_BlueTooth_gap_manager_CallBack(enum gap_evt_type type, union gap_evt_u *evt, uint8_t con_idx);
static void Bsp_BlueTooth_gatt_manager_CallBack(enum gatt_evt_type type, union gatt_evt_u *evt, uint8_t con_idx);
static void Bsp_BlueTooth_prf_hid_server_CallBack(enum hid_evt_type type, union hid_evt_u *evt);
static void Bsp_BlueTooth_prf_batt_server_CallBack(enum bass_evt_type type, union bass_evt_u *evt);
static void Bsp_BlueTooth_Create_adv_Obj(void);
static bool Bsp_BlueTooth_hid_con_rssi_judge(int8_t con_rssi, const uint8_t *init_addr, uint8_t init_addr_type);
static void Bsp_BlueTooth_prf_Added_Handler(struct profile_added_evt *evt);
static void Bsp_BlueTooth_Gap_GetDeviceName(struct gap_dev_info_dev_name *dev_name_ptr, uint8_t con_idx);
static void Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(uint8_t att_idx, uint8_t con_idx);
static void Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(uint8_t att_idx, uint8_t con_idx, uint16_t length, uint8_t const *value);
static void Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(uint8_t con_idx);

/* Public function prototypes=========================================================*/

static void Bsp_BlueTooth_Init(void);
static void Bsp_BlueTooth_Start_adv(void);
static void Bsp_BlueTooth_Send_Notification(void);

/* Private variables=========================================================*/
// 蓝牙服务 UUID标识
static const uint8_t ble_svc_uuid_128[] =
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00
};
// 蓝牙串口接收服务 UUID标识
static const uint8_t ble_rx_char_uuid_128[] =
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xe1, 0xff, 0x00, 0x00
};
// 蓝牙串口发送服务 UUID标识
static const uint8_t ble_tx_char_uuid_128[] =
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xe2, 0xff, 0x00, 0x00
};

static const uint8_t att_decl_char_array[] = {0x03, 0x28};  // 定义 ATT 属性声明 UUID
static const uint8_t att_desc_client_char_cfg_array[] = {0x02, 0x29};   // 定义 ATT 描述符客户端特征配置 UUID
static const uint8_t dis_char_pnpid_uuid[] = {0x50, 0x2A};  // 特征和对象类型UUID --- PnP ID(设备信息服务PNP)
static const uint8_t dis_svc_uuid[] = {0x0A, 0x18}; // 服务UUID --- 设备信息
/* HID设备描述符 */
const uint8_t hid_report_map[] =
{
    0x05, 0x0c, /* USAGE_PAGE (Consumer Devices) */
    0x09, 0x01, /* USAGE (Consumer Control) */
    0xa1, 0x01, /* COLLECTION (Application) */

    0x15, 0x00,       /* LOGICAL_MINIMUM (0) */
    0x25, 0x01,       /* LOGICAL_MAXIMUM (1) */
    0x09, 0xEA,       /* (Volume Down) */
    0x09, 0xE9,       /* (Volume Up) */
    0x09, 0x82,       /* Usage (Mode Step) 前后摄像头切换 发0x04 */
    0x09, 0xA0,       /* Usage (VCR Plus) 拍照/录像切换  发0x08 */
    0x0A, 0x2D, 0x02, /* Usage (AC Zoom In) 发0x10 */
    0x0A, 0x2E, 0x02, /* Usage (AC Zoom Out) */
    0x0A, 0x32, 0x02, /* Usage (Tracking Decrement) */
    0x0A, 0x32, 0X02, /* Usage (tracking Increment) */
    0x75, 0x01,       /* REPORT_SIZE (1)  --- 1个 */
    0x95, 0x08,       /* REPORT_COUNT (8) --- 8bit */
    0x81, 0x02,       /* Input (Data,Value,Relative,Bit Field) */

    0xc0 /* END_COLLECTION */
};

static struct gatt_svc_env ls_ble_server_svc_env;   // GATT服务 -- 蓝牙服务句柄
static struct gatt_svc_env dis_server_svc_env;      // GAT服务 -- 设备信息gatt服务句柄

tinyfs_dir_t hid_dir;   // hid文件目录结构体变量

// 属性声明(复制例程即可)
static const struct att_decl ls_ble_server_att_decl[BLE_SVC_ATT_NUM] =
{
    // 【UART 接收特征句柄】
    [BLE_SVC_IDX_RX_CHAR] =
    {
        .uuid = att_decl_char_array,  // 属性声明 UUID
        .s.max_len = 0,               // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT, // UUID 长度
        .s.read_indication = 1,       // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,         // 读请求启用
    },
    // 【UART 接收值句柄】
    [BLE_SVC_IDX_RX_VAL] =
    {
        .uuid = ble_rx_char_uuid_128,    // 属性声明 UUID
        .s.max_len = BLE_SVC_RX_MAX_LEN, // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_128BIT,   // UUID 长度
        .s.read_indication = 1,          // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.wr_cmd = 1,           // 写入允许
        .char_prop.wr_req = 1,           // 写请求启用
    },
    // 【UART 发送特征句柄】
    [BLE_SVC_IDX_TX_CHAR] =
    {
        .uuid = att_decl_char_array,  // 属性声明 UUID
        .s.max_len = 0,               // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT, // UUID 长度
        .s.read_indication = 1,       // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,         // 读请求启用
    },
    // 【UART 发送值句柄】
    [BLE_SVC_IDX_TX_VAL] =
    {
        .uuid = ble_tx_char_uuid_128,    // 属性声明 UUID
        .s.max_len = BLE_SVC_TX_MAX_LEN, // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_128BIT,   // UUID 长度
        .s.read_indication = 1,          // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.ntf_en = 1,           // 启用通知
    },
    // 【UART 发送通知配置句柄】
    [BLE_SVC_IDX_TX_NTF_CFG] =
    {
        .uuid = att_desc_client_char_cfg_array, // 属性声明 UUID
        .s.max_len = BLE_SVC_TX_MAX_LEN,                         // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT,           // UUID 长度
        .s.read_indication = 1,                 // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,                   // 读请求启用
        .char_prop.wr_req = 1,                  // 写请求启用
    }
};

// 属性声明完成后进行服务声明(复制例程即可)
static const struct svc_decl ls_ble_server_svc =
{
    .uuid = ble_svc_uuid_128,                           // 服务的 UUID
    .att = (struct att_decl *)ls_ble_server_att_decl,   // 服务中包含的属性(结构体指针)
    .nb_att = BLE_SVC_ATT_NUM,                          // 服务中包含的属性数量
    .uuid_len = UUID_LEN_128BIT,                        // 服务的 UUID 的长度
};

// DIS服务属性声明
static const struct att_decl dis_server_att_decl[DIS_SVC_ATT_NUM] =
{
    [DIS_SVC_IDX_PNP_ID_CHAR] =
    {
        .uuid = att_decl_char_array,
        .s.max_len = 0,
        .s.uuid_len = UUID_LEN_16BIT,
        .s.read_indication = 1, // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,   // // 读请求启用
    },
    [DIS_SVC_IDX_PNP_ID_VAL] =
    {
        .uuid = dis_char_pnpid_uuid,
        .s.max_len = DIS_SVC_PNPID_LEN,
        .s.uuid_len = UUID_LEN_16BIT,
        .s.read_indication = 1, // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,   // // 读请求启用
    },
};
// DIS服务声明
static const struct svc_decl dis_server_svc =
{
    .uuid = dis_svc_uuid,
    .att = (struct att_decl *)dis_server_att_decl,
    .nb_att = DIS_SVC_ATT_NUM,  // 服务中包含的属性数量
    .uuid_len = UUID_LEN_16BIT, // 服务的 UUID 的长度
    .sec_lvl = 1,   // 安全级别 --- 未认证状态
};

// 配对参数设置结构体变量
struct pair_feature feat_param =
{
#if PAIR_Host_Initiative_PASSW
    .iocap = BLE_GAP_IO_CAPS_DISPLAY_ONLY,  // 只显示
#elif PAIR_Host_Initiative_None
    .iocap = BLE_GAP_IO_CAPS_NONE,   // 设置 IO 能力，指示设备的输入输出能力 --- 无输入输出功能
#endif
    .oob = PAIR_OOB_DATA_FLAG,       // 是否支持 Out-of-Band（OOB）数据交换
    .auth = PAIR_AUTHREQ,            // 设置认证方式，指示设备的认证模式
    .key_size = PAIR_KEY_SIZE,       // 表示支持的最大LTK大小
    .ikey_dist = PAIR_INIT_KEY_DIST, // 设置初始密钥分发
    .rkey_dist = PAIR_RESP_KEY_DIST  // 设置响应密钥分发
};

#if PAIR_Host_Initiative_PASSW
// 蓝牙连接密码
struct gap_pin_str passkey =
{
    .pin = PAIR_PASSKEY_NUMBER,
    .str_pad = 0
};
#endif

/*广播相关*/
static uint8_t ble_scan_rsp_data[31];   // 扫描回复数据包数组(这里不需要回复！)
char ble_adv_name[sizeof(BLE_NAME) + 3] = BLE_NAME; // 蓝牙广播名字存储数组(+3是为了后面修改名字时加后缀)
uint8_t ble_mac_addr[6] = {0};  // 蓝牙MAC地址
uint8_t ble_adv_data[28] = {0};    // 广播数据数组


/* Public variables==========================================================*/
BLE_AdvInfo_st BLE_AdvInfo =
{
    .ble_name_count = 0,
    .ble_adv_name_ptr = ble_adv_name,
    .ble_mac_addr_ptr = ble_mac_addr,
    .ble_adv_handle = 0,
    .ble_adv_data_ptr = ble_adv_data
};

BLE_UartInfo_st BLE_UartInfo =
{
    .reply_data = 0,
    .ble_server_mtu = BLE_SERVER_MTU_DFT
};

Bsp_BlueTooth_st Bsp_BlueTooth =
{
    .ble_adv_info_Instance = &BLE_AdvInfo,
    .ble_uart_info_Instance = &BLE_UartInfo,

    .ble_connect_id = BLE_DISCONNECTED_ID,

    .Bsp_BlueTooth_Init = &Bsp_BlueTooth_Init,
    .Bsp_BlueTooth_Start_adv = &Bsp_BlueTooth_Start_adv,
    .Bsp_BlueTooth_Send_Notification = &Bsp_BlueTooth_Send_Notification
};


/**
* @param    None
* @retval   None
* @brief    蓝牙协议栈/服务初始化
**/
static void Bsp_BlueTooth_Init(void)
{
    dev_manager_init(Bsp_BlueTooth_dev_manager_CallBack);   // 初始化 设备管理器回调函数
    gap_manager_init(Bsp_BlueTooth_gap_manager_CallBack);   // 初始化 GAP回调函数
    gatt_manager_init(Bsp_BlueTooth_gatt_manager_CallBack); // 初始化 GATT回调函数
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙回调函数部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    type -> 设备管理器中的事件类型
* @param    *evt -> 设备事件联合体
* @retval   None
* @brief    处理设备管理器的事件回调(包括初始化 BLE 栈、添加服务、注册 GATT 服务、创建广播对象、启动广播等)
**/
static void Bsp_BlueTooth_dev_manager_CallBack(enum dev_evt_type type, union dev_evt_u *evt)
{
    LOG_I_Bsp_BlueTooth("[Device evt:%d]", type);

    switch (type)
    {
    case STACK_INIT: // 【栈初始化事件】
    {
        struct ble_stack_cfg cfg =
        {
            .private_addr = false,
            .controller_privacy = false,
        };
        dev_manager_stack_init(&cfg);
        break;
    }
    case STACK_READY: // 【栈就绪事件】
    {
        /*获取MAC地址*/
        bool type;
        dev_manager_get_identity_bdaddr(Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr, &type);
        LOG_I_Bsp_BlueTooth("type: %d, addr:", type);                                                       // 【调试】
        LOG_HEX_Bsp_BlueTooth(Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr, sizeof(ble_mac_addr)); // 【调试】

        /*接收信号的强度指示*/
        con_rssi_thld_init(Bsp_BlueTooth_hid_con_rssi_judge);

        /*添加服务*/
        dev_manager_add_service((struct svc_decl *)&ls_ble_server_svc);  // 添加蓝牙服务

        /*电池管理*/
        bas_batt_lvl_update(10, 100);                  // 电量更新(参数1：索引 参数2：电量值)

        /*其他用户代码*/
        Bsp_SysTimer.Bsp_SysTimer_Init();             // 系统软件定时器初始化
        Bsp_Key.Bsp_key_Timer_Init();                 // 按键软件定时器初始化
        if (FLAG_true == System_Status.motorcal_mode) // 进入电机校准模式下
        {
            // LED状态
            Bsp_Led.Bsp_Led_EnterMotorCalibration_StatusUpdate_Handler();
        }
        break;
    }
    case SERVICE_ADDED: // 【添加服务事件】
    {
        static uint8_t svc_added_service_cnt = 0;  // 静态变量添加服务顺序计数

        switch (svc_added_service_cnt)
        {
        case 0:
        {
            /*在GATT管理器中注册服务*/
            // 参数1: 服务的启动句柄 参数2: 服务属性数量 参数3：要注册的服务指针
            gatt_manager_svc_register(evt->service_added.start_hdl, BLE_SVC_ATT_NUM, &ls_ble_server_svc_env);

            /*添加服务*/
            dev_manager_add_service((struct svc_decl *)&dis_server_svc);

            svc_added_service_cnt++;
            break;
        }
        case 1:
        {
            /*在GATT管理器中注册服务*/
            gatt_manager_svc_register(evt->service_added.start_hdl, DIS_SVC_ATT_NUM, &dis_server_svc_env);

            /*添加【配置文件服务*/
            struct bas_db_cfg db_cfg =
            {
                .ins_num = 1,
                .ntf_enable[0] = 1,
            };
            dev_manager_prf_bass_server_add(NO_SEC, &db_cfg, sizeof(db_cfg));   // 添加电池配置文件服务
            svc_added_service_cnt++;
            break;
        }
        default:
        {
            svc_added_service_cnt = 0;
            break;
        }
        }
        break;
    }
    case PROFILE_ADDED: // 【添加配置文件事件】
    {
        Bsp_BlueTooth_prf_Added_Handler(&evt->profile_added);   // pre配置文件服务添加处理
        break;
    }
    case ADV_OBJ_CREATED: // 【创建广播对象事件】
    {
        LS_ASSERT(0 == evt->obj_created.status);                                       // 值不为 0，则会触发断言失败
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle = evt->obj_created.handle; // 获取句柄
        if (FLAG_true == System_Status.sys_power_switch)                               // 开机即开始广播
        {
            Bsp_BlueTooth_Start_adv();
        }
        break;
    }
    case ADV_STOPPED: // 【广播停止事件】
    {
        LOG_I_Bsp_BlueTooth("adv stop!");
        break;
    }
    case SCAN_STOPPED: // 【扫描停止事件】
    {
        break;
    }
    default:
        break;
    }
}

/**
* @param    type -> GAP中的事件类型
* @param    *evt -> GAP联合体
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    处理 GAP 回调函数(设备如何发现、连接，以及为用户提供有用的信息)
**/
static void Bsp_BlueTooth_gap_manager_CallBack(enum gap_evt_type type, union gap_evt_u *evt, uint8_t con_idx)
{
    LOG_I_Bsp_BlueTooth("[gap ev: %d]", type);    // 【调试】
    uint16_t ntf_cfg;
    uint16_t len = sizeof(ntf_cfg);
    uint8_t ret;

    switch (type)
    {
    case CONNECTED: // 【连接事件】
    {
        // 开机 && 非按键测试模式 && 非检测电量下
        if((FLAG_true == System_Status.sys_power_switch) && (System_Status.key_test_mode != FLAG_true)
                && System_Status.check_electric != FLAG_true)
        {
            _Led4_Conrol(PIN_SET);
        }
        System_Status.bluetooth = FLAG_true;    // 蓝牙连接信号置1
        Bsp_BlueTooth.ble_connect_id = con_idx; // 存储连接ID
        LOG_I_Bsp_BlueTooth(".......connected!.......");    // 【调试】

        // 从hid_dir文件夹中读取指定键值为 TINYFS_RECORD_KEY1 的数据 读取的数据被存储在 ntf_cfg 变量
        ret = tinyfs_read(hid_dir, TINYFS_RECORD_KEY1, (uint8_t *)&ntf_cfg, &len);  // 文件系统读取操作
        if(ret != TINYFS_NO_ERROR)
        {
            LOG_I_Bsp_BlueTooth("hid tinyfs read:%d", ret);    // 【调试】
        }
        // 参数1：报告通知配置值 参数2：连接实例 参数3：对端设备实例
        hid_ntf_cfg_init(ntf_cfg, con_idx, evt->connected.peer_id); // 报告通知配置
        break;
    }
    case DISCONNECTED: // 【断开连接事件】
    {
        // 开机 && 非按键测试模式 && 非检测电量下
        if((FLAG_true == System_Status.sys_power_switch) && (System_Status.key_test_mode != FLAG_true)
                && System_Status.check_electric != FLAG_true)
        {
            _Led4_Conrol(PIN_RESET);
        }
        System_Status.bluetooth = FLAG_false;    // 蓝牙连接信号置1
        Bsp_BlueTooth.ble_connect_id = BLE_DISCONNECTED_ID;    // 未连接状态

        LOG_I_Bsp_BlueTooth(".......disconnected!.......");    // 【调试】
        // 如果开机则重新打开广播
        if(FLAG_true == System_Status.sys_power_switch)
        {
            Bsp_BlueTooth.Bsp_BlueTooth_Start_adv();
        }
        break;
    }
    case CONN_PARAM_REQ: // 【连接参数请求事件】
    {
        break;
    }
    case CONN_PARAM_UPDATED: // 【连接参数更新事件】
    {
        break;
    }
    case MASTER_PAIR_REQ: // 【主机配对请求事件】
    {
        /*
        在这个从设备的dis这个服务上设置一个安全级别（.sec_lvl = 1,），只有主设备在发现这个服务的时候便会发现自己的安全级别不够
        就会触发主设备发起配对加密这个流程，这个时候从机就会上一个 MASTER_PAIR_REQ 事件，这个时候我们就将发送自己的配对参数以及
        配对的密钥，这样只要主机输入的密钥正确就可以连接上这个设备
        */
        // 参数1：连接设备ID 参数2：是否保存主配对信息 参数3：配对参数设置
        gap_manager_slave_pair_response_send(Bsp_BlueTooth.ble_connect_id, FLAG_true, &feat_param);
#if PAIR_Host_Initiative_PASSW
        gap_manager_passkey_input(con_idx, &passkey);
#endif
        break;
    }
    case ENCRYPT_FAIL:  // 【加密失败事件(即配对失败会触发)】
    {
        // 参数1：断开的设备ID 参数2：断开连接的理由
        gap_manager_disconnect(con_idx, HCI_AUTHENTICATION_FAILURE);    // 断开连接
        // 会断开连接后自动触发 DISCONNECTED事件
        break;
    }
    case SLAVE_SECURITY_REQ: // 【从机安全请求事件】
    {
        break;
    }
    case PAIR_DONE: // 【配对完成事件】
    {
        LOG_I_Bsp_BlueTooth(".....pair done.....");
        break;
    }
    case ENCRYPT_DONE: // 【加密完成事件】
    {
        break;
    }
    case DISPLAY_PASSKEY: // 【显示配对码事件】
    {
        break;
    }
    case REQUEST_PASSKEY: // 【请求配对码事件】
    {
        break;
    }
    case NUMERIC_COMPARE: // 【数字比对事件】
    {
        break;
    }
    case GET_DEV_INFO_DEV_NAME: // 【获取设备信息中的设备名称】
    {
        Bsp_BlueTooth_Gap_GetDeviceName((struct gap_dev_info_dev_name *)evt, con_idx);
        break;
    }
    default:
        break;
    }
}

/**
* @param    type -> GATT中的事件类型
* @param    *evt -> GATT联合体
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    处理 GATT 回调函数(为蓝牙设备间的通信提供了标准的规范和约束)
**/
static void Bsp_BlueTooth_gatt_manager_CallBack(enum gatt_evt_type type, union gatt_evt_u *evt, uint8_t con_idx)
{
    LOG_I_Bsp_BlueTooth("[gatt ev: %d]", type); // 【调试】

    switch (type)
    {
    case SERVER_READ_REQ: // 【APP读请求】
    {
        LOG_I_Bsp_BlueTooth("gatt read req");   // 【调试】
        Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(evt->server_read_req.att_idx, con_idx);
        break;
    }
    case SERVER_WRITE_REQ: // 【APP写请求(在这里进行接收数据)】
    {
        LOG_I_Bsp_BlueTooth("gatt write req");   // 【调试】
        // 参数1： 属性索引 参数2： 连接ID 参数3： 收到的数据长度 参数4： 收到的数据指针
        Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(evt->server_write_req.att_idx, con_idx, evt->server_write_req.length, evt->server_write_req.value);
        break;
    }
    case SERVER_NOTIFICATION_DONE: // 【服务器发送通知完成(即APP发送数据完成)】
    {
        LOG_I_Bsp_BlueTooth("gatt ntf done");   // 【调试】
        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag = FLAG_true;   // 设置状态为发送不忙
        break;
    }
    case MTU_CHANGED_INDICATION: // 【MTU交换指示，适用于客户端和服务器】
    {
        Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu = evt->mtu_changed_ind.mtu;
        LOG_I_Bsp_BlueTooth("gatt mtu: %d", Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu);   // 【调试】
        Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(con_idx); // 更新指定连接的数据包长度
        break;
    }
    case CLIENT_RECV_INDICATION: // 【客户端接收指示】
    {
        LOG_I_Bsp_BlueTooth("clinent recv ind,hdl:%d", evt->client_recv_notify_indicate.handle);   // 【调试】 通知/指示句柄
        break;
    }
    default:
    {
        LOG_I_Bsp_BlueTooth("gatt not handled!");   // 【调试】
        break;
    }
    }
}

/**
* @param    None
* @retval   None
* @brief    HID服务器配置文件的回调函数
**/
static void Bsp_BlueTooth_prf_hid_server_CallBack(enum hid_evt_type type, union hid_evt_u *evt)
{
    uint16_t ntf_cfg;  // 用于存储通知配置的变量
    uint8_t ret;  // 用于存储函数返回值的变量

    switch (type)
    {
    case HID_REPORT_READ: // 【读取HID报告值配置(描述符)】
    {
        evt->read_report_req.length = 0;  // 重置读取请求的长度
        if (APP_HOGPD_REPORT_MAP == evt->read_report_req.type)  // 报告图特征
        {
            evt->read_report_req.value = (uint8_t *)hid_report_map; // 分配读取请求的值
            evt->read_report_req.length = HID_REPORT_MAP_LEN;  // 设置HID报告映射的长度
        }
        break;
    }
    case HID_NTF_CFG: // 【HID通知配置】
    {
        LOG_I_Bsp_BlueTooth("ntf_cfg save 2:%08x", evt->ntf_cfg.value);  // 【调试】记录通知配置

        ntf_cfg = evt->ntf_cfg.value;  // 保存通知配置
        ret = tinyfs_write(hid_dir, TINYFS_RECORD_KEY1, (uint8_t *)&ntf_cfg, sizeof(ntf_cfg)); // 将配置写入文件系统

        if (ret != TINYFS_NO_ERROR)
        {
            LOG_I_Bsp_BlueTooth("hid tinyfs write:%d", ret);  // 【调试】如果写入出错，则记录写入错误
        }
        tinyfs_write_through(); // 立即写入文件系统
        break;
    }
    case HID_NTF_DONE: // 【HID通知完成】
    {
        LOG_I_Bsp_BlueTooth("HID NTF DONE"); // 记录HID通知完成
        break;
    }
    case HID_REPORT_WRITE: // 【APP数据写入】
    {
        LOG_I_Bsp_BlueTooth("HID REPORT WRITE"); // 记录APP数据写入
        break;
    }
    default:
    {
        LOG_I_Bsp_BlueTooth("HID NONE"); // 【调试】
        break;
    }
    }
}

/**
* @param    None
* @retval   None
* @brief    电池服务配置文件的回调函数
**/
static void Bsp_BlueTooth_prf_batt_server_CallBack(enum bass_evt_type type, union bass_evt_u *evt)
{
    // 待添加
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--dev部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    con_rssi
* @param    *init_addr
* @param    init_addr_type
* @retval   rssi大于-70 返回FLAG_true 否则返回FLAG_false
* @brief    RSSI信号强度判断（-90 ~ 0之间）
**/
static bool Bsp_BlueTooth_hid_con_rssi_judge(int8_t con_rssi, const uint8_t *init_addr, uint8_t init_addr_type)
{
    /*
    -50 ~ 0 之间信号强度很好，使用感知好
    -70 ~ -50 之间信号强度好。使用感知略差，但体验上无明显影响
    -70以下 信号就不是太好了，使用上感知就不好
    */
    LOG_I_Bsp_BlueTooth("con_rssi: %d, addr_type: %d", con_rssi, init_addr_type);    // 【调试】
    LOG_HEX_Bsp_BlueTooth(init_addr, BLE_ADDR_LEN);     // 【调试】打印mac地址

    // 只接受rssi大于-70的con_req
    return con_rssi > -70 ? FLAG_true : FLAG_false;
}

/**
* @param    *evt -> 配置文件添加事件结构体指针
* @retval   None
* @brief    pre配置文件服务添加处理
**/
static void Bsp_BlueTooth_prf_Added_Handler(struct profile_added_evt *evt)
{
    LOG_I_Bsp_BlueTooth("profile:%d, start handle:0x%x\n", evt->id, evt->start_hdl);    // 【调试】配置文件的ID + 表示配置文件的起始句柄

    switch (evt->id)
    {
    case PRF_HID:   // 【表示HID（Human Interface Device）配置文件】
    {
        prf_hid_server_callback_init(Bsp_BlueTooth_prf_hid_server_CallBack); // 初始化 HID 服务器的回调函数

        // 创建一个新的目录，并将其添加到嵌入式文件系统中
        uint8_t ret = tinyfs_mkdir(&hid_dir, ROOT_DIR, TINYFS_DIR_NAME);

        if (ret != TINYFS_NO_ERROR)
        {
            LOG_I_Bsp_BlueTooth("hid tinyfs mkdir:%d", ret);
        }

        Bsp_BlueTooth_Create_adv_Obj();   // 创建广播对象
        break;
    }
    case PRF_BASS:  // 【表示电池服务配置文件】
    {
        struct hid_db_cfg db_cfg = {0}; // HID数据库配置结构
        db_cfg.hids_nb = 1; // HID服务数量
        db_cfg.cfg[0].svc_features = HID_KEYBOARD;  // HID 服务支持的功能 --- HID设备作为键盘运行
        db_cfg.cfg[0].report_nb = 1;    // HID信息特征的值
        db_cfg.cfg[0].report_id[0] = 0; // report id
        db_cfg.cfg[0].report_cfg[0] = HID_REPORT_IN;    // HID 服务中每个报告特征支持的功能 --- 该报告是输入报告
        db_cfg.cfg[0].info.bcdHID = 0X0111; // HID 类规范版本号（二进制编码的十进制）
        db_cfg.cfg[0].info.bCountryCode = 0;    // 硬件目标国家/地区
        db_cfg.cfg[0].info.flags = HID_WKUP_FOR_REMOTE | HID_NORM_CONN; // 标志位 --- 通知 HID 设备是否能够向 HID 主机提供唤醒信号 | 通知 HID 设备是否可以正常连接

        /*HID配置文件服务添加*/
        // 参数1：安全级别 参数2：配置HID服务信息的结构变量 参数3：hid_db_cfg 的长度
        dev_manager_prf_hid_server_add(NO_SEC, &db_cfg, sizeof(db_cfg));
        prf_bass_server_callback_init(Bsp_BlueTooth_prf_batt_server_CallBack);  // 初始化 电池服务的回调函数
        break;
    }
    default:
        break;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--GAP相关★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    None
 * @retval   None
 * @brief    GAP 获取设备名字信息
 **/
static void Bsp_BlueTooth_Gap_GetDeviceName(struct gap_dev_info_dev_name *dev_name_ptr, uint8_t con_idx)
{
    LS_ASSERT(dev_name_ptr);
    dev_name_ptr->value = (uint8_t *)APP_HID_DEV_NAME;
    dev_name_ptr->length = APP_HID_DEV_NAME_LEN;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--GATT相关★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    att_idx -> 属性索引
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    蓝牙服务器(APP)读请求处理(即在这里进行回复APP内容)
**/
static void Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(uint8_t att_idx, uint8_t con_idx)
{
    uint16_t handle = 0; // 初始化属性句柄为0

    // 设备信息服务
    if(att_idx == DIS_SVC_IDX_PNP_ID_VAL)
    {
        // 获取DIS服务的属性句柄
        handle = gatt_manager_get_svc_att_handle(&dis_server_svc_env, att_idx);
        // 回复读取请求，数据为APP_DIS_PNP_ID，数据长度为APP_DIS_PNP_ID_LEN字节
        gatt_manager_server_read_req_reply(con_idx, handle, 0, (void *)APP_DIS_PNP_ID, APP_DIS_PNP_ID_LEN);
    }
    // 发送通知配置
    else if(att_idx == BLE_SVC_IDX_TX_NTF_CFG)
    {
        // 获取BLE服务的属性句柄
        handle = gatt_manager_get_svc_att_handle(&ls_ble_server_svc_env, att_idx);
        // 回复读取请求 数据长度为2字节(uint16_t类型)
        gatt_manager_server_read_req_reply(con_idx, handle, 0, (void *)&Bsp_BlueTooth.ble_uart_info_Instance->reply_data, 2);
    }
    LOG_I_Bsp_BlueTooth("gatt att_idx: %d", att_idx);    // 【调试】属性索引
}

/**
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    更新指定连接的数据包长度
**/
static void Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(uint8_t con_idx)
{
    // 定义一个结构体变量 dlu_param，用于设置数据包大小
    struct gap_set_pkt_size dlu_param =
    {
        .pkt_size = 251, // 设置数据包大小为251字节
    };
    // 调用 gap_manager_set_pkt_size 函数设置连接索引为 con_idx 的数据包大小为 251 字节
    gap_manager_set_pkt_size(con_idx, &dlu_param);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--广播部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/**
* @param    None
* @retval   None
* @brief    开始广播
**/
static void Bsp_BlueTooth_Start_adv(void)
{
    uint16_t uuid_value = GATT_UUID_HID;
    uint16_t ble_appearance = GAP_APPEARANCE_HID_MOUSE;

    LS_ASSERT(Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle != 0xff); // 断言不等于0xff，如果等于则会触发断言错误

    /* 发送广播数据 */
    uint8_t adv_data_length = ADV_DATA_PACK(
                                  Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_data_ptr,         // 广播数据存储的数组
                                  3,                                                             // 广播数据项的数量
                                  GAP_ADV_TYPE_SHORTENED_NAME,                                   // 广播类型---缩短的蓝牙名称
                                  Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_name_ptr,         // 广播数据---蓝牙名称存储数组
                                  sizeof(ble_adv_name),                                          // 广播数据的长度(此处不能用指针)
                                  GAP_ADV_TYPE_COMPLETE_LIST_16_BIT_UUID,                        // 广播类型---16位UUID完整列表
                                  &uuid_value,                                                   // 广播数据---HID人机接口设备UUID
                                  sizeof(uuid_value),                                            // 广播数据的长度
                                  GAP_ADV_TYPE_APPEARANCE,                                       // // 广播类型：外观
                                  &ble_appearance,                                               // 广播数据---外观UUID
                                  sizeof(ble_appearance)                                         // 广播数据的长度
                              );
    // 开始广播  参数1：广播句柄 参数2：广播数据 参数3：广播数据长度 参数4：响应数据包 参数5：响应数据包长度(如果没有advertising_data或scan_response_data，对应的length需要填0。不可以填如与实际内容不匹配的length)
    dev_manager_start_adv(Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle, Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_data_ptr, adv_data_length, ble_scan_rsp_data, 0);
    LOG_I_Bsp_BlueTooth("adv start");
}

/**
* @param    None
* @retval   None
* @brief    创建广播对象
**/
static void Bsp_BlueTooth_Create_adv_Obj(void)
{
    // 定义结构体 legacy_adv_obj_param 的实例 adv_param，并初始化成员变量
    struct legacy_adv_obj_param adv_param =
    {
        // 广播间隔的最小值(一般跟max配置成同一个值)，单位为 0.625ms（0.625 * 32 = 20ms）
        .adv_intv_min = 0x20,
        // 广播间隔的最大值，单位为 0.625ms
        .adv_intv_max = 0x20,
        // 广播发送设备的地址类型(蓝牙MAC地址)---可以是公有地址或随机静态地址
        .own_addr_type = PUBLIC_OR_RANDOM_STATIC_ADDR,
        // 广播过滤策略，此处为不进行过滤
        .filter_policy = 0,
        // 广播信道映射，指定广播数据发送的信道---Bit0:启用37通道 Bit1:启用38通道 Bit2:启用39通道(7表示3个通道上都发送)
        .ch_map = 0x7,
        // 广播模式，此处为通用可发现模式，即不针对特定设备进行广告
        .disc_mode = ADV_MODE_GEN_DISC,
        // 广播属性，包括是否可连接、是否可扫描、是否定向广告、是否高占空比
        .prop = {
            // 可连接属性，是否被连接
            .connectable = 1,
            // 可扫描属性，是否可被其他设备扫描到
            .scannable = 1,
            // 定向属性，不定向
            .directed = 0,
            // 高占空比属性
            .high_duty_cycle = 0,
        },
    };
    // 创建广播对象
    dev_manager_create_legacy_adv_object(&adv_param);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--串口部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    None
* @retval   None
* @brief    蓝牙发送通知(数据)给客户端
**/
static void Bsp_BlueTooth_Send_Notification(void)
{
    // 串口发送缓存有数据且发送缓冲不忙
    if ((Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index > 0) && (FLAG_true == Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag))
    {
        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag = FLAG_false;    // 正在进行服务器通知(忙状态)

        uint16_t handle = gatt_manager_get_svc_att_handle(&ls_ble_server_svc_env, BLE_SVC_IDX_TX_VAL);  // 获取发送数据服务句柄
        // 判断是否超过最大发送长度  co_min->返回两个数最小一个
        uint16_t tx_len = Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index > co_min(BLE_SERVER_MAX_DATA_LEN, BLE_SVC_TX_MAX_LEN) ? co_min(BLE_SERVER_MAX_DATA_LEN, BLE_SVC_TX_MAX_LEN) : Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index;

        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index -= tx_len;
        // 发送数据到客户端（UART）
        gatt_manager_server_send_notification(Bsp_BlueTooth.ble_connect_id, handle, Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr, tx_len, NULL);
        // 将没发送的数据向左偏移
        memcpy((void *)&Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr[0], (void *)&Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr[tx_len], Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index);
    }
}

/**
* @param    att_idx -> 属性索引
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @param    length -> 收到的数据长度
* @param    *value -> 收到的数据指针
* @retval   None
* @brief    蓝牙服务器(APP)写请求处理(即在这里进行接收APP发送过来的内容)
**/
static void Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(uint8_t att_idx, uint8_t con_idx, uint16_t length, uint8_t const *value)
{
    if (att_idx == BLE_SVC_IDX_RX_VAL)  // 是接收属性则
    {
        if (FLAG_false == Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag)   // 接收忙
        {
            LOG_I_Bsp_BlueTooth("tx busy, data discard!");
        }
        else
        {
            Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag = FLAG_false;    // 设置状态为接收忙

            // 如果蓝牙接收数据大于蓝牙接收缓冲,则需要分批存储
            if ((Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index + length) > BLE_SVC_BUFFER_SIZE)
            {
                // 存入底部(即缓冲区剩余可存储的部分)
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index], (uint8_t *)value, BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index);
                // 覆盖顶部
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[0], (uint8_t *)(value + BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index), length - (BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index));

                // 开机 或者 电机校准模式下
                if ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode))
                {
                    if (FLAG_true == System_Status.update_mode) // 升级模式
                    {
                        Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                    else
                    {
                        Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                }
                Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index = length - (BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index);
            }
            else
            {
                // 直接写入缓存
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index], (uint8_t *)value, length);

                if ((FLAG_true == System_Status.sys_power_switch) || (FLAG_true == System_Status.motorcal_mode))
                {
                    if (FLAG_true == System_Status.update_mode) // 升级模式
                    {
                        Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                    else
                    {
                        Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                }
                Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index += length;
            }

            Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag = FLAG_true;    // 设置状态为接收空闲
        }
    }
    else if (att_idx == BLE_SVC_IDX_TX_NTF_CFG) // 发送通知配置
    {
        LS_ASSERT(2 == length);
        memcpy(&Bsp_BlueTooth.ble_uart_info_Instance->reply_data, value, length);
    }
}