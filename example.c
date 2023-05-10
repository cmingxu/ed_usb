#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "./include/ftd3xx.h"
#include "include/WinTypes.h"
#include "utils.h"

//DEFINE_GUID(GUID_DEVINTERFACE_FOR_FTDI_DEVICES,
//	0x36fc9e60, 0xc465, 0x11cf, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
//
/*Functions */
static const char * statusString (FT_STATUS status)
{
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


void get_version(void)
{
  printf("get_version \n");
  DWORD dwVersion;

  FT_GetDriverVersion(NULL, &dwVersion);
  printf("Driver version:%d.%d.%d\r\n", dwVersion >> 24,
         (uint8_t)(dwVersion >> 16), dwVersion & 0xFFFF);

  FT_GetLibraryVersion(&dwVersion);
  printf("Library version:%d.%d.%d\r\n", dwVersion >> 24,
         (uint8_t)(dwVersion >> 16), dwVersion & 0xFFFF);
}

void device_count(void){
  printf("device_count \n");
  DWORD count;
  ULONG status;
  status = FT_CreateDeviceInfoList(&count);
  if(FT_SUCCESS(status)) {
    printf("Number of devices is %d \n", count);
  }
}

void print_device_info(FT_DEVICE_LIST_INFO_NODE *devInfo)
{
  printf("Flags=0x%x\n ", devInfo->Flags);
  printf("Type=0x%x\n ", devInfo->Type);
  printf("ID=0x%x\n ", devInfo->ID);
  printf("SerialNumber=%s\n ", devInfo->SerialNumber);
  printf("Description=%s\n ", devInfo->Description);
}

void print_config_info(FT_60XCONFIGURATION *config) {
  printf("VendorID=0x%x\n ", config->VendorID);
  printf("ProductID=0x%x\n ", config->ProductID);
  printf("StringDescriptors=%s\n ", config->StringDescriptors);
  printf("Interval=%d\n ", config->bInterval);
  printf("FIFOMode=%d\n ", config->FIFOMode);
  printf("FIFOClock=%d\n ", config->FIFOClock);
  printf("ChannelConfig=%d\n ", config->ChannelConfig);
  printf("OptionalFeatureSupport=%d\n", config->OptionalFeatureSupport);
}

void get_device_lists(void){
  printf("get_device_lists \n");
  DWORD count;
  FT_DEVICE_LIST_INFO_NODE nodes[16];
  ULONG status;
  status = FT_CreateDeviceInfoList(&count);
  if(FT_SUCCESS(status)) {
    printf("Number of devices is %d \n", count);
  }
  status = FT_GetDeviceInfoList(nodes, &count);
  if(FT_FAILED(status)) {
    printf("%s", statusString(status));
    return;
  }

  for (size_t i = 0; i < count; i++) {
    printf("Device %lu:\n", i);
    print_device_info(&nodes[i]);
  }
}


void get_chip_configuration(void)
{
  printf("get_chip_configuration \n");
  FT_HANDLE                    ftHandle;
  FT_STATUS                    ftStatus;
  FT_60XCONFIGURATION          chipConfig;
  DWORD dwType = FT_DEVICE_UNKNOWN;

 ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &ftHandle);
  if (FT_FAILED(ftStatus))
  {
    printf("%s:%d: ERROR: FT_Create failed (%s)\n",__FILE__,__LINE__,statusString(ftStatus));
    return;
  }


  FT_GetDeviceInfoDetail(0,NULL, &dwType, NULL, NULL, NULL, NULL, NULL);

  memset(&chipConfig, 0, sizeof(chipConfig));
  ftStatus = FT_GetChipConfiguration(ftHandle, &chipConfig);
  if (FT_FAILED(ftStatus))
  {
    FT_Close(ftHandle);
    printf("%s:%d: ERROR: FT_GetChipConfiguration failed (%s)\n",__FILE__,__LINE__,statusString(ftStatus));
  }

  print_config_info(&chipConfig);

  FT_Close(ftHandle);
  return;
}

