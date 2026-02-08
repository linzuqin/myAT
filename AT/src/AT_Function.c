#include "AT_Function.h"
#include "cmsis_os.h"
#include "AT_MQTT.h"
#include "AT_HTTP.h"

static char at_msg_buf[AT_MSG_SIZE];
extern char *version;

AT_STATUS_t at_status[AT_STATUS_NUM] =
{
    [0] = {.status = AT_HW_INIT, .callback = AT_HW_INIT_CallBack},                  // AT硬件初始化
    [1] = {.status = AT_REGISTER, .callback = AT_REGISTER_CallBack},                // AT设备注册
    [2] = {.status = AT_INIT, .callback = AT_INIT_CallBack},                        // AT指令初始化
    [3] = {.status = AT_IDLE, .callback = AT_IDLE_CallBack},                        // AT空闲状态 解析平台下发的指令
    [4] = {.status = AT_HTTP_OTA_IDLE, .callback = AT_HTTP_OTA_IDLE_CallBack},                    
    [5] = {.status = AT_HTTP_OTA_DOWNLOAD, .callback = AT_HTTP_OTA_DOWNLOAD_CallBack},                      
    [6] = {.status = AT_HTTP_OTA_DOWNLOAD_FINISH, .callback = AT_HTTP_OTA_DOWNLOAD_FINISH_CallBack},                      
    [7] = {.status = AT_HTTP_OTA_DOWNLOAD_FAIL, .callback = AT_HTTP_OTA_DOWNLOAD_FAIL_CallBack},                       

};

AT_upload_t at_upload = 
{
	.refreshFlag = 0,
	.ackFlag = 0,
};

AT_Device_t AT_Device = {
    .CMD_TABLE = NULL,
    .URC_TABLE = NULL,
    .msg_buf = at_msg_buf,
    .init_step = 0,
    .status = AT_HW_INIT,
    .wifi_params = 
    {
        .WiFi_SSID = DEFAULT_WIFI_SSID,
        .WiFi_Password = DEFAULT_WIFI_PWD
    },
    .recv_flag = 0,
};

ota_packet_info_t ota_packet_info = 
{
	.signal_download_size = 512,
	
};
/********************/

/*at部分延时*/
void at_delay_ms(uint32_t delay_time)
{
    osDelay(delay_time);
}

/*at数据发送*/
void net_send(uint8_t *buf, uint16_t size)
{
    HAL_UART_Transmit(uart_devices[AT_UART_DEVICE].uartHandle , buf , size , 1000);
}

/*将数据从串口缓冲区复制到at缓冲区*/
void at_get_msg(uint8_t *data_buf , uint16_t buf_size)
{
    uint16_t start_addr = strlen(AT_Device.msg_buf);
    if(start_addr + buf_size < AT_MSG_SIZE)
    {
        memcpy(&AT_Device.msg_buf[start_addr] , data_buf , buf_size);
        AT_Device.recv_flag = 1;
    }
    else
    {
        // 超出缓冲区大小
        memset(AT_Device.msg_buf, 0, AT_MSG_SIZE);
        memcpy(AT_Device.msg_buf , data_buf , buf_size < AT_MSG_SIZE ? buf_size : AT_MSG_SIZE);
        AT_Device.recv_flag = 1;
    }
    memset(data_buf , 0 , buf_size);
}

/*清空相关的数组*/
void at_clear(void)
{
    memset(AT_Device.msg_buf , 0 , AT_MSG_SIZE);
    AT_Device.recv_flag = 0;
}

at_err_t AT_SendCmd(const char *cmd, const char *response, uint16_t timeout, uint8_t *data_buf)
{
    // 清空接收缓冲区
    at_clear();

    // 发送命令到串口（确保以\r\n结尾）
    char full_cmd[256];
    memset(full_cmd, 0, 256);
    snprintf(full_cmd, sizeof(full_cmd), "%s", cmd);
    net_send((uint8_t *)full_cmd, strlen(full_cmd));

    // 等待应答
    uint8_t timeout_count = 0;
    while (1)
    {
        at_delay_ms(timeout / 5);

        if(AT_Device.recv_flag == 1)
        {
            if(strstr((char *)AT_Device.msg_buf, response) != NULL)
						{
							if (data_buf != NULL)
							{
									memcpy(data_buf, AT_Device.msg_buf, AT_MSG_SIZE);
							}
							at_clear();
							return AT_CMD_OK; // 命令执行成功
						}
        }
        if(timeout_count ++ > 5)
        {
            return AT_ERR_TIMEOUT; // 命令执行失败或超时
        }
    }
}

