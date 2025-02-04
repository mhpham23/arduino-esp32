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
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BLEService.h"
#if CONFIG_BT_NIMBLE_EXT_ADV
#include "BLEExtAdvertising.h"
#else
#include "BLEAdvertising.h"
#endif
#include "BLEDevice.h"
#include "BLEUtils.h"

#include <string>

static const char *LOG_TAG = "BLEService";

/**
 * @brief Construct an instance of the BLEService
 * @param [in] uuid The UUID of the service.
 */
BLEService::BLEService(const char *uuid) : BLEService(BLEUUID(uuid)) {}

/**
 * @brief Construct an instance of the BLEService
 * @param [in] uuid The UUID of the service.
 */
BLEService::BLEService(const BLEUUID &uuid) : BLELocalAttribute{uuid, 0}, m_pSvcDef{{0, getUUID().getBase(), nullptr, nullptr}, {}} {}

/**
 * @brief Destructor, make sure we release the resources allocated for the service.
 */
BLEService::~BLEService() {
  if (m_pSvcDef->characteristics) {
    if (m_pSvcDef->characteristics->descriptors) {
      delete[] m_pSvcDef->characteristics->descriptors;
    }
    delete[] m_pSvcDef->characteristics;
  }

  for (const auto &it : m_vChars) {
    delete it;
  }
}  // ~BLEService

/**
 * @brief Dump details of this BLE GATT service.
 */
void BLEService::dump() const {
  log_d(LOG_TAG, "Service: uuid:%s, handle: 0x%2x", getUUID().toString().c_str(), getHandle());

  std::string res;
  int count = 0;
  char hex[5];
  for (const auto &it : m_vChars) {
    if (count > 0) {
      res += "\n";
    }
    snprintf(hex, sizeof(hex), "%04x", it->getHandle());
    count++;
    res += "handle: 0x";
    res += hex;
    res += ", uuid: " + std::string(it->getUUID());
  }

  log_d(LOG_TAG, "Characteristics:\n%s", res.c_str());
}  // dump

/**
 * @brief Builds the database of characteristics/descriptors for the service
 * and registers it with the BLE stack.
 * @return bool success/failure .
 */
bool BLEService::start() {
  log_d(LOG_TAG, ">> start(): Starting service: %s", toString().c_str());

  // If started previously and no characteristics have been added or removed,
  // then we can skip the service registration process.
  if (m_pSvcDef->characteristics && !getServer()->m_svcChanged) {
    return true;
  }

  // If started previously, clear everything and start over
  if (m_pSvcDef->characteristics) {
    if (m_pSvcDef->characteristics->descriptors) {
      delete[] m_pSvcDef->characteristics->descriptors;
    }
    delete[] m_pSvcDef->characteristics;
  }

  size_t numChrs = 0;
  for (const auto &chr : m_vChars) {
    if (chr->getRemoved()) {
      continue;
    }
    ++numChrs;
  }

  log_d(LOG_TAG, "Adding %d characteristics for service %s", numChrs, toString().c_str());
  if (numChrs) {
    int i = 0;

    // Nimble requires the last characteristic to have it's uuid = 0 to indicate the end
    // of the characteristics for the service. We create 1 extra and set it to null
    // for this purpose.
    ble_gatt_chr_def *pChrs = new ble_gatt_chr_def[numChrs + 1]{};
    for (const auto &chr : m_vChars) {
      if (chr->getRemoved()) {
        continue;
      }

      size_t numDscs = 0;
      for (const auto &dsc : chr->m_vDescriptors) {
        if (dsc->getRemoved()) {
          continue;
        }
        ++numDscs;
      }

      if (numDscs) {
        int j = 0;

        // Must have last descriptor uuid = 0 so we have to create 1 extra
        ble_gatt_dsc_def *pDscs = new ble_gatt_dsc_def[numDscs + 1]{};
        for (const auto &dsc : chr->m_vDescriptors) {
          if (dsc->getRemoved()) {
            continue;
          }

          pDscs[j].uuid = dsc->getUUID().getBase();
          pDscs[j].att_flags = dsc->getProperties();
          pDscs[j].min_key_size = 0;
          pDscs[j].access_cb = BLEServer::handleGattEvent;
          pDscs[j].arg = dsc;
          ++j;
        }

        pChrs[i].descriptors = pDscs;
      }

      pChrs[i].uuid = chr->getUUID().getBase();
      pChrs[i].access_cb = BLEServer::handleGattEvent;
      pChrs[i].arg = chr;
      pChrs[i].flags = chr->getProperties();
      pChrs[i].min_key_size = 0;
      pChrs[i].val_handle = &chr->m_handle;
      ++i;
    }

    m_pSvcDef->characteristics = pChrs;
  }

  m_pSvcDef->type = BLE_GATT_SVC_TYPE_PRIMARY;
  int rc = ble_gatts_count_cfg(m_pSvcDef);
  if (rc != 0) {
    log_e(LOG_TAG, "ble_gatts_count_cfg failed, rc= %d, %s", rc, BLEUtils::returnCodeToString(rc));
    return false;
  }

  rc = ble_gatts_add_svcs(m_pSvcDef);
  if (rc != 0) {
    log_e(LOG_TAG, "ble_gatts_add_svcs, rc= %d, %s", rc, BLEUtils::returnCodeToString(rc));
    return false;
  }

  log_d(LOG_TAG, "<< start()");
  return true;
}  // start

