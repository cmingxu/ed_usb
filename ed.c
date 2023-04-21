#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "ed.h"
#include "include/ftd3xx.h"
#include "utils.h"

// default buf size
static const unsigned int BUFSIZE = 2 << 10;
static const unsigned int MTU = 1464;

// 指令码
static const uint8_t CODE_CONNECT_REQURST[8] = { 0xC3, 0x5A, 0xD8, 0x47, 0xC6, 0x59, 0xB3, 0x31};
static const uint8_t CODE_CONNECT_RESPONSE[8] = {0x43, 0x6F, 0x4E, 0x6E, 0x65, 0x63, 0x74, 0x21};
static const uint8_t CODE_SEND_CONFIG_REQUEST[8] = {0x65, 0xED, 0xEF, 0x43, 0x61, 0xF4, 0xE1, 0x44};
static const uint8_t CODE_SEND_CONFIG_RESPONSE[] = {0x43, 0x66, 0x67, 0x20, 0x52, 0x63, 0x76, 0x23};
static const uint8_t CODE_START_COLLECT[8] = {0x49, 0x6E, 0x74, 0x2D, 0x54, 0x72, 0x69, 0x67};
static const uint8_t CODE_STOP_COLLECT_REQUEST[8] = {0x53, 0x74, 0x6F, 0x70, 0x2B, 0x41, 0x63, 0x71};
static const uint8_t CODE_STOP_COLLECT_RESPONSE[8] = {0x46, 0x69, 0x6E, 0x3E, 0x73, 0x74, 0x6F, 0x50};

static int _pack_config(config_t *, uint8_t *);
static size_t _write(addr_t *, uint8_t *buf, size_t);
static size_t _read(addr_t *, uint8_t *buf, size_t);
static void _settimeout(addr_t *, unsigned int);
static char* statusString(FT_STATUS status);

static void reset_device() {
  FT_STATUS status;
  FT_HANDLE handle;
  status = FT_Create(0, FT_OPEN_BY_INDEX, &handle);
  if(FT_FAILED(status)) {
    ED_LOG("FT_Create failed: %s\n", statusString(status));
    return;
  }

  status = FT_SetChipConfiguration(handle, NULL);
  if(FT_FAILED(status)) {
    ED_LOG("FT_SetChipConfiguration failed: %s\n", statusString(status));
    return;
  }

  FT_Close(handle);

}
int 
create_handle(config_t *c, addr_t *addr) {
  if(addr->handle !=0) {
    return 1;
  }

  if(addr->handle == 0) {
    FT_STATUS ftStatus;
    ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &addr->handle);
    if (ftStatus != FT_OK) {
      ED_LOG("%s \n", statusString(ftStatus))

      reset_device();
      ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &addr->handle);
      return ftStatus;
    }
  } 
  return 0;
}


int close_handle(config_t *c, addr_t *addr) {
  if(addr->handle != 0){
    FT_Close(addr->handle);
  }

  addr->handle = 0;
  return 0;
}

// 核心功能函数
int 
dev_connect(config_t *c, addr_t *addr) {
  assert(c);
  assert(addr);
  // sending connect request
  uint8_t message[32];
  memset(message, '\0', 32);
  _pack_uint8_arr(message, CODE_CONNECT_REQURST, 8);
  if(_write(addr, message, 32) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return CONNECT_FAIL;
  }

  _settimeout(addr, 100);
  // recv connect response
  uint8_t connect_resp[32];
  int nread;
  if((nread = _read(addr, connect_resp, 32)) != 32) {
    ED_LOG("read faild: %d %s", nread, strerror(errno));
    return CONNECT_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_CONNECT_RESPONSE, 8);
  if(memcmp(connect_resp, expected, 8) != 0 ) {
    return CONNECT_VERIFY_ERR;
  }
  return CONNECT_SUCCESS;
}

int
send_config_to_device(config_t *c, addr_t *addr) {
  ED_LOG("send_config_to_device: %s", addr->handle);

  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_config(c, buf);
  _debug_hex(buf, 32);

  if(_write(addr, buf, 32) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return SEND_CONFIG_FAIL;
  }

  _settimeout(addr, 100);
  // recv connect response
  uint8_t send_config_resp[32];
  if(_read(addr, send_config_resp, 32) != 32) {
    ED_LOG("read faild: %s", strerror(errno));
    return SEND_CONFIG_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_SEND_CONFIG_RESPONSE, 8);
  _debug_hex(send_config_resp, 32);
  if(memcmp(send_config_resp, expected, 8) != 0 ) {
    return SEND_CONFIG_VERIFY_ERR;
  }

  return SEND_CONFIG_SUCCESS;
}



