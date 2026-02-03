#pragma once

#include "wlan.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <usbdrvce.h>

typedef struct
{
  usb_device_t device;
  bool attached;
  const char *model_name;
} wlndrvce_rtl_driver_t;

wlan_result_t wlndrvce_load_firmware_chunks_for_id(const char id_letters[4]);

usb_error_t wlndrvce_enabled_handler(usb_device_t device);
usb_error_t wlndrvce_event_handler(usb_event_t event, void *data,
                                   void *user_data);

usb_error_t wlndrvce_send_firmware_block(usb_device_t device,
                                         const uint8_t *data, size_t len);
