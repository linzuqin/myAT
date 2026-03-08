#include "AT_Function.h"
#include "AT_MQTT.h"
#include "AT_HTTP.h"
#include "AT_RingBuf.h"
#include "main.h"
#include "usart.h"

#define WAIT_RETRY	10

// 状态超时时间(秒)
#define AT_STATE_TIMEOUT_SECONDS 30

static char at_msg_buf[AT_MSG_SIZE];
extern char *version;
static uint8_t at_ring_buf[AT_RING_BUF_SIZE];

AT_STATUS_t at_status[AT_STATUS_NUM] =
{
    [0] = {.status = AT_HW_INIT, .callback = AT_HW_INIT_CallBack},   // AT硬件初始化
    [1] = {.status = AT_REGISTER, .callback = AT_REGISTER_CallBack}, // AT设备注册
    [2] = {.status = AT_INIT, .callback = AT_INIT_CallBack},         // AT指令初始化
    [3] = {.status = AT_IDLE, .callback = AT_IDLE_CallBack},         // AT空闲状态 解析平台下发的指令
};

AT_upload_t at_upload =
{
    .refreshFlag = 0,
    .ackFlag = 0,
};

AT_Device_t AT_Device = {
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

/*指令集*/
AT_CMD_t AT_Cmd_table[AT_COMMAND_ARRAY_SIZE] = {
    {"ATE0\r\n", "OK", 1000, NULL},
    {"AT+CWMODE=1\r\n", "OK", 1000, NULL},
    {"AT+CWDHCP=1,1\r\n", "OK", 1000, NULL},
    {"AT+MQTTCLEAN=0\r\n", "ERROR", 1000, NULL}, // 返回error说明当前无mqtt链接 可以进行新的连接
};

AT_URC_t AT_URC_table[AT_COMMAND_ARRAY_SIZE] = {
    {"+MQTTDISCONNECTED", MQTTDISCONNECTED_CallBack},
    {"+MQTTSUBRECV:", mqtt_parse},
};

const char *resp_table[AT_COMMAND_ARRAY_SIZE] = {
    "OK",
    "ERROR"
};

/*at部分延时*/
static void at_delay_ms(uint32_t delay_time)
{
    // 实现延时函数
	// HAL_Delay(delay_time);
}

/*at数据发送*/
static void net_send(uint8_t *buf, uint16_t size)
{
    // HAL_UART_Transmit(&huart2 , buf , size , 1000);
}

//判断数据类型
static AT_MSG_TYPE_t at_get_msg_type(const char *msg)
{
    AT_MSG_TYPE_t result = AT_MSG_TYPE_ERROR;
    uint8_t i = 0;
    for(i = 0; i < AT_COMMAND_ARRAY_SIZE; i++)
    {
        if(strstr(msg, resp_table[i]) != NULL)
        {
            result = AT_MSG_TYPE;
            break;
        }
        else if(strstr(msg, AT_URC_table[i].urc_msg) != NULL)
        {
            result = AT_MSG_TYPE_URC;
            break;
        }
    }
    return result;
}

//将数据从环形缓冲区中读出保存至at的缓冲区中
static void at_get_msg_from_ringbuf(AT_Device_t *at_device)
{
    AT_MSG_TYPE_t msg_type = AT_MSG_TYPE_ERROR;
    char *end = NULL;
		uint32_t i = 0 , len = 0;
	
    ringbuf_read(&AT_RingBuf, (uint8_t *)at_device->msg_buf, AT_MSG_SIZE);
    end = strstr(at_device->msg_buf, "\r\n"); //这里获取以\r\n结尾的行
    if(end == NULL)
    {
        return;
    }

    len = end - at_device->msg_buf + 2; //获取单条指令的长度 来作为实际读取的长度

    at_device->msg_buf[len] = '\0'; // 确保字符串以空字符结束
    ringbuf_update_read_ptr(&AT_RingBuf, len);//读取后更新缓冲区

    msg_type = at_get_msg_type(at_device->msg_buf);
    if(msg_type == AT_MSG_TYPE_ERROR)
    {
        return;
    }

    if(msg_type == AT_MSG_TYPE)
    {
        
    }
    else if(msg_type == AT_MSG_TYPE_URC)
    {
        for(i = 0; i < AT_COMMAND_ARRAY_SIZE; i++)
        {
            if(strstr(at_device->msg_buf , AT_URC_table[i].urc_msg) != NULL)
            {
                if (AT_URC_table[i].callback!= NULL)
                {
                    AT_URC_table[i].callback(at_device, at_device->msg_buf);
                }
                break;
            }
        }
    }

    if(ringbuf_is_empty(&AT_RingBuf) != 1)//检查环形缓冲区是否为空 如果不为空 下个循环继续处理
    {
        at_device->recv_flag = 1;
        return;
    }
}

/*将数据从串口缓冲区复制到at缓冲区*/
void at_get_msg(uint8_t *data_buf, uint16_t buf_size)
{
    if(ringbuf_write(&AT_RingBuf, data_buf, buf_size) == buf_size)
    {
//        ringbuf_update_write_ptr(&AT_RingBuf, buf_size);
        AT_Device.recv_flag = 1;
    }
}

/*清空相关的数组*/
void at_clear(void)
{
    memset(AT_Device.msg_buf, 0, AT_MSG_SIZE);
//    AT_Device.recv_flag = 0;
	
//	ringbuf_reset(&AT_RingBuf);

}

at_err_t AT_SendCmd(const char *cmd, const char *response, uint16_t timeout, uint8_t *data_buf)
{
	at_err_t result = AT_CMD_OK;
    uint8_t retry_count = 0;
    const uint8_t max_retry = 10;
	
    // 清空接收缓冲区
    at_clear();

    // 发送命令到串口
    net_send((uint8_t *)cmd, strlen(cmd));

    while (retry_count < WAIT_RETRY)
    {
        // 检查是否有数据收到
        if (AT_Device.recv_flag == 1)
        {
			at_get_msg_from_ringbuf(&AT_Device);

            // 检查是否包含预期响应
            if (strstr((char *)AT_Device.msg_buf, response) != NULL)
            {
                if (data_buf != NULL)
                {
                    memcpy(data_buf, AT_Device.msg_buf, AT_MSG_SIZE);
                }
                at_clear();
                result = AT_CMD_OK; // 命令执行成功
								break;
            }
            else
            {
                // 收到了其他数据，清空缓冲区继续等待
                at_clear();
            }
        }
        
        // 延时一小段时间
        at_delay_ms(timeout/WAIT_RETRY);
        
        // 检查是否超时
        retry_count++;
        if (retry_count >= max_retry)
        {
		    result = AT_ERR_TIMEOUT;
            break;
        }
    }
    
    at_clear();
    return result; // 命令执行失败或超时
}

static void at_device_register(AT_Device_t *at_device, AT_CMD_t *cmd_table, AT_URC_t *urc_table)
{
    if (!at_device)
        return;
    at_clear();
    at_device->init_step = 0;
}

at_err_t AT_Cmd_Register(const char *response, uint16_t timeout, void (*callback_response)(void), int insert_count, const char *cmd, ...)
{
    if (!cmd || !response)
    {
        return AT_ERR_INVALID_PARAM;
    }

    // 格式化命令字符串
    char cmd_buf[256];
    va_list ap;
    va_start(ap, cmd);
    int len = vsnprintf(cmd_buf, sizeof(cmd_buf), cmd, ap);
    va_end(ap);
    
    if (len < 0 || len >= sizeof(cmd_buf))
    {
        return AT_ERR_INVALID_PARAM; // 格式化失败或缓冲区溢出
    }

    // 确定插入位置
    int slot;
    if (insert_count >= 0 && insert_count < AT_COMMAND_ARRAY_SIZE)
    {
        slot = insert_count;
    }
    else
    {
        // 查找空闲槽位
        for (slot = 0; slot < AT_COMMAND_ARRAY_SIZE; slot++)
        {
            if (AT_Cmd_table[slot].cmd == NULL)
            {
                break;
            }
        }
        
        if (slot >= AT_COMMAND_ARRAY_SIZE)
        {
            return AT_ERR_MEMORY;
        }
    }

    // 分配内存并复制命令
    char *cmd_copy = (char *)malloc(len + 1);
    if (!cmd_copy)
    {
        return AT_ERR_MEMORY;
    }
    memcpy(cmd_copy, cmd_buf, len + 1);

    // 释放旧命令内存（如果存在）
    if (AT_Cmd_table[slot].cmd)
    {
        free((char *)AT_Cmd_table[slot].cmd);
    }

    // 注册新命令
    AT_Cmd_table[slot] = (AT_CMD_t){
        .cmd = cmd_copy,
        .response = response,
        .timeout = timeout,
        .callback_response = callback_response
    };

    return AT_CMD_ADD_SUCCESS;
}

void AT_HW_INIT_CallBack(void *device, void *arg2)
{
    AT_Device_t *at_device = device;
    // 初始化UART

    at_device->status = AT_REGISTER;
	
		ringbuf_init(&AT_RingBuf , at_ring_buf , AT_RING_BUF_SIZE);
}

void AT_INIT_CallBack(void *device, void *arg2)
{
    AT_Device_t *at_device = device;

    if (at_device->init_step >= AT_COMMAND_ARRAY_SIZE)
    {
        at_device->init_step = 0;
        at_device->status = AT_IDLE;
        return;
    }
    else if (AT_Cmd_table[at_device->init_step].cmd == NULL)
    {
        at_device->init_step = 0;
        at_device->status = AT_IDLE;
        return;
    }
    else
    {
        if (AT_SendCmd(AT_Cmd_table[at_device->init_step].cmd, AT_Cmd_table[at_device->init_step].response, AT_Cmd_table[at_device->init_step].timeout, NULL) == AT_CMD_OK)
        {
            if (AT_Cmd_table[at_device->init_step].callback_response != NULL)
            {
                AT_Cmd_table[at_device->init_step].callback_response();
            }
            at_device->init_step++;
						ringbuf_reset(&AT_RingBuf);
        }
        else
        {
            //					at_device->init_step = 0;
        }
    }
}

void AT_REGISTER_CallBack(void *device, void *arg2)
{
    AT_Device_t *at_device = device;

    // 注册at设备
    at_device_register(at_device, NULL, NULL);

    // 添加at mqtt指令
    mqtt_cmd_init(at_device);

    // 进入at初始化状态
    at_device->status = AT_INIT;
}

void AT_IDLE_CallBack(void *device, void *arg2)
{
    AT_Device_t *at_device = device;

    if (at_device->recv_flag == 0)
    {
		ringbuf_reset(&AT_RingBuf);
        return;
    }
    else
    {
        at_device->recv_flag = 0;
    }

    at_get_msg_from_ringbuf(at_device); //从环形缓冲区中读取数据到at的缓冲区中 
    
    at_clear();
    if (at_device->status < AT_IDLE) // 若当前状态排在IDLE之前,说明为空闲状态,则保持原状态
    {

    }
    else
    {
        at_device->status = AT_IDLE;
    }
}

// 状态机轮询函数 主要负责解析数据与初始化
void AT_poll(void)
{
    AT_STATUS_TYPE_t current_status = AT_Device.status;

    if (current_status >= AT_STATUS_MAX)
    {
        AT_Device.status = AT_HW_INIT;
        return;
    }

    AT_STATUS_t *status_entry = &at_status[current_status];
    if (status_entry->callback != NULL)
    {
        status_entry->callback(&AT_Device, NULL);
    }
}

// 数据上传轮询函数 主要负责执行上报
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

// at的定时回调函数,状态检测及定时上报的计时均在此函数实现,需要实现对应功能将此函数放在1s的定时中断里
void at_time_callback(void)
{
    static uint32_t state_timeout_count = 0;
    static uint32_t reporting_count = 0;

    if (AT_Device.status < AT_IDLE)
    {
        // 非空闲状态超时检测
        if (++state_timeout_count >= AT_STATE_TIMEOUT_SECONDS)
        {
            AT_Device.status = AT_INIT;
            state_timeout_count = 0;
        }
    }
    else
    {
        // 空闲状态下的定时上报
        state_timeout_count = 0;
        
        uint16_t interval = AT_Device.mqtt_params.reporting_interval;
        if (interval > 0 && ++reporting_count >= interval)
        {
            at_upload.refreshFlag = 1;
            reporting_count = 0;
        }
    }
}
