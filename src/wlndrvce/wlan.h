
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <usbdrvce.h>

typedef enum
{
  WLAN_SUCCESS = 0,
  WLAN_ERROR_NONE = 0,
  WLAN_ERROR_UNKNOWN,
  WLAN_ERROR_TIMEOUT,
  WLAN_ERROR_NO_DEVICE,
  WLAN_ERROR_INVALID_PARAM,
  WLAN_ERROR_OUT_OF_MEMORY,
  WLAN_ERROR_FIRMWARE_NOT_FOUND,
  WLAN_ERROR_FIRMWARE_LOAD_FAILED,
  WLAN_ERROR_USB_TRANSFER_FAILED,
} wlan_result_t;

typedef enum
{
  CHIPSET_UNKNOWN = 0,
  CHIPSET_ATH9K_HTC,
} wlan_chipset_t;

typedef struct
{
  usb_device_t device;
  bool attached;
  const char *model_name;
  wlan_chipset_t chipset;
  uint8_t mac[6];
} wlan_driver_t;

extern wlan_driver_t wlan_driver;

typedef void (*wlan_progress_cb_t)(const char *step_name, size_t processed, size_t total);
typedef void (*wlan_log_cb_t)(const char *msg);

wlan_result_t wlan_init(void);

void wlan_service(void);

void wlan_handle_device_connected(usb_device_t device);
void wlan_handle_device_enabled(usb_device_t device);
void wlan_handle_device_disconnected(usb_device_t device);

void wlan_attach_supported_device(usb_device_t device, const char *model_name,
                                  wlan_chipset_t chipset);
wlan_result_t wlan_initialize_chipset(wlan_progress_cb_t cb);
void wlan_debug_dump_state(wlan_log_cb_t log_cb);

wlan_result_t wlan_stream_firmware_chunks(usb_device_t device, const char id_letters[4], wlan_progress_cb_t cb);

wlan_result_t wlan_send_firmware_block(usb_device_t device, const uint8_t *data,
                                       size_t len, uint32_t offset)
    __attribute__((warn_unused_result));
