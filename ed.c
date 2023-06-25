#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "ed.h"
#include "include/WinTypes.h"
#include "include/ftd3xx.h"
#include "utils.h"

// default buf size
static const unsigned int BUFSIZE = 2 << 10;
static const unsigned int DEFAULT_TIMEOUT = 1000;

// 指令码
static const uint8_t CODE_CONNECT_REQURST[8] = { 0xC3, 0x5A, 0xD8, 0x47, 0xC6, 0x59, 0xB3, 0x31};
static const uint8_t CODE_CONNECT_RESPONSE[8] = {0x43, 0x6F, 0x4E, 0x6E, 0x65, 0x63, 0x74, 0x21};
static const uint8_t CODE_SEND_CONFIG_REQUEST[8] = {0x65, 0xED, 0xEF, 0x43, 0x61, 0xF4, 0xE1, 0x44};
static const uint8_t CODE_SEND_CONFIG_RESPONSE[] = {0x43, 0x66, 0x67, 0x20, 0x52, 0x63, 0x76, 0x23};
static const uint8_t CODE_START_COLLECT[8] = {0x49, 0x6E, 0x74, 0x2D, 0x54, 0x72, 0x69, 0x67};
static const uint8_t CODE_STOP_COLLECT_REQUEST[8] = {0x53, 0x74, 0x6F, 0x70, 0x2B, 0x41, 0x63, 0x71};
static const uint8_t CODE_STOP_COLLECT_RESPONSE[8] = {0x46, 0x69, 0x6E, 0x3E, 0x73, 0x74, 0x6F, 0x50};

static int _pack_config(usb_config_t *, uint8_t *);
static size_t _write(usb_addr_t *, uint8_t *buf, size_t, unsigned int);
static size_t _read(usb_addr_t *, uint8_t *buf, size_t, unsigned int);
static char* statusString(FT_STATUS status);

static void set_trans_conf() {
  FT_TRANSFER_CONF conf;
  conf.pipe[0].bURBCount = 7;
  conf.pipe[1].bURBCount = 7;
  conf.pipe[0].dwURBBufferSize = 1024;
  conf.pipe[1].dwURBBufferSize = 1024;
  for (DWORD i = 0; i < 4; i++)
    FT_SetTransferParams(&conf, i);
}

void sdk_info() {
  unsigned int version[100] = {0};
  unsigned int ldpwVersion[100] = {0};
  FT_GetLibraryVersion(version);
  printf("SDK Version: %x\n", *version);

  set_trans_conf();

  FT_HANDLE handle;
  FT_STATUS status = FT_Create("000000000001", FT_OPEN_BY_SERIAL_NUMBER, &handle);
  printf("FT_Create: %s\n", statusString(status));
  FT_GetDriverVersion(handle, ldpwVersion);
  printf("Driver Version: %x\n", *ldpwVersion);
  FT_Close(handle);
}


static FT_STATUS ft_create_wrapper( PVOID pvArg, DWORD dwFlags,
  FT_HANDLE *handle) {
  set_trans_conf();
  return FT_Create(pvArg, dwFlags, handle);
}

static FT_STATUS ft_close_wrapper(FT_HANDLE *handle) {
  //FT_ResetDevicePort(handle);
  FT_STATUS status =  FT_Close(handle);
  // set_trans_conf();
  // unsigned int count;
  // FT_CreateDeviceInfoList(&count);
  return status;
}


