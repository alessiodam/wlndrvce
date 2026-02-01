#include "wlan.h"
#include "chipsets/ar9271.h"
#include "driver.h"
#include <stdio.h>
#include <tice.h>
#include <usbdrvce.h>

wlan_driver_t wlan_driver = {0};
static wlan_log_callback_t s_log_callback = NULL;

void wlan_register_log_callback(wlan_log_callback_t callback) {
  s_log_callback = callback;
}

void wlndrvce_show_status(const char *line1, const char *line2) {
  char buf[128];
  if (s_log_callback) {
    snprintf(buf, sizeof(buf), "%s: %s", line1, line2);
    s_log_callback(buf);
  }
}

void wlan_init(void) {
  wlan_driver.device = NULL;
  wlan_driver.attached = false;
  wlan_driver.model_name = NULL;
  wlan_driver.chipset = CHIPSET_UNKNOWN;
}

void wlan_service(void) {
  // periodic WLAN tasks here if needed
}

void wlan_handle_device_connected(usb_device_t device) { (void)device; }

void wlan_handle_device_enabled(usb_device_t device) { (void)device; }

void wlan_handle_device_disconnected(usb_device_t device) {
  (void)device;
  if (wlan_driver.attached && wlan_driver.device == device) {
    if (wlan_driver.chipset == CHIPSET_AR9271) {
      ar9271_deinit(&wlan_driver);
    }
    wlan_driver.attached = false;
    wlan_driver.device = NULL;
    wlan_driver.model_name = NULL;
  }
}

void wlan_attach_supported_device(usb_device_t device, const char *model_name,
                                  wlan_chipset_t chipset) {
  wlan_driver.device = device;
  wlan_driver.attached = true;
  wlan_driver.model_name = model_name ? model_name : "WLAN Adapter";
  wlan_driver.chipset = chipset;
}

void wlan_initialize_chipset(void) {
  switch (wlan_driver.chipset) {
  case CHIPSET_AR9271:
    ar9271_init(&wlan_driver);
    break;
  default:
    break;
  }
}

void wlan_stream_firmware_chunks(const char id_letters[4]) {
  wlndrvce_load_firmware_chunks_for_id(id_letters);
}

usb_error_t wlan_send_firmware_block(usb_device_t device, const uint8_t *data,
                                     size_t len) {
  /* TODO: send firmware package to adapter
   *  Implement transfer logic over USB in wlndrvce_send_firmware_block (no UI).*/
  return wlndrvce_send_firmware_block(device, data, len);
}