// 内部触发命令
int
start_collect(config_t *c, addr_t *addr){
  ED_LOG("start_collect: %s", c->device_ip);

  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_uint8_arr(buf, CODE_START_COLLECT, 8);
  _debug_hex(buf, 32);

  _settimeout(addr, 100);
  if(_write(addr, buf, 32) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return START_COLLECT_FAIL;
  }

  return START_COLLECT_SUCCESS;
}

int 
start_recv_by_repeat(config_t *c, addr_t *addr, repeat_response_t *resp, unsigned int repeat, unsigned int timeout){
  ED_LOG("start_recv_by_repeat: %s, repeat %d, resp->data_size: %ld", c->device_ip, repeat, resp->data_size);

  assert(resp->data);
  assert(resp->data_size > 0);
  resp->recv_data_size = 0;
  resp->recv_packet_count = 0;

  unsigned int packet_start = packet_count_of_each_repeat(c) * repeat;
  unsigned int packet_end = packet_count_of_each_repeat(c) * (repeat + 1);

  resp->packet_count = packet_count_of_each_repeat(c);
  memset(resp->data, '\0', resp->data_size);

  _settimeout(addr, timeout);

  uint8_t tmp[MTU + 8];
  for (;;) {
    int nread = _read(addr, tmp, MTU + 8);

    if(nread == -1) {
      ED_LOG("start_recv failed:%s, %d, %d", strerror(errno), errno, errno == EAGAIN);
      return START_RECV_INVALID_PACKET_LEN;
    }

    if(nread < 8) {
      ED_LOG("start_recv failed: nead length less than %d", 8);
      return START_RECV_INVALID_PACKET_LEN;
    }

    unsigned int packet_no = parse_packet_no(tmp);
    if(packet_no >= packet_start && packet_no < packet_end) {
      //ED_LOG("\nrepeat: %d, start: %d, end: %d, recv_packet_no: %d, recv_packet_count: %d, nread:%d, total: %ld, data_size: %ld",
      //  	    repeat, packet_start, packet_end, packet_no, resp->recv_packet_count, nread, resp->recv_data_size, resp->data_size);
      memcpy(resp->data + resp->recv_data_size, tmp, nread);
      resp->recv_data_size += nread;
      resp->recv_packet_count += 1;
    }

    // continue read until 
    if(packet_no < packet_start) {};

    if(packet_no >= packet_end - 1) {
      break;
    }
  }

  return START_RECV_SUCCESS;
}


// 停止采集
// NOTES: stop_collect response take more then 1s to return
int
stop_collect(config_t *c, addr_t *addr){
  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_uint8_arr(buf, CODE_STOP_COLLECT_REQUEST, 8);
  _debug_hex(buf, 32);

  _settimeout(addr, 100);
  if(_write(addr, buf, 32) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return STOP_COLLECT_FAIL;
  }

  _settimeout(addr, 5000);
  // recv connect response
  uint8_t stop_collect_resp[32];
  int nread;
AGAIN:
  nread = _read(addr, stop_collect_resp, 32);
  if(nread != 32 && errno == EAGAIN) {
    ED_LOG("read faild: %s", strerror(errno));
    goto AGAIN;
  }

  if(nread != 32) {
    ED_LOG("read faild: %s", strerror(errno));
    return STOP_COLLECT_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_STOP_COLLECT_RESPONSE, 8);
  _debug_hex(stop_collect_resp, 32);
  if(memcmp(stop_collect_resp, expected, 8) != 0 ) {
    return STOP_COLLECT_VERIFY_FAIL;
  }

  return STOP_COLLECT_SUCCESS;
}

unsigned int 
package_count(config_t *c) {
  ED_LOG("package_count: %s", c->device_ip);
  assert(c);

  return packet_count_of_each_repeat(c) * c->repeat_count;
}

unsigned int
bytes_count(config_t *c) {
  ED_LOG("bytes_count: %s", c->device_ip);
  assert(c);

  return c->sample_count * 2 * c->ad_channel * c->repeat_count;
}

unsigned int repeat_bytes_count(config_t *c) {
  return  c->sample_count * 2 * c->ad_channel;
}

unsigned int repeat_bytes_count_with_heading(config_t *c) {
  return  repeat_bytes_count(c) + packet_count_of_each_repeat(c) * 8;
}

unsigned int
packet_count_of_each_repeat(config_t *c) {
  return ceil(repeat_bytes_count(c) / (float)MTU);
}

