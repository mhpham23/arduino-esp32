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

#ifndef NIMBLE_CPP_DEVICE_H_
#define NIMBLE_CPP_DEVICE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)
#ifndef CONFIG_IDF_TARGET_ESP32P4
#include <esp_bt.h>
#endif

#include <host/ble_gap.h>

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include <string>
#include <vector>

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#include <array>
class BLEClient;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)
class BLEScan;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)
#if CONFIG_BT_NIMBLE_EXT_ADV
class BLEExtAdvertising;
#else
class BLEAdvertising;
#endif
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
class BLEServer;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL) || defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
class BLEConnInfo;
#endif

class BLEAddress;

#define BLEDevice                    BLEDevice
#define BLEClient                    BLEClient
#define BLERemoteService             BLERemoteService
#define BLERemoteCharacteristic      BLERemoteCharacteristic
#define BLERemoteDescriptor          BLERemoteDescriptor
#define BLEAdvertisedDevice          BLEAdvertisedDevice
#define BLEScan                      BLEScan
#define BLEUUID                      BLEUUID
#define BLEAddress                   BLEAddress
#define BLEUtils                     BLEUtils
#define BLEClientCallbacks           BLEClientCallbacks
#define BLEAdvertisedDeviceCallbacks BLEScanCallbacks
#define BLEScanResults               BLEScanResults
#define BLEServer                    BLEServer
#define BLEService                   BLEService
#define BLECharacteristic            BLECharacteristic
#define BLEAdvertising               BLEAdvertising
#define BLEServerCallbacks           BLEServerCallbacks
#define BLECharacteristicCallbacks   BLECharacteristicCallbacks
#define BLEAdvertisementData         BLEAdvertisementData
#define BLEDescriptor                BLEDescriptor
#define BLE2904                      BLE2904
#define BLEDescriptorCallbacks       BLEDescriptorCallbacks
#define BLEBeacon                    BLEBeacon
#define BLEEddystoneTLM              BLEEddystoneTLM
#define BLEEddystoneURL              BLEEddystoneURL
#define BLEConnInfo                  BLEConnInfo

#ifdef CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#define NIMBLE_MAX_CONNECTIONS CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#else
#define NIMBLE_MAX_CONNECTIONS CONFIG_NIMBLE_MAX_CONNECTIONS
#endif

enum class BLETxPowerType {
  All = 0,
  Advertise = 1,
  Scan = 2,
  Connection = 3
};

typedef int (*gap_event_handler)(ble_gap_event *event, void *arg);

/**
 * @brief A model of a BLE Device from which all the BLE roles are created.
 */
class BLEDevice {
public:
  static bool init(const std::string &deviceName);
  static bool deinit(bool clearAll = false);
  static bool setDeviceName(const std::string &deviceName);
  static bool isInitialized();
  static BLEAddress getAddress();
  static std::string toString();
  static bool whiteListAdd(const BLEAddress &address);
  static bool whiteListRemove(const BLEAddress &address);
  static bool onWhiteList(const BLEAddress &address);
  static size_t getWhiteListCount();
  static BLEAddress getWhiteListAddress(size_t index);
  static bool setOwnAddrType(uint8_t type);
  static bool setOwnAddr(const BLEAddress &addr);
  static bool setOwnAddr(const uint8_t *addr);
  static void setScanDuplicateCacheSize(uint16_t cacheSize);
  static void setScanFilterMode(uint8_t type);
  static bool setCustomGapHandler(gap_event_handler handler);
  static void setSecurityAuth(bool bonding, bool mitm, bool sc);
  static void setSecurityAuth(uint8_t auth);
  static void setSecurityIOCap(uint8_t iocap);
  static void setSecurityInitKey(uint8_t initKey);
  static void setSecurityRespKey(uint8_t respKey);
  static void setSecurityPasskey(uint32_t passKey);
  static uint32_t getSecurityPasskey();
  static bool startSecurity(uint16_t connHandle, int *rcPtr = nullptr);
  static bool setMTU(uint16_t mtu);
  static uint16_t getMTU();
  static void onReset(int reason);
  static void onSync(void);
  static void host_task(void *param);
  static int getPower(BLETxPowerType type = BLETxPowerType::All);
  static bool setPower(int8_t dbm, BLETxPowerType type = BLETxPowerType::All);

#ifndef CONFIG_IDF_TARGET_ESP32P4
  static esp_power_level_t getPowerLevel(esp_ble_power_type_t powerType = ESP_BLE_PWR_TYPE_DEFAULT);
  static bool setPowerLevel(esp_power_level_t powerLevel, esp_ble_power_type_t powerType = ESP_BLE_PWR_TYPE_DEFAULT);
#endif

#if CONFIG_BT_NIMBLE_EXT_ADV
  static bool setDefaultPhy(uint8_t txPhyMask, uint8_t rxPhyMask);
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)
  static BLEScan *getScan();
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
  static BLEServer *createServer();
  static BLEServer *getServer();
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL) || defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  static bool injectConfirmPasskey(const BLEConnInfo &peerInfo, bool accept);
  static bool injectPassKey(const BLEConnInfo &peerInfo, uint32_t pin);
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)
#if CONFIG_BT_NIMBLE_EXT_ADV
  static BLEExtAdvertising *getAdvertising();
  static bool startAdvertising(uint8_t instId, int duration = 0, int maxEvents = 0);
  static bool stopAdvertising(uint8_t instId);
  static bool stopAdvertising();
