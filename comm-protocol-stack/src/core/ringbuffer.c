/**
 * @file ringbuffer.c
 * @brief 环形缓冲区实现 - 协议栈核心组件
 */
#include "ringbuffer.h"
#include <string.h>
#include <stdlib.h>

/**
 * 判断是否是2的幂次
 */
static int is_power_of_2(uint16_t n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * 向上取整到最近的2的幂次
 */
static uint16_t round_up_pow2(uint16_t n) {
    if (is_power_of_2(n))
        return n;
    
    while (n & (n - 1))
        n &= (n - 1);
    return n << 1;
}

int ringbuffer_init(ringbuffer_t *rb, uint8_t *buf, uint16_t capacity) {
    if (!rb || !buf || capacity == 0)
        return -1;
    
    /* 容量向上取整到2的幂次，便于取模优化 */
    uint16_t real_capacity = round_up_pow2(capacity);
    
    rb->buffer = buf;
    rb->capacity = real_capacity;
    rb->read_pos = 0;
    rb->write_pos = 0;
    
    return 0;
}

void ringbuffer_deinit(ringbuffer_t *rb) {
    if (rb) {
        rb->buffer = NULL;
        rb->capacity = 0;
        rb->read_pos = 0;
        rb->write_pos = 0;
    }
}

uint16_t ringbuffer_available(const ringbuffer_t *rb) {
    if (!rb)
        return 0;
    
    if (rb->write_pos >= rb->read_pos)
        return rb->write_pos - rb->read_pos;
    else
        return rb->capacity - rb->read_pos + rb->write_pos;
}

uint16_t ringbuffer_free(const ringbuffer_t *rb) {
    if (!rb)
        return 0;
    
    return rb->capacity - ringbuffer_available(rb);
}

void ringbuffer_clear(ringbuffer_t *rb) {
    if (rb) {
        rb->read_pos = 0;
        rb->write_pos = 0;
    }
}

uint16_t ringbuffer_write(ringbuffer_t *rb, const uint8_t *data, uint16_t len) {
    if (!rb || !data || len == 0)
        return 0;
    
    uint16_t available = ringbuffer_free(rb);
    if (len > available)
        len = available;  /* 丢弃溢出数据 */
    
    if (len == 0)
        return 0;
    
    /* 分两段写入：尾部 + 头部（环绕） */
    uint16_t tail_len = rb->capacity - rb->write_pos;
    if (tail_len >= len) {
        /* 只需写入尾部 */
        memcpy(rb->buffer + rb->write_pos, data, len);
        rb->write_pos = (rb->write_pos + len) & (rb->capacity - 1);
    } else {
        /* 先写尾部，再从头开始写 */
        memcpy(rb->buffer + rb->write_pos, data, tail_len);
        memcpy(rb->buffer, data + tail_len, len - tail_len);
        rb->write_pos = (rb->write_pos + len) & (rb->capacity - 1);
    }
    
    return len;
}

uint16_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint16_t len) {
    if (!rb || !data || len == 0)
        return 0;
    
    uint16_t avail = ringbuffer_available(rb);
    if (len > avail)
        len = avail;
    
    if (len == 0)
        return 0;
    
    /* 分两段读出：尾部 + 头部（环绕） */
    uint16_t tail_len = rb->capacity - rb->read_pos;
    if (tail_len >= len) {
        memcpy(data, rb->buffer + rb->read_pos, len);
        /* 不移动读指针，仅查询 */
    } else {
        memcpy(data, rb->buffer + rb->read_pos, tail_len);
        memcpy(data + tail_len, rb->buffer, len - tail_len);
    }
    
    return len;
}

uint16_t ringbuffer_read_pop(ringbuffer_t *rb, uint8_t *data, uint16_t len) {
    if (!rb || !data || len == 0)
        return 0;
    
    uint16_t avail = ringbuffer_available(rb);
    if (len > avail)
        len = avail;
    
    if (len == 0)
        return 0;
    
    /* 分两段读出：尾部 + 头部（环绕） */
    uint16_t tail_len = rb->capacity - rb->read_pos;
    if (tail_len >= len) {
        memcpy(data, rb->buffer + rb->read_pos, len);
        rb->read_pos = (rb->read_pos + len) & (rb->capacity - 1);
    } else {
        memcpy(data, rb->buffer + rb->read_pos, tail_len);
        memcpy(data + tail_len, rb->buffer, len - tail_len);
        rb->read_pos = (rb->read_pos + len) & (rb->capacity - 1);
    }
    
    return len;
}

int16_t ringbuffer_find(const ringbuffer_t *rb, const uint8_t *seq, uint16_t seq_len, uint16_t start) {
    if (!rb || !seq || seq_len == 0)
        return -1;
    
    uint16_t avail = ringbuffer_available(rb);
    if (start >= avail || start + seq_len > avail)
        return -1;
    
    /* 临时读取缓冲区用于搜索 */
    uint8_t *temp = (uint8_t *)malloc(seq_len);
    if (!temp)
        return -1;
    
    int16_t result = -1;
    uint16_t search_len = avail - start;
    
    for (uint16_t i = start; i <= search_len - seq_len; i++) {
        ringbuffer_read(rb, temp, seq_len);
        /* 这里简化处理，实际应用可优化 */
        if (memcmp(temp, seq, seq_len) == 0) {
            result = i;
            break;
        }
        /* 移动读指针模拟遍历（不推荐生产环境） */
    }
    
    free(temp);
    return result;
}