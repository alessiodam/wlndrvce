#include "wlndrvce/wlan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tice.h>
#include <usbdrvce.h>

typedef struct {
  usb_control_setup_t setup;
  usb_device_descriptor_t device_descriptor;
} probe_item_t;

extern wlan_driver_t wlan_driver;

static usb_error_t probe_descriptor_handler(usb_endpoint_t endpoint,
                                            usb_transfer_status_t status,
                                            size_t transferred,
                                            usb_transfer_data_t *data) {
  (void)data;
  usb_device_t device = usb_GetEndpointDevice(endpoint);
  probe_item_t *item = usb_GetDeviceData(device);

  if (item == NULL) {
    return USB_SUCCESS;
  }

  if (status != USB_TRANSFER_COMPLETED ||
      transferred != sizeof(usb_device_descriptor_t) ||
      item->device_descriptor.bLength < transferred ||
      item->device_descriptor.bDescriptorType != USB_DEVICE_DESCRIPTOR) {
    usb_SetDeviceData(device, NULL);
    free(item);
    return USB_SUCCESS;
  }

  if (item->device_descriptor.idVendor == 0x0CF3 &&
      item->device_descriptor.idProduct == 0x9271) {
    wlan_attach_supported_device(device, "Atheros AR9271", CHIPSET_AR9271);
  }

  usb_SetDeviceData(device, NULL);
  free(item);
  return USB_SUCCESS;
}

static usb_error_t wlan_usb_enabled_handler(usb_device_t device) {
  probe_item_t *item = usb_GetDeviceData(device);
  if (item == NULL) {
    return USB_SUCCESS;
  }
  item->setup.bmRequestType =
      USB_DEVICE_TO_HOST | USB_STANDARD_REQUEST | USB_RECIPIENT_DEVICE;
  item->setup.bRequest = USB_GET_DESCRIPTOR_REQUEST;
  item->setup.wValue = USB_DEVICE_DESCRIPTOR << 8;
  item->setup.wIndex = 0;
  item->setup.wLength = sizeof(usb_device_descriptor_t);

  return usb_ScheduleDefaultControlTransfer(device, &item->setup,
                                            &item->device_descriptor,
                                            &probe_descriptor_handler, NULL);
}

static usb_error_t wlan_usb_event_handler(usb_event_t event, void *data,
                                          void *user_data) {
  (void)user_data;
  usb_device_t device = data;
  probe_item_t *item;

  switch (event) {
  case USB_DEVICE_CONNECTED_EVENT:
    wlan_handle_device_connected(device);
    item = malloc(sizeof(*item));
    if (item == NULL) {
      return USB_SUCCESS;
    }
    item->device_descriptor.bLength = 0;
    usb_SetDeviceData(device, item);
    if (!(usb_GetRole() & USB_ROLE_DEVICE)) {
      usb_ResetDevice(device);
    }
    break;

  case USB_DEVICE_ENABLED_EVENT:
    wlan_handle_device_enabled(device);
    wlan_usb_enabled_handler(device);
    break;

  case USB_DEVICE_DISCONNECTED_EVENT:
    wlan_handle_device_disconnected(device);
    item = usb_GetDeviceData(device);
    if (item != NULL) {
      usb_SetDeviceData(device, NULL);
      free(item);
    }
    break;

  default:
    break;
  }

  return USB_SUCCESS;
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
  wlan_result_t res;
  char buf[64];

  os_ClrHome();
  os_PutStrFull("RTL819x Driver Loader");
  os_NewLine();
  os_PutStrFull("Waiting for hardware...");
  os_NewLine();

  wlan_init();

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

      os_PutStrFull("Initializing Chipset...");
      os_NewLine();

      res = wlan_initialize_chipset();
      if (res == WLAN_SUCCESS) {
        os_PutStrFull("Chipset Init Success");
        os_NewLine();
        sprintf(buf, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", wlan_driver.mac[0],
                wlan_driver.mac[1], wlan_driver.mac[2], wlan_driver.mac[3],
                wlan_driver.mac[4], wlan_driver.mac[5]);
        os_PutStrFull(buf);
        os_NewLine();
      } else {
        sprintf(buf, "Chipset Init Failed: %d", res);
        os_PutStrFull(buf);
        os_NewLine();
      }

      was_attached = true;
    } else if (!wlan_driver.attached && was_attached) {
      was_attached = false;
      os_PutStrFull("Driver Detached");
      os_NewLine();
    }
  }

  usb_Cleanup();
  return 0;
}
