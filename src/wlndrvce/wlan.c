#include "wlan.h"
#include "chipsets/ar9271.h"
#include "driver.h"
#include <stdio.h>
#include <tice.h>
#include <usbdrvce.h>

wlan_driver_t wlan_driver = {0};

wlan_result_t wlan_init(void)
{
  wlan_driver.device = NULL;
  wlan_driver.attached = false;
  wlan_driver.model_name = NULL;
  wlan_driver.chipset = CHIPSET_UNKNOWN;
  return WLAN_SUCCESS;
}

void wlan_service(void)
{
  // periodic WLAN tasks here if needed
}

void wlan_handle_device_connected(usb_device_t device) { (void)device; }

void wlan_handle_device_enabled(usb_device_t device) { (void)device; }

void wlan_handle_device_disconnected(usb_device_t device)
{
  (void)device;
  if (wlan_driver.attached && wlan_driver.device == device)
  {
    if (wlan_driver.chipset == CHIPSET_AR9271)
    {
      ar9271_deinit(&wlan_driver);
    }
    wlan_driver.attached = false;
    wlan_driver.device = NULL;
    wlan_driver.model_name = NULL;
  }
}

void wlan_attach_supported_device(usb_device_t device, const char *model_name,
                                  wlan_chipset_t chipset)
{
  wlan_driver.device = device;
  wlan_driver.attached = true;
  wlan_driver.model_name = model_name ? model_name : "WLAN Adapter";
  wlan_driver.chipset = chipset;
}

wlan_result_t wlan_initialize_chipset(wlan_progress_cb_t cb)
{
  switch (wlan_driver.chipset)
  {
  case CHIPSET_AR9271:
    return ar9271_init(&wlan_driver, cb);
  default:
    return WLAN_ERROR_UNKNOWN;
  }
}

void wlan_debug_dump_state(wlan_log_cb_t log_cb)
{
    if (wlan_driver.chipset == CHIPSET_AR9271)
    {
        ar9271_debug_dump(&wlan_driver, log_cb);
    }
    else
    {
        if(log_cb) log_cb("No unknown chipset");
    }
}

wlan_result_t wlan_stream_firmware_chunks(usb_device_t device, const char id_letters[4], wlan_progress_cb_t cb)
{
  return wlndrvce_load_firmware_chunks_for_id(device, id_letters, cb);
}

wlan_result_t wlan_send_firmware_block(usb_device_t device, const uint8_t *data,
                                       size_t len, uint32_t offset)
{
  /* TODO: send firmware package to adapter
   *  Implement transfer logic over USB in wlndrvce_send_firmware_block (no UI).*/
  if (wlndrvce_send_firmware_block(device, data, len, offset) == USB_SUCCESS)
  {
    return WLAN_SUCCESS;
  }
  return WLAN_ERROR_USB_TRANSFER_FAILED;
}
