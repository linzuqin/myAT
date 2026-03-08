#include "AT_HTTP.h"

//通过at指令进行http发送 包括了post和get
at_err_t http_send(char *method, uint8_t content_type, char *url, uint8_t transport_type, char *data, char *header , char *response , uint16_t rsp_size)
{
    char at_command[512];
    at_err_t ret = AT_ERR_ACK;

    // 参数检查
    if (method == NULL || url == NULL)
    {
        return AT_ERR_INVALID_PARAM;
    }

    // 确定请求方法
    uint8_t opt = 0;
    if (strcmp(method, "GET") == 0)
    {
        opt = 2;
    }
    else if (strcmp(method, "POST") == 0)
    {
        opt = 3;
    }
    else
    {
        return AT_ERR_INVALID_PARAM;
    }

    // 构造AT指令的核心逻辑
    int cmd_len;

    if (opt == 2)
    { // GET请求
        if (header != NULL && strlen(header) > 0)
        {
            cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d,\"%s\"\r\n", opt, content_type, url, transport_type, header);
        }
        else
        {
            cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d\r\n", opt, content_type, url, transport_type);
        }
    }
    else
    { // POST请求
        if (data != NULL && strlen(data) > 0)
        {
            if (header != NULL && strlen(header) > 0)
            {
                cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d,\"%s\",\"%s\"\r\n", opt, content_type, url, transport_type, header, data);
            }
            else
            {
                // 注意：data前的连续两个逗号，第一个是header占位，第二个是分隔符
                cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d,,\"%s\"\r\n", opt, content_type, url, transport_type, data);
            }
        }
        else
        {
            // POST无数据
            if (header != NULL && strlen(header) > 0)
            {
                cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d,\"%s\"\r\n", opt, content_type, url, transport_type, header);
            }
            else
            {
                cmd_len = snprintf(at_command, sizeof(at_command), "AT+HTTPCLIENT=%d,%d,\"%s\",,,%d\r\n", opt, content_type, url, transport_type);
            }
        }
    }

    if (cmd_len >= sizeof(at_command))
    {
        return AT_MALLOC_FAIL; // 缓冲区溢出
    }
    ret = AT_SendCmd(at_command, NULL, 10000, (uint8_t *)response);

    if (ret != AT_CMD_OK)
    {
        return ret; // 发送失败或超时
    }

    char *json_start = strstr(response, "+HTTPCLIENT:");
    if (json_start == NULL)
    {
        json_start = strchr(response, '{');
    }
    else
    {
        json_start = strchr(json_start, ',');
        if (json_start)
            json_start++; // 跳过逗号
    }

    if (json_start)
    {
        char *json_end = strrchr(json_start, '}');
        if (json_end)
        {
            uint32_t json_len = json_end - json_start + 1;
            if (json_len < rsp_size)
            {
                strncpy(response, json_start, json_len);
                response[json_len] = '\0'; // 确保字符串结束
            }
            else
            {
                return AT_MALLOC_FAIL;
            }
        }
    }
    return ret;
}

//将ota参数清除为初始状态
void ota_info_clean(ota_packet_info_t *info)
{
    memset(info->target_version , 0 , strlen(info->target_version));
    info->task_id = 0;
    info->packet_size = 0;
    memset(info->check_md5 , 0 , strlen(info->check_md5));
    info->status = 0;
    info->packet_type = 0;
    memset(info->ota_token , 0 , strlen(info->ota_token));
    info->signal_download_size = 0;
    info->download_total_count = 0;
    info->download_count = 0;

}

//解析检测ota升级任务应答
at_err_t parse_ota_check_response(const char *json_str, ota_packet_info_t *info)
{
    if (json_str == NULL || info == NULL)
    {
        return AT_ERR_INVALID_PARAM;
    }

    memset(info, 0, sizeof(ota_packet_info_t));

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        // parse解析错误 可能是内存分配失败 也可能是传入参数不符合json格式
        return AT_MALLOC_FAIL;
    }

    at_err_t ret = AT_CMD_OK;

    // 检查返回码
    cJSON *code_item = cJSON_GetObjectItem(root, "code");
    if (code_item->valueint != 0)
    {
        cJSON_Delete(root);
        return AT_HTTP_RESPONSE_ERROR;
    }

    cJSON *data_item = cJSON_GetObjectItem(root, "data");
    if (data_item == NULL)
    {
        cJSON_Delete(root);
        return AT_HTTP_PARSE_ERROR;
    }

    // target_version
    cJSON *target_item = cJSON_GetObjectItem(data_item, "target");
    if (target_item->valuestring != NULL)
    {
        strncpy(info->target_version, target_item->valuestring,
                sizeof(info->target_version) - 1);
        info->target_version[sizeof(info->target_version) - 1] = '\0';
    }

    // task_id
    cJSON *tid_item = cJSON_GetObjectItem(data_item, "tid");
    info->task_id = (uint16_t)tid_item->valueint;

    // packet_size
    cJSON *size_item = cJSON_GetObjectItem(data_item, "size");
    info->packet_size = (uint32_t)size_item->valueint;

    // check_md5
    cJSON *md5_item = cJSON_GetObjectItem(data_item, "md5");
    if (md5_item->valuestring != NULL)
    {
        strncpy(info->check_md5, target_item->valuestring,
                sizeof(info->check_md5) - 1);
        info->check_md5[sizeof(info->check_md5) - 1] = '\0';
    }

    // 7. 解析status
    cJSON *status_item = cJSON_GetObjectItem(data_item, "status");
    info->status = (uint8_t)status_item->valueint;

    // 8. 解析packet_type
    cJSON *type_item = cJSON_GetObjectItem(data_item, "type");
    info->packet_type = (uint8_t)type_item->valueint;

    cJSON_Delete(root);

    return ret;
}