#define FIFO_CHANNEL_1	0
#define FIFO_CHANNEL_2	1
#define FIFO_CHANNEL_3	2
#define FIFO_CHANNEL_4	3
#define TIMEOUT			1000 //0xFFFFFFFF
//
uint8_t CODE_CONNECT_REQURST[8] = {0x31, 0xB3, 0x59, 0xC6, 0x47, 0xD8, 0x5A, 0xC3};
uint8_t CODE_CONNECT_REQURST1[32] = { 0xC3, 0x5A, 0xD8, 0x47, 0xC6, 0x59, 0xB3, 0x31};
uint8_t CODE_CONNECT_RESPONSE[8] = {0x21, 0x74, 0x63, 0x65, 0x6E, 0x4E, 0x6F, 0x43};
void read_and_write() {
  printf("read_and_write \n");
  FT_HANDLE handle;
  FT_STATUS status;
 //status =  FT_Create("FTDI SuperSpeed-FIFO Bridge", FT_OPEN_BY_DESCRIPTION, &handle);
 //status =  FT_Create("1234567890ABCde", FT_OPEN_BY_SERIAL_NUMBER, &handle);
 status =  FT_Create(0, FT_OPEN_BY_INDEX, &handle);
  if (FT_FAILED(status)) {
    printf("failed created %s\n", statusString(status));
    return;
  }

  // status = FT_SetChipConfiguration(handle, NULL);
  // if (FT_FAILED(status)) {
  //   printf("failed set ship config %s\n", statusString(status));
  //   return;
  // }
  // FT_Close(handle);
  // printf("set config done");
  // // // 
  // usleep(10000);
  // while(true) {
  //   status = FT_Create(0, FT_OPEN_BY_INDEX, &handle);
  //   if (FT_FAILED(status)) {
  //     printf("failed created %s\n", statusString(status));
  //     usleep(1000 * 100);
  //   }else {
  //     break;
  //   }
  // }
  
  printf("Device created\r\n");

  FT_SetPipeTimeout(handle,0x02,1000);
  FT_SetPipeTimeout(handle,0x82,10000);
  

  uint8_t *in_buf = (uint8_t *)malloc(32);

  uint8_t message[32];
  memset(message, '\0', 32);
  // for (size_t i = 0; i < 8; i++) {
  //   *(message + i) =  CODE_CONNECT_REQURST[7-i];
  // }
  _pack_uint8_arr(message, CODE_CONNECT_REQURST, 8);

  DWORD count;

  FT_AbortPipe(handle, 0x02);
  status = FT_WritePipe(handle, 0x02, CODE_CONNECT_REQURST1, 32,
                        &count, NULL);
  if (FT_FAILED(status)) {
    printf("Failed to write %s\n", statusString(status));
    return;
  }
  printf("1 Wrote %d bytes\n", count);

  // status = FT_WritePipe(handle, 0x02, message, 32,
  //                       &count, NULL);
  // if (FT_FAILED(status)) {
  //   printf("Failed to write %s\n", statusString(status));
  //   return;
  // }
  // printf("2 Wrote %d bytes\n", count);

 // FT_SetPipeTimeout(handle, 0xFF, 1000);
  status = FT_OK;
  status = FT_ReadPipe(handle, 0x82, in_buf, 32, &count, NULL);
  if (FT_FAILED(status)) {
    printf("Failed to read %s %d\n", statusString(status), count);
    FT_Close(handle);
    return;
  }
  printf("Read %d bytes\r\n", count);
  for (size_t i = 0; i < 8; i++) {
    printf("%x \r\n", in_buf[i]);
  }
  fflush(stdout);
  FT_Close(handle);
}


void set_ft600_optional_features(USHORT *features)
{
  USHORT flag;

  flag = CONFIGURATION_OPTIONAL_FEATURE_ENABLENOTIFICATIONMESSAGE_INCHALL;
  if (flag & *features) {
    *features &= ~flag;
    printf("Notification feature is not supported\r\n");
  }

  flag = CONFIGURATION_OPTIONAL_FEATURE_DISABLECANCELSESSIONUNDERRUN;
  if (!(flag & *features)) {
    *features |= flag;
    printf("Cancel session on underrun is not supported\r\n");
  }

  flag = CONFIGURATION_OPTIONAL_FEATURE_DISABLEUNDERRUN_INCHALL;
  if (!(flag & *features)) {
    *features |= flag;
    printf("Set 'disable underrun' to get better performance\r\n");
  }
}

static uint8_t in_ch_cnt;
static uint8_t out_ch_cnt;

void set_ft600_channels(UCHAR *channels, bool is_600_mode)
{
  if (in_ch_cnt == 1 && out_ch_cnt == 0) {
    *channels = CONFIGURATION_CHANNEL_CONFIG_1_INPIPE;
  } else if (in_ch_cnt == 0 && out_ch_cnt == 1)
    *channels = CONFIGURATION_CHANNEL_CONFIG_1_OUTPIPE;
  else {
      UCHAR total = in_ch_cnt < out_ch_cnt ? out_ch_cnt : in_ch_cnt;

      if (total == 4)
        *channels = CONFIGURATION_CHANNEL_CONFIG_4;
      else if (total == 2)
          *channels = CONFIGURATION_CHANNEL_CONFIG_2;
        else
          *channels = CONFIGURATION_CHANNEL_CONFIG_1;

      if (!is_600_mode && total > 1) {
        printf("245 mode only support single channel\r\n");
        exit (-1);
      }
    }
}