bool SetStringDescriptorsExample(UCHAR* pStringDescriptors, ULONG ulSize,
                          CONST CHAR* pManufacturer, CONST CHAR* pProductDescription, CONST CHAR* pSerialNumber) {
  LONG lLen = 0; UCHAR bLen = 0; UCHAR* pPtr = pStringDescriptors;
  lLen = strlen(pManufacturer);
  if (lLen < 1 || lLen >= 16) return false;
  for (LONG i = 0; i < lLen; i++) if (!isprint(pManufacturer[i])) return FALSE;
  // Product Description: Should be 31 bytes maximum printable characters
  lLen = strlen(pProductDescription);
  if (lLen < 1 || lLen >= 32) return false;
  for (LONG i = 0; i < lLen; i++) if (!isprint(pProductDescription[i])) return FALSE;
  // Serial Number: Should be 15 bytes maximum alphanumeric characters
  lLen = strlen(pSerialNumber);
  if (lLen < 1 || lLen >= 16) return false;
  for (LONG i = 0; i < lLen; i++) if (!isalnum(pSerialNumber[i])) return FALSE;
  // Manufacturer
  bLen = strlen(pManufacturer);
  pPtr[0] = bLen * 2 + 2; pPtr[1] = 0x03;
  for (LONG i = 2, j = 0; i < pPtr[0]; i += 2, j++) {
    pPtr[i] = pManufacturer[j]; pPtr[i + 1] = '\0'; }
  pPtr += pPtr[0];
  // Product Description
  bLen = strlen(pProductDescription);
  pPtr[0] = bLen * 2 + 2; pPtr[1] = 0x03;
  for (LONG i = 2, j = 0; i < pPtr[0]; i += 2, j++) {
    pPtr[i] = pProductDescription[j]; pPtr[i + 1] = '\0'; }
  pPtr += pPtr[0];

  // Serial Number
  bLen = strlen(pSerialNumber);
  pPtr[0] = bLen * 2 + 2; pPtr[1] = 0x03;
  for (LONG i = 2, j = 0; i < pPtr[0]; i += 2, j++) {
    pPtr[i] = pSerialNumber[j]; pPtr[i + 1] = '\0'; }
  return true;
}

void abort_pipe(usb_addr_t *addr) {
  FT_AbortPipe(addr->handle, 0x02);
  FT_AbortPipe(addr->handle, 0x82);
  FT_AbortPipe(addr->handle, 2);

  return;
}

void reset_device601() {
  FT_STATUS ftStatus = FT_OK;
  FT_HANDLE ftHandle;
  FT_60XCONFIGURATION oConfigurationData = { 0 };
  ftStatus = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &ftHandle);
  if (ftStatus != FT_OK) {
    printf("FT_Create FAILED_TO_WRITE_DEVICE %s\n", statusString(ftStatus));
    return;
  }
  // Do NOt Change
  // Do NOt Change
  oConfigurationData.VendorID = CONFIGURATION_DEFAULT_VENDORID;
  oConfigurationData.ProductID = CONFIGURATION_DEFAULT_PRODUCTID_601;
  // Do NOt Change
  // Do NOt Change
  //
  oConfigurationData.PowerAttributes = CONFIGURATION_DEFAULT_POWERATTRIBUTES;
  oConfigurationData.PowerConsumption = CONFIGURATION_DEFAULT_POWERCONSUMPTION;
  oConfigurationData.FIFOClock = CONFIGURATION_DEFAULT_FIFOCLOCK;
  oConfigurationData.BatteryChargingGPIOConfig = CONFIGURATION_DEFAULT_BATTERYCHARGING;
  oConfigurationData.MSIO_Control = CONFIGURATION_DEFAULT_MSIOCONTROL;
  oConfigurationData.GPIO_Control = CONFIGURATION_DEFAULT_GPIOCONTROL;
  //oConfigurationData.Reserved = 0;
  oConfigurationData.Reserved2 = 0;
  oConfigurationData.FlashEEPROMDetection = 0;
  oConfigurationData.FIFOMode = CONFIGURATION_FIFO_MODE_600;
  oConfigurationData.ChannelConfig = CONFIGURATION_CHANNEL_CONFIG_1;
  //oConfigurationData.ChannelConfig = CONFIGURATION_CHANNEL_CONFIG_4;
  //oConfigurationData.OptionalFeatureSupport = CONFIGURATION_OPTIONAL_FEATURE_DISABLECANCELSESSIONUNDERRUN;

 //  USHORT flag;
	// flag = CONFIGURATION_OPTIONAL_FEATURE_ENABLENOTIFICATIONMESSAGE_INCHALL;
	// if (flag & oConfigurationData.OptionalFeatureSupport) {
	// 	oConfigurationData.OptionalFeatureSupport &= ~flag;
	// 	printf("Notification feature is not supported\r\n");
	// }
	//
	// flag = CONFIGURATION_OPTIONAL_FEATURE_DISABLECANCELSESSIONUNDERRUN;
	// if (!(flag & oConfigurationData.OptionalFeatureSupport)) {
	// 	oConfigurationData.OptionalFeatureSupport |= flag;
	// 	printf("Cancel session on underrun is not supported\r\n");
	// }
	//
	// flag = CONFIGURATION_OPTIONAL_FEATURE_DISABLEUNDERRUN_INCHALL;
	// if (!(flag & oConfigurationData.OptionalFeatureSupport)) {
	// 	oConfigurationData.OptionalFeatureSupport |= flag;
	// 	printf("Set 'disable underrun' to get better performance\r\n");
	// }
 //  if(SetStringDescriptorsExample(oConfigurationData.StringDescriptors,
 //                       sizeof(oConfigurationData.StringDescriptors),
 //                       "MyComp601", "This Is My Product DescriptioN", "1234567890ABXCM")) {
 //    printf("ues\n");
 //  }
  ftStatus = FT_SetChipConfiguration(ftHandle, &oConfigurationData);
  if (ftStatus != FT_OK) {
    printf("FT_SetChipConfiguration not ok %s\n", statusString( ftStatus)); 
    return;
  }
  ft_close_wrapper(ftHandle);

  //return true;
}

