#include "wlndrvce/wlan.h"
#include <stdlib.h>
#include <string.h>
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

usb_error_t wlan_usb_enabled_handler(usb_device_t device) {
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

usb_error_t wlan_usb_event_handler(usb_event_t event, void *data,
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