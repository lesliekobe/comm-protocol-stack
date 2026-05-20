# API参考手册
# API Reference Manual

**版本：** v1.0.0
**最后更新：** 2024-05-20

---

## 目录

1. [环形缓冲区 API](#1-环形缓冲区-api)
2. [CRC校验 API](#2-crc校验-api)
3. [协议帧 API](#3-协议帧-api)
4. [解包状态机 API](#4-解包状态机-api)
5. [封包组帧 API](#5-封包组帧-api)
6. [会话管理 API](#6-会话管理-api)
7. [协议栈核心 API](#7-协议栈核心-api)
8. [传输适配层 API](#8-传输适配层-api)

---

## 1. 环形缓冲区 API

### ringbuffer_init

初始化环形缓冲区。

```c
int ringbuffer_init(ringbuffer_t *rb, uint8_t *buf, uint16_t capacity);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| rb | ringbuffer_t* | 环形缓冲区句柄 |
| buf | uint8_t* | 外部缓冲区指针 |
| capacity | uint16_t | 缓冲区大小（会自动取整到2的幂次） |

**返回值：** 0成功，-1失败

**示例：**
```c
ringbuffer_t rb;
uint8_t buffer[256];
ringbuffer_init(&rb, buffer, 256);
```

---

### ringbuffer_write

写入数据到环形缓冲区。

```c
uint16_t ringbuffer_write(ringbuffer_t *rb, const uint8_t *data, uint16_t len);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| rb | ringbuffer_t* | 环形缓冲区句柄 |
| data | const uint8_t* | 数据指针 |
| len | uint16_t | 数据长度 |

**返回值：** 实际写入的字节数（满时返回可用空间大小）

---

### ringbuffer_read

读取数据（不删除）。

```c
uint16_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint16_t len);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| rb | ringbuffer_t* | 环形缓冲区句柄 |
| data | uint8_t* | 读取缓冲区 |
| len | uint16_t | 期望读取长度 |

**返回值：** 实际读取的字节数

---

### ringbuffer_read_pop

读取数据（删除已读）。

```c
uint16_t ringbuffer_read_pop(ringbuffer_t *rb, uint8_t *data, uint16_t len);
```

**参数：** 同 `ringbuffer_read`

**返回值：** 实际读取的字节数

---

### ringbuffer_available

获取当前可用数据长度。

```c
uint16_t ringbuffer_available(const ringbuffer_t *rb);
```

---

### ringbuffer_free

获取剩余空间大小。

```c
uint16_t ringbuffer_free(const ringbuffer_t *rb);
```

---

### ringbuffer_clear

清空环形缓冲区。

```c
void ringbuffer_clear(ringbuffer_t *rb);
```

---

[回到目录](#目录)

---

## 2. CRC校验 API

### crc16_modbus

CRC16-Modbus 校验（协议默认使用）。

```c
uint16_t crc16_modbus(const uint8_t *data, uint32_t len);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| data | const uint8_t* | 数据指针 |
| len | uint32_t | 数据长度 |

**返回值：** CRC16校验码

---

### crc16_xmodem

CRC16-CCITT (XModem标准)。

```c
uint16_t crc16_xmodem(const uint8_t *data, uint32_t len);
```

---

### crc32_ethernet

CRC32 以太网标准。

```c
uint32_t crc32_ethernet(const uint8_t *data, uint32_t len);
```

---

### crc8_dallas

CRC8 Dallas (1-Wire)。

```c
uint8_t crc8_dallas(const uint8_t *data, uint32_t len);
```

---

### checksum_sum8

8位累加校验和。

```c
uint8_t checksum_sum8(const uint8_t *data, uint32_t len);
```

---

### checksum_sum16

16位累加校验和（带进位循环）。

```c
uint16_t checksum_sum16(const uint8_t *data, uint32_t len);
```

---

[回到目录](#目录)

---

## 3. 协议帧 API

### 协议帧结构

```c
typedef struct {
    uint8_t  addr;           /* 设备地址 */
    uint8_t  cmd;            /* 命令码 */
    uint16_t data_len;       /* 数据域长度 */
    uint8_t  data[PROTO_MAX_DATA_SIZE]; /* 数据域 */
    uint16_t crc;            /* CRC16校验码 */
} proto_frame_t;
```

### 协议配置结构

```c
typedef struct {
    uint8_t  local_addr;     /* 本机地址 */
    uint16_t max_frame_size; /* 最大帧长度 */
    uint16_t rx_timeout_ms;  /* 接收超时 ms */
    uint16_t tx_retry;       /* 发送重试次数 */
    uint16_t tx_retry_ms;    /* 重试间隔 ms */
} proto_config_t;
```

### 默认配置

```c
#define PROTO_CONFIG_DEFAULT { 0x01, PROTO_MAX_FRAME_SIZE, 1000, 3, 500 }
```

### 命令码枚举

```c
typedef enum {
    PROTO_CMD_READ       = 0x01,   /* 读数据 */
    PROTO_CMD_WRITE      = 0x02,   /* 写数据 */
    PROTO_CMD_REPORT     = 0x03,   /* 主动上报 */
    PROTO_CMD_HEARTBEAT  = 0x04,   /* 心跳 */
    PROTO_CMD_ACK        = 0x05,   /* 应答确认 */
    PROTO_CMD_NACK       = 0x06,   /* 应答否定 */
    PROTO_CMD_RESET      = 0x07,   /* 复位 */
    PROTO_CMD_UPDATE     = 0x08,   /* 固件升级 */
    PROTO_CMD_ERROR      = 0xFF,   /* 错误响应 */
} proto_cmd_t;
```

---

[回到目录](#目录)

---

## 4. 解包状态机 API

### proto_parser_init

初始化解析器。

```c
void proto_parser_init(proto_parser_t *parser);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| parser | proto_parser_t* | 解析器句柄 |

---

### proto_parser_feed

向解析器喂入一个字节。

```c
int proto_parser_feed(proto_parser_t *parser, uint8_t byte);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| parser | proto_parser_t* | 解析器句柄 |
| byte | uint8_t | 接收到的字节 |

**返回值：**
| 值 | 说明 |
|-----|------|
| 0 | 成功解析到完整帧 |
| -1 | 解析中 |
| -2 | 解析错误 |

---

### proto_parser_parse_rb

从环形缓冲区解析帧。

```c
int proto_parser_parse_rb(proto_parser_t *parser, ringbuffer_t *rb, proto_frame_t *frame);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| parser | proto_parser_t* | 解析器句柄 |
| rb | ringbuffer_t* | 环形缓冲区 |
| frame | proto_frame_t* | 解析出的帧（输出） |

**返回值：** 0成功，-1无完整帧，-2解析错误

---

### proto_parser_reset

重置解析器到初始状态。

```c
void proto_parser_reset(proto_parser_t *parser);
```

---

[回到目录](#目录)

---

## 5. 封包组帧 API

### proto_pack

组装完整协议帧。

```c
uint16_t proto_pack(uint8_t addr, uint8_t cmd, const uint8_t *data, uint16_t data_len,
                    uint8_t *frame_buf, uint16_t buf_size);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| addr | uint8_t | 目标设备地址 |
| cmd | uint8_t | 命令码 |
| data | const uint8_t* | 业务数据指针 |
| data_len | uint16_t | 数据长度 |
| frame_buf | uint8_t* | 输出帧缓冲区 |
| buf_size | uint16_t | 缓冲区大小 |

**返回值：** 实际组装的帧长度，0失败

**示例：**
```c
uint8_t frame[64];
uint8_t reg_addr[] = { 0x00, 0x10 };
uint16_t len = proto_pack(0x02, PROTO_CMD_READ, reg_addr, sizeof(reg_addr), 
                          frame, sizeof(frame));
send(sockfd, frame, len);
```

---

### proto_frame_to_str

帧转换为可读字符串（用于日志）。

```c
uint16_t proto_frame_to_str(const proto_frame_t *frame, char *buf, uint16_t buf_size);
```

---

### bytes_to_hex

字节数组转十六进制字符串。

```c
uint16_t bytes_to_hex(const uint8_t *data, uint16_t len, char *buf, uint16_t buf_size);
```

---

### hex_to_bytes

十六进制字符串解析为字节数组。

```c
uint16_t hex_to_bytes(const char *hex_str, uint8_t *buf, uint16_t buf_size);
```

**示例：**
```c
uint8_t data[16];
uint16_t len = hex_to_bytes("AA BB CC DD", data, sizeof(data));
// len = 4, data = { 0xAA, 0xBB, 0xCC, 0xDD }
```

---

[回到目录](#目录)

---

## 6. 会话管理 API

### session_init

初始化会话。

```c
void session_init(session_t *session, uint8_t addr, uint16_t timeout_ms, 
                  uint16_t max_retry, uint16_t heartbeat_ms);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| session | session_t* | 会话句柄 |
| addr | uint8_t | 远端设备地址 |
| timeout_ms | uint16_t | 超时时间(ms) |
| max_retry | uint16_t | 最大重试次数 |
| heartbeat_ms | uint16_t | 心跳间隔(ms) |

---

### session_on_send

标记已发送数据（启动超时计时）。

```c
void session_on_send(session_t *session, uint32_t now_ms);
```

---

### session_on_recv

标记收到数据（清除等待ACK）。

```c
void session_on_recv(session_t *session, uint32_t now_ms);
```

---

### session_is_timeout

检查会话是否超时。

```c
bool session_is_timeout(session_t *session, uint32_t now_ms);
```

**返回值：** true超时

---

### session_should_retry

检查是否需要重试。

```c
bool session_should_retry(session_t *session, uint32_t now_ms);
```

**返回值：** true需要重试

---

### session_should_heartbeat

检查是否需要发送心跳。

```c
bool session_should_heartbeat(session_t *session, uint32_t now_ms);
```

**返回值：** true需要发送心跳

---

### session_state_str

获取状态描述字符串。

```c
const char* session_state_str(session_state_t state);
```

---

[回到目录](#目录)

---

## 7. 协议栈核心 API

### proto_stack_init

初始化协议栈。

```c
int proto_stack_init(proto_stack_t *stack, const proto_config_t *config,
                     transport_send_t send_fn, transport_recv_t recv_fn, void *transport_arg);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| stack | proto_stack_t* | 协议栈句柄 |
| config | proto_config_t* | 配置参数 |
| send_fn | transport_send_t | 发送回调函数 |
| recv_fn | transport_recv_t | 接收回调函数 |
| transport_arg | void* | 传输层参数（如UART句柄） |

**返回值：** 0成功，-1失败

---

### proto_stack_set_callback

设置回调函数。

```c
void proto_stack_set_callback(proto_stack_t *stack,
    void (*on_frame_recv)(const proto_frame_t *, void *),
    void (*on_session_timeout)(uint8_t, void *),
    void (*on_error)(proto_err_t, uint8_t, void *),
    void *user_arg);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| on_frame_recv | function | 收到完整帧回调 |
| on_session_timeout | function | 会话超时回调 |
| on_error | function | 错误回调 |
| user_arg | void* | 用户参数（透传到回调） |

**回调函数签名：**
```c
void on_frame_recv(const proto_frame_t *frame, void *arg);
void on_session_timeout(uint8_t addr, void *arg);
void on_error(proto_err_t err, uint8_t addr, void *arg);
```

---

### proto_stack_send

发送协议帧。

```c
uint16_t proto_stack_send(proto_stack_t *stack, uint8_t addr, uint8_t cmd, 
                           const uint8_t *data, uint16_t len);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| stack | proto_stack_t* | 协议栈句柄 |
| addr | uint8_t | 目标地址 |
| cmd | uint8_t | 命令码 |
| data | const uint8_t* | 数据指针 |
| len | uint16_t | 数据长度 |

**返回值：** 发送的帧长度，0失败

---

### proto_stack_poll

轮询接收数据。

```c
int proto_stack_poll(proto_stack_t *stack, uint32_t now_ms);
```

**参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| stack | proto_stack_t* | 协议栈句柄 |
| now_ms | uint32_t | 当前时间戳(ms) |

**返回值：** 0有收到帧，-1无数据，-2错误

---

### proto_stack_process

处理会话状态（心跳、超时、重试）。

```c
int proto_stack_process(proto_stack_t *stack, uint32_t now_ms);
```

**返回值：** 0正常，1需要重发，2会话断开

---

### proto_stack_send_heartbeat

发送心跳帧。

```c
int proto_stack_send_heartbeat(proto_stack_t *stack, uint8_t addr);
```

---

### proto_stack_send_ack

发送ACK确认。

```c
int proto_stack_send_ack(proto_stack_t *stack, uint8_t addr, uint8_t cmd);
```

---

### proto_stack_session_state

获取当前会话状态。

```c
session_state_t proto_stack_session_state(proto_stack_t *stack);
```

---

[回到目录](#目录)

---

## 8. 传输适配层 API

### UART 传输层

#### uart_open

打开串口。

```c
uart_handle_t* uart_open(const char *port, const uart_config_t *config);
```

#### uart_close

关闭串口。

```c
void uart_close(uart_handle_t *handle);
```

#### uart_transport_send

UART发送适配。

```c
int uart_transport_send(const uint8_t *data, uint16_t len, void *arg);
```

#### uart_transport_recv

UART接收适配。

```c
int uart_transport_recv(uint8_t *buf, uint16_t buf_size, void *arg);
```

---

### Socket 传输层

#### socket_create

创建Socket连接。

```c
socket_handle_t* socket_create(const socket_config_t *config);
```

#### socket_destroy

销毁Socket连接。

```c
void socket_destroy(socket_handle_t *handle);
```

#### socket_transport_send

Socket发送适配。

```c
int socket_transport_send(const uint8_t *data, uint16_t len, void *arg);
```

#### socket_transport_recv

Socket接收适配。

```c
int socket_transport_recv(uint8_t *buf, uint16_t buf_size, void *arg);
```

---

[回到目录](#目录)

---

## 附录：错误码对照表

| 错误码 | 值 | 说明 |
|--------|-----|------|
| PROTO_ERR_NONE | 0x00 | 无错误 |
| PROTO_ERR_CRC | 0x01 | CRC校验错误 |
| PROTO_ERR_LEN | 0x02 | 长度错误 |
| PROTO_ERR_ADDR | 0x03 | 地址错误 |
| PROTO_ERR_CMD | 0x04 | 命令码错误 |
| PROTO_ERR_FRAME | 0x05 | 帧格式错误 |
| PROTO_ERR_TIMEOUT | 0x06 | 超时错误 |
| PROTO_ERR_OVERFLOW | 0x07 | 缓冲区溢出 |
| PROTO_ERR_BUSY | 0x08 | 设备忙 |
| PROTO_ERR_PARAM | 0x09 | 参数错误 |

---

**文档版本：** v1.0.0