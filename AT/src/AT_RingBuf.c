#include "AT_RingBuf.h"

ringbuf_t AT_RingBuf;


/* 初始化环形缓冲区 */
uint8_t ringbuf_init(ringbuf_t *rb, uint8_t *buffer, uint32_t size)
{
    if (rb == NULL || buffer == NULL || size == 0) {
        return 0;
    }
    
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    
    return 1;
}

/* 重置环形缓冲区 */
void ringbuf_reset(ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }
    
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
		
		memset(rb->buffer , 0 , AT_RING_BUF_SIZE);
}

/* 获取空闲空间大小 */
uint32_t ringbuf_free_size(const ringbuf_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    
    return rb->size - rb->count;
}

/* 获取已用空间大小 */
uint32_t ringbuf_used_size(const ringbuf_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    
    return rb->count;
}

/* 检查缓冲区是否为空 */
uint8_t ringbuf_is_empty(const ringbuf_t *rb)
{
    if (rb == NULL) {
        return 1;
    }
    
    return (rb->count == 0);
}

/* 检查缓冲区是否已满 */
uint8_t ringbuf_is_full(const ringbuf_t *rb)
{
    if (rb == NULL) {
        return 1;
    }
    
    return (rb->count >= rb->size);
}

/* 写入单个字节 */
uint8_t ringbuf_put(ringbuf_t *rb, uint8_t data)
{
    if (rb == NULL || ringbuf_is_full(rb)) {
        return 0;
    }
    
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    
    return 1;
}

/* 读取单个字节 */
uint8_t ringbuf_get(ringbuf_t *rb, uint8_t *data)
{
    if (rb == NULL || data == NULL || ringbuf_is_empty(rb)) {
        return 0;
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    
    return 1;
}

/* 写入多个字节 */
uint32_t ringbuf_write(ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }
    
    uint32_t free_size = ringbuf_free_size(rb);
    uint32_t write_len = (len < free_size) ? len : free_size;
    
    if (write_len == 0) {
        return 0;
    }
    
    // 计算从head到缓冲区末尾的空间
    uint32_t to_end = rb->size - rb->head;
    
    if (write_len <= to_end) {
        // 数据可以连续写入
        memcpy(&rb->buffer[rb->head], data, write_len);
    } else {
        // 数据需要分两次写入
        memcpy(&rb->buffer[rb->head], data, to_end);
        memcpy(rb->buffer, &data[to_end], write_len - to_end);
    }
    
    rb->head = (rb->head + write_len) % rb->size;
    rb->count += write_len;
    
    return write_len;
}

/* 读取多个字节 */
uint32_t ringbuf_read(ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }
    
    uint32_t used_size = ringbuf_used_size(rb);
    uint32_t read_len = (len < used_size) ? len : used_size;
    
    if (read_len == 0) {
        return 0;
    }
    
    // 计算从tail到缓冲区末尾的数据量
    uint32_t to_end = rb->size - rb->tail;
    
    if (read_len <= to_end) {
        // 数据可以连续读取
        memcpy(data, &rb->buffer[rb->tail], read_len);
    } else {
        // 数据需要分两次读取
        memcpy(data, &rb->buffer[rb->tail], to_end);
        memcpy(&data[to_end], rb->buffer, read_len - to_end);
    }
    
//    rb->tail = (rb->tail + read_len) % rb->size;
//    rb->count -= read_len;
    
    return read_len;
}

/* 查看缓冲区中的数据(不读取) */
uint32_t ringbuf_peek(const ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }
    
    uint32_t used_size = ringbuf_used_size(rb);
    uint32_t peek_len = (len < used_size) ? len : used_size;
    
    if (peek_len == 0) {
        return 0;
    }
    
    uint32_t tail = rb->tail;
    uint32_t to_end = rb->size - tail;
    
    if (peek_len <= to_end) {
        memcpy(data, &rb->buffer[tail], peek_len);
    } else {
        memcpy(data, &rb->buffer[tail], to_end);
        memcpy(&data[to_end], rb->buffer, peek_len - to_end);
    }
    
    return peek_len;
}

/* 跳过指定字节数 */
uint8_t ringbuf_skip(ringbuf_t *rb, uint32_t len)
{
    if (rb == NULL || len == 0) {
        return 0;
    }
    
    uint32_t used_size = ringbuf_used_size(rb);
    uint32_t skip_len = (len < used_size) ? len : used_size;
    
    if (skip_len == 0) {
        return 0;
    }
    
    rb->tail = (rb->tail + skip_len) % rb->size;
    rb->count -= skip_len;
    
    return 1;
}

/* 获取连续的读取缓冲区指针和长度 */
uint32_t ringbuf_get_read_ptr(const ringbuf_t *rb, uint8_t **ptr)
{
    if (rb == NULL || ptr == NULL || ringbuf_is_empty(rb)) {
        if (ptr != NULL) {
            *ptr = NULL;
        }
        return 0;
    }
    
    *ptr = &rb->buffer[rb->tail];
    
    // 计算从tail到缓冲区末尾的连续数据长度
    uint32_t to_end = rb->size - rb->tail;
    uint32_t used_size = ringbuf_used_size(rb);
    
    return (used_size < to_end) ? used_size : to_end;
}

/* 获取连续的写入缓冲区指针和长度 */
uint32_t ringbuf_get_write_ptr(const ringbuf_t *rb, uint8_t **ptr)
{
    if (rb == NULL || ptr == NULL || ringbuf_is_full(rb)) {
        if (ptr != NULL) {
            *ptr = NULL;
        }
        return 0;
    }
    
    *ptr = &rb->buffer[rb->head];
    
    // 计算从head到缓冲区末尾的连续空间长度
    uint32_t to_end = rb->size - rb->head;
    uint32_t free_size = ringbuf_free_size(rb);
    
    return (free_size < to_end) ? free_size : to_end;
}

/* 更新读指针(在直接读取后使用) */
void ringbuf_update_read_ptr(ringbuf_t *rb, uint32_t len)
{
    if (rb == NULL || len == 0) {
        return;
    }
    
    uint32_t used_size = ringbuf_used_size(rb);
    uint32_t update_len = (len < used_size) ? len : used_size;
    
    rb->tail = (rb->tail + update_len) % rb->size;
    rb->count -= update_len;
}

/* 更新写指针(在直接写入后使用) */
void ringbuf_update_write_ptr(ringbuf_t *rb, uint32_t len)
{
    if (rb == NULL || len == 0) {
        return;
    }
    
    uint32_t free_size = ringbuf_free_size(rb);
    uint32_t update_len = (len < free_size) ? len : free_size;
    
    rb->head = (rb->head + update_len) % rb->size;
    rb->count += update_len;
}
