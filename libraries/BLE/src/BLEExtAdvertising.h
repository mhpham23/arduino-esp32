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

#ifndef NIMBLE_CPP_EXTADVERTISING_H_
#define NIMBLE_CPP_EXTADVERTISING_H_

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && CONFIG_BT_NIMBLE_EXT_ADV

#include "host/ble_gap.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include "BLEAddress.h"

#include <string>
#include <vector>

class BLEExtAdvertisingCallbacks;
class BLEUUID;

/**
 * @brief Extended advertisement data
 */
class BLEExtAdvertisement {
public:
  BLEExtAdvertisement(uint8_t priPhy = BLE_HCI_LE_PHY_1M, uint8_t secPhy = BLE_HCI_LE_PHY_1M);
  bool setAppearance(uint16_t appearance);
  bool addServiceUUID(const BLEUUID &serviceUUID);
  bool addServiceUUID(const char *serviceUUID);
  bool removeServiceUUID(const BLEUUID &serviceUUID);
  bool removeServiceUUID(const char *serviceUUID);
  bool removeServices();
  bool setCompleteServices(const BLEUUID &uuid);
  bool setCompleteServices16(const std::vector<BLEUUID> &uuids);
  bool setCompleteServices32(const std::vector<BLEUUID> &uuids);
  bool setFlags(uint8_t flag);
  bool setManufacturerData(const uint8_t *data, size_t length);
  bool setManufacturerData(const std::string &data);
  bool setManufacturerData(const std::vector<uint8_t> &data);
  bool setURI(const std::string &uri);
  bool setName(const std::string &name, bool isComplete = true);
  bool setPartialServices(const BLEUUID &uuid);
  bool setPartialServices16(const std::vector<BLEUUID> &uuids);
  bool setPartialServices32(const std::vector<BLEUUID> &uuids);
  bool setServiceData(const BLEUUID &uuid, const uint8_t *data, size_t length);
  bool setServiceData(const BLEUUID &uuid, const std::string &data);
  bool setServiceData(const BLEUUID &uuid, const std::vector<uint8_t> &data);
  bool setShortName(const std::string &name);
  bool setData(const uint8_t *data, size_t length);
  bool addData(const uint8_t *data, size_t length);
  bool addData(const std::string &data);
  bool setPreferredParams(uint16_t min, uint16_t max);
  bool addTxPower();
  void setLegacyAdvertising(bool enable);
  void setConnectable(bool enable);
  void setScannable(bool enable);
  void setMinInterval(uint32_t mininterval);
  void setMaxInterval(uint32_t maxinterval);
  void setPrimaryPhy(uint8_t phy);
  void setSecondaryPhy(uint8_t phy);
  void setScanFilter(bool scanRequestWhitelistOnly, bool connectWhitelistOnly);
  void setDirectedPeer(const BLEAddress &addr);
  void setDirected(bool enable, bool high_duty = true);
  void setAnonymous(bool enable);
  void setPrimaryChannels(bool ch37, bool ch38, bool ch39);
  void setTxPower(int8_t dbm);
  void setAddress(const BLEAddress &addr);
  void enableScanRequestCallback(bool enable);
  void clearData();
  int getDataLocation(uint8_t type) const;
  bool removeData(uint8_t type);
  size_t getDataSize() const;
  std::string toString() const;

private:
  friend class BLEExtAdvertising;

  bool setServices(bool complete, uint8_t size, const std::vector<BLEUUID> &uuids);

  std::vector<uint8_t> m_payload{};
  ble_gap_ext_adv_params m_params{};
  BLEAddress m_advAddress{};
};  // BLEExtAdvertisement

/**
 * @brief Extended advertising class.
 */
class BLEExtAdvertising {
public:
  BLEExtAdvertising();
  ~BLEExtAdvertising();
  bool start(uint8_t instId, int duration = 0, int maxEvents = 0);
  bool setInstanceData(uint8_t instId, BLEExtAdvertisement &adv);
  bool setScanResponseData(uint8_t instId, BLEExtAdvertisement &data);
  bool removeInstance(uint8_t instId);
  bool removeAll();
  bool stop(uint8_t instId);
  bool stop();
  bool isActive(uint8_t instId);
  bool isAdvertising();
  void setCallbacks(BLEExtAdvertisingCallbacks *callbacks, bool deleteCallbacks = true);

private:
  friend class BLEDevice;
  friend class BLEServer;

  void onHostSync();
  static int handleGapEvent(struct ble_gap_event *event, void *arg);

  bool m_deleteCallbacks;
  BLEExtAdvertisingCallbacks *m_pCallbacks;
  std::vector<bool> m_advStatus;
};

/**
 * @brief Callbacks associated with BLEExtAdvertising class.
 */
class BLEExtAdvertisingCallbacks {
public:
  virtual ~BLEExtAdvertisingCallbacks(){};

  /**
     * @brief Handle an advertising stop event.
     * @param [in] pAdv A convenience pointer to the extended advertising interface.
     * @param [in] reason The reason code for stopping the advertising.
     * @param [in] instId The instance ID of the advertisement that was stopped.
     */
  virtual void onStopped(BLEExtAdvertising *pAdv, int reason, uint8_t instId);

  /**
     * @brief Handle a scan response request.
     * This is called when a scanning device requests a scan response.
     * @param [in] pAdv A convenience pointer to the extended advertising interface.
     * @param [in] instId The instance ID of the advertisement that the scan response request was made.
     * @param [in] addr The address of the device making the request.
     */
  virtual void onScanRequest(BLEExtAdvertising *pAdv, uint8_t instId, BLEAddress addr);
};  // BLEExtAdvertisingCallbacks

#endif  // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_BROADCASTER && CONFIG_BT_NIMBLE_EXT_ADV
#endif  // NIMBLE_CPP_EXTADVERTISING_H_
