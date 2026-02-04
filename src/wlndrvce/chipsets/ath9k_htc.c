// this fuckass thing is a mess, doesn't work and is taking my sanity
// lost 4 hours trying to find anything
// i fucking hate having poor to no documentation
// linux source code is nice tho but still pretty big
#include "ath9k_htc.h"
#include "../driver.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <usbdrvce.h>

#define ATH9K_HTC_USB_VENQT_READ \
  (USB_DEVICE_TO_HOST | USB_VENDOR_REQUEST | USB_RECIPIENT_DEVICE)
#define ATH9K_HTC_USB_VENQT_WRITE \
  (USB_HOST_TO_DEVICE | USB_VENDOR_REQUEST | USB_RECIPIENT_DEVICE)

#define ATH9K_HTC_REQ_REG_READ 0x01
#define ATH9K_HTC_REQ_REG_WRITE 0x02

#define ATH9K_HTC_REG_MAC_ADDR 0x1000
#define ATH9K_HTC_REG_RESET 0x4000

static usb_error_t ath9k_htc_write_reg(wlan_driver_t *dev, uint32_t addr,
                                    uint32_t val)
{
  if (!dev || !dev->attached || !dev->device)
    return USB_ERROR_NO_DEVICE;

  usb_control_setup_t setup = {
      .bmRequestType = ATH9K_HTC_USB_VENQT_WRITE,
      .bRequest = ATH9K_HTC_REQ_REG_WRITE,
      .wValue = (uint16_t)((addr >> 16) & 0xFFFF),
      .wIndex = (uint16_t)(addr & 0xFFFF),
      .wLength = 4};
  return usb_DefaultControlTransfer(dev->device, &setup, &val, 1000, NULL);
}

static usb_error_t ath9k_htc_read_reg(wlan_driver_t *dev, uint32_t addr, uint32_t *val)
{
  if (!dev || !val || !dev->attached || !dev->device)
    return USB_ERROR_NO_DEVICE;
  usb_control_setup_t setup = {
      .bmRequestType = ATH9K_HTC_USB_VENQT_READ,
      .bRequest = ATH9K_HTC_REQ_REG_READ,
      .wValue = (uint16_t)((addr >> 16) & 0xFFFF),
      .wIndex = (uint16_t)(addr & 0xFFFF),
      .wLength = 4};
  return usb_DefaultControlTransfer(dev->device, &setup, val, 1000, NULL);
}

#define ATH9K_HTC_EEPROM_BASE 0x2000
#define ATH9K_HTC_EEPROM_START_OFFSET 64
#define ATH9K_HTC_EEPROM_MAC_OFFSET 6

static usb_error_t ath9k_htc_read_eeprom_word(wlan_driver_t *dev, uint16_t offset, uint16_t *val)
{
  uint32_t reg_addr = ATH9K_HTC_EEPROM_BASE + ((offset + ATH9K_HTC_EEPROM_START_OFFSET) << 2);
  uint32_t reg_val;
  usb_error_t err = ath9k_htc_read_reg(dev, reg_addr, &reg_val);
  if (err == USB_SUCCESS)
  {
    *val = (uint16_t)(reg_val & 0xFFFF);
  }
  return err;
}

wlan_result_t ath9k_htc_init(wlan_driver_t *dev, wlan_progress_cb_t cb)
{
  uint16_t mac_word[3];
  if (!dev)
    return WLAN_ERROR_NO_DEVICE;
  wlan_result_t res = wlan_stream_firmware_chunks(dev->device, "AAA", cb);
  if (res == WLAN_SUCCESS)
    return WLAN_SUCCESS;

  msleep(200);

  uint32_t srev;
  if (ath9k_htc_read_reg(dev, 0x4020, &srev) != USB_SUCCESS)
    return WLAN_ERROR_TIMEOUT;

  if ((srev & 0xFF) != 0xFF || ((srev >> 12) & 0xFFF) != 0x140)
    if (srev == 0)
      return WLAN_ERROR_INVALID_PARAM;

  if (
      ath9k_htc_read_eeprom_word(dev, ATH9K_HTC_EEPROM_MAC_OFFSET, &mac_word[0]) == USB_SUCCESS &&
      ath9k_htc_read_eeprom_word(dev, ATH9K_HTC_EEPROM_MAC_OFFSET + 1, &mac_word[1]) == USB_SUCCESS &&
      ath9k_htc_read_eeprom_word(dev, ATH9K_HTC_EEPROM_MAC_OFFSET + 2, &mac_word[2]) == USB_SUCCESS)
  {
    if (mac_word[0] == 0 && mac_word[1] == 0 && mac_word[2] == 0)
    {
      uint16_t magic;
      ath9k_htc_read_eeprom_word(dev, -6, &magic);
      if (magic == 0xa55a)
      {
        dev->mac[0] = 0x00;
        dev->mac[1] = 0x03;
        dev->mac[2] = 0x7F;
        dev->mac[3] = 0x00;
        dev->mac[4] = 0x00;
        dev->mac[5] = 0x01;
      }
      else
        return 8;
    }
    else
    {
      dev->mac[0] = (mac_word[0] >> 8) & 0xFF;
      dev->mac[1] = mac_word[0] & 0xFF;
      dev->mac[2] = (mac_word[1] >> 8) & 0xFF;
      dev->mac[3] = mac_word[1] & 0xFF;
      dev->mac[4] = (mac_word[2] >> 8) & 0xFF;
      dev->mac[5] = mac_word[2] & 0xFF;
    }
  }
  else
  {
    return WLAN_ERROR_TIMEOUT;
  }

  return WLAN_SUCCESS;
}

void ath9k_htc_debug_dump(wlan_driver_t *dev, wlan_log_cb_t log_cb)
{
  if (!dev || !dev->device || !log_cb)
    return;

  char buf[64];
  uint32_t val;

  ath9k_htc_read_reg(dev, 0x4020, &val);
  snprintf(buf, sizeof(buf), "SREV (0x4020): %08X", val);
  log_cb(buf);

  ath9k_htc_read_reg(dev, 0x0014, &val);
  snprintf(buf, sizeof(buf), "CFG  (0x0014): %08X", val);
  log_cb(buf);

  ath9k_htc_read_reg(dev, 0x8000, &val);
  snprintf(buf, sizeof(buf), "STA0 (0x8000): %08X", val);
  log_cb(buf);

  ath9k_htc_read_reg(dev, 0x806C, &val);
  snprintf(buf, sizeof(buf), "OBS1 (0x806C): %08X", val);
  log_cb(buf);

  log_cb("--- EEPROM DUMP ---");
  for (int i = 0; i < 8; i++)
  {
    uint16_t w_val = 0;
    ath9k_htc_read_eeprom_word(dev, i, &w_val);
    snprintf(buf, sizeof(buf), "EEP[%02d]: %04X", i, w_val);
    log_cb(buf);
  }
}

void ath9k_htc_deinit(wlan_driver_t *dev)
{
  if (!dev)
    return;
  ath9k_htc_write_reg(dev, ATH9K_HTC_REG_RESET, 1);
}
