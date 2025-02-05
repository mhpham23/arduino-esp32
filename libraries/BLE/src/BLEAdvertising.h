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

#ifndef COMPONENTS_CPP_UTILS_BLEADVERTISING_H_
#define COMPONENTS_CPP_UTILS_BLEADVERTISING_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if (defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && !CONFIG_BT_NIMBLE_EXT_ADV)

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

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEAdvertising.h
 *
 *  Created on: Jun 21, 2017
 *      Author: kolban
 */

#include <esp_gap_ble_api.h>
#include "BLEUUID.h"
#include <vector>
#include "RTOS.h"

/**
 * @brief Advertisement data set by the programmer to be published by the %BLE server.
 */
class BLEAdvertisementData {
  // Only a subset of the possible BLE architected advertisement fields are currently exposed.  Others will
  // be exposed on demand/request or as time permits.
  //
public:
  void setAppearance(uint16_t appearance);
  void setCompleteServices(BLEUUID uuid);
  void setFlags(uint8_t);
  void setManufacturerData(String data);
  void setName(String name);
  void setPartialServices(BLEUUID uuid);
  void setServiceData(BLEUUID uuid, String data);
  void setShortName(String name);
  void addData(String data);  // Add data to the payload.
  String getPayload();        // Retrieve the current advert payload.

private:
  friend class BLEAdvertising;
  String m_payload;  // The payload of the advertisement.
};  // BLEAdvertisementData

/**
 * @brief Perform and manage %BLE advertising.
 *
 * A %BLE server will want to perform advertising in order to make itself known to %BLE clients.
 */
class BLEAdvertising {
public:
  BLEAdvertising();
  void addServiceUUID(BLEUUID serviceUUID);
  void addServiceUUID(const char *serviceUUID);
  bool removeServiceUUID(int index);
  bool removeServiceUUID(BLEUUID serviceUUID);
  bool removeServiceUUID(const char *serviceUUID);
  void start();
  void stop();
  void setAppearance(uint16_t appearance);
  void setAdvertisementType(esp_ble_adv_type_t adv_type);
  void setAdvertisementChannelMap(esp_ble_adv_channel_t channel_map);
  void setMaxInterval(uint16_t maxinterval);
  void setMinInterval(uint16_t mininterval);
  void setAdvertisementData(BLEAdvertisementData &advertisementData);
  void setScanFilter(bool scanRequestWhitelistOnly, bool connectWhitelistOnly);
  void setScanResponseData(BLEAdvertisementData &advertisementData);
  void setPrivateAddress(esp_ble_addr_type_t type = BLE_ADDR_TYPE_RANDOM);
  void setDeviceAddress(esp_bd_addr_t addr, esp_ble_addr_type_t type = BLE_ADDR_TYPE_RANDOM);

  void handleGAPEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
  void setMinPreferred(uint16_t);
  void setMaxPreferred(uint16_t);
  void setScanResponse(bool);

private:
  esp_ble_adv_data_t m_advData;
  esp_ble_adv_data_t m_scanRespData;  // Used for configuration of scan response data when m_scanResp is true
  esp_ble_adv_params_t m_advParams;
  std::vector<BLEUUID> m_serviceUUIDs;
  bool m_customAdvData = false;           // Are we using custom advertising data?
  bool m_customScanResponseData = false;  // Are we using custom scan response data?
  FreeRTOS::Semaphore m_semaphoreSetAdv = FreeRTOS::Semaphore("startAdvert");
  bool m_scanResp = true;
};

#ifdef SOC_BLE_50_SUPPORTED

class BLEMultiAdvertising {
private:
  esp_ble_gap_ext_adv_params_t *params_arrays;
  esp_ble_gap_ext_adv_t *ext_adv;
  uint8_t count;

public:
  BLEMultiAdvertising(uint8_t num = 1);
  ~BLEMultiAdvertising() {}

  bool setAdvertisingParams(uint8_t instance, const esp_ble_gap_ext_adv_params_t *params);
  bool setAdvertisingData(uint8_t instance, uint16_t length, const uint8_t *data);
  bool setScanRspData(uint8_t instance, uint16_t length, const uint8_t *data);
  bool start();
  bool start(uint8_t num, uint8_t from);
  void setDuration(uint8_t instance, int duration = 0, int max_events = 0);
  bool setInstanceAddress(uint8_t instance, esp_bd_addr_t rand_addr);
  bool stop(uint8_t num_adv, const uint8_t *ext_adv_inst);
  bool remove(uint8_t instance);
  bool clear();
  bool setPeriodicAdvertisingParams(uint8_t instance, const esp_ble_gap_periodic_adv_params_t *params);
  bool setPeriodicAdvertisingData(uint8_t instance, uint16_t length, const uint8_t *data);
  bool startPeriodicAdvertising(uint8_t instance);
};

#endif  // SOC_BLE_50_SUPPORTED

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEADVERTISING_H_ */
