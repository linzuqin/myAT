#ifndef __AT_RINGBUF_H__
#define __AT_RINGBUF_H__

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define AT_RING_BUF_SIZE	1024

/* 环形缓冲区结构体 */
typedef struct {
    uint8_t *buffer;        // 缓冲区指针
    uint32_t size;          // 缓冲区大小
    uint32_t head;          // 写指针
    uint32_t tail;          // 读指针
    uint32_t count;         // 当前数据量
} ringbuf_t;

/* 初始化环形缓冲区 */
uint8_t ringbuf_init(ringbuf_t *rb, uint8_t *buffer, uint32_t size);

/* 重置环形缓冲区 */
void ringbuf_reset(ringbuf_t *rb);

/* 获取空闲空间大小 */
uint32_t ringbuf_free_size(const ringbuf_t *rb);

/* 获取已用空间大小 */
uint32_t ringbuf_used_size(const ringbuf_t *rb);

/* 检查缓冲区是否为空 */
uint8_t ringbuf_is_empty(const ringbuf_t *rb);

/* 检查缓冲区是否已满 */
uint8_t ringbuf_is_full(const ringbuf_t *rb);

/* 写入单个字节 */
uint8_t ringbuf_put(ringbuf_t *rb, uint8_t data);

/* 读取单个字节 */
uint8_t ringbuf_get(ringbuf_t *rb, uint8_t *data);

/* 写入多个字节 */
uint32_t ringbuf_write(ringbuf_t *rb, const uint8_t *data, uint32_t len);

/* 读取多个字节 */
uint32_t ringbuf_read(ringbuf_t *rb, uint8_t *data, uint32_t len);

/* 查看缓冲区中的数据(不读取) */
uint32_t ringbuf_peek(const ringbuf_t *rb, uint8_t *data, uint32_t len);

/* 跳过指定字节数 */
uint8_t ringbuf_skip(ringbuf_t *rb, uint32_t len);

/* 获取连续的读取缓冲区指针和长度 */
uint32_t ringbuf_get_read_ptr(const ringbuf_t *rb, uint8_t **ptr);

/* 获取连续的写入缓冲区指针和长度 */
uint32_t ringbuf_get_write_ptr(const ringbuf_t *rb, uint8_t **ptr);

/* 更新读指针(在直接读取后使用) */
void ringbuf_update_read_ptr(ringbuf_t *rb, uint32_t len);

/* 更新写指针(在直接写入后使用) */
void ringbuf_update_write_ptr(ringbuf_t *rb, uint32_t len);

extern ringbuf_t AT_RingBuf;

#endif
