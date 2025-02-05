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

#ifndef COMPONENTS_CPP_UTILS_BLEADVERTISEDDEVICE_H_
#define COMPONENTS_CPP_UTILS_BLEADVERTISEDDEVICE_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

typedef enum {
  BLE_UNKNOWN_FRAME,
  BLE_EDDYSTONE_UUID_FRAME,
  BLE_EDDYSTONE_URL_FRAME,
  BLE_EDDYSTONE_TLM_FRAME,
  BLE_FRAME_MAX
} ble_frame_type_t;

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)

#include "BLEAddress.h"
#include "BLEScan.h"
#include "BLEUUID.h"

#include "host/ble_hs_adv.h"
#include "host/ble_gap.h"

#include <vector>

class BLEScan;
/**
 * @brief A representation of a %BLE advertised device found by a scan.
 *
 * When we perform a %BLE scan, the result will be a set of devices that are advertising.  This
 * class provides a model of a detected device.
 */
class BLEAdvertisedDevice {
public:
  BLEAdvertisedDevice() = default;

  uint8_t getAdvType() const;
  uint8_t getAdvFlags() const;
  uint16_t getAppearance() const;
  uint16_t getAdvInterval() const;
  uint16_t getMinInterval() const;
  uint16_t getMaxInterval() const;
  uint8_t getManufacturerDataCount() const;
  const BLEAddress &getAddress() const;
  std::string getManufacturerData(uint8_t index = 0) const;
  std::string getURI() const;
  std::string getPayloadByType(uint16_t type, uint8_t index = 0) const;
  std::string getName() const;
  int8_t getRSSI() const;
  BLEScan *getScan() const;
  uint8_t getServiceDataCount() const;
  std::string getServiceData(uint8_t index = 0) const;
  std::string getServiceData(const BLEUUID &uuid) const;
  BLEUUID getServiceDataUUID(uint8_t index = 0) const;
  BLEUUID getServiceUUID(uint8_t index = 0) const;
  uint8_t getServiceUUIDCount() const;
  BLEAddress getTargetAddress(uint8_t index = 0) const;
  uint8_t getTargetAddressCount() const;
  int8_t getTXPower() const;
  uint8_t getAdvLength() const;
  uint8_t getAddressType() const;
  bool isAdvertisingService(const BLEUUID &uuid) const;
  bool haveAppearance() const;
  bool haveManufacturerData() const;
  bool haveName() const;
  bool haveServiceData() const;
  bool haveServiceUUID() const;
  bool haveTXPower() const;
  bool haveConnParams() const;
  bool haveAdvInterval() const;
  bool haveTargetAddress() const;
  bool haveURI() const;
  bool haveType(uint16_t type) const;
  std::string toString() const;
  bool isConnectable() const;
  bool isScannable() const;
  bool isLegacyAdvertisement() const;
#if CONFIG_BT_NIMBLE_EXT_ADV
  uint8_t getSetId() const;
  uint8_t getPrimaryPhy() const;
  uint8_t getSecondaryPhy() const;
  uint16_t getPeriodicInterval() const;
#endif

  const std::vector<uint8_t> &getPayload() const;
  const std::vector<uint8_t>::const_iterator begin() const;
  const std::vector<uint8_t>::const_iterator end() const;

  // Compatibility with Bluedroid
  [[deprecated]]
  ble_frame_type_t getFrameType() const;

  /**
     * @brief A template to convert the service data to <type\>.
     * @tparam T The type to convert the data to.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getManufacturerData<type>(skipSizeCheck);</tt>
     */
  template<typename T> T getManufacturerData(bool skipSizeCheck = false) const {
    std::string data = getManufacturerData();
    if (!skipSizeCheck && data.size() < sizeof(T)) {
      return T();
    }
    const char *pData = data.data();
    return *((T *)pData);
  }

  /**
     * @brief A template to convert the service data to <tt><type\></tt>.
     * @tparam T The type to convert the data to.
     * @param [in] index The vector index of the service data requested.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getServiceData<type>(skipSizeCheck);</tt>
     */
  template<typename T> T getServiceData(uint8_t index = 0, bool skipSizeCheck = false) const {
    std::string data = getServiceData(index);
    if (!skipSizeCheck && data.size() < sizeof(T)) {
      return T();
    }
    const char *pData = data.data();
    return *((T *)pData);
  }

  /**
     * @brief A template to convert the service data to <tt><type\></tt>.
     * @tparam T The type to convert the data to.
     * @param [in] uuid The uuid of the service data requested.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getServiceData<type>(skipSizeCheck);</tt>
     */
  template<typename T> T getServiceData(const BLEUUID &uuid, bool skipSizeCheck = false) const {
    std::string data = getServiceData(uuid);
    if (!skipSizeCheck && data.size() < sizeof(T)) {
      return T();
    }
    const char *pData = data.data();
    return *((T *)pData);
  }

private:
  friend class BLEScan;

  BLEAdvertisedDevice(const ble_gap_event *event, uint8_t eventType);
  void update(const ble_gap_event *event, uint8_t eventType);
  uint8_t findAdvField(uint8_t type, uint8_t index = 0, size_t *data_loc = nullptr) const;
  size_t findServiceData(uint8_t index, uint8_t *bytes) const;

