#ifndef ED_H
#define ED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "./include/ftd3xx.h"

struct usb_repeat_response {
  // 设备采样数据
  void *data;
  // 接受data_size字节数， 此sample的其余丢弃
  size_t data_size;
  // 从设备收集到的数据大小
  size_t recv_data_size;
  
  // 应该收到的包总量
  size_t packet_count;
  // 实际收到包总量
  size_t recv_packet_count;
};
typedef struct usb_repeat_response usb_repeat_response_t;

struct usb_addr{
  FT_HANDLE handle;
};
typedef struct usb_addr usb_addr_t;

struct usb_config{
  // 采样点数
  uint32_t sample_count;
  // 延时点数
  uint32_t delay_count;
  // 重复次数
  uint16_t repeat_count;
  // 降采样点数
  uint16_t down_sample_count;
  // 本机IP地址
  char local_ip[32];
  // 本机UDP端口地址
  unsigned int local_port;
  // 设备IP地址
  char device_ip[32];
  // 设备端口地址
  unsigned int device_port;

  short ad_channel;
  short ad_bit;
  short trigger;
  short outer_trigger;

  // 新IP
  char new_local_ip[32];
  // 新port
  unsigned int new_local_port;
};
typedef struct usb_config usb_config_t;

// create default global config
int usb_load_default_config(usb_config_t *);


// AD 通道数 默认 1(1:单通道;2:双通道);
#define USB_ADCHANNEL_SINGLE 1
#define USB_ADCHANNEL_BIDIRECTIONAL 2

// AD 位数，无符号，1Byte，默认 12(12:12bit;14:14bit); 
#define USB_ADBIT_12 12
#define USB_ADBIT_14 14

// 触发方式，无符号，1Byte，默认 0(0x0:外触发，0x1:内触发);
#define USB_TRIGGER_OUTER 0
#define USB_TRIGGER_INNER 1

// 外触发边沿，无符号，1Byte，默认 0(0x0:下降沿，0x1:上升沿);
#define USB_OUTER_TRIGGER_DOWN 0
#define USB_OUTER_TRIGGER_UP 1

// 初始化USB设备
#define USB_ESTABLISH_CONNECTION_SUCCESS 0
int usb_create_handle(usb_config_t *t, usb_addr_t *);
int usb_create_handle_with_serial_num(usb_config_t *t, usb_addr_t *);
// 清理US
int usb_close_handle(usb_config_t *, usb_addr_t *);

void sdk_info();
void reset_default();
void reset_device601();
void reset_device245();
void reset_devicenull();
void get_chip_configuration();
void abort_pipe(usb_addr_t *);
void get_queue_status();

// 建立连接
#define USB_CONNECT_SUCCESS 0
#define USB_CONNECT_FAIL 1
#define USB_CONNECT_VERIFY_ERR 2
int usb_dev_connect(usb_config_t *, usb_addr_t *);

// 发送配置信息
#define USB_SEND_CONFIG_SUCCESS 0
#define USB_SEND_CONFIG_FAIL 10001
#define USB_SEND_CONFIG_TIMEOUT 10002
#define USB_SEND_CONFIG_VERIFY_ERR 10003
int usb_send_config_to_device(usb_config_t *, usb_addr_t *);

// 存储配置信息
#define USB_WRITE_CONFIG_FAIL -1
#define USB_WRITE_CONFIG_SUCCESS 0
int usb_write_config(usb_config_t*, FILE *);

// 读取配置信息
#define USB_READ_CONFIG_SUCCESS 0
#define USB_READ_CONFIG_FAIL -1
int usb_load_config(usb_config_t*, FILE *);

// 计算数据bytecount
unsigned int usb_bytes_count(usb_config_t *);

int usb_start_recv_repeat_n(usb_config_t *, usb_addr_t *, int repeat, uint8_t*, size_t, unsigned int);

// 内部触发
#define USB_START_COLLECT_SUCCESS 0
#define USB_START_COLLECT_FAIL 10001
#define USB_START_COLLECT_TIMEOUT 10002
#define USB_START_COLLECT_VERIFY_ERR 10003
int usb_start_collect(usb_config_t *, usb_addr_t *);

// 停止采集
#define USB_STOP_COLLECT_SUCCESS 0
#define USB_STOP_COLLECT_FAIL 10001
#define USB_STOP_COLLECT_VERIFY_FAIL 10002
int usb_stop_collect(usb_config_t *, usb_addr_t *);

#ifdef ED_DEBUG
#define MYLOG(fmt, ...) do { \
fprintf(stdout, "[LOG] %s:%d: " fmt "\n", __FUNCTION__,__LINE__,__VA_ARGS__ ); \
fflush(stdout); \
} while(0);

#define ED_LOG( fmt, ... ) MYLOG(fmt,__VA_ARGS__)
#else
#define ED_LOG( fmt, ... ) {}
#endif

#ifdef __cplusplus
}
#endif
#endif
