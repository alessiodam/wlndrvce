#include <fileioc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include <usbdrvce.h>

wlan_result_t wlndrvce_load_firmware_chunks_for_id(usb_device_t device, const char id_letters[4], wlan_progress_cb_t cb)
{
  char name[9];
  int loaded = 0;
  size_t total_size = 0;
  size_t current_progress = 0;

  for (int i = 0; i < 26; i++)
  {
    int n = snprintf(name, sizeof(name), "WLFW%s%c", id_letters, (char)('A' + i));
    if (n <= 0 || n >= (int)sizeof(name))
      break;

    ti_var_t h = ti_Open(name, "r");
    if (!h)
    {
      if (i == 0)
        return WLAN_ERROR_FIRMWARE_NOT_FOUND;
      break;
    }
    total_size += ti_GetSize(h);
    ti_Close(h);
  }

  for (int i = 0; i < 26; i++)
  {
    int n =
        snprintf(name, sizeof(name), "WLFW%s%c", id_letters, (char)('A' + i));
    if (n <= 0 || n >= (int)sizeof(name))
    {
      break;
    }

    ti_var_t h = ti_Open(name, "r");
    if (!h)
    {
      if (loaded == 0)
      {
        return WLAN_ERROR_FIRMWARE_NOT_FOUND;
      }
      break;
    }

    const size_t BLK = 4096;
    uint8_t *buf = malloc(BLK);
    if (!buf)
    {
      ti_Close(h);
      return WLAN_ERROR_OUT_OF_MEMORY;
    }

    while (true)
    {
      size_t nread = ti_Read(buf, 1, BLK, h);
      if (nread == 0)
      {
        break;
      }

      if (wlndrvce_send_firmware_block(device, buf, nread, current_progress) != USB_SUCCESS)
      {
          ti_Close(h);
          free(buf);
          return WLAN_ERROR_USB_TRANSFER_FAILED;
      }

      current_progress += nread;
      if (cb)
      {
        cb("Uploading FW", current_progress, total_size);
      }
    }
    ti_Close(h);
    free(buf);
    loaded++;
  }

  if (loaded > 0)
  {
      if (cb) cb("Booting FW", 0, 0);
      wlndrvce_finish_firmware_upload(device);
      
      return WLAN_SUCCESS;
  }
  return WLAN_ERROR_FIRMWARE_NOT_FOUND;
}

#define AR9271_USB_VENQT_WRITE (USB_HOST_TO_DEVICE | USB_VENDOR_REQUEST | USB_RECIPIENT_DEVICE)
#define AR9271_REQ_FW_DOWNLOAD 0x30
#define AR9271_REQ_FW_DOWNLOAD_COMP 0x31

#define AR9271_FIRMWARE 0x501000
#define AR9271_FIRMWARE_TEXT 0x903000

usb_error_t wlndrvce_send_firmware_block(usb_device_t device,
                                         const uint8_t *data, size_t len, uint32_t offset)
{
  if (!device) return USB_SUCCESS;

  uint32_t real_addr = AR9271_FIRMWARE + offset;

  usb_control_setup_t setup = {
      .bmRequestType = AR9271_USB_VENQT_WRITE,
      .bRequest = AR9271_REQ_FW_DOWNLOAD,
      .wValue = (uint16_t)(real_addr >> 8),
      .wIndex = 0,
      .wLength = (uint16_t)len
  };

  return usb_DefaultControlTransfer(device, &setup, (void*)data, 1000, NULL);
}

usb_error_t wlndrvce_finish_firmware_upload(usb_device_t device)
{
  if (!device) return USB_SUCCESS;

  usb_control_setup_t setup = {
      .bmRequestType = AR9271_USB_VENQT_WRITE,
      .bRequest = AR9271_REQ_FW_DOWNLOAD_COMP,
      .wValue = (uint16_t)(AR9271_FIRMWARE_TEXT >> 8),
      .wIndex = 0,
      .wLength = 0
  };
  return usb_DefaultControlTransfer(device, &setup, NULL, 50, NULL);
}