//////////////////////////// STATIC FUNCTIONS ///////////////////
static int
_pack_config(config_t *c, uint8_t *packed) {
  uint8_t *pos = packed;
  _pack_uint8_arr(pos, CODE_SEND_CONFIG_REQUEST, 8);
  pos+=8;

  _pack_uint32(pos, c->sample_count);
  pos+=4;

  _pack_uint32(pos, c->delay_count);
  pos+=4;

  _pack_uint16(pos, c->repeat_count);
  pos+=2;

  _pack_uint16(pos, c->down_sample_count);
  pos+=2;

  pos+=4;

  _pack_short(pos, c->trigger);
  pos+=1;

  _pack_short(pos, c->outer_trigger);
  pos+=1;

  pos+=4;

  return 0;
}

static size_t 
_write(addr_t *addr, uint8_t *buf, size_t size){
  ED_LOG("write %lu bytes to %d", size, addr->handle);
  unsigned int count;
  FT_WritePipe(addr->handle, 0x02, buf, size, &count, NULL);
  return count;
}

static size_t 
_read(addr_t *addr, uint8_t *buf, size_t size){
  unsigned int count;
  FT_ReadPipe(addr->handle, 0x82, buf, size, &count, NULL);
  return count;
}

static void
_settimeout(addr_t *addr, unsigned int milliseconds) {
  FT_SetPipeTimeout(addr->handle, 
                    0xFF, milliseconds);
}

static char *
statusString (FT_STATUS status) {
  switch (status)
    {
      case FT_OK:
        return "OK";
      case FT_INVALID_HANDLE:
        return "INVALID_HANDLE";
      case FT_DEVICE_NOT_FOUND:
        return "DEVICE_NOT_FOUND";
      case FT_DEVICE_NOT_OPENED:
        return "DEVICE_NOT_OPENED";
      case FT_IO_ERROR:
        return "IO_ERROR";
      case FT_INSUFFICIENT_RESOURCES:
        return "INSUFFICIENT_RESOURCES";
      case FT_INVALID_PARAMETER:
        return "INVALID_PARAMETER";
      case FT_INVALID_BAUD_RATE:
        return "INVALID_BAUD_RATE";
      case FT_DEVICE_NOT_OPENED_FOR_ERASE:
        return "DEVICE_NOT_OPENED_FOR_ERASE";
      case FT_DEVICE_NOT_OPENED_FOR_WRITE:
        return "DEVICE_NOT_OPENED_FOR_WRITE";
      case FT_FAILED_TO_WRITE_DEVICE:
        return "FAILED_TO_WRITE_DEVICE";
      case FT_EEPROM_READ_FAILED:
        return "EEPROM_READ_FAILED";
      case FT_EEPROM_WRITE_FAILED:
        return "EEPROM_WRITE_FAILED";
      case FT_EEPROM_ERASE_FAILED:
        return "EEPROM_ERASE_FAILED";
      case FT_EEPROM_NOT_PRESENT:
        return "EEPROM_NOT_PRESENT";
      case FT_EEPROM_NOT_PROGRAMMED:
        return "EEPROM_NOT_PROGRAMMED";
      case FT_INVALID_ARGS:
        return "INVALID_ARGS";
      case FT_NOT_SUPPORTED:
        return "NOT_SUPPORTED";
      case FT_NO_MORE_ITEMS:
        return "NO_MORE_ITEMS";
      case FT_TIMEOUT:
        return "TIMEOUT";
      case FT_OPERATION_ABORTED:
        return "OPERATION_ABORTED";
      case FT_RESERVED_PIPE:
        return "RESERVED_PIPE";
      case FT_INVALID_CONTROL_REQUEST_DIRECTION:
        return "INVALID_CONTROL_REQUEST_DIRECTION";
      case FT_INVALID_CONTROL_REQUEST_TYPE:
        return "INVALID_CONTROL_REQUEST_TYPE";
      case FT_IO_PENDING:
        return "IO_PENDING";
      case FT_IO_INCOMPLETE:
        return "IO_INCOMPLETE";
      case FT_HANDLE_EOF:
        return "HANDLE_EOF";
      case FT_BUSY:
        return "BUSY";
      case FT_NO_SYSTEM_RESOURCES:
        return "NO_SYSTEM_RESOURCES";
      case FT_DEVICE_LIST_NOT_READY:
        return "DEVICE_LIST_NOT_READY";
      case FT_OTHER_ERROR:
        return "OTHER_ERROR";
      default:
        return "UNKNOWN ERROR";
    }
}