void reset_device245() {
  FT_STATUS ftStatus = FT_OK;
  FT_HANDLE ftHandle;
  FT_60XCONFIGURATION oConfigurationData = { 0 };
  ftStatus = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &ftHandle);
  if (ftStatus != FT_OK) {
    printf("FT_Create FAILED_TO_WRITE_DEVICE %s\n", statusString(ftStatus));
    return;
  }
  // Do NOt Change
  // Do NOt Change
  oConfigurationData.VendorID = CONFIGURATION_DEFAULT_VENDORID;
  oConfigurationData.ProductID = CONFIGURATION_DEFAULT_PRODUCTID_601;
  // Do NOt Change
  // Do NOt Change
  //
  oConfigurationData.PowerAttributes = CONFIGURATION_DEFAULT_POWERATTRIBUTES;
  oConfigurationData.PowerConsumption = CONFIGURATION_DEFAULT_POWERCONSUMPTION;
  oConfigurationData.FIFOClock = CONFIGURATION_DEFAULT_FIFOCLOCK;
  oConfigurationData.BatteryChargingGPIOConfig = CONFIGURATION_DEFAULT_BATTERYCHARGING;
  oConfigurationData.BatteryChargingGPIOConfig = 0; //CONFIGURATION_DEFAULT_BATTERYCHARGING;
  oConfigurationData.MSIO_Control = CONFIGURATION_DEFAULT_MSIOCONTROL;
  oConfigurationData.GPIO_Control = CONFIGURATION_DEFAULT_GPIOCONTROL;
  //oConfigurationData.Reserved = 0;
  oConfigurationData.Reserved2 = 0;
  //oConfigurationData.FlashEEPROMDetection = 0x0;
  oConfigurationData.FIFOMode = CONFIGURATION_FIFO_MODE_245;
  oConfigurationData.ChannelConfig = CONFIGURATION_CHANNEL_CONFIG_1;
  //oConfigurationData.ChannelConfig = CONFIGURATION_CHANNEL_CONFIG_4;
  oConfigurationData.OptionalFeatureSupport = 0;
  if(SetStringDescriptorsExample(oConfigurationData.StringDescriptors,
                       sizeof(oConfigurationData.StringDescriptors),
                       "FIFO", "FIDI SuperSpeed-FIFO Bridge", "000000000000001")) {
    printf("ues\n");
  }
  ftStatus = FT_SetChipConfiguration(ftHandle, &oConfigurationData);
  if (ftStatus != FT_OK) {
    printf("FT_SetChipConfiguration not ok %s\n", statusString( ftStatus)); 
    return;
  }
  ft_close_wrapper(ftHandle);

  //return true;
}

// dangeous, comment out
void reset_devicenull() {
  FT_STATUS status;
  FT_HANDLE handle;
  status = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &handle);
  if(FT_FAILED(status)) {
    ED_LOG("FT_Create failed: %s\n", statusString(status));
    return;
  }

 //  DWORD dwType;
	// FT_GetDeviceInfoDetail(0, NULL, &dwType, NULL, NULL, NULL, NULL, NULL);
 //  printf("dwType: %d\n", dwType);
	//
 //  status = FT_SetChipConfiguration(handle, NULL);
 //  if(FT_FAILED(status)) {
 //    ED_LOG("FT_SetChipConfiguration failed: %s\n", statusString(status));
 //    ft_close_wrapper(handle);
 //    return;
 //  }
	//
  ft_close_wrapper(handle);
}

