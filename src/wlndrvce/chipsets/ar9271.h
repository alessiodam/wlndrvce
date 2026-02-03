#pragma once

#include "../wlan.h"

/**
 * @brief Initialize the Atheros AR9271 chipset.
 *
 * This function performs the necessary initialization steps for the
 * Atheros AR9271 chipset, including downloading firmware and
 * configuring basic parameters.
 *
 * @param dev Pointer to the WLAN driver structure.
 * @param cb Callback for firmware load progress
 * @return WLAN_SUCCESS on success, error code on failure.
 */
wlan_result_t ar9271_init(wlan_driver_t *dev, wlan_progress_cb_t cb);

/**
 * @brief Deinitialize the Atheros AR9271 chipset.
 *
 * This function performs cleanup and power-down operations for the
 * Atheros AR9271 chipset.
 *
 * @param dev Pointer to the WLAN driver structure.
 */
void ar9271_deinit(wlan_driver_t *dev);
