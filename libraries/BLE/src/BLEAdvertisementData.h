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

#ifndef NIMBLE_CPP_ADVERTISEMENT_DATA_H_
#define NIMBLE_CPP_ADVERTISEMENT_DATA_H_

#include "sdkconfig.h"
#if (defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && !CONFIG_BT_NIMBLE_EXT_ADV)

#include <cstdint>
#include <string>
#include <vector>

class BLEUUID;
/**
 * @brief Advertisement data set by the programmer to be published by the BLE server.
 */
class BLEAdvertisementData {
  // Only a subset of the possible BLE architected advertisement fields are currently exposed.  Others will
  // be exposed on demand/request or as time permits.
  //
public:
  bool addData(const uint8_t *data, size_t length);
  bool addData(const std::vector<uint8_t> &data);
  bool setAppearance(uint16_t appearance);
  bool setFlags(uint8_t);
  bool addTxPower();
  bool setPreferredParams(uint16_t minInterval, uint16_t maxInterval);
  bool addServiceUUID(const BLEUUID &serviceUUID);
  bool addServiceUUID(const char *serviceUUID);
  bool removeServiceUUID(const BLEUUID &serviceUUID);
  bool removeServiceUUID(const char *serviceUUID);
  bool removeServices();
  bool setManufacturerData(const uint8_t *data, size_t length);
  bool setManufacturerData(const std::string &data);
  bool setManufacturerData(const std::vector<uint8_t> &data);
  bool setURI(const std::string &uri);
  bool setName(const std::string &name, bool isComplete = true);
  bool setShortName(const std::string &name);
  bool setCompleteServices(const BLEUUID &uuid);
  bool setCompleteServices16(const std::vector<BLEUUID> &uuids);
  bool setCompleteServices32(const std::vector<BLEUUID> &uuids);
  bool setPartialServices(const BLEUUID &uuid);
  bool setPartialServices16(const std::vector<BLEUUID> &uuids);
  bool setPartialServices32(const std::vector<BLEUUID> &uuids);
  bool setServiceData(const BLEUUID &uuid, const uint8_t *data, size_t length);
  bool setServiceData(const BLEUUID &uuid, const std::string &data);
  bool setServiceData(const BLEUUID &uuid, const std::vector<uint8_t> &data);
  bool removeData(uint8_t type);
  void clearData();
  int getDataLocation(uint8_t type) const;

  std::string toString() const;
  std::vector<uint8_t> getPayload() const;

private:
  friend class BLEAdvertising;

  bool setServices(bool complete, uint8_t size, const std::vector<BLEUUID> &v_uuid);
  std::vector<uint8_t> m_payload{};
};  // BLEAdvertisementData

#endif  // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_BROADCASTER && !CONFIG_BT_NIMBLE_EXT_ADV
#endif  // NIMBLE_CPP_ADVERTISEMENT_DATA_H_
