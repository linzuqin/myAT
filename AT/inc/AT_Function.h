#ifndef _AT_FUNCTION_H_
#define _AT_FUNCTION_H_

/*add user lib*/
#include "myusart.h"
#include "cJSON.h"
#include "onenet.h"

/*add c lib*/
//#include "string.h"
//#include "stdio.h"
//#include "stdlib.h"
//#include "math.h"
//#include <stdint.h>
//#include <stdbool.h>
//#include "math.h"
//#include <stdarg.h>
//#include "time.h"

/*esp8266参数定义*/
#define AT_UART_DEVICE 2

#define WIFI_SSID "main"

#define WIFI_PASSWORD "12345678"

#define MQTT_KEEP_ALIVE 30

#define NTP_SERVER "time.windows.com"

#define DEFAULT_WIFI_SSID 					WIFI_SSID // 默认WiFi SSID
#define DEFAULT_WIFI_PWD  					WIFI_PASSWORD // 默认WiFi密码

#define IP_ADDRESS 									"mqtts.heclouds.com"
#define PORT_NUMBER 								1883
#define PRODUCT_ID 									"2Its5wq8a3"
#define DEVICE_NAME 								"lot_device"
#define MY_SECRET_KEY               "WmhibHUzS0JqeTMwTHlEVGVSM2p5dmlpcVRUbUdjeU0="
#define TOKEN												"version=2018-10-31&res=products%2F2Its5wq8a3&et=1956499200&method=md5&sign=7sklBTPpND6BDv%2BD1VZxWQ%3D%3D"
#define OTA_TOKEN										"vversion=2022-05-01&res=products%2F2Its5wq8a3&et=1956499200&method=md5&sign=%2BLyzuErgEXieVgmL%2FPZ%2B1g%3D%3D"

#define POST_TOPIC		    					"$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/post"
#define SET_TOPIC_ALL     					"$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/#"
#define OTA_TOPIC_GET     					"$sys/"PRODUCT_ID"/"DEVICE_NAME"/ota/inform"
#define OTA_TOPIC_ACK     					"$sys/"PRODUCT_ID"/"DEVICE_NAME"/ota/inform_reply"


#define SET_TOPIC          				 	"$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set"
#define SET_ACK_TOPIC       				"$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set_reply"

/*wifi模块的复位引脚*/
#define AT_RST_PORT     GPIOC
#define AT_RST_PIN      GPIO_Pin_5
/****************************************************************************************************/

#define AT_COMMAND_ARRAY_SIZE 16
#define AT_MSG_SIZE 1024                       //AT设备消息缓冲区大小

#define MAX_JSON_SIZE							1024

//AT状态机状态
#define AT_STATUS_NUM 8
typedef enum
{
    AT_HW_INIT = 0,//AT设备硬件初始化
    AT_REGISTER,//AT指令注册
    AT_INIT,//AT设备软件初始化
    AT_IDLE,//AT设备空闲状态 此状态下等待平台的指令

    /*MQTT*/
    AT_MQTT_PARSE,//AT设备解析平台下发的数据

    /*HTTP*/
    AT_HTTP_OTA_IDLE,//检查是否有升级任务
    AT_HTTP_OTA_DOWNLOAD,//检查是否有升级任务
    AT_HTTP_OTA_DOWNLOAD_FINISH,//检查是否有升级任务
    AT_HTTP_OTA_DOWNLOAD_FAIL,//检查是否有升级任务

} AT_STATUS_TYPE_t;

//状态机对应的回调函数
typedef struct
{
	AT_STATUS_TYPE_t status;
	void (*callback)(void *arg1 , void *arg2);
}AT_STATUS_t;

//AT指令参数
typedef struct
{
    const char *cmd;                            // AT command string
    const char *response;                       // Expected response string
    uint16_t timeout;                     // Timeout in milliseconds
    void (*callback_response)(void);     // Expected response string
} AT_CMD_t;

//URC指令对应的回调函数
typedef struct
{
    const char *urc_msg;              // AT command string
    uint8_t (*callback)(void *device ,void *a);     // Expected response string
} AT_URC_t;

