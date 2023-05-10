#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "../ed.h"
#include "../utils.h"

int main(int argc, const char *argv[])
{
  config_t *cfg = (config_t *)malloc(sizeof(config_t));
  load_default_config(cfg);
  cfg->sample_count = 880000;
  cfg->delay_count = 453333;
  cfg->repeat_count = 30;

  reset_device245();


  // sleep(10);
  // // 连接设备
  // addr_t *addr = (addr_t *)malloc(sizeof(addr_t));
  // memset(addr, '\0', sizeof(addr_t));
  // int establish_res;
  // if((establish_res = create_handle(cfg, addr)) != 0) {
  //   printf("establish_connection failed %d\n", establish_res);
  //   exit(1);
  // }
  //
  // // 建立连接， 向设备发连接指令
  // int connect_res = dev_connect(cfg, addr);
  // if (connect_res != CONNECT_SUCCESS) {
  //   exit(1);
  // }
  //
  // close_handle(cfg, addr);

  return 0;
}
