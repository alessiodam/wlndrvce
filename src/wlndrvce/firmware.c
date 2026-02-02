#include <fileioc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include <usbdrvce.h>

wlan_result_t wlndrvce_load_firmware_chunks_for_id(const char id_letters[4]) {
  char name[9];
  int loaded = 0;

  for (int i = 0; i < 26; i++) {
    int n =
        snprintf(name, sizeof(name), "WLFW%s%c", id_letters, (char)('A' + i));
    if (n <= 0 || n >= (int)sizeof(name)) {
      break;
    }

    ti_var_t h = ti_Open(name, "r");
    if (!h) {
      if (loaded == 0) {
        return WLAN_ERROR_FIRMWARE_NOT_FOUND;
      }
      break;
    }

    const size_t BLK = 1024;
    uint8_t *buf = malloc(BLK);
    if (!buf) {
      ti_Close(h);
      return WLAN_ERROR_OUT_OF_MEMORY;
    }

    size_t total = 0;
    while (true) {
      size_t nread = ti_Read(buf, 1, BLK, h);
      if (nread == 0) {
        break;
      }
      (void)total;
      (void)nread;
      (void)buf;
      total += nread;
    }
    ti_Close(h);
    free(buf);
    loaded++;
  }
  return loaded > 0 ? WLAN_SUCCESS : WLAN_ERROR_FIRMWARE_NOT_FOUND;
}

usb_error_t wlndrvce_send_firmware_block(usb_device_t device,
                                         const uint8_t *data, size_t len) {
  (void)device;
  (void)data;
  (void)len;
  /* TODO: send firmware package to adapter
   *  Implement bulk/interrupt/control transfer logic to push 'data' of 'len'
   *  bytes to the appropriate endpoint on 'device', with error handling and
   *  retries.*/
  return USB_SUCCESS;
}
