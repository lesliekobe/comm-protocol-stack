/**
 * @file ringbuffer.c
 * @brief 环形缓冲区实现 - 协议栈核心组件
 */
#include "ringbuffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * 向上取整到最近的2的幂次
 */
static uint16_t round_up_pow2(uint16_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n++;
    return n;
}

int ringbuffer_init(ringbuffer_t *rb, uint8_t *buf, uint16_t capacity) {
    if (!rb || !buf || capacity == 0)
        return -1;
    
    uint16_t cap = round_up_pow2(capacity);
    
    rb->buffer = buf;
    rb->capacity = cap;
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
    
    uint16_t free = ringbuffer_free(rb);
    if (len > free)
        len = free;
    
    if (len == 0)
        return 0;
    
    uint16_t tail = rb->capacity - rb->write_pos;
    
    if (tail >= len) {
        /* 只需写入尾部 */
        memcpy(rb->buffer + rb->write_pos, data, len);
        rb->write_pos = (rb->write_pos + len);
        if (rb->write_pos >= rb->capacity)
            rb->write_pos -= rb->capacity;
    } else {
        /* 先写尾部，再从头开始写（环绕） */
        memcpy(rb->buffer + rb->write_pos, data, tail);
        memcpy(rb->buffer, data + tail, len - tail);
        rb->write_pos = len - tail;
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
    
    uint16_t tail = rb->capacity - rb->read_pos;
    
    if (tail >= len) {
        memcpy(data, rb->buffer + rb->read_pos, len);
    } else {
        memcpy(data, rb->buffer + rb->read_pos, tail);
        memcpy(data + tail, rb->buffer, len - tail);
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
    
    uint16_t tail = rb->capacity - rb->read_pos;
    
    if (tail >= len) {
        memcpy(data, rb->buffer + rb->read_pos, len);
        rb->read_pos += len;
        if (rb->read_pos >= rb->capacity)
            rb->read_pos -= rb->capacity;
    } else {
        memcpy(data, rb->buffer + rb->read_pos, tail);
        memcpy(data + tail, rb->buffer, len - tail);
        rb->read_pos = len - tail;
    }
    
    return len;
}

int16_t ringbuffer_find(const ringbuffer_t *rb, const uint8_t *seq, uint16_t seq_len, uint16_t start) {
    if (!rb || !seq || seq_len == 0)
        return -1;
    
    uint16_t avail = ringbuffer_available(rb);
    if (start >= avail || start + seq_len > avail)
        return -1;
    
    uint8_t *temp = (uint8_t *)malloc(seq_len);
    if (!temp)
        return -1;
    
    int16_t result = -1;
    
    for (uint16_t i = start; i <= avail - seq_len; i++) {
        /* 临时读取（不弹出）来比较 */
        ringbuffer_read(rb, temp, seq_len);
        if (memcmp(temp, seq, seq_len) == 0) {
            result = i;
            break;
        }
    }
    
    free(temp);
    return result;
}