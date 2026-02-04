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
} wlan_usb_rtl_driver_t;

wlan_result_t wlan_usb_load_firmware_chunks_for_id(usb_device_t device, const char id_letters[4], wlan_progress_cb_t cb);

usb_error_t wlan_usb_enabled_handler(usb_device_t device);
usb_error_t wlan_usb_event_handler(usb_event_t event, void *data,
                                   void *user_data);

usb_error_t wlan_usb_send_firmware_block(usb_device_t device,
                                         const uint8_t *data, size_t len, uint32_t offset);
usb_error_t wlan_usb_finish_firmware_upload(usb_device_t device);