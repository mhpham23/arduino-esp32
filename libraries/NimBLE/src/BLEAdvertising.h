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

#ifndef NIMBLE_CPP_ADVERTISING_H_
#define NIMBLE_CPP_ADVERTISING_H_

#include "sdkconfig.h"
#if (defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && !CONFIG_BT_NIMBLE_EXT_ADV)

#include "host/ble_gap.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include "BLEUUID.h"
#include "BLEAddress.h"
#include "BLEAdvertisementData.h"

#include <functional>
#include <string>
#include <vector>

class BLEAdvertising;
typedef std::function<void(BLEAdvertising *)> advCompleteCB_t;

/**
 * @brief Perform and manage BLE advertising.
 *
 * A BLE server will want to perform advertising in order to make itself known to BLE clients.
 */
class BLEAdvertising {
public:
  BLEAdvertising();
  bool start(uint32_t duration = 0, const BLEAddress *dirAddr = nullptr);
  void setAdvertisingCompleteCallback(advCompleteCB_t callback);
  bool stop();
  bool setConnectableMode(uint8_t mode);
  bool setDiscoverableMode(uint8_t mode);
  bool reset();
  bool isAdvertising();
  void setScanFilter(bool scanRequestWhitelistOnly, bool connectWhitelistOnly);
  void enableScanResponse(bool enable);
  void setAdvertisingInterval(uint16_t interval);
  void setMaxInterval(uint16_t maxInterval);
  void setMinInterval(uint16_t minInterval);

  bool setAdvertisementData(const BLEAdvertisementData &advertisementData);
  bool setScanResponseData(const BLEAdvertisementData &advertisementData);
  const BLEAdvertisementData &getAdvertisementData();
  const BLEAdvertisementData &getScanData();
  void clearData();
  bool refreshAdvertisingData();

  bool addServiceUUID(const BLEUUID &serviceUUID);
  bool addServiceUUID(const char *serviceUUID);
  bool removeServiceUUID(const BLEUUID &serviceUUID);
  bool removeServiceUUID(const char *serviceUUID);
  bool removeServices();
  bool setAppearance(uint16_t appearance);
  bool setPreferredParams(uint16_t minInterval, uint16_t maxInterval);
  bool addTxPower();
  bool setName(const std::string &name);
  bool setManufacturerData(const uint8_t *data, size_t length);
  bool setManufacturerData(const std::string &data);
  bool setManufacturerData(const std::vector<uint8_t> &data);
  bool setURI(const std::string &uri);
  bool setServiceData(const BLEUUID &uuid, const uint8_t *data, size_t length);
  bool setServiceData(const BLEUUID &uuid, const std::string &data);
  bool setServiceData(const BLEUUID &uuid, const std::vector<uint8_t> &data);

private:
  friend class BLEDevice;
  friend class BLEServer;

  void onHostSync();
  static int handleGapEvent(ble_gap_event *event, void *arg);

  BLEAdvertisementData m_advData;
  BLEAdvertisementData m_scanData;
  ble_gap_adv_params m_advParams;
  advCompleteCB_t m_advCompCb;
  uint8_t m_slaveItvl[4];
  uint32_t m_duration;
  bool m_scanResp   : 1;
  bool m_advDataSet : 1;
};

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_BROADCASTER  && !CONFIG_BT_NIMBLE_EXT_ADV */
#endif /* NIMBLE_CPP_ADVERTISING_H_ */
