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

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BLECharacteristic.h"
#include "BLE2904.h"
#include "BLEDevice.h"
#include "BLELog.h"

static BLECharacteristicCallbacks defaultCallback;
static const char *LOG_TAG = "BLECharacteristic";

/**
 * @brief Construct a characteristic
 * @param [in] uuid - UUID (const char*) for the characteristic.
 * @param [in] properties - Properties for the characteristic.
 * @param [in] maxLen - The maximum length in bytes that the characteristic value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pService - pointer to the service instance this characteristic belongs to.
 */
BLECharacteristic::BLECharacteristic(const char *uuid, uint16_t properties, uint16_t maxLen, BLEService *pService)
  : BLECharacteristic(BLEUUID(uuid), properties, maxLen, pService) {}

/**
 * @brief Construct a characteristic
 * @param [in] uuid - UUID for the characteristic.
 * @param [in] properties - Properties for the characteristic.
 * @param [in] maxLen - The maximum length in bytes that the characteristic value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pService - pointer to the service instance this characteristic belongs to.
 */
BLECharacteristic::BLECharacteristic(const BLEUUID &uuid, uint16_t properties, uint16_t maxLen, BLEService *pService)
  : BLELocalValueAttribute{uuid, 0, maxLen}, m_pCallbacks{&defaultCallback}, m_pService{pService} {
  setProperties(properties);
}  // BLECharacteristic

/**
 * @brief Destructor.
 */
BLECharacteristic::~BLECharacteristic() {
  for (const auto &dsc : m_vDescriptors) {
    delete dsc;
  }
}  // ~BLECharacteristic

/**
 * @brief Create a new BLE Descriptor associated with this characteristic.
 * @param [in] uuid - The UUID of the descriptor.
 * @param [in] properties - The properties of the descriptor.
 * @param [in] maxLen - The max length in bytes of the descriptor value.
 * @return The new BLE descriptor.
 */
BLEDescriptor *BLECharacteristic::createDescriptor(const char *uuid, uint32_t properties, uint16_t maxLen) {
  return createDescriptor(BLEUUID(uuid), properties, maxLen);
}

/**
 * @brief Create a new BLE Descriptor associated with this characteristic.
 * @param [in] uuid - The UUID of the descriptor.
 * @param [in] properties - The properties of the descriptor.
 * @param [in] maxLen - The max length in bytes of the descriptor value.
 * @return The new BLE descriptor.
 */
BLEDescriptor *BLECharacteristic::createDescriptor(const BLEUUID &uuid, uint32_t properties, uint16_t maxLen) {
  BLEDescriptor *pDescriptor = nullptr;
  if (uuid == BLEUUID(static_cast<uint16_t>(0x2904))) {
    NIMBLE_LOGW(LOG_TAG, "0x2904 descriptor should be created with create2904()");
    pDescriptor = create2904();
  } else {
    pDescriptor = new BLEDescriptor(uuid, properties, maxLen, this);
  }

  addDescriptor(pDescriptor);
  return pDescriptor;
}  // createDescriptor

/**
 * @brief Create a Characteristic Presentation Format Descriptor for this characteristic.
 * @return A pointer to a BLE2904 descriptor.
 */
BLE2904 *BLECharacteristic::create2904() {
  BLE2904 *pDescriptor = new BLE2904(this);
  addDescriptor(pDescriptor);
  return pDescriptor;
}  // create2904

/**
 * @brief Add a descriptor to the characteristic.
 * @param [in] pDescriptor A pointer to the descriptor to add.
 */
void BLECharacteristic::addDescriptor(BLEDescriptor *pDescriptor) {
  bool foundRemoved = false;
  if (pDescriptor->getRemoved() > 0) {
    for (const auto &dsc : m_vDescriptors) {
      if (dsc == pDescriptor) {
        foundRemoved = true;
        pDescriptor->setRemoved(0);
      }
    }
  }

  // Check if the descriptor is already in the vector and if so, return.
  for (const auto &dsc : m_vDescriptors) {
    if (dsc == pDescriptor) {
      pDescriptor->setCharacteristic(this);  // Update the characteristic pointer in the descriptor.
      return;
    }
  }

  if (!foundRemoved) {
    m_vDescriptors.push_back(pDescriptor);
  }

  pDescriptor->setCharacteristic(this);
  BLEDevice::getServer()->serviceChanged();
}

/**
 * @brief Remove a descriptor from the characteristic.
 * @param[in] pDescriptor A pointer to the descriptor instance to remove from the characteristic.
 * @param[in] deleteDsc If true it will delete the descriptor instance and free it's resources.
 */