void reset_default() {
  FT_STATUS status;
  FT_HANDLE handle;
  status = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &handle);
  if(FT_FAILED(status)) {
    ED_LOG("FT_Create failed: %s\n", statusString(status));
    return;
  }

  DWORD dwType;
	FT_GetDeviceInfoDetail(0, NULL, &dwType, NULL, NULL, NULL, NULL, NULL);
  printf("dwType: %d\n", dwType);

  FT_60XCONFIGURATION oConfigurationData = { 0 };
  status = FT_GetChipConfiguration(handle, &oConfigurationData);
  if (status != FT_OK) {
    printf("FT_GetChipConfiguration not ok %s\n", statusString( status)); 
    return;
  }

  status = FT_SetChipConfiguration(handle, &oConfigurationData);
  if(FT_FAILED(status)) {
    ED_LOG("FT_SetChipConfiguration failed: %s\n", statusString(status));
    ft_close_wrapper(handle);
    return;
  }

  ft_close_wrapper(handle);
}

void get_chip_configuration(){
  FT_STATUS ftStatus = FT_OK;
  FT_HANDLE ftHandle;
  FT_60XCONFIGURATION oConfigurationData = { 0 };
  ftStatus = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &ftHandle);
  if (ftStatus != FT_OK) {
    printf("FT_Create not ok %s\n", statusString( ftStatus)); 
    return;
  }
  ftStatus = FT_GetChipConfiguration(ftHandle, &oConfigurationData);
  if (ftStatus != FT_OK) {
    printf("FT_GetChipConfiguration not ok %s\n", statusString( ftStatus)); 
    return;
  }
  printf("VendorID: %x\n", oConfigurationData.VendorID);
  printf("ProductID: %x\n", oConfigurationData.ProductID);
  printf("PowerAttributes: %x\n", oConfigurationData.PowerAttributes);
  printf("PowerConsumption: %x\n", oConfigurationData.PowerConsumption);
  printf("FIFOClock: %x\n", oConfigurationData.FIFOClock);
  printf("BatteryChargingGPIOConfig: %x\n", oConfigurationData.BatteryChargingGPIOConfig);
  printf("MSIO_Control: %x\n", oConfigurationData.MSIO_Control);
  printf("GPIO_Control: %x\n", oConfigurationData.GPIO_Control);
//  printf("Reserved: %d\n", oConfigurationData.Reserved);
  printf("Reserved2: %x\n", oConfigurationData.Reserved2);
  printf("FlashEEPROMDetection: %x\n", oConfigurationData.FlashEEPROMDetection);
  printf("FIFOMode: %x\n", oConfigurationData.FIFOMode);
  printf("FIFOMode ENUM: CONFIGURATION_FIFO_MODE_245: %d CONFIGURATION_FIFO_MODE_600 %d \n", CONFIGURATION_FIFO_MODE_245, CONFIGURATION_FIFO_MODE_600);
  printf("ChannelConfig: %x\n", oConfigurationData.ChannelConfig);
  printf("ChannelConfig ENUM: CONFIGURATION_CHANNEL_CONFIG_1: %d "
         "CONFIGURATION_CHANNEL_CONFIG_2 %d CONFIGURATION_CHANNEL_CONFIG_4 %d\n", CONFIGURATION_CHANNEL_CONFIG_1, CONFIGURATION_CHANNEL_CONFIG_2, CONFIGURATION_CHANNEL_CONFIG_4);
  printf("OptionalFeatureSupport: %x\n", oConfigurationData.OptionalFeatureSupport);
  printf("StringDescriptors: %s\n", oConfigurationData.StringDescriptors);
  printf("StringDescriptors: %s\n", oConfigurationData.StringDescriptors);

  //ft_close_wrapper(ftHandle);

  //printf("11111111111");
  //fflush(stdout);
  //ftStatus = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &ftHandle);
  //if (ftStatus != FT_OK) {
  //  printf("FT_Create not ok %s\n", statusString( ftStatus)); 
  //  return;
  //}
  //printf("222222222");
  //fflush(stdout);
  //ftStatus = FT_SetChipConfiguration(ftHandle, &oConfigurationData);
  //if (ftStatus != FT_OK) {
  //  printf("ft set chip config %s\n", statusString( ftStatus)); 
  //  return;
  //}

  ft_close_wrapper(ftHandle);
}