//执行ota升级任务检测
at_err_t ota_check_update(ota_packet_info_t *info ,char *pro_id, char *dev_name, int type,char *version, char *auth)
{
    char url[256];
    char header[256];
    char json_response[512]; // 存储HTTP响应中的JSON数据
    at_err_t ret;

    ota_info_clean(info);

    snprintf(url, sizeof(url),"https://iot-api.heclouds.com/fuse-ota/%s/%s/check?type=%d&version=%s",pro_id, dev_name, type, version);

    snprintf(header, sizeof(header), "Authorization: %s", auth);

    // 发送GET请求并获取响应
    ret = http_send("GET", 1, url, 1, NULL, header, json_response, sizeof(json_response));

    if (ret != AT_CMD_OK)
    {
        return ret; // HTTP请求失败
    }

    ret = parse_ota_check_response(json_response, info);

    return ret;
}

//解析下载的固件包的帧头
void parse_response_headers(char *headers, uint32_t length)
{
    char *save_ptr;
    char *line = strtok_r(headers, "\r\n", &save_ptr);
    
    while (line != NULL) {
        // 检查Content-Range头
        if (strncmp(line, "Content-Range:", 14) == 0) {
            // printf("Content-Range: %s\n", line + 14);
            
            // 解析格式：bytes start-end/total
            // 例如：bytes 0-1023/2048
            char *bytes_ptr = strstr(line, "bytes ");
            if (bytes_ptr) {
                bytes_ptr += 6; // 跳过"bytes "
                char *dash_ptr = strchr(bytes_ptr, '-');
                char *slash_ptr = strchr(bytes_ptr, '/');
                
                if (dash_ptr && slash_ptr) {
                    *dash_ptr = '\0';
                    uint32_t start = atoi(bytes_ptr);
                    *dash_ptr = '-';
                    
                    dash_ptr++;
                    *slash_ptr = '\0';
                    uint32_t end = atoi(dash_ptr);
                    *slash_ptr = '/';
                    
                    slash_ptr++;
                    uint32_t total = atoi(slash_ptr);
                    
                    // printf("分片范围: %lu-%lu，总大小: %lu\n", start, end, total);
                }
            }
        }
        
        // 检查Content-Length头
        else if (strncmp(line, "Content-Length:", 15) == 0) {
            // printf("Content-Length: %s\n", line + 15);
        }
        
        // 检查Content-Disposition头
        else if (strncmp(line, "Content-Disposition:", 20) == 0) {
            // printf("Content-Disposition: %s\n", line + 20);
            
            // 提取文件名
            char *filename_ptr = strstr(line, "filename=");
            if (filename_ptr) {
                filename_ptr += 9; // 跳过"filename="
                // printf("文件名: %s\n", filename_ptr);
            }
        }
        
        // 检查Ota-Errno头
        else if (strncmp(line, "Ota-Errno:", 10) == 0) {
            int errno = atoi(line + 10);
            // printf("OTA错误码: %d - ", errno);
            
            // switch (errno) {
            //     case 0: printf("成功\n"); break;
            //     case 1: printf("设备不存在\n"); break;
            //     case 2: printf("文件资源不存在\n"); break;
            //     case 3: printf("对象存储中资源不存在\n"); break;
            //     case 4: printf("获取文件失败，文件大小不一致\n"); break;
            //     case 5: printf("请求参数错误、任务已过期或完成\n"); break;
            //     case 6: printf("鉴权失败\n"); break;
            //     default: printf("未知错误\n"); break;
            // }
            
            // 如果errno不为0，可能需要特殊处理
            if (errno != 0) {
                // 这里可以设置错误状态或进行其他处理
            }
        }
        
        // 获取下一行
        line = strtok_r(NULL, "\r\n", &save_ptr);
    }
}

