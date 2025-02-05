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

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BLEService.h"
#include "BLEDescriptor.h"

#include <string>

static const char *LOG_TAG = "BLEDescriptor";
static BLEDescriptorCallbacks defaultCallbacks;

/**
 * @brief Construct a descriptor
 * @param [in] uuid - UUID (const char*) for the descriptor.
 * @param [in] properties - Properties for the descriptor.
 * @param [in] max_len - The maximum length in bytes that the descriptor value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pCharacteristic - pointer to the characteristic instance this descriptor belongs to.
 */
BLEDescriptor::BLEDescriptor(const char *uuid, uint16_t properties, uint16_t max_len, BLECharacteristic *pCharacteristic)
  : BLEDescriptor(BLEUUID(uuid), properties, max_len, pCharacteristic) {}

/**
 * @brief Construct a descriptor
 * @param [in] uuid - UUID (const char*) for the descriptor.
 * @param [in] properties - Properties for the descriptor.
 * @param [in] max_len - The maximum length in bytes that the descriptor value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pCharacteristic - pointer to the characteristic instance this descriptor belongs to.
 */
BLEDescriptor::BLEDescriptor(const BLEUUID &uuid, uint16_t properties, uint16_t max_len, BLECharacteristic *pCharacteristic)
  : BLELocalValueAttribute{uuid, 0, max_len}, m_pCallbacks{&defaultCallbacks}, m_pCharacteristic{pCharacteristic} {
  // Check if this is the client configuration descriptor and set to removed if true.
  if (uuid == BLEUUID((uint16_t)0x2902)) {
    log_w(LOG_TAG, "Manually created 2902 descriptor has no functionality; please remove.");
    setRemoved(NIMBLE_ATT_REMOVE_HIDE);
  }

  // convert uint16_t properties to uint8_t for descriptor properties
  uint8_t descProperties = 0;
  if (properties & BLECharacteristic::PROPERTY_READ) {
    descProperties |= BLE_ATT_F_READ;
  }
  if (properties & (BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE)) {
    descProperties |= BLE_ATT_F_WRITE;
  }
  if (properties & BLECharacteristic::PROPERTY_READ_ENC) {
    descProperties |= BLE_ATT_F_READ_ENC;
  }
  if (properties & BLECharacteristic::PROPERTY_READ_AUTHEN) {
    descProperties |= BLE_ATT_F_READ_AUTHEN;
  }
  if (properties & BLECharacteristic::PROPERTY_READ_AUTHOR) {
    descProperties |= BLE_ATT_F_READ_AUTHOR;
  }
  if (properties & BLECharacteristic::PROPERTY_WRITE_ENC) {
    descProperties |= BLE_ATT_F_WRITE_ENC;
  }
  if (properties & BLECharacteristic::PROPERTY_WRITE_AUTHEN) {
    descProperties |= BLE_ATT_F_WRITE_AUTHEN;
  }
  if (properties & BLECharacteristic::PROPERTY_WRITE_AUTHOR) {
    descProperties |= BLE_ATT_F_WRITE_AUTHOR;
  }

  setProperties(descProperties);
}  // BLEDescriptor

/**
 * @brief Get the characteristic this descriptor belongs to.
 * @return A pointer to the characteristic this descriptor belongs to.
 */
BLECharacteristic *BLEDescriptor::getCharacteristic() const {
  return m_pCharacteristic;
}  // getCharacteristic

/**
 * @brief Set the callback handlers for this descriptor.
 * @param [in] pCallbacks An instance of a callback structure used to define any callbacks for the descriptor.
 */
void BLEDescriptor::setCallbacks(BLEDescriptorCallbacks *pCallbacks) {
  if (pCallbacks != nullptr) {
    m_pCallbacks = pCallbacks;
  } else {
    m_pCallbacks = &defaultCallbacks;
  }
}  // setCallbacks

/**
 * @brief Set the characteristic this descriptor belongs to.
 * @param [in] pChar A pointer to the characteristic this descriptor belongs to.
 */
void BLEDescriptor::setCharacteristic(BLECharacteristic *pChar) {
  m_pCharacteristic = pChar;
}  // setCharacteristic

/**
 * @brief Return a string representation of the descriptor.
 * @return A string representation of the descriptor.
 */
std::string BLEDescriptor::toString() const {
  char hex[5];
  snprintf(hex, sizeof(hex), "%04x", getHandle());
  std::string res = "UUID: " + m_uuid.toString() + ", handle: 0x" + hex;
  return res;
}  // toString

void BLEDescriptor::readEvent(BLEConnInfo &connInfo) {
  m_pCallbacks->onRead(this, connInfo);
}  // readEvent

void BLEDescriptor::writeEvent(const uint8_t *val, uint16_t len, BLEConnInfo &connInfo) {
  setValue(val, len);
  m_pCallbacks->onWrite(this, connInfo);
}  // writeEvent

/**
 * @brief Callback function to support a read request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
 */
void BLEDescriptorCallbacks::onRead(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo) {
  log_d("BLEDescriptorCallbacks", "onRead: default");
}  // onRead

/**
 * @brief Callback function to support a write request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
 */
void BLEDescriptorCallbacks::onWrite(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo) {
  log_d("BLEDescriptorCallbacks", "onWrite: default");
}  // onWrite

#endif /* CONFIG_NIMBLE_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
