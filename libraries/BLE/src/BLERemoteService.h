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

#ifndef COMPONENTS_CPP_UTILS_BLEREMOTESERVICE_H_
#define COMPONENTS_CPP_UTILS_BLEREMOTESERVICE_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLEAttribute.h"
#include <vector>

class BLERemoteCharacteristic;
class BLEClient;
class BLEAttValue;

/**
 * @brief A model of a remote BLE service.
 */
class BLERemoteService : public BLEAttribute {
public:
  BLERemoteCharacteristic *getCharacteristic(const char *uuid) const;
  BLERemoteCharacteristic *getCharacteristic(const BLEUUID &uuid) const;
  void deleteCharacteristics() const;
  size_t deleteCharacteristic(const BLEUUID &uuid) const;
  BLEClient *getClient(void) const;
  BLEAttValue getValue(const BLEUUID &characteristicUuid) const;
  bool setValue(const BLEUUID &characteristicUuid, const BLEAttValue &value) const;
  std::string toString(void) const;
  uint16_t getStartHandle() const {
    return getHandle();
  }
  uint16_t getEndHandle() const {
    return m_endHandle;
  }

  const std::vector<BLERemoteCharacteristic *> &getCharacteristics(bool refresh = false) const;
  std::vector<BLERemoteCharacteristic *>::iterator begin() const;
  std::vector<BLERemoteCharacteristic *>::iterator end() const;

private:
  friend class BLEClient;

  BLERemoteService(BLEClient *pClient, const struct ble_gatt_svc *service);
  ~BLERemoteService();
  bool retrieveCharacteristics(const BLEUUID *uuidFilter = nullptr) const;
  static int characteristicDiscCB(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg);

  mutable std::vector<BLERemoteCharacteristic *> m_vChars{};
  BLEClient *m_pClient{nullptr};
  uint16_t m_endHandle{0};
};  // BLERemoteService

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLERemoteService.h
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#include <map>

#include "BLEClient.h"
#include "BLERemoteCharacteristic.h"
#include "BLEUUID.h"
#include "RTOS.h"

class BLEClient;
class BLERemoteCharacteristic;

/**
 * @brief A model of a remote %BLE service.
 */
class BLERemoteService {
public:
  virtual ~BLERemoteService();

  // Public methods
  BLERemoteCharacteristic *getCharacteristic(const char *uuid);  // Get the specified characteristic reference.
  BLERemoteCharacteristic *getCharacteristic(BLEUUID uuid);      // Get the specified characteristic reference.
  BLERemoteCharacteristic *getCharacteristic(uint16_t uuid);     // Get the specified characteristic reference.
  std::map<std::string, BLERemoteCharacteristic *> *getCharacteristics();
  std::map<uint16_t, BLERemoteCharacteristic *> *getCharacteristicsByHandle();  // Get the characteristics map.
  void getCharacteristics(std::map<uint16_t, BLERemoteCharacteristic *> **pCharacteristicMap);

  BLEClient *getClient(void);                               // Get a reference to the client associated with this service.
  uint16_t getHandle();                                     // Get the handle of this service.
  BLEUUID getUUID(void);                                    // Get the UUID of this service.
  String getValue(BLEUUID characteristicUuid);              // Get the value of a characteristic.
  void setValue(BLEUUID characteristicUuid, String value);  // Set the value of a characteristic.
  String toString(void);

private:
  // Private constructor ... never meant to be created by a user application.
  BLERemoteService(esp_gatt_id_t srvcId, BLEClient *pClient, uint16_t startHandle, uint16_t endHandle);

  // Friends
  friend class BLEClient;
  friend class BLERemoteCharacteristic;

  // Private methods
  void retrieveCharacteristics(void);  // Retrieve the characteristics from the BLE Server.
  esp_gatt_id_t *getSrvcId(void);
  uint16_t getStartHandle();  // Get the start handle for this service.
  uint16_t getEndHandle();    // Get the end handle for this service.

  void gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *evtParam);

  void removeCharacteristics();

  // Properties

  // We maintain a map of characteristics owned by this service keyed by a string representation of the UUID.
  std::map<std::string, BLERemoteCharacteristic *> m_characteristicMap;

  // We maintain a map of characteristics owned by this service keyed by a handle.
  std::map<uint16_t, BLERemoteCharacteristic *> m_characteristicMapByHandle;

  bool m_haveCharacteristics;  // Have we previously obtained the characteristics.
  BLEClient *m_pClient;
  FreeRTOS::Semaphore m_semaphoreGetCharEvt = FreeRTOS::Semaphore("GetCharEvt");
  esp_gatt_id_t m_srvcId;
  BLEUUID m_uuid;          // The UUID of this service.
  uint16_t m_startHandle;  // The starting handle of this service.
  uint16_t m_endHandle;    // The ending handle of this service.
};  // BLERemoteService

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTESERVICE_H_ */