/*指令集*/
AT_CMD_t AT_Cmd_table[AT_COMMAND_ARRAY_SIZE] = {
    {"AT\r\n", "OK", 1000, NULL},
//    {"AT+RST\r\n", "OK", 5000, NULL},
    {"AT+CWMODE=1\r\n", "OK", 1000, NULL},
    {"AT+CWDHCP=1,1\r\n", "OK", 1000, NULL},
    {"AT+MQTTCLEAN=0\r\n", "ERROR", 1000, NULL}, // 返回error说明当前无mqtt链接 可以进行新的连接
};

AT_URC_t AT_URC_table[AT_COMMAND_ARRAY_SIZE] = {
    {"+MQTTDISCONNECTED", MQTTDISCONNECTED_CallBack},
    {"+MQTTSUBRECV:", mqtt_parse},
//    {"ERROR", MQTTDISCONNECTED_CallBack},
};

static void at_device_register(AT_Device_t *at_device, AT_CMD_t *cmd_table, AT_URC_t *urc_table)
{
    if (!at_device)
        return;

    at_device->CMD_TABLE = cmd_table ? cmd_table : AT_Cmd_table;
    at_device->URC_TABLE = urc_table ? urc_table : AT_URC_table;
    at_device->init_step = 0;
}

uint8_t AT_Cmd_Register(AT_Device_t *at_device, const char *response,uint16_t timeout, void (*callback_response)(void),int insert_count, const char *cmd, ...)
{
    if (!at_device || !cmd || !response)
    {
        //        LOG_E("Invalid parameters");
        return AT_ERR_INVALID_PARAM;
    }

    if (!at_device->CMD_TABLE)
    {
        //        LOG_E("CMD_TABLE not initialized");
        return AT_ERR_TABLE_NOT_INIT;
    }

    if (insert_count >= 0 && insert_count < AT_COMMAND_ARRAY_SIZE) 
    {
        if (at_device->CMD_TABLE[insert_count].cmd) 
        {
            free((void*)at_device->CMD_TABLE[insert_count].cmd);
        }
    }


    char cmd_buf[256];
    va_list ap;
    va_start(ap, cmd);
    vsnprintf(cmd_buf, sizeof(cmd_buf), cmd, ap);
    va_end(ap);

    // LOG_I("Registering command: %s", cmd_buf);

    int slot = -1;

    if (insert_count >= 0 && insert_count < AT_COMMAND_ARRAY_SIZE)
    {
        slot = insert_count;
    }
    else
    {
        for (int i = 0; i < AT_COMMAND_ARRAY_SIZE; i++)
        {
            if (at_device->CMD_TABLE[i].cmd == NULL)
            {
                slot = i;
                break;
            }
        }
    }

    if (slot == -1)
    {
        return AT_ERR_TABLE_FULL;
    }

    size_t cmd_len = strlen(cmd_buf) + 1;
    char *cmd_copy = (char *)malloc(cmd_len);
    if (!cmd_copy)
    {
        return AT_ERR_MEMORY;
    }
    strncpy(cmd_copy, cmd_buf, cmd_len);

    if (at_device->CMD_TABLE[slot].cmd)
    {
        free((char *)at_device->CMD_TABLE[slot].cmd);
    }

    at_device->CMD_TABLE[slot] = (AT_CMD_t){
        .cmd = cmd_copy,
        .response = response,
        .timeout = timeout,
        .callback_response = callback_response};

    return AT_CMD_ADD_SUCCESS;
}

void AT_HW_INIT_CallBack(void *device , void *arg2)
{
    AT_Device_t *at_device = device;
    // 初始化UART

    at_device->status = AT_REGISTER;
}

void AT_INIT_CallBack(void *device , void *arg2)
{
    AT_Device_t *at_device = device;

    if (at_device->init_step >= AT_COMMAND_ARRAY_SIZE)
    {
        at_device->init_step = 0;
        at_device->status = AT_IDLE;
        return;
    }
    else if (at_device->CMD_TABLE[at_device->init_step].cmd == NULL)
    {
        at_device->init_step = 0;
        at_device->status = AT_IDLE;
        return;
    }
    else
    {
        if (AT_SendCmd(at_device->CMD_TABLE[at_device->init_step].cmd, at_device->CMD_TABLE[at_device->init_step].response, at_device->CMD_TABLE[at_device->init_step].timeout, NULL) == AT_CMD_OK)
        {
            if (at_device->CMD_TABLE[at_device->init_step].callback_response != NULL)
            {
                at_device->CMD_TABLE[at_device->init_step].callback_response();
            }
            at_device->init_step++;
        }
        else
        {
            //					at_device->init_step = 0;
        }
    }
}