#endif
#if !CONFIG_BT_NIMBLE_EXT_ADV
  static BLEAdvertising *getAdvertising();
  static bool startAdvertising(uint32_t duration = 0);
  static bool stopAdvertising();
#endif
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  static BLEClient *createClient();
  static BLEClient *createClient(const BLEAddress &peerAddress);
  static bool deleteClient(BLEClient *pClient);
  static BLEClient *getClientByHandle(uint16_t connHandle);
  static BLEClient *getClientByPeerAddress(const BLEAddress &peerAddress);
  static BLEClient *getDisconnectedClient();
  static size_t getCreatedClientCount();
  static std::vector<BLEClient *> getConnectedClients();
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL) || defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
  static bool deleteBond(const BLEAddress &address);
  static int getNumBonds();
  static bool isBonded(const BLEAddress &address);
  static bool deleteAllBonds();
  static BLEAddress getBondedAddress(int index);
#endif

private:
  static bool m_synced;
  static bool m_initialized;
  static uint32_t m_passkey;
  static ble_gap_event_listener m_listener;
  static uint8_t m_ownAddrType;
  static std::vector<BLEAddress> m_whiteList;

#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)
  static BLEScan *m_pScan;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
  static BLEServer *m_pServer;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)
#if CONFIG_BT_NIMBLE_EXT_ADV
  static BLEExtAdvertising *m_bleAdvertising;
#else
  static BLEAdvertising *m_bleAdvertising;
#endif
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  static std::array<BLEClient *, NIMBLE_MAX_CONNECTIONS> m_pClients;
#endif

#ifdef CONFIG_BTDM_BLE_SCAN_DUPL
  static uint16_t m_scanDuplicateSize;
  static uint8_t m_scanFilterMode;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  friend class BLEClient;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)
  friend class BLEScan;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
  friend class BLEServer;
  friend class BLECharacteristic;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)
  friend class BLEAdvertising;
#if CONFIG_BT_NIMBLE_EXT_ADV
  friend class BLEExtAdvertising;
  friend class BLEExtAdvertisement;
#endif
#endif
};

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#include "BLEClient.h"
#include "BLERemoteService.h"
#include "BLERemoteCharacteristic.h"
#include "BLERemoteDescriptor.h"
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER)
#include "BLEScan.h"
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
#include "BLEServer.h"
#include "BLEService.h"
#include "BLECharacteristic.h"
#include "BLEDescriptor.h"
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)
#if CONFIG_BT_NIMBLE_EXT_ADV
#include "BLEExtAdvertising.h"
#else
#include "BLEAdvertising.h"
#endif
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL) || defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
#include "BLEConnInfo.h"
#endif

#include "BLEUtils.h"

#endif  // CONFIG_BT_ENABLED
#endif  // NIMBLE_CPP_DEVICE_H_
