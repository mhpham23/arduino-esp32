/*
 * Copyright 2020-2024 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLERemoteDescriptor.h"
#include "BLERemoteCharacteristic.h"

/**
 * @brief Remote descriptor constructor.
 * @param [in] pRemoteCharacteristic A pointer to the Characteristic that this belongs to.
 * @param [in] dsc A pointer to the struct that contains the descriptor information.
 */
BLERemoteDescriptor::BLERemoteDescriptor(const BLERemoteCharacteristic *pRemoteCharacteristic, const ble_gatt_dsc *dsc)
  : BLERemoteValueAttribute{dsc->uuid, dsc->handle}, m_pRemoteCharacteristic{pRemoteCharacteristic} {}  // BLERemoteDescriptor

/**
 * @brief Get the characteristic that owns this descriptor.
 * @return The characteristic that owns this descriptor.
 */
BLERemoteCharacteristic *BLERemoteDescriptor::getRemoteCharacteristic() const {
  return const_cast<BLERemoteCharacteristic *>(m_pRemoteCharacteristic);
}  // getRemoteCharacteristic

/**
 * @brief Return a string representation of this Remote Descriptor.
 * @return A string representation of this Remote Descriptor.
 */
std::string BLERemoteDescriptor::toString() const {
  std::string res = "Descriptor: uuid: " + getUUID().toString();
  char val[6];
  res += ", handle: ";
  snprintf(val, sizeof(val), "%d", getHandle());
  res += val;

  return res;
}  // toString

BLEClient *BLERemoteDescriptor::getClient() const {
  return m_pRemoteCharacteristic->getClient();
}

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
