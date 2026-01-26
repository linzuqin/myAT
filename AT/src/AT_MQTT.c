#include "AT_MQTT.h"

uint8_t MQTTDISCONNECTED_CallBack(void *device, void *data)
{
	AT_Device_t *at_device = device;
	at_device->init_step = 0;
	at_device->status = AT_INIT;
	
	return 0;
}

at_err_t mqtt_pub(const char *topic, const char *data, uint16_t len)
{
    char cmd[128] = {0};

    // 构造MQTT发布命令
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0\r\n", topic, len);

    if (AT_SendCmd(cmd, ">", 1000, NULL) == AT_CMD_OK)
    {
        if (AT_SendCmd(data, "OK", 1000, NULL) == AT_CMD_OK)
        {
            return AT_MQTT_SEND_SUCCESS;
        }
    }
    return AT_MQTT_SEND_FAIL;
}

void mqtt_cmd_init(AT_Device_t *at_device)
{
    at_device->mqtt_params.IP_Address = IP_ADDRESS;
    at_device->mqtt_params.Port = PORT_NUMBER;
    at_device->mqtt_params.Product_ID = PRODUCT_ID;
    at_device->mqtt_params.Device_Name = DEVICE_NAME;
    at_device->mqtt_params.SECRET_KEY = MY_SECRET_KEY;
    at_device->mqtt_params.keepalive = MQTT_KEEP_ALIVE;
    at_device->mqtt_params.reporting_interval = MQTT_KEEP_ALIVE;

	
		memcpy(at_device->mqtt_params.Token , TOKEN , sizeof(TOKEN));
//    OneNET_Authorization("2018-10-31", at_device->mqtt_params.Product_ID, 1956499200, at_device->mqtt_params.SECRET_KEY, at_device->mqtt_params.Device_Name, at_device->mqtt_params.Token, sizeof(at_device->mqtt_params.Token), 1);

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+CWJAP=\"%s\",\"%s\"\r\n", at_device->wifi_params.WiFi_SSID, at_device->wifi_params.WiFi_Password);

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n");

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
                    at_device->mqtt_params.Device_Name, at_device->mqtt_params.Product_ID, at_device->mqtt_params.Token);

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+MQTTCONNCFG=0,%d,0,\"\",\"\",0,0\r\n",at_device->mqtt_params.keepalive);

    //    AT_Cmd_Register(at_device, "OK", 100, NULL, -1,
    //                    "AT+MQTTCLIENTID=0,\"%s\"\r\n",
    //                    clientId);

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+MQTTCONN=0,\"%s\",%d,1\r\n", IP_ADDRESS, PORT_NUMBER);

    AT_Cmd_Register(at_device, "OK", 1000, NULL, -1,
                    "AT+MQTTSUB=0,\"%s\",1\r\n", SET_TOPIC);
}

at_err_t set_ack(char *id , uint16_t code , char *msg)
{
    char ack_buf[128];
    memset(ack_buf, 0, sizeof(ack_buf));
    sprintf(ack_buf, "{\"id\":\"%s\",\"code\":%d,\"msg\":\"%s\"}", id , code , msg);
    return mqtt_pub(SET_ACK_TOPIC, ack_buf, strlen(ack_buf));
}

uint8_t mqtt_parse(void *device, void *buf)
{
    uint8_t *data = buf;
    AT_Device_t *at_device = device;
    at_device->status = AT_MQTT_PARSE;
    if (data != NULL)
    {
        char *json_start = strchr((char *)data, '{');
        char *json_end = strrchr((char *)data, '}');

        if (!json_start || !json_end || json_end < json_start)
        {
            at_device->status = AT_IDLE;
            return 1;
        }

        size_t json_len = json_end - json_start + 1;
        if (json_len > MAX_JSON_SIZE)
        {
            at_device->status = AT_IDLE;
            return 2;
        }

        char json_buf[MAX_JSON_SIZE + 1];
        strncpy(json_buf, json_start, json_len);
        json_buf[json_len] = '\0';

        cJSON *IPD_js = cJSON_Parse(json_buf);
        if (IPD_js == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            return 3;
        }
        cJSON *id_js = cJSON_GetObjectItemCaseSensitive(IPD_js, "id");
        if (id_js != NULL)
        {
            //根据获取的id信息设置上报信息
            memset(at_upload.ackId , 0 , sizeof(at_upload.ackId));
            memcpy(at_upload.ackId , id_js->valuestring , strlen(id_js->valuestring));

            memset(at_upload.ackMsg , 0 , sizeof(at_upload.ackMsg));
            memcpy(at_upload.ackMsg , "success" , strlen("success"));

            at_upload.ackCode = 200;
            at_upload.ackFlag = 1;
        }
        else
        {
            cJSON_Delete(IPD_js);
            return 4;
        }
        cJSON *params_js = cJSON_GetObjectItem(IPD_js, "params");
        if (params_js != NULL)
        {
					
        }
        cJSON_Delete(IPD_js);
    }
    at_device->status = AT_IDLE;

    return 0;
}

at_err_t mqtt_refresh(void)
{
    at_err_t result = AT_MQTT_SEND_FAIL;

    return result;
}
