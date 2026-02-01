
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <usbdrvce.h>

typedef enum {
  CHIPSET_UNKNOWN = 0,
  CHIPSET_AR9271,
} wlan_chipset_t;

typedef struct {
  usb_device_t device;
  bool attached;
  const char *model_name;
  wlan_chipset_t chipset;
  uint8_t mac[6];
} wlan_driver_t;

extern wlan_driver_t wlan_driver;

typedef void (*wlan_log_callback_t)(const char *msg);
void wlan_register_log_callback(wlan_log_callback_t callback);

void wlan_init(void);

void wlan_service(void);
usb_error_t wlan_usb_event_handler(usb_event_t event, void *data,
                                   void *user_data);
usb_error_t wlan_usb_enabled_handler(usb_device_t device);

void wlan_handle_device_connected(usb_device_t device);
void wlan_handle_device_enabled(usb_device_t device);
void wlan_handle_device_disconnected(usb_device_t device);

void wlan_attach_supported_device(usb_device_t device, const char *model_name,
                                  wlan_chipset_t chipset);
void wlan_initialize_chipset(void);

void wlan_stream_firmware_chunks(const char id_letters[4]);

usb_error_t wlan_send_firmware_block(usb_device_t device, const uint8_t *data,
                                     size_t len)
    __attribute__((warn_unused_result));