/**
 * @brief Create a new BLE Characteristic associated with this service.
 * @param [in] uuid - The UUID of the characteristic.
 * @param [in] properties - The properties of the characteristic.
 * @param [in] max_len - The maximum length in bytes that the characteristic value can hold.
 * @return The new BLE characteristic.
 */
BLECharacteristic *BLEService::createCharacteristic(const char *uuid, uint32_t properties, uint16_t max_len) {
  return createCharacteristic(BLEUUID(uuid), properties, max_len);
}  // createCharacteristic

/**
 * @brief Create a new BLE Characteristic associated with this service.
 * @param [in] uuid - The UUID of the characteristic.
 * @param [in] properties - The properties of the characteristic.
 * @param [in] max_len - The maximum length in bytes that the characteristic value can hold.
 * @return The new BLE characteristic.
 */
BLECharacteristic *BLEService::createCharacteristic(const BLEUUID &uuid, uint32_t properties, uint16_t max_len) {
  BLECharacteristic *pChar = new BLECharacteristic(uuid, properties, max_len, this);
  if (getCharacteristic(uuid) != nullptr) {
    log_d(LOG_TAG, "Adding a duplicate characteristic with UUID: %s", std::string(uuid).c_str());
  }

  addCharacteristic(pChar);
  return pChar;
}  // createCharacteristic

/**
 * @brief Add a characteristic to the service.
 * @param[in] pChar A pointer to the characteristic instance to add to the service.
 */
void BLEService::addCharacteristic(BLECharacteristic *pChar) {
  bool foundRemoved = false;
  if (pChar->getRemoved() > 0) {
    for (const auto &chr : m_vChars) {
      if (chr == pChar) {
        foundRemoved = true;
        pChar->setRemoved(0);
      }
    }
  }

  // Check if the characteristic is already in the service and if so, return.
  for (const auto &chr : m_vChars) {
    if (chr == pChar) {
      pChar->setService(this);  // Update the service pointer in the characteristic.
      return;
    }
  }

  if (!foundRemoved) {
    m_vChars.push_back(pChar);
  }

  pChar->setService(this);
  getServer()->serviceChanged();
}  // addCharacteristic

/**
 * @brief Remove a characteristic from the service.
 * @param[in] pChar A pointer to the characteristic instance to remove from the service.
 * @param[in] deleteChr If true it will delete the characteristic instance and free it's resources.
 */