void BLECharacteristic::removeDescriptor(BLEDescriptor *pDescriptor, bool deleteDsc) {
  // Check if the descriptor was already removed and if so, check if this
  // is being called to delete the object and do so if requested.
  // Otherwise, ignore the call and return.
  if (pDescriptor->getRemoved() > 0) {
    if (deleteDsc) {
      for (auto it = m_vDescriptors.begin(); it != m_vDescriptors.end(); ++it) {
        if ((*it) == pDescriptor) {
          delete (*it);
          m_vDescriptors.erase(it);
          break;
        }
      }
    }

    return;
  }

  pDescriptor->setRemoved(deleteDsc ? NIMBLE_ATT_REMOVE_DELETE : NIMBLE_ATT_REMOVE_HIDE);
  BLEDevice::getServer()->serviceChanged();
}  // removeDescriptor

/**
 * @brief Return the BLE Descriptor for the given UUID.
 * @param [in] uuid The UUID of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
BLEDescriptor *BLECharacteristic::getDescriptorByUUID(const char *uuid) const {
  return getDescriptorByUUID(BLEUUID(uuid));
}  // getDescriptorByUUID

/**
 * @brief Return the BLE Descriptor for the given UUID.
 * @param [in] uuid The UUID of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
BLEDescriptor *BLECharacteristic::getDescriptorByUUID(const BLEUUID &uuid) const {
  for (const auto &dsc : m_vDescriptors) {
    if (dsc->getUUID() == uuid) {
      return dsc;
    }
  }
  return nullptr;
}  // getDescriptorByUUID

/**
 * @brief Return the BLE Descriptor for the given handle.
 * @param [in] handle The handle of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
BLEDescriptor *BLECharacteristic::getDescriptorByHandle(uint16_t handle) const {
  for (const auto &dsc : m_vDescriptors) {
    if (dsc->getHandle() == handle) {
      return dsc;
    }
  }
  return nullptr;
}  // getDescriptorByHandle

/**
 * @brief Get the properties of the characteristic.
 * @return The properties of the characteristic.
 */
uint16_t BLECharacteristic::getProperties() const {
  return m_properties;
}  // getProperties

/**
 * @brief Get the service that owns this characteristic.
 */
BLEService *BLECharacteristic::getService() const {
  return m_pService;
}  // getService

void BLECharacteristic::setService(BLEService *pService) {
  m_pService = pService;
}  // setService

/**
 * @brief Send an indication.
 * @param[in] connHandle Connection handle to send an individual indication, or BLE_HS_CONN_HANDLE_NONE to send
 * the indication to all subscribed clients.
 * @return True if the indication was sent successfully, false otherwise.
 */
bool BLECharacteristic::indicate(uint16_t connHandle) const {
  return sendValue(nullptr, 0, false, connHandle);
}  // indicate

/**
 * @brief Send an indication.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] connHandle Connection handle to send an individual indication, or BLE_HS_CONN_HANDLE_NONE to send
 * the indication to all subscribed clients.
 * @return True if the indication was sent successfully, false otherwise.
 */
bool BLECharacteristic::indicate(const uint8_t *value, size_t length, uint16_t connHandle) const {
  return sendValue(value, length, false, connHandle);
}  // indicate

/**
 * @brief Send a notification.
 * @param[in] connHandle Connection handle to send an individual notification, or BLE_HS_CONN_HANDLE_NONE to send
 * the notification to all subscribed clients.
 * @return True if the notification was sent successfully, false otherwise.
 */
bool BLECharacteristic::notify(uint16_t connHandle) const {
  return sendValue(nullptr, 0, true, connHandle);
}  // notify

/**
 * @brief Send a notification.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] connHandle Connection handle to send an individual notification, or BLE_HS_CONN_HANDLE_NONE to send
 * the notification to all subscribed clients.
 * @return True if the notification was sent successfully, false otherwise.
 */
bool BLECharacteristic::notify(const uint8_t *value, size_t length, uint16_t connHandle) const {
  return sendValue(value, length, true, connHandle);
}  // indicate

/**
 * @brief Sends a notification or indication.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] isNotification if true sends a notification, false sends an indication.
 * @param[in] connHandle Connection handle to send to a specific peer.
 * @return True if the value was sent successfully, false otherwise.
 */
