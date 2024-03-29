#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>

#include "ed.h"
#include "utils.h"

// config keys
static const char *KEY_SAMPLE_COUNT = "sample_count";
static const char *KEY_DELAY_COUNT = "delay_count";
static const char *KEY_REPEAT_COUNT = "repeat_count";
static const char *KEY_DOWN_SAMPLE_COUNT = "down_sample_count";
static const char *KEY_AD_CHANNEL = "ad_channel";
static const char *KEY_AD_BIT = "ad_bit";
static const char *KEY_TRIGGER = "trigger";
static const char *KEY_OUTER_TRIGGER = "outer_trigger";

static bool _write_config_prop_uint32(FILE *, const char *, uint32_t);
static bool _write_config_prop_uint16(FILE *, const char *, uint16_t);
static bool _write_config_prop_unsigned(FILE *, const char *, unsigned int);
static bool _write_config_prop_short(FILE *, const char *, short);
static bool _write_config_prop_str(FILE *, const char *, const char*);

static bool _read_config_prop_uint32(const char *, uint32_t *);
static bool _read_config_prop_uint16(const char *, uint16_t *);
static bool _read_config_prop_unsigned(const char *, unsigned int *);
static bool _read_config_prop_short(const char *, short *);
static bool _read_config_prop_str(const char *, char *);

static bool has_prefix(const char*, const char *) ;


int usb_load_default_config(usb_config_t * c) {
  assert(c);
  c->sample_count = 1000000;
  c->down_sample_count = 2;
  c->delay_count = 320000;
  c->repeat_count = 3;

  c->ad_channel = USB_ADCHANNEL_SINGLE;
  c->ad_bit = USB_ADBIT_12;
  //c->trigger = TRIGGER_OUTER;
  c->trigger = USB_TRIGGER_INNER;
  c->outer_trigger = USB_OUTER_TRIGGER_DOWN;

  return 0;
}

// update local addr, local ip and local port update respectively
int set_local_addr(usb_config_t *c, const char*ip, unsigned int port) {
  strcpy(c->local_ip, ip);
  c->local_port = port;
  return 0;
}

// update local addr, local ip and local port update respectively
int set_device_addr(usb_config_t *c, const char*ip, unsigned int port) {
  strcpy(c->device_ip, ip);
  c->device_port = port;
  return 0;
}

int write_config(usb_config_t *c, FILE *cf) {
  assert(c);

  ftruncate(fileno(cf), 0);
  _write_config_prop_uint32(cf, KEY_SAMPLE_COUNT, c->sample_count);
  _write_config_prop_uint32(cf, KEY_DELAY_COUNT, c->delay_count);
  _write_config_prop_uint16(cf, KEY_REPEAT_COUNT, c->repeat_count);
  _write_config_prop_uint16(cf, KEY_DOWN_SAMPLE_COUNT, c->down_sample_count);
  _write_config_prop_short(cf, KEY_AD_CHANNEL, c->ad_channel);
  _write_config_prop_short(cf, KEY_AD_BIT, c->ad_bit);
  _write_config_prop_short(cf, KEY_TRIGGER, c->trigger);
  _write_config_prop_short(cf, KEY_OUTER_TRIGGER, c->outer_trigger);
  fflush(cf);

  return USB_WRITE_CONFIG_SUCCESS;
}

int load_config(usb_config_t *c, FILE *config_file) {
  assert(c);

  char local_ip_buf[128], device_ip_buf[128];
  memset(local_ip_buf, '\0', 128);
  memset(device_ip_buf, '\0', 128);

  char buf[128];
  rewind(config_file);
  while(fgets(buf, 128, config_file) != NULL){
    if(has_prefix(buf, KEY_SAMPLE_COUNT)) {
      _read_config_prop_uint32(buf, &c->sample_count);
    }

    if(has_prefix(buf, KEY_DELAY_COUNT)) {
      _read_config_prop_uint32(buf, &c->delay_count);
    }

    if(has_prefix(buf, KEY_REPEAT_COUNT)) {
      _read_config_prop_uint16(buf, &c->repeat_count);
    }

    if(has_prefix(buf, KEY_DOWN_SAMPLE_COUNT)) {
      _read_config_prop_uint16(buf, &c->down_sample_count);
    }

    if(has_prefix(buf, KEY_AD_CHANNEL)) {
      _read_config_prop_short(buf, &c->ad_channel);
    }

    if(has_prefix(buf, KEY_AD_BIT)) {
      _read_config_prop_short(buf, &c->ad_bit);
    }

    if(has_prefix(buf, KEY_TRIGGER)) {
      _read_config_prop_short(buf, &c->trigger);
    }

    if(has_prefix(buf, KEY_OUTER_TRIGGER)) {
      _read_config_prop_short(buf, &c->outer_trigger);
    }
  }

  return USB_READ_CONFIG_SUCCESS;
}



static bool _write_config_prop_str(FILE *config_file, const char *key, const char*str){
  fprintf(config_file, "%s=%s\n", key, str);
  return true;
}

static bool _write_config_prop_uint32(FILE *config_file, const char *key, uint32_t value){
  fprintf(config_file, "%s=%d\n", key, value);
  return true;
}

static bool _write_config_prop_uint16(FILE *config_file, const char *key, uint16_t value){
  fprintf(config_file, "%s=%d\n", key, value);
  return true;
}

static bool _write_config_prop_unsigned(FILE *config_file, const char *key, unsigned int value){
  fprintf(config_file, "%s=%d\n", key, value);
  return true;
}

static bool _write_config_prop_short(FILE *config_file, const char *key, short value){
  fprintf(config_file, "%s=%d\n", key, value);
  return true;
}

static bool _read_config_prop_uint32(const char *buf, uint32_t *value){
  char *pos = strchr(buf, '=');
  if(pos != NULL) {
    *value = (uint32_t)atoi(pos+1);
  } else{
    *value = 0;
  }
  return true;
}

static bool _read_config_prop_uint16(const char *buf, uint16_t *value){
  char *pos = strchr(buf, '=');
  if(pos != NULL) {
    *value = (uint16_t)atoi(pos+1);
  } else{
    *value = 0;
  }
  return true;
}

static bool _read_config_prop_unsigned(const char *buf, unsigned int *value){
  char *pos = strchr(buf, '=');
  if(pos != NULL) {
    *value = (unsigned int)atoi(pos+1);
  } else{
    *value = 0;
  }
  return true;
}

static bool _read_config_prop_short(const char *buf, short *value){
  char *pos = strchr(buf, '=');
  if(pos != NULL) {
    *value = (short)atoi(pos+1);
  } else{
    *value = 0;
  }
  return true;
}

static bool _read_config_prop_str(const char *buf, char *value){
  char *pos = strchr(buf, '=');
  if(pos != NULL) {
    strcpy(value, pos + 1);
  }
  return true;
}

static bool has_prefix(const char*str, const char *pre) {
  size_t lenpre = strlen(pre),
         lenstr = strlen(str);
  return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}
