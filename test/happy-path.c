#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "ed.h"
#include "utils.h"

int main(int argc, const char *argv[])
{
  usb_config_t *cfg = (usb_config_t *)malloc(sizeof(usb_config_t));
  usb_load_default_config(cfg);
  cfg->sample_count = 1000000;
  cfg->delay_count = 320000;
  cfg->repeat_count = 3;
  cfg->ad_channel = 1;
  cfg->trigger = USB_TRIGGER_OUTER;

  // 连接设备
  usb_addr_t *addr = (usb_addr_t *)malloc(sizeof(usb_addr_t));
  memset(addr, '\0', sizeof(usb_addr_t));
  int establish_res;
  if((establish_res = usb_create_handle(cfg, addr)) != 0) {
    printf("establish_connection failed %d\n", establish_res);
    exit(1);
  }

  abort_pipe(addr);

  // 停止接受, 防止之前有采集任务, 可多次调用
 int stop_res = usb_stop_collect(cfg, addr);
 if(stop_res != USB_STOP_COLLECT_SUCCESS) {
   exit(1);
 }

  // 建立连接， 向设备发连接指令
  int connect_res = usb_dev_connect(cfg, addr);
  if (connect_res != USB_CONNECT_SUCCESS) {
    printf("connect failed , code %d", connect_res);
    goto END;
  }

  //  发送配置到设备
  int config_status = usb_send_config_to_device(cfg, addr);
  if(config_status != USB_SEND_CONFIG_SUCCESS) {
    printf("config failed , code %d", config_status);
    goto END;
  }


  // 通知设备开始采集
  int start_status = usb_start_collect(cfg, addr);
  if(start_status != USB_START_COLLECT_SUCCESS) {
    printf("start failed , code %d", start_status);
    goto END;
  }

  // 准备接收数据的内存
  // 这里注意数据要及时转走
  int cnt = cfg->sample_count * cfg->ad_channel * 2;

  unsigned int timeout = 1000;
  // 如果是外触发，一直等待 or 等待5s
  // 0就是一直等待
  if(cfg->trigger == USB_TRIGGER_OUTER) {
    timeout = 5 * timeout;
  }
  for(int i = 0; i < cfg->repeat_count; i++) {
    void *buf = malloc(cnt);
    int recvCnt = usb_start_recv_repeat_n(cfg, addr, i, buf, cnt, timeout);
    printf("repeat index %d => recvCnt %d\n", i, recvCnt);
    if(recvCnt != cnt ) {
      goto END;
    }
  }

  //get_queue_status(addr->handle);
  usb_stop_collect(cfg, addr);
END: 
  usb_close_handle(cfg, addr);

  return 0;
}
