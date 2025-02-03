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

#ifndef NIMBLE_CPP_SERVICE_H_
#define NIMBLE_CPP_SERVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class BLEService;

#include "BLEAttribute.h"
#include "BLEServer.h"
#include "BLECharacteristic.h"

/**
 * @brief The model of a BLE service.
 *
 */
class BLEService : public BLELocalAttribute {
public:
  BLEService(const char *uuid);
  BLEService(const BLEUUID &uuid);
  ~BLEService();

  BLEServer *getServer() const;
  std::string toString() const;
  void dump() const;
  bool isStarted() const;
  bool start();
  BLECharacteristic *
    createCharacteristic(const char *uuid, uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);

  BLECharacteristic *
    createCharacteristic(const BLEUUID &uuid, uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);
  void addCharacteristic(BLECharacteristic *pCharacteristic);
  void removeCharacteristic(BLECharacteristic *pCharacteristic, bool deleteChr = false);
  BLECharacteristic *getCharacteristic(const char *uuid, uint16_t instanceId = 0) const;
  BLECharacteristic *getCharacteristic(const BLEUUID &uuid, uint16_t instanceId = 0) const;
  BLECharacteristic *getCharacteristicByHandle(uint16_t handle) const;

  const std::vector<BLECharacteristic *> &getCharacteristics() const;
  std::vector<BLECharacteristic *> getCharacteristics(const char *uuid) const;
  std::vector<BLECharacteristic *> getCharacteristics(const BLEUUID &uuid) const;

private:
  friend class BLEServer;

  std::vector<BLECharacteristic *> m_vChars{};
  // Nimble requires an array of services to be sent to the api
  // Since we are adding 1 at a time we create an array of 2 and set the type
  // of the second service to 0 to indicate the end of the array.
  ble_gatt_svc_def m_pSvcDef[2]{};
};  // BLEService

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
#endif /* NIMBLE_CPP_SERVICE_H_ */
