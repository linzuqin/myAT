#ifndef _AT_MQTT_H_
#define _AT_MQTT_H_
#include "AT_Function.h"

uint8_t MQTTDISCONNECTED_CallBack(void *device, void *data);
at_err_t mqtt_pub(const char *topic, const char *data, uint16_t len);
void mqtt_cmd_init(AT_Device_t *at_device);
at_err_t set_ack(char *id , uint16_t code , char *msg);
uint8_t mqtt_parse(void *device, void *buf);
at_err_t mqtt_refresh(void);



#endif
