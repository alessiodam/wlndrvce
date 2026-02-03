#include "ar9271.h"
#include "../driver.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <usbdrvce.h>

#define AR9271_USB_VENQT_READ \
  (USB_DEVICE_TO_HOST | USB_VENDOR_REQUEST | USB_RECIPIENT_DEVICE)
#define AR9271_USB_VENQT_WRITE \
  (USB_HOST_TO_DEVICE | USB_VENDOR_REQUEST | USB_RECIPIENT_DEVICE)

#define AR9271_REQ_REG_READ 0x01
#define AR9271_REQ_REG_WRITE 0x02

#define AR9271_REG_MAC_ADDR 0x1000
#define AR9271_REG_RESET 0x4000

static usb_error_t ar9271_write_reg(wlan_driver_t *dev, uint32_t addr,
                                    uint32_t val)
{
  if (!dev)
    return USB_ERROR_NO_DEVICE;

  usb_control_setup_t setup = {.bmRequestType = AR9271_USB_VENQT_WRITE,
                               .bRequest = AR9271_REQ_REG_WRITE,
                               .wValue = (uint16_t)((addr >> 16) & 0xFFFF),
                               .wIndex = (uint16_t)(addr & 0xFFFF),
                               .wLength = 4};
  return usb_DefaultControlTransfer(dev->device, &setup, &val, 1000, NULL);
}

static usb_error_t ar9271_read_reg(wlan_driver_t *dev, uint32_t addr,
                                   uint32_t *val)
{
  if (!dev || !val)
    return USB_ERROR_NO_DEVICE;

  usb_control_setup_t setup = {.bmRequestType = AR9271_USB_VENQT_READ,
                               .bRequest = AR9271_REQ_REG_READ,
                               .wValue = (uint16_t)((addr >> 16) & 0xFFFF),
                               .wIndex = (uint16_t)(addr & 0xFFFF),
                               .wLength = 4};
  return usb_DefaultControlTransfer(dev->device, &setup, val, 1000, NULL);
}

wlan_result_t ar9271_init(wlan_driver_t *dev)
{
  usb_error_t err;
  uint32_t mac_low, mac_high;

  if (!dev)
  {
    return WLAN_ERROR_NO_DEVICE;
  }

  wlan_stream_firmware_chunks("AAA"); // from firmwares/manifest.json

  msleep(200);

  err = ar9271_read_reg(dev, AR9271_REG_MAC_ADDR, &mac_low);
  if (err == USB_SUCCESS)
  {
    ar9271_read_reg(dev, AR9271_REG_MAC_ADDR + 4, &mac_high);

    dev->mac[0] = (uint8_t)(mac_low >> 0);
    dev->mac[1] = (uint8_t)(mac_low >> 8);
    dev->mac[2] = (uint8_t)(mac_low >> 16);
    dev->mac[3] = (uint8_t)(mac_low >> 24);
    dev->mac[4] = (uint8_t)(mac_high >> 0);
    dev->mac[5] = (uint8_t)(mac_high >> 8);
  }

  return WLAN_SUCCESS;
}

void ar9271_deinit(wlan_driver_t *dev)
{
  if (!dev)
    return;

  ar9271_write_reg(dev, AR9271_REG_RESET, 1);
}