int 
usb_create_handle_with_serial_num(usb_config_t *c, usb_addr_t *addr) {
  if(addr->handle !=0) {
    return 1;
  }

  if(addr->handle == 0) {
    FT_STATUS ftStatus;
    //ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &addr->handle);
    ftStatus = ft_create_wrapper("000000000001", FT_OPEN_BY_SERIAL_NUMBER, &addr->handle);
    if (ftStatus != FT_OK) {
      ED_LOG("failed %s \n", statusString(ftStatus));
      return ftStatus;
    }
  } 

  //FT_AbortPipe(addr->handle, 0x02);
  //FT_AbortPipe(addr->handle, 0x82);
  return 0;
}

void get_queue_status(FT_HANDLE handle)
{
		DWORD dwBufferred;

		if (FT_OK != FT_GetUnsentBuffer(handle, 2,
					NULL, &dwBufferred)) {
			printf("Failed to get unsent buffer size\n");
		}


		FT_GetReadQueueStatus(handle, 2, &dwBufferred);
		printf("CH%d IN unread buffer size in queue:%u\n", 2, dwBufferred);
}

int 
usb_create_handle(usb_config_t *c, usb_addr_t *addr) {
  ED_LOG("create_handle %s", "entering");
  if(addr->handle !=0) {
    return 1;
  }

  if(addr->handle == 0) {
    FT_STATUS ftStatus;
    ftStatus = ft_create_wrapper(0, FT_OPEN_BY_INDEX, &addr->handle);
    if (ftStatus != FT_OK) {
      ED_LOG("failed %s \n", statusString(ftStatus));
      return ftStatus;
    }
  } 

  // FT_AbortPipe(addr->handle, 0x02);
  // FT_AbortPipe(addr->handle, 0x82);
  return 0;
}


int 
usb_close_handle(usb_config_t *c, usb_addr_t *addr) {
  ED_LOG("close_handle %s", "entering");
  if(addr->handle != 0){
    ft_close_wrapper(addr->handle);
  }

  addr->handle = 0;
  return 0;
}

// 核心功能函数
int
usb_dev_connect(usb_config_t *c, usb_addr_t *addr) {
  ED_LOG("dev_connect %s", "entering");
  assert(c);
  assert(addr);
  // sending connect request
  uint8_t message[32];
  memset(message, '\0', 32);
  _pack_uint8_arr(message, CODE_CONNECT_REQURST, 8);
  _debug_hex(message, 32);
  if(_write(addr, message, 32, DEFAULT_TIMEOUT) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return USB_CONNECT_FAIL;
  }

  //_settimeout(addr, 100);
  // recv connect response
  uint8_t connect_resp[32];
  int nread;
  if((nread = _read(addr, connect_resp, 32, DEFAULT_TIMEOUT)) != 32) {
    ED_LOG("read faild: 1   %d %s", nread, strerror(errno));
    return USB_CONNECT_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_CONNECT_RESPONSE, 8);
  if(memcmp(connect_resp, expected, 8) != 0 ) {
    return USB_CONNECT_VERIFY_ERR;
  }
  return USB_CONNECT_SUCCESS;
}

int
usb_send_config_to_device(usb_config_t *c, usb_addr_t *addr) {
  ED_LOG("send_config_to_device: %s", addr->handle);

  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_config(c, buf);
  _debug_hex(buf, 32);

  if(_write(addr, buf, 32, DEFAULT_TIMEOUT) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return USB_SEND_CONFIG_FAIL;
  }

  // recv connect response
  uint8_t send_config_resp[32];
  if(_read(addr, send_config_resp, 32, DEFAULT_TIMEOUT) != 32) {
    ED_LOG("read faild: %s", strerror(errno));
    return USB_SEND_CONFIG_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_SEND_CONFIG_RESPONSE, 8);
  _debug_hex(send_config_resp, 32);
  if(memcmp(send_config_resp, expected, 8) != 0 ) {
    return USB_SEND_CONFIG_VERIFY_ERR;
  }

  return USB_SEND_CONFIG_SUCCESS;
}