bool BLECharacteristic::sendValue(const uint8_t *value, size_t length, bool isNotification, uint16_t connHandle) const {
  int rc = 0;

  if (value != nullptr && length > 0) {  // custom notification value
    // Notify all connected peers unless a specific handle is provided
    for (const auto &ch : BLEDevice::getServer()->getPeerDevices()) {
      if (connHandle != BLE_HS_CONN_HANDLE_NONE && ch != connHandle) {
        continue;  // only send to the specific handle, minor inefficiency but saves code.
      }

      // Must re-create the data buffer on each iteration because it is freed by the calls bellow.
      os_mbuf *om = ble_hs_mbuf_from_flat(value, length);
      if (!om) {
        NIMBLE_LOGE(LOG_TAG, "<< sendValue: failed to allocate mbuf");
        return false;
      }

      if (isNotification) {
        rc = ble_gattc_notify_custom(ch, m_handle, om);
      } else {
        rc = ble_gattc_indicate_custom(ch, m_handle, om);
      }

      if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "<< sendValue: failed to send value, rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
        break;
      }
    }
  } else if (connHandle != BLE_HS_CONN_HANDLE_NONE) {  // only sending to specific peer
    // Null buffer will read the value from the characteristic
    if (isNotification) {
      rc = ble_gattc_notify_custom(connHandle, m_handle, NULL);
    } else {
      rc = ble_gattc_indicate_custom(connHandle, m_handle, NULL);
    }
  } else {  // Notify or indicate to all connected peers the characteristic value
    ble_gatts_chr_updated(m_handle);
  }

  return rc == 0;
}  // sendValue

void BLECharacteristic::readEvent(BLEConnInfo &connInfo) {
  m_pCallbacks->onRead(this, connInfo);
}  // readEvent

void BLECharacteristic::writeEvent(const uint8_t *val, uint16_t len, BLEConnInfo &connInfo) {
  setValue(val, len);
  m_pCallbacks->onWrite(this, connInfo);
}  // writeEvent

/**
 * @brief Set the callback handlers for this characteristic.
 * @param [in] pCallbacks An instance of a BLECharacteristicCallbacks class\n
 * used to define any callbacks for the characteristic.
 */
void BLECharacteristic::setCallbacks(BLECharacteristicCallbacks *pCallbacks) {
  if (pCallbacks != nullptr) {
    m_pCallbacks = pCallbacks;
  } else {
    m_pCallbacks = &defaultCallback;
  }
}  // setCallbacks

/**
 * @brief Get the callback handlers for this characteristic.
 */
BLECharacteristicCallbacks *BLECharacteristic::getCallbacks() const {
  return m_pCallbacks;
}  // getCallbacks

/**
 * @brief Return a string representation of the characteristic.
 * @return A string representation of the characteristic.
 */
std::string BLECharacteristic::toString() const {
  std::string res = "UUID: " + m_uuid.toString() + ", handle : 0x";
  char hex[5];
  snprintf(hex, sizeof(hex), "%04x", getHandle());
  res += hex;
  res += " ";
  if (m_properties & BLE_GATT_CHR_PROP_READ) {
    res += "Read ";
  }
  if (m_properties & BLE_GATT_CHR_PROP_WRITE) {
    res += "Write ";
  }
  if (m_properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) {
    res += "WriteNoResponse ";
  }
  if (m_properties & BLE_GATT_CHR_PROP_BROADCAST) {
    res += "Broadcast ";
  }
  if (m_properties & BLE_GATT_CHR_PROP_NOTIFY) {
    res += "Notify ";
  }
  if (m_properties & BLE_GATT_CHR_PROP_INDICATE) {
    res += "Indicate ";
  }
  return res;
}  // toString

/**
 * @brief Callback function to support a read request.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
 */
void BLECharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo) {
  NIMBLE_LOGD("BLECharacteristicCallbacks", "onRead: default");
}  // onRead

/**
 * @brief Callback function to support a write request.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
 */
void BLECharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo) {
  NIMBLE_LOGD("BLECharacteristicCallbacks", "onWrite: default");
}  // onWrite

/**
 * @brief Callback function to support a Notify/Indicate Status report.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] code Status return code from the BLE stack.
 * @details The status code for success is 0 for notifications and BLE_HS_EDONE for indications,
 * any other value is an error.
 */
void BLECharacteristicCallbacks::onStatus(BLECharacteristic *pCharacteristic, int code) {
  NIMBLE_LOGD("BLECharacteristicCallbacks", "onStatus: default");
}  // onStatus

/**
 * @brief Callback function called when a client changes subscription status.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
 * @param [in] subValue The subscription status:
 * * 0 = Un-Subscribed
 * * 1 = Notifications
 * * 2 = Indications
 * * 3 = Notifications and Indications
 */
void BLECharacteristicCallbacks::onSubscribe(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo, uint16_t subValue) {
  NIMBLE_LOGD("BLECharacteristicCallbacks", "onSubscribe: default");
}

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