//建立mqtt连接时的参数
typedef struct
{
  char *IP_Address;               // IP 地址
  uint16_t Port;                  // 端口号
  char *Product_ID;              // 产品 ID
  char *Device_Name;              // 设备名称
  char *SECRET_KEY;              // 设备密钥
  char Token[256];                    // 令牌
	uint16_t keepalive;
  uint16_t reporting_interval;    //主动上报间隔
}mqtt_connect_params_t;

//wifi参数
typedef struct
{
  /* data */
  char WiFi_SSID[16];                // WiFi SSID
  char WiFi_Password[16];            // WiFi 密码
}wifi_params_t;

// 定义 AT 设备结构体
typedef struct {
    AT_STATUS_TYPE_t status;                 // 设备状态
    AT_CMD_t *CMD_TABLE;                // AT 命令数组指针
    AT_URC_t *URC_TABLE;                // AT 命令数组指针
    char *msg_buf;                   // 消息缓冲区
		uint8_t init_step;									//初始化步骤
    mqtt_connect_params_t mqtt_params; // MQTT 连接参数
    wifi_params_t wifi_params;   // WiFi 连接参数
	  uint8_t recv_flag;
} AT_Device_t;

//AT上行数据参数
typedef struct{
  uint8_t refreshFlag;
  uint8_t ackFlag;
  char ackId[8];
  char ackMsg[32];
  uint16_t ackCode;
}AT_upload_t;


typedef enum
{
	/*AT指令相关错误类型*/
	AT_CMD_OK = 0,                          //AT指令发送成功
	AT_ERR_INVALID_PARAM,                   //AT指令参数错误
	AT_ERR_TIMEOUT,                         //AT指令应答超时
	AT_ERR_ACK,                             //AT指令应答错误
	
	/*AT指令列表相关错误类型*/
  AT_CMD_ADD_SUCCESS,                     //AT指令添加成功
	AT_ERR_TABLE_NOT_INIT,                  //AT设备指令集未初始化
	AT_ERR_TABLE_FULL,                      //AT指令集已满
	AT_ERR_MEMORY,                          //内存不足
	
	/*NTP相关错误类型*/
  AT_NTP_GET_SUCCESS,                     //NTP指令获取成功
	AT_NTP_SEND_ERROR,                      //NTP获取指令发送失败
  AT_NTP_ACK_ERROR,                       //NTP指令应答失败
	AT_NTP_PARSE_ERROR,
	
	/*MQTT相关错误类型*/
	AT_MQTT_SEND_SUCCESS,                   //MQTT指令上报成功
	AT_MQTT_SEND_FAIL,                      //MQTT指令上报失败

  AT_MALLOC_FAIL,                         //内存分配失败

  //HTTP相关错误类型
  AT_HTTP_RESPONSE_ERROR,                 //HTTP响应错误
  AT_HTTP_PARSE_ERROR,                    //HTTP响应参数错误

}at_err_t;

extern AT_Device_t AT_Device;
extern AT_upload_t at_upload;

void net_send(uint8_t *buf, uint16_t size);
void at_delay_ms(uint32_t delay_time);
void at_get_msg(uint8_t *data_buf , uint16_t buf_size);
void at_clear(void);
at_err_t AT_SendCmd(const char *cmd, const char *response, uint16_t timeout, uint8_t *data_buf);

void AT_poll(void); 
uint8_t AT_Cmd_Register(AT_Device_t *at_device, const char *response,uint16_t timeout, void (*callback_response)(void),int insert_count, const char *cmd, ...);
at_err_t AT_SendCmd(const char *cmd, const char *response, uint16_t timeout , uint8_t *data_buf);
void AT_HW_INIT_CallBack(void *device , void *arg2);
void AT_INIT_CallBack(void *device , void *arg2);
void AT_REGISTER_CallBack(void *device , void *arg2);
void AT_IDLE_CallBack(void *device , void *arg2);
void AT_upload_poll(void);
void at_time_callback(void);
void AT_HTTP_OTA_IDLE_CallBack(void *device , void *info);
void AT_HTTP_OTA_DOWNLOAD_CallBack(void *device , void *info);
void AT_HTTP_OTA_DOWNLOAD_FINISH_CallBack(void *device , void *info);
void AT_HTTP_OTA_DOWNLOAD_FAIL_CallBack(void *device , void *info);
#endif