//解析下载命令的响应
at_err_t parse_http_binary_response(char *raw_response, uint8_t *binary_buffer,uint32_t buffer_size, uint32_t *received_size)
{
    char *response_ptr = strstr(raw_response, "+HTTPCLIENT:");
    if (response_ptr == NULL) {
        return AT_HTTP_RESPONSE_ERROR;
    }
    
    // 跳过"+HTTPCLIENT:"
    response_ptr += strlen("+HTTPCLIENT:");
    
    // 解析数据总长度
    char *comma_ptr = strchr(response_ptr, ',');
    if (comma_ptr == NULL) {
        return AT_HTTP_RESPONSE_ERROR;
    }
    
    *comma_ptr = '\0'; // 临时结束字符串
    uint32_t total_length = atoi(response_ptr);
    *comma_ptr = ','; // 恢复原状
    
    // 获取数据开始位置
    char *data_start = comma_ptr + 1;
    
    // 查找HTTP响应头的结束位置（两个连续的CRLF）
    char *header_end = strstr(data_start, "\r\n\r\n");
    if (header_end == NULL) {
        // 如果没有找到标准的结束标记，尝试其他方式
        header_end = strstr(data_start, "\n\n");
        if (header_end == NULL) {
            // 可能没有响应头，直接是数据
            header_end = data_start;
        } else {
            header_end += 2; // 跳过\n\n
        }
    } else {
        header_end += 4; // 跳过\r\n\r\n
    }
    
    // 解析响应头
    char *headers_start = data_start;
    uint32_t headers_length = header_end - headers_start;
    
    // 提取重要响应头字段
    parse_response_headers(headers_start, headers_length);
    
    // 计算二进制数据部分
    char *binary_data = header_end;
    uint32_t binary_data_length = total_length - (header_end - data_start);
    
    // 确保不超过提供的缓冲区
    uint32_t copy_length = (binary_data_length < buffer_size) ? 
                           binary_data_length : buffer_size;
    
    // 复制二进制数据
    if (binary_buffer != NULL && copy_length > 0) {
        memcpy(binary_buffer, binary_data, copy_length);
        *received_size = copy_length;
        
        printf("接收到二进制数据: %lu 字节\n", copy_length);
    }
    
    return AT_CMD_OK;
}

//下载固件包
at_err_t ota_download_packet(ota_packet_info_t *info ,char *pro_id, char *dev_name,char *auth , uint8_t *buf , uint32_t buf_size)
{
    char url[256];
    char json_response[1024]; // 存储HTTP响应中的JSON数据
    at_err_t ret;

    uint16_t download_size = info->signal_download_size;
    uint16_t download_count = info->download_count;

    snprintf(url, sizeof(url),"https://iot-api.heclouds.com/fuse-ota/%s/%s/%ddownload",pro_id, dev_name, info->task_id);

    // snprintf(header, sizeof(header), "Authorization: %s", auth);
    char header[512];
    snprintf(header, sizeof(header),"Authorization: %s\r\n""Range:%d-%d\r\n",auth, download_count * download_size , (download_count + 1) * download_size);

    // 发送GET请求并获取响应
    ret = http_send("GET", 2, url, 1, NULL, header, json_response ,sizeof(json_response));

    if (ret != AT_CMD_OK)
    {
        return ret; // HTTP请求失败
    }
    
    uint32_t response = 0;

    //应答成功后 开始解析
    ret = parse_http_binary_response(json_response , buf , buf_size , &response);

    if(ret == AT_CMD_OK && response > 0)
    {
        info->download_count ++;//当解析成功且响应数据长度大于0时 说明此次下载成功 下载次数加1
        info->download_size += info->signal_download_size;//累计下载数据大小
    }
		
		return ret;
}

//void http_ota_poll(AT_STATUS_t *at_dev , ota_packet_info_t *info)
//{
//    char *pro_id;
//    char *dev_name;
//    char *auth;
//    char *version;

//    switch(info->ota_status)
//    {
//        case HTTP_OTA_IDLE:
//        {
//            if(ota_check_update(info , pro_id , dev_name , 1 , version , auth) == AT_CMD_OK)//解析到当前存在下载任务
//            {
//                info->ota_status = HTTP_OTA_DOWNLOAD;
//            }
//            break;
//        }

//        case HTTP_OTA_DOWNLOAD:
//        {
//            uint8_t *download_buf = (uint8_t *)malloc(info->packet_size);
//            if(download_buf == NULL)return;
//            if(ota_download_packet(info , pro_id , dev_name , auth , download_buf , info->packet_size))
//            {
//                //下载成功 执行保存操作

//							if(info->download_count >= info->download_total_count)
//							{
//								
//							}
//            }
//            break;
//        }
//				
//				case HTTP_OTA_DOWNLOAD_FINISH:
//				{
//					//OTA下载全部完成
//					
//					
//					at_dev->status = AT_IDLE;//不管是否重启 都将at状态机恢复至空闲状态
//					break;
//				}
//				
//				case HTTP_OTA_DOWNLOAD_FAIL:
//				{
//					//OTA下载失败
//				
//					at_dev->status = AT_IDLE;//不管是否重启 都将at状态机恢复至空闲状态
//					break;
//				}
//    }
//}