void set_ft600_channel_config(FT_60XCONFIGURATION *cfg,
                              CONFIGURATION_FIFO_CLK clock, bool is_600_mode)
{
  cfg->FIFOClock = clock;
  cfg->FIFOMode = is_600_mode ? CONFIGURATION_FIFO_MODE_600 :
    CONFIGURATION_FIFO_MODE_245;
  set_ft600_optional_features(&cfg->OptionalFeatureSupport);
  set_ft600_channels(&cfg->ChannelConfig, is_600_mode);

  const char *const CH_STR[] = {
    "4CH",
    "2CH",
    "1CH",
    "1CH OUT",
    "1CH IN",
  };
  const char *const FIFO_STR[] = {
    "FT245",
    "FT600",
  };
  printf("%s %s @ 100MHz\r\n",
         CH_STR[cfg->ChannelConfig], FIFO_STR[cfg->FIFOMode]);
}

bool set_channel_config(bool is_600_mode, CONFIGURATION_FIFO_CLK clock)
{
  printf("set_channel_config \n");
  FT_HANDLE handle = NULL;
  DWORD dwType = FT_DEVICE_UNKNOWN;

  FT_Create(0, FT_OPEN_BY_INDEX, &handle);
  if (!handle) {
    printf("failed to created device in set channl config");
    return false;
  }

  FT_GetDeviceInfoDetail(0, NULL, &dwType, NULL, NULL, NULL, NULL, NULL);

  union CHIP_CONFIGURATION {
    FT_60XCONFIGURATION ft600;
  } old_cfg, new_cfg;

  if (FT_OK != FT_GetChipConfiguration(handle, &old_cfg)) {
    printf("Failed to get chip conf\r\n");
    return false;
  }
  memcpy(&new_cfg, &old_cfg, sizeof(union CHIP_CONFIGURATION));

  if (dwType == FT_DEVICE_600 || dwType == FT_DEVICE_601)
    set_ft600_channel_config(&new_cfg.ft600, clock, is_600_mode);
  else
    return false;
  if (memcmp(&new_cfg, &old_cfg, sizeof(union CHIP_CONFIGURATION))) {
    FT_STATUS status =  FT_SetChipConfiguration(handle, &new_cfg);
    if (FT_OK !=  status){
      printf("Failed to set chip conf %s\r\n", statusString(status));
      exit(-1);
    } else {
      printf("Configuration changed\r\n");
      get_device_lists();
    }
  }
  FT_Close(handle);
  return true;
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

bool SetChipConfigurationExample() {
  FT_STATUS ftStatus = FT_OK;
  FT_HANDLE ftHandle;
  FT_60XCONFIGURATION oConfigurationData = { 0 };
  ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &ftHandle);
  oConfigurationData.VendorID = CONFIGURATION_DEFAULT_VENDORID;
  oConfigurationData.ProductID = CONFIGURATION_DEFAULT_PRODUCTID_601;
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
  oConfigurationData.OptionalFeatureSupport =
    CONFIGURATION_OPTIONAL_FEATURE_DISABLECANCELSESSIONUNDERRUN;
  if(SetStringDescriptorsExample(oConfigurationData.StringDescriptors,
                       sizeof(oConfigurationData.StringDescriptors),
                       "MyCompany", "This Is My Product Description", "1234567890ABXCM")) {
    printf("ues");
  }
  FT_SetChipConfiguration(ftHandle, &oConfigurationData);
  FT_Close(ftHandle);
  return true;
}


void null_config() {
  FT_STATUS status;
  FT_HANDLE handle;
 status =  FT_Create(0, FT_OPEN_BY_INDEX, &handle);
  if (FT_FAILED(status)) {
    printf("failed created %s\n", statusString(status));
    return;
  }

  status = FT_SetChipConfiguration(handle, NULL);
  if (FT_FAILED(status)) {
    printf("failed set ship config %s\n", statusString(status));
    return;
  }
  FT_Close(handle);
  printf("set config done");
}

int main (int argc, char *argv[]) {

  //  get_version();
  //  device_count();
  //  get_device_lists();
  //get_chip_configuration();
  //
  //out_ch_cnt = 3;
  ///in_ch_cnt = 1;
  //set_channel_config(true, CONFIGURATION_FIFO_CLK_100);
  //  get_device_lists();
  SetChipConfigurationExample();
 // get_chip_configuration();

  //null_config();
  //read_and_write();
  return 0;
}

