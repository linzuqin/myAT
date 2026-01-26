#ifndef _LOT_FUNCTION_H
#define _LOT_FUNCTION_H
// #include "uart.h"
#include "cJSON.h"
#include "base64.h"
#include "hmac_sha1.h"

/*add c lib*/
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include <stdarg.h>
#include "time.h"

#define LOT_MSG_SIZE 16                 //平台功能点数量
#define LOT_ARRAY_SIZE	10							//平台下发数组的最大长度
#define LOT_STRING_SIZE	256							//平台下发字符串的最大长度

typedef enum{
    LOT_ADD_SUCCESS = 0x01, // 添加参数成功
    LOT_MALLOC_FAIL, // 内存分配失败
    LOT_PARAM_ERR, // 参数错误
    LOT_NOT_FOUND, // 未找到指定参数
}lot_err_t;

typedef struct {
    cJSON* root; // JSON根对象
    char* id; // 指令ID
    char* version; // 版本信息
    char* str; // 输出字符串
    uint16_t len; // 字符串长度
} lot_msg_t;

/*属性类型*/
typedef enum
{
    object_type = 0,
    int_type,
    float_type,
    string_type,
    bool_type,
    array_type,
    enum_type,
}property_type_t;

/*权限类型*/
typedef enum
{
    ONLY_READ = 0, // 只读
    ONLY_WRITE, // 只写
    READ_WRITE, // 读写
}property_authority_t;

#define property_value_max_size 64

typedef struct
{
    char *key;            //字段
    union 
    {
      _Bool *Bool_value;      //布尔类型
			char *str_value;				//字符串类型
			int *int_value;					//整形
			double* float_value;			//浮点类型
			uint8_t *enum_value;		//枚举类型
			uint8_t *array_value;		//数组类型
	    struct property_msg_t *child; //结构体类型
    } value;
    uint16_t child_num;							//子项长度
    property_type_t type; //类型
    property_authority_t authority; //权限
}property_msg_t;

/**
 * @brief 创建JSON根节点，包含id、版本信息和params对象（用于存放指令参数）。
 * 
 * @param msg 指向lot_msg_t结构体的指针，用于存放生成的JSON根节点。
 * @return lot_err_t 返回LOT_ADD_SUCCESS表示成功，否则返回错误码（如LOT_MALLOC_FAIL）。
 */
lot_err_t lot_create_root(lot_msg_t *msg);

/**
 * @brief 向params对象中添加一个字符串类型的参数。
 * 
 * @param msg 指向lot_msg_t结构体的指针，包含JSON根节点。
 * @param key 参数的键名。
 * @param value 参数的字符串值。
 * @return lot_err_t 返回LOT_ADD_SUCCESS表示添加成功，否则返回错误码。
 */
lot_err_t lot_Add_String(cJSON *root, const char* key, const char* value);

/**
 * @brief 向params对象中添加一个数值类型的参数。
 * 
 * @param msg 指向lot_msg_t结构体的指针，包含JSON根节点。
 * @param key 参数的键名。
 * @param value 参数的数值。
 * @return lot_err_t 返回LOT_ADD_SUCCESS表示添加成功，否则返回错误码。
 */
lot_err_t lot_Add_Number(cJSON *root, const char* key, double value);

/**
 * @brief 向params对象中添加一个布尔类型的参数。
 * 
 * @param msg 指向lot_msg_t结构体的指针，包含JSON根节点。
 * @param key 参数的键名。
 * @param value 参数的布尔值。
 * @return lot_err_t 返回LOT_ADD_SUCCESS表示添加成功，否则返回错误码。
 */
lot_err_t lot_Add_Bool(cJSON *root, const char* key, _Bool value);

lot_err_t lot_Add_Array(cJSON *root, const char* key, uint8_t *arr_buf , size_t arr_szie);
/**
 * @brief 生成JSON字符串，并保存到msg->str中，同时设置字符串长度msg->len。
 * 
 * @param msg 指向lot_msg_t结构体的指针，包含JSON根节点。
 * @return lot_err_t 返回LOT_ADD_SUCCESS表示生成成功，否则返回错误码。
 */
lot_err_t lot_generate_str(lot_msg_t *msg);

/**
 * @brief 释放lot_msg_t结构体中分配的JSON对象和字符串内存。
 * 
 * @param msg 指向lot_msg_t结构体的指针。
 */
void lot_clean(lot_msg_t *msg);


#endif

