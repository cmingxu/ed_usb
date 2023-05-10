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

  // 连接设备
  addr_t *addr = (addr_t *)malloc(sizeof(addr_t));
  memset(addr, '\0', sizeof(addr_t));
 create_handle_with_serial_num(cfg, addr);


  return 0;
}
