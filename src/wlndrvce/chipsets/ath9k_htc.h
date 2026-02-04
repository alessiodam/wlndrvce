#pragma once

#include "../wlan.h"

/**
 * @brief Initialize the Atheros AR9271/ATH9K_HTC chipset.
 *
 * This function performs the necessary initialization steps for the
 * chipset, including downloading firmware and configuring basic parameters.
 *
 * @param dev Pointer to the WLAN driver structure.
 * @param cb Callback for firmware load progress
 * @return WLAN_SUCCESS on success, error code on failure.
 */
wlan_result_t ath9k_htc_init(wlan_driver_t *dev, wlan_progress_cb_t cb);

/**
 * @brief Deinitialize the Atheros AR9271/ATH9K_HTC chipset.
 *
 * This function performs cleanup and power-down operations for the
 * chipset.
 *
 * @param dev Pointer to the WLAN driver structure.
 */
void ath9k_htc_deinit(wlan_driver_t *dev);

/**
 * @brief Dump debug information for the Atheros AR9271/ATH9K_HTC chipset.
 *
 * @param dev Pointer to the WLAN driver structure.
 * @param log_cb Callback for logging debug messages.
 */
void ath9k_htc_debug_dump(wlan_driver_t *dev, wlan_log_cb_t log_cb);