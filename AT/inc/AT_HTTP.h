#ifndef _AT_HTTP_H_
#define _AT_HTTP_H_
#include "AT_Function.h"

typedef struct
{
    char target_version[8];//目标版本号
    uint16_t task_id;//任务ID
    uint32_t packet_size;//文件大小
    char check_md5[256];//md5校验值
    uint8_t status;//升级状态 1:待升级 2:下载中 3:升级中
    uint8_t packet_type;//1:完整包 2:差分包
    char ota_token[256];

    uint16_t signal_download_size;//单次下载大小
    uint32_t download_size;//已下载文件大小
    uint16_t download_total_count;//根据文件大小 与 单次下载大小计算出来的下载总次数
    uint16_t download_count;//当前已下载次数

}ota_packet_info_t;

at_err_t ota_check_update(ota_packet_info_t *info ,char *pro_id, char *dev_name, int type,char *version, char *auth);
at_err_t ota_download_packet(ota_packet_info_t *info ,char *pro_id, char *dev_name,char *auth , uint8_t *buf , uint32_t buf_size);


extern ota_packet_info_t ota_packet_info;





#endif