// 内部触发命令
int
usb_start_collect(usb_config_t *c, usb_addr_t *addr){
  ED_LOG("start_collect: %s", c->device_ip);

  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_uint8_arr(buf, CODE_START_COLLECT, 8);
  _debug_hex(buf, 32);

  if(_write(addr, buf, 32, DEFAULT_TIMEOUT) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return USB_START_COLLECT_FAIL;
  }

  return USB_START_COLLECT_SUCCESS;
}

int 
usb_start_recv_repeat_n(usb_config_t *c, usb_addr_t *addr, int repeat_index, uint8_t *data, size_t len, unsigned int timeout){
  ED_LOG("start_recv_repeat_n: %d", repeat_index);
  assert(repeat_index < c->repeat_count);
  int nread = _read(addr, data, len, timeout);
  ED_LOG("read count %d", nread)
  return nread;
}

// 停止采集
// NOTES: stop_collect response take more then 1s to return
int
usb_stop_collect(usb_config_t *c, usb_addr_t *addr){
  //abort_pipe(addr);

  ED_LOG("stop_collect: %d", addr->handle);
  uint8_t buf[32];
  memset(buf, '\0', 32);
  _pack_uint8_arr(buf, CODE_STOP_COLLECT_REQUEST, 8);
  _debug_hex(buf, 32);

  if(_write(addr, buf, 32, DEFAULT_TIMEOUT) != 32) {
    ED_LOG("write faild: %s", strerror(errno));
    return USB_STOP_COLLECT_FAIL;
  }

  //_settimeout(addr, 5000);
  // recv connect response
  uint8_t stop_collect_resp[32];
  int nread;
  nread = _read(addr, stop_collect_resp, 32, DEFAULT_TIMEOUT);
  if(nread != 32 && errno == EAGAIN) {
    ED_LOG("read faild: %s nread %d", strerror(errno), nread);
    return USB_STOP_COLLECT_FAIL;
  }

  if(nread != 32) {
    ED_LOG("read faild: %s", strerror(errno));
    return USB_STOP_COLLECT_FAIL;
  }

  uint8_t expected[8];
  _pack_uint8_arr(expected, CODE_STOP_COLLECT_RESPONSE, 8);
  _debug_hex(stop_collect_resp, 32);
  if(memcmp(stop_collect_resp, expected, 8) != 0 ) {
    return USB_STOP_COLLECT_VERIFY_FAIL;
  }

  return USB_STOP_COLLECT_SUCCESS;
}

unsigned int
usb_bytes_count(usb_config_t *c) {
  ED_LOG("bytes_count: %s", c->device_ip);
  assert(c);

  return c->sample_count * 2 * c->ad_channel * c->repeat_count;
}

unsigned int repeat_bytes_count(usb_config_t *c) {
  return  c->sample_count * 2 * c->ad_channel;
}

//////////////////////////// STATIC FUNCTIONS ///////////////////
static int
_pack_config(usb_config_t *c, uint8_t *packed) {
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
_write(usb_addr_t *addr, uint8_t *buf, size_t size, unsigned int timeout){
  ED_LOG("about to write %lu bytes to %d", size, addr->handle);
  FT_SetPipeTimeout(addr->handle, 2, timeout);
  unsigned int count;
  FT_STATUS status = FT_WritePipe(addr->handle, 2, buf, size, &count, NULL);
  if(status != FT_OK) {
    ED_LOG("write faild: %s", statusString(status));
  }
  return count;
}

static size_t 
_read(usb_addr_t *addr, uint8_t *buf, size_t size, unsigned int timeout){
  ED_LOG("about to read %lu bytes from %d with timeout: %d", size, addr->handle, timeout);
  FT_SetPipeTimeout(addr->handle, 0x82, timeout);
  unsigned int count;
  FT_STATUS status = FT_ReadPipe(addr->handle, 2, buf, size, &count, NULL);
  if(status != FT_OK) {
    ED_LOG("read faild: %d %s %d\n", size, statusString(status), count);
  }
  return count;
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
