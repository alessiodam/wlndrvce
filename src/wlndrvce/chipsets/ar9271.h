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
 * @return 0 on success, negative error code on failure.
 */
int ar9271_init(wlan_driver_t *dev);

/**
 * @brief Deinitialize the Atheros AR9271 chipset.
 *
 * This function performs cleanup and power-down operations for the
 * Atheros AR9271 chipset.
 *
 * @param dev Pointer to the WLAN driver structure.
 */
void ar9271_deinit(wlan_driver_t *dev);