void BLEService::removeCharacteristic(BLECharacteristic *pChar, bool deleteChr) {
  // Check if the characteristic was already removed and if so, check if this
  // is being called to delete the object and do so if requested.
  // Otherwise, ignore the call and return.
  if (pChar->getRemoved() > 0) {
    if (deleteChr) {
      for (auto it = m_vChars.begin(); it != m_vChars.end(); ++it) {
        if ((*it) == pChar) {
          delete (*it);
          m_vChars.erase(it);
          break;
        }
      }
    }

    return;
  }

  pChar->setRemoved(deleteChr ? NIMBLE_ATT_REMOVE_DELETE : NIMBLE_ATT_REMOVE_HIDE);
  getServer()->serviceChanged();
}  // removeCharacteristic

/**
 * @brief Get a pointer to the characteristic object with the specified UUID.
 * @param [in] uuid The UUID of the characteristic.
 * @param idx The index of the characteristic to return (used when multiple characteristics have the same UUID).
 * @return A pointer to the characteristic object or nullptr if not found.
 */
BLECharacteristic *BLEService::getCharacteristic(const char *uuid, uint16_t idx) const {
  return getCharacteristic(BLEUUID(uuid), idx);
}  // getCharacteristic

/**
 * @brief Get a pointer to the characteristic object with the specified UUID.
 * @param [in] uuid The UUID of the characteristic.
 * @param idx The index of the characteristic to return (used when multiple characteristics have the same UUID).
 * @return A pointer to the characteristic object or nullptr if not found.
 */
BLECharacteristic *BLEService::getCharacteristic(const BLEUUID &uuid, uint16_t idx) const {
  uint16_t position = 0;
  for (const auto &chr : m_vChars) {
    if (chr->getUUID() == uuid) {
      if (position == idx) {
        return chr;
      }
      position++;
    }
  }

  return nullptr;
}  // getCharacteristic

/**
 * @brief Get a pointer to the characteristic object with the specified handle.
 * @param handle The handle of the characteristic.
 * @return A pointer to the characteristic object or nullptr if not found.
 */
BLECharacteristic *BLEService::getCharacteristicByHandle(uint16_t handle) const {
  for (const auto &chr : m_vChars) {
    if (chr->getHandle() == handle) {
      return chr;
    }
  }

  return nullptr;
}  // getCharacteristicByHandle

/**
 * @return A vector containing pointers to each characteristic associated with this service.
 */
const std::vector<BLECharacteristic *> &BLEService::getCharacteristics() const {
  return m_vChars;
}  // getCharacteristics

/**
 * @return A vector containing pointers to each characteristic with the provided UUID associated with this service.
 */
std::vector<BLECharacteristic *> BLEService::getCharacteristics(const char *uuid) const {
  return getCharacteristics(BLEUUID(uuid));
}  // getCharacteristics

/**
 * @return A vector containing pointers to each characteristic with the provided UUID associated with this service.
 */
std::vector<BLECharacteristic *> BLEService::getCharacteristics(const BLEUUID &uuid) const {
  std::vector<BLECharacteristic *> result;
  for (const auto &chr : m_vChars) {
    if (chr->getUUID() == uuid) {
      result.push_back(chr);
    }
  }

  return result;
}  // getCharacteristics

/**
 * @brief Return a string representation of this service.
 * A service is defined by:
 * * Its UUID
 * * Its handle
 * @return A string representation of this service.
 */
std::string BLEService::toString() const {
  std::string res = "UUID: " + getUUID().toString();
  char hex[5];
  snprintf(hex, sizeof(hex), "%04x", getHandle());
  res += ", handle: 0x";
  res += hex;
  return res;
}  // toString

/**
 * @brief Get the BLE server associated with this service.
 * @return The BLEServer associated with this service.
 */
BLEServer *BLEService::getServer() const {
  return BLEDevice::getServer();
}  // getServer

/**
 * @brief Checks if the service has been started.
 * @return True if the service has been started.
 */
bool BLEService::isStarted() const {
  return m_pSvcDef->type > 0;
}  // isStarted

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