  BLEAddress m_address{};
  uint8_t m_advType{};
  int8_t m_rssi{};
  uint8_t m_callbackSent{};
  uint8_t m_advLength{};

#if CONFIG_BT_NIMBLE_EXT_ADV
  bool m_isLegacyAdv{};
  uint8_t m_sid{};
  uint8_t m_primPhy{};
  uint8_t m_secPhy{};
  uint16_t m_periodicItvl{};
#endif

  std::vector<uint8_t> m_payload;
};

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEAdvertisedDevice.h
 *
 *  Created on: Jul 3, 2017
 *      Author: kolban
 */

#include <esp_gattc_api.h>

#include <map>

#include "BLEAddress.h"
#include "BLEScan.h"
#include "BLEUUID.h"

class BLEScan;
/**
 * @brief A representation of a %BLE advertised device found by a scan.
 *
 * When we perform a %BLE scan, the result will be a set of devices that are advertising.  This
 * class provides a model of a detected device.
 */
class BLEAdvertisedDevice {
public:
  BLEAdvertisedDevice();

  BLEAddress getAddress();
  uint16_t getAppearance();
  String getManufacturerData();
  String getName();
  int getRSSI();
  BLEScan *getScan();
  String getServiceData();
  String getServiceData(int i);
  BLEUUID getServiceDataUUID();
  BLEUUID getServiceDataUUID(int i);
  BLEUUID getServiceUUID();
  BLEUUID getServiceUUID(int i);
  int getServiceDataCount();
  int getServiceDataUUIDCount();
  int getServiceUUIDCount();
  int8_t getTXPower();
  uint8_t *getPayload();
  size_t getPayloadLength();
  esp_ble_addr_type_t getAddressType();
  ble_frame_type_t getFrameType();
  void setAddressType(esp_ble_addr_type_t type);

  bool isAdvertisingService(BLEUUID uuid);
  bool haveAppearance();
  bool haveManufacturerData();
  bool haveName();
  bool haveRSSI();
  bool haveServiceData();
  bool haveServiceUUID();
  bool haveTXPower();

  String toString();

private:
  friend class BLEScan;

  void parseAdvertisement(uint8_t *payload, size_t total_len = 62);
  void setPayload(uint8_t *payload, size_t total_len = 62);
  void setAddress(BLEAddress address);
  void setAdFlag(uint8_t adFlag);
  void setAdvertizementResult(uint8_t *payload);
  void setAppearance(uint16_t appearance);
  void setManufacturerData(String manufacturerData);
  void setName(String name);
  void setRSSI(int rssi);
  void setScan(BLEScan *pScan);
  void setServiceData(String data);
  void setServiceDataUUID(BLEUUID uuid);
  void setServiceUUID(const char *serviceUUID);
  void setServiceUUID(BLEUUID serviceUUID);
  void setTXPower(int8_t txPower);

  bool m_haveAppearance;
  bool m_haveManufacturerData;
  bool m_haveName;
  bool m_haveRSSI;
  bool m_haveTXPower;

  BLEAddress m_address = BLEAddress((uint8_t *)"\0\0\0\0\0\0");
  uint8_t m_adFlag;
  uint16_t m_appearance;
  int m_deviceType;
  String m_manufacturerData;
  String m_name;
  BLEScan *m_pScan;
  int m_rssi;
  std::vector<BLEUUID> m_serviceUUIDs;
  int8_t m_txPower;
  std::vector<String> m_serviceData;
  std::vector<BLEUUID> m_serviceDataUUIDs;
  uint8_t *m_payload;
  size_t m_payloadLength = 0;
  esp_ble_addr_type_t m_addressType;
};

/**
 * @brief A callback handler for callbacks associated device scanning.
 *
 * When we are performing a scan as a %BLE client, we may wish to know when a new device that is advertising
 * has been found.  This class can be sub-classed and registered such that when a scan is performed and
 * a new advertised device has been found, we will be called back to be notified.
 */
class BLEAdvertisedDeviceCallbacks {
public:
  virtual ~BLEAdvertisedDeviceCallbacks() {}
  /**
	 * @brief Called when a new scan result is detected.
	 *
	 * As we are scanning, we will find new devices.  When found, this call back is invoked with a reference to the
	 * device that was found.  During any individual scan, a device will only be detected one time.
	 */
  virtual void onResult(BLEAdvertisedDevice advertisedDevice) = 0;
};

#ifdef SOC_BLE_50_SUPPORTED
class BLEExtAdvertisingCallbacks {
public:
  virtual ~BLEExtAdvertisingCallbacks() {}
  /**
	 * @brief Called when a new scan result is detected.
	 *
	 * As we are scanning, we will find new devices.  When found, this call back is invoked with a reference to the
	 * device that was found.  During any individual scan, a device will only be detected one time.
	 */
  virtual void onResult(esp_ble_gap_ext_adv_report_t report) = 0;
};
#endif  // SOC_BLE_50_SUPPORTED

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEADVERTISEDDEVICE_H_ */
