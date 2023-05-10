#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "../ed.h"
#include "../utils.h"

int main(int argc, const char *argv[])
{
  config_t *cfg = (config_t *)malloc(sizeof(config_t));
  load_default_config(cfg);
  cfg->sample_count = 880000;
  // cfg->delay_count = 453333;
  cfg->repeat_count = 30;

  // 连接设备
  addr_t *addr = (addr_t *)malloc(sizeof(addr_t));
  memset(addr, '\0', sizeof(addr_t));
  int establish_res;
  if((establish_res = create_handle(cfg, addr)) != 0) {
    printf("establish_connection failed %d\n", establish_res);
    exit(1);
  }

  abort_pipe(addr);

  // 停止接受, 防止之前有采集任务, 可多次调用
  int stop_res = stop_collect(cfg, addr);
  if(stop_res != STOP_COLLECT_SUCCESS) {
    exit(1);
  }

  // 建立连接， 向设备发连接指令
  int connect_res = dev_connect(cfg, addr);
  if (connect_res != CONNECT_SUCCESS) {
    printf("connect failed , code %d", connect_res);
    goto END;
  }

  //  // 发送配置到设备
  int config_status = send_config_to_device(cfg, addr);
  if(config_status != SEND_CONFIG_SUCCESS) {
    printf("config failed , code %d", config_status);
    goto END;
  }


  // 通知设备开始采集
  int start_status = start_collect(cfg, addr);
  if(start_status != START_COLLECT_SUCCESS) {
    printf("start failed , code %d", start_status);
    goto END;
  }

  // 准备接收数据的内存
  // 这里注意数据要及时转走
  int cnt = cfg->sample_count * cfg->ad_channel * 2;

  for(int i = 0; i < cfg->repeat_count; i++) {
    void *buf = malloc(cnt);
    int recvCnt = start_recv_repeat_n(cfg, addr, i, buf, cnt);
    printf("repeat index %d => recvCnt %d\n", i, recvCnt);
    usleep(1000);
    if(recvCnt != cnt ) {
      goto END;
    }
  }

  //get_queue_status(addr->handle);
  stop_collect(cfg, addr);
END: 
  close_handle(cfg, addr);

  return 0;
}