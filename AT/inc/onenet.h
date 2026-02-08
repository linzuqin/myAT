#ifndef _ONENET_H_
#define _ONENET_H_

// 只包含必要的头文件
#include "base64.h"
#include "hmac_sha1.h"
#include "string.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include <stdarg.h>
#include "time.h"
// 移除重复的标准库头文件

unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
                 char *authorization_buf, unsigned short authorization_buf_len, _Bool flag);

#endif
