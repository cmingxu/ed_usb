#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./include/ftd3xx.h"
#include "include/WinTypes.h"

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
  DWORD dwVersion;

  FT_GetDriverVersion(NULL, &dwVersion);
  printf("Driver version:%d.%d.%d\r\n", dwVersion >> 24,
         (uint8_t)(dwVersion >> 16), dwVersion & 0xFFFF);

  FT_GetLibraryVersion(&dwVersion);
  printf("Library version:%d.%d.%d\r\n", dwVersion >> 24,
         (uint8_t)(dwVersion >> 16), dwVersion & 0xFFFF);
}

void device_count(void){
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
  printf("SerialNumber=%s\n ", config->StringDescriptors);
  printf("Interval=%d\n ", config->bInterval);
}

void get_device_lists(void){
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
    FT_HANDLE                    ftHandle;
    FT_STATUS                    ftStatus;
    FT_60XCONFIGURATION          chipConfig;
	  DWORD dwType = FT_DEVICE_UNKNOWN;

    ftStatus = FT_Create(0, FT_OPEN_BY_INDEX, &ftHandle);
    if (FT_FAILED(ftStatus))
    {
        printf("%s:%d: ERROR: FT_Create failed (%s)\n",__FILE__,__LINE__,statusString(ftStatus));
    }

	// DWORD dwIndex,
	// LPDWORD lpdwFlags,
	// LPDWORD lpdwType,
	// LPDWORD lpdwID,
	// LPDWORD lpdwLocId,
	// LPVOID lpSerialNumber,
	// LPVOID lpDescription,
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
#define TIMEOUT			0 //0xFFFFFFFF
void read_and_write() {
  FT_HANDLE handle;
  FT_Create(0, FT_OPEN_BY_INDEX, &handle);

  if (!handle) {
    printf("Failed to create device\r\n");
    return;
  }
  printf("Device created\r\n");

  FT_SetPipeTimeout(handle,0x02,0);
  FT_SetPipeTimeout(handle,0x82,0);

  uint8_t *out_buf = (uint8_t *)malloc(2);
  uint8_t *in_buf = (uint8_t *)malloc(2);
  DWORD count;

  if (FT_OK != FT_WritePipeEx(handle, FIFO_CHANNEL_1, out_buf, 2,
                              &count, TIMEOUT)) {
    printf("Failed to write\r\n");
    return;
  }
  printf("Wrote %d bytes\r\n", count);

  if (FT_OK != FT_ReadPipeEx(handle, FIFO_CHANNEL_1, in_buf, 2,
                             &count, TIMEOUT)) {
    printf("Failed to read\r\n");
    return;
  }
  printf("Read %d bytes\r\n", count);
}

int main (int argc, char *argv[])
{
  printf("hello\n");
  get_version();
  device_count();
  get_device_lists();
  get_chip_configuration();

  read_and_write();
  return 0;
}

