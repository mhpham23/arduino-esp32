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

#ifndef NIMBLE_CPP_SCAN_H_
#define NIMBLE_CPP_SCAN_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)

#include "BLEAdvertisedDevice.h"
#include "BLEUtils.h"

#include "host/ble_gap.h"

#include <vector>

class BLEDevice;
class BLEScan;
class BLEAdvertisedDevice;
class BLEScanCallbacks;
class BLEAddress;

/**
 * @brief A class that contains and operates on the results of a BLE scan.
 * @details When a scan completes, we have a set of found devices.  Each device is described
 * by a BLEAdvertisedDevice object.  The number of items in the set is given by
 * getCount().  We can retrieve a device by calling getDevice() passing in the
 * index (starting at 0) of the desired device.
 */
class BLEScanResults {
public:
  void dump() const;
  int getCount() const;
  const BLEAdvertisedDevice *getDevice(uint32_t idx) const;
  const BLEAdvertisedDevice *getDevice(const BLEAddress &address) const;
  std::vector<BLEAdvertisedDevice *>::const_iterator begin() const;
  std::vector<BLEAdvertisedDevice *>::const_iterator end() const;

private:
  friend BLEScan;
  std::vector<BLEAdvertisedDevice *> m_deviceVec;
};

/**
 * @brief Perform and manage %BLE scans.
 *
 * Scanning is associated with a %BLE client that is attempting to locate BLE servers.
 */
class BLEScan {
public:
  bool start(uint32_t duration, bool isContinue = false, bool restart = true);
  bool isScanning();
  void setScanCallbacks(BLEScanCallbacks *pScanCallbacks, bool wantDuplicates = false);
  void setActiveScan(bool active);
  void setInterval(uint16_t intervalMs);
  void setWindow(uint16_t windowMs);
  void setDuplicateFilter(uint8_t enabled);
  void setLimitedOnly(bool enabled);
  void setFilterPolicy(uint8_t filter);
  bool stop();
  void clearResults();
  BLEScanResults getResults();
  BLEScanResults getResults(uint32_t duration, bool is_continue = false);
  void setMaxResults(uint8_t maxResults);
  void erase(const BLEAddress &address);
  void erase(const BLEAdvertisedDevice *device);

#if CONFIG_BT_NIMBLE_EXT_ADV
  enum Phy {
    SCAN_1M = 0x01,
    SCAN_CODED = 0x02,
    SCAN_ALL = 0x03
  };
  void setPhy(Phy phyMask);
  void setPeriod(uint32_t periodMs);
#endif

private:
  friend class BLEDevice;

  BLEScan();
  ~BLEScan();
  static int handleGapEvent(ble_gap_event *event, void *arg);
  void onHostSync();

  BLEScanCallbacks *m_pScanCallbacks;
  ble_gap_disc_params m_scanParams;
  BLEScanResults m_scanResults;
  BLETaskData *m_pTaskData;
  uint8_t m_maxResults;

#if CONFIG_BT_NIMBLE_EXT_ADV
  uint8_t m_phy{SCAN_ALL};
  uint16_t m_period{0};
#endif
};

/**
 * @brief A callback handler for callbacks associated device scanning.
 */
class BLEScanCallbacks {
public:
  virtual ~BLEScanCallbacks() {}

  /**
     * @brief Called when a new device is discovered, before the scan result is received (if applicable).
     * @param [in] advertisedDevice The device which was discovered.
     */
  virtual void onDiscovered(const BLEAdvertisedDevice *advertisedDevice);

  /**
     * @brief Called when a new scan result is complete, including scan response data (if applicable).
     * @param [in] advertisedDevice The device for which the complete result is available.
     */
  virtual void onResult(const BLEAdvertisedDevice *advertisedDevice);

  /**
     * @brief Called when a scan operation ends.
     * @param [in] scanResults The results of the scan that ended.
     * @param [in] reason The reason code for why the scan ended.
     */
  virtual void onScanEnd(const BLEScanResults &scanResults, int reason);
};

#endif  // CONFIG_BT_ENABLED CONFIG_BT_NIMBLE_ROLE_OBSERVER
#endif  // NIMBLE_CPP_SCAN_H_