void AT_REGISTER_CallBack(void *device , void *arg2)
{
    AT_Device_t *at_device = device;

    // 注册at设备
    at_device_register(at_device, NULL, NULL);

    // 添加at mqtt指令
    mqtt_cmd_init(at_device);

    // 进入at初始化状态
    at_device->status = AT_INIT;
}

void AT_IDLE_CallBack(void *device , void *arg2)
{
    AT_Device_t *at_device = device;

    if ( at_device->recv_flag == 0 )
    return;

    at_device->recv_flag = 0;

    for (uint16_t i = 0; i < AT_COMMAND_ARRAY_SIZE && at_device->URC_TABLE[i].urc_msg; i++)
    {
        char *urc_msg = strstr((char *)at_device->msg_buf, at_device->URC_TABLE[i].urc_msg);
        if (urc_msg != NULL)
        {
            if (at_device->URC_TABLE[i].callback)
            {
                at_device->URC_TABLE[i].callback(at_device, urc_msg);
            }
            break;
        }
    }
    at_clear();
    if(at_device->status < AT_IDLE ) //若当前状态排在IDLE之前,说明为空闲状态,则保持原状态
    {

    }
    else
    {
        at_device->status = AT_IDLE;
    }
}

void AT_HTTP_OTA_IDLE_CallBack(void *device , void *info)
{
	AT_Device_t *at_dev = device;
	ota_packet_info_t *ota_info = info;
	if(ota_check_update(ota_info , at_dev->mqtt_params.Product_ID , at_dev->mqtt_params.Device_Name , 1 , version , OTA_TOKEN) == AT_CMD_OK)//解析到当前存在下载任务
	{
			at_dev->status = AT_HTTP_OTA_DOWNLOAD;
	}
}

void AT_HTTP_OTA_DOWNLOAD_CallBack(void *device , void *info)
{
	AT_Device_t *at_dev = device;
	ota_packet_info_t *ota_info = info;
	
	uint8_t *download_buf = (uint8_t *)malloc(ota_info->packet_size);
	if(download_buf == NULL)return;
	if(ota_download_packet(ota_info , at_dev->mqtt_params.Product_ID  , at_dev->mqtt_params.Device_Name , OTA_TOKEN , download_buf , ota_info->packet_size))
	{
			//下载成功 执行保存操作

		if(ota_info->download_count >= ota_info->download_total_count)
		{
			at_dev->status = AT_HTTP_OTA_DOWNLOAD_FINISH;
		}
	}
}

void AT_HTTP_OTA_DOWNLOAD_FINISH_CallBack(void *device , void *info)
{
	
}

void AT_HTTP_OTA_DOWNLOAD_FAIL_CallBack(void *device , void *info)
{
	
}

void AT_poll(void) 
{    
    AT_STATUS_TYPE_t index_status = AT_Device.status;

    if (at_status[index_status].callback == NULL) 
    {
        return;
    }
    at_status[index_status].callback(&AT_Device , &ota_packet_info);
}

void AT_upload_poll(void)
{
    if(at_upload.ackFlag == 1)
    {
        if(set_ack(at_upload.ackId , at_upload.ackCode , at_upload.ackMsg) == AT_MQTT_SEND_SUCCESS)
        {
            at_upload.ackFlag = 0;
            AT_Device.status = AT_IDLE;
        }
    }
    else if(at_upload.refreshFlag == 1)
    {
        if(mqtt_refresh() == AT_MQTT_SEND_SUCCESS)
        {
            at_upload.refreshFlag = 0;
            AT_Device.status = AT_IDLE;
        }
    }
}

//at的定时回调函数,状态检测及定时上报的计时均在此函数实现,需要实现对应功能将此函数放在1s的定时中断里
void at_time_callback(void)
{
    static uint32_t at_count = 0;
    static uint32_t reporting_interval = 0;

    if(AT_Device.status != AT_IDLE)
    {
        at_count ++;
        if(at_count > 30)
        {
            AT_Device.status = AT_INIT;
            at_count = 0;
        }
    }
    else
    {
        at_count = 0;
        reporting_interval ++;
        if(reporting_interval % AT_Device.mqtt_params.reporting_interval == 0 && reporting_interval != 0)
        {
            at_upload.refreshFlag = 1;
            reporting_interval = 0;
        }
    }
}
