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

#ifndef NIMBLE_CPP_REMOTE_SERVICE_H_
#define NIMBLE_CPP_REMOTE_SERVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

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

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
#endif /* NIMBLE_CPP_REMOTE_SERVICE_H_*/
