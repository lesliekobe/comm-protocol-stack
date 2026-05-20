/**
 * @file ringbuffer.h
 * @brief 环形缓冲区 - 协议栈核心组件，解决流式字节粘包问题
 *
 * 环形缓冲区是协议栈的灵魂：
 * - 不管串口还是TCP，都是源源不断字节流
 * - 必须用环缓冲存起来，再慢慢解包
 * - 支持"半包"累积、"粘包"拆分
 */
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>
#include <stddef.h>

/**
 * 环形缓冲区结构
 *
 * 设计要点：
 * - 读写指针分开，写推进读，追上则满
 * - 满时拒绝写入（可选覆盖模式）
 * - 读写操作均为原子性，保护数据一致性
 */
typedef struct {
    uint8_t *buffer;     // 缓冲区起始地址
    uint16_t capacity;   // 总容量（必须是2的幂次，便于取模优化）
    uint16_t count;      // 当前实际数据字节数（解决writePos环绕后与readPos重叠的歧义）
    uint16_t read_pos;   // 读指针位置
    uint16_t write_pos;  // 写指针位置
} ringbuffer_t;

/**
 * 初始化环形缓冲区
 * @param rb       环形缓冲区句柄
 * @param buf      外部缓冲区（可静态分配）
 * @param capacity 缓冲区大小（必须是2的幂次，若不是会自动向上取整）
 * @return 0成功，-1失败
 */
int ringbuffer_init(ringbuffer_t *rb, uint8_t *buf, uint16_t capacity);

/**
 * 销毁环形缓冲区（仅释放内部指针，不释放外部buffer）
 */
void ringbuffer_deinit(ringbuffer_t *rb);

/**
 * 写入数据到环形缓冲区
 * @param rb    环形缓冲区句柄
 * @param data  数据指针
 * @param len   数据长度
 * @return 实际写入字节数
 */
uint16_t ringbuffer_write(ringbuffer_t *rb, const uint8_t *data, uint16_t len);

/**
 * 从环形缓冲区读取数据（不删除）
 * @param rb    环形缓冲区句柄
 * @param data  读取缓冲区
 * @param len   期望读取长度
 * @return 实际读取字节数
 */
uint16_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint16_t len);

/**
 * 从环形缓冲区读取数据（删除已读数据）
 * @param rb    环形缓冲区句柄
 * @param data  读取缓冲区
 * @param len   期望读取长度
 * @return 实际读取字节数
 */
uint16_t ringbuffer_read_pop(ringbuffer_t *rb, uint8_t *data, uint16_t len);

/**
 * 获取当前可用数据长度
 */
uint16_t ringbuffer_available(const ringbuffer_t *rb);

/**
 * 获取剩余空间大小
 */
uint16_t ringbuffer_free(const ringbuffer_t *rb);

/**
 * 清空环形缓冲区
 */
void ringbuffer_clear(ringbuffer_t *rb);

/**
 * 查找特定字节序列的位置
 * @param rb      环形缓冲区句柄
 * @param seq     要查找的字节序列
 * @param seq_len 序列长度
 * @param start   起始查找位置（相对于读指针）
 * @return 相对于读指针的偏移，-1未找到
 */
int16_t ringbuffer_find(const ringbuffer_t *rb, const uint8_t *seq, uint16_t seq_len, uint16_t start);

#endif /* RINGBUFFER_H */