#include "wlndrvce/wlan.h"
#include <tice.h>
#include <usbdrvce.h>

static void log_callback(const char *msg) {
  os_PutStrFull(msg);
  os_NewLine();
}

static usb_error_t event_handler(usb_event_t event, void *data,
                                 void *user_data) {
  switch (event) {
  case USB_DEVICE_CONNECTED_EVENT:
    os_PutStrFull("Device connected");
    os_NewLine();
    break;
  case USB_DEVICE_ENABLED_EVENT:
    os_ClrHome();
    os_PutStrFull("Device enabled");
    os_NewLine();
    break;
  case USB_DEVICE_DISCONNECTED_EVENT:
    os_PutStrFull("Device removed");
    os_NewLine();
    break;
  default:
    break;
  }
  return wlan_usb_event_handler(event, data, user_data);
}

int main(void) {
  bool was_attached = false;

  os_ClrHome();
  os_PutStrFull("RTL819x Driver Loader");
  os_NewLine();
  os_PutStrFull("Waiting for hardware...");
  os_NewLine();
  wlan_init();
  wlan_register_log_callback(log_callback);

  if (usb_Init(event_handler, NULL, NULL, USB_DEFAULT_INIT_FLAGS)) {
    os_ClrHome();
    os_PutStrFull("USB Error!");
    os_NewLine();
    while (os_GetCSC() != sk_Clear)
      ;
    return 1;
  }

  while (os_GetCSC() != sk_Clear) {
    if (usb_HandleEvents() != USB_SUCCESS) {
      break;
    }
    wlan_service();

    if (wlan_driver.attached && !was_attached) {
      os_ClrHome();
      os_PutStrFull("Driver Attached");
      os_NewLine();
      if (wlan_driver.model_name) {
        os_PutStrFull(wlan_driver.model_name);
        os_NewLine();
      }
      wlan_initialize_chipset();
      was_attached = true;
    } else if (!wlan_driver.attached && was_attached) {
      was_attached = false;
    }
  }

  usb_Cleanup();
  return 0;
}
