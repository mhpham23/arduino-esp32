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

#ifndef NIMBLE_CPP_CLIENT_H_
#define NIMBLE_CPP_CLIENT_H_

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "host/ble_gap.h"

#include "BLEAddress.h"

#include <stdint.h>
#include <vector>
#include <string>

class BLEAddress;
class BLEUUID;
class BLERemoteService;
class BLERemoteCharacteristic;
class BLEAdvertisedDevice;
class BLEAttValue;
class BLEClientCallbacks;
class BLEConnInfo;
struct BLETaskData;

/**
 * @brief A model of a BLE client.
 */
class BLEClient {
public:
  bool connect(const BLEAdvertisedDevice *device, bool deleteAttributes = true, bool asyncConnect = false, bool exchangeMTU = true);
  bool connect(const BLEAddress &address, bool deleteAttributes = true, bool asyncConnect = false, bool exchangeMTU = true);
  bool connect(bool deleteAttributes = true, bool asyncConnect = false, bool exchangeMTU = true);
  bool disconnect(uint8_t reason = BLE_ERR_REM_USER_CONN_TERM);
  bool cancelConnect() const;
  void setSelfDelete(bool deleteOnDisconnect, bool deleteOnConnectFail);
  BLEAddress getPeerAddress() const;
  bool setPeerAddress(const BLEAddress &address);
  int getRssi() const;
  bool isConnected() const;
  void setClientCallbacks(BLEClientCallbacks *pClientCallbacks, bool deleteCallbacks = true);
  std::string toString() const;
  uint16_t getConnHandle() const;
  uint16_t getMTU() const;
  bool exchangeMTU();
  bool secureConnection(bool async = false) const;
  void setConnectTimeout(uint32_t timeout);
  bool setDataLen(uint16_t txOctets);
  bool discoverAttributes();
  BLEConnInfo getConnInfo() const;
  int getLastError() const;
  bool updateConnParams(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout);
  void
    setConnectionParams(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout, uint16_t scanInterval = 16, uint16_t scanWindow = 16);
  const std::vector<BLERemoteService *> &getServices(bool refresh = false);
  std::vector<BLERemoteService *>::iterator begin();
  std::vector<BLERemoteService *>::iterator end();
  BLERemoteCharacteristic *getCharacteristic(uint16_t handle);
  BLERemoteService *getService(const char *uuid);
  BLERemoteService *getService(const BLEUUID &uuid);
  void deleteServices();
  size_t deleteService(const BLEUUID &uuid);
  BLEAttValue getValue(const BLEUUID &serviceUUID, const BLEUUID &characteristicUUID);
  bool setValue(const BLEUUID &serviceUUID, const BLEUUID &characteristicUUID, const BLEAttValue &value, bool response = false);

#if CONFIG_BT_NIMBLE_EXT_ADV
  void setConnectPhy(uint8_t phyMask);
  bool updatePhy(uint8_t txPhysMask, uint8_t rxPhysMask, uint16_t phyOptions = 0);
  bool getPhy(uint8_t *txPhy, uint8_t *rxPhy);
#endif

  struct Config {
    uint8_t deleteCallbacks     : 1;  // Delete the callback object when the client is deleted.
    uint8_t deleteOnDisconnect  : 1;  // Delete the client when disconnected.
    uint8_t deleteOnConnectFail : 1;  // Delete the client when a connection attempt fails.
    uint8_t asyncConnect        : 1;  // Connect asynchronously.
    uint8_t exchangeMTU         : 1;  // Exchange MTU after connection.
  };

  Config getConfig() const;
  void setConfig(Config config);

private:
  BLEClient(const BLEAddress &peerAddress);
  ~BLEClient();
  BLEClient(const BLEClient &) = delete;
  BLEClient &operator=(const BLEClient &) = delete;

  bool retrieveServices(const BLEUUID *uuidFilter = nullptr);
  static int handleGapEvent(struct ble_gap_event *event, void *arg);
  static int exchangeMTUCb(uint16_t conn_handle, const ble_gatt_error *error, uint16_t mtu, void *arg);
  static int serviceDiscoveredCB(uint16_t connHandle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);

  BLEAddress m_peerAddress;
  mutable int m_lastErr;
  int32_t m_connectTimeout;
  mutable BLETaskData *m_pTaskData;
  std::vector<BLERemoteService *> m_svcVec;
  BLEClientCallbacks *m_pClientCallbacks;
  uint16_t m_connHandle;
  uint8_t m_terminateFailCount;
  mutable uint8_t m_asyncSecureAttempt;
  Config m_config;

#if CONFIG_BT_NIMBLE_EXT_ADV
  uint8_t m_phyMask;
#endif
  ble_gap_conn_params m_connParams;

  friend class BLEDevice;
  friend class BLEServer;
};  // class BLEClient

/**
 * @brief Callbacks associated with a %BLE client.
 */
class BLEClientCallbacks {
public:
  virtual ~BLEClientCallbacks(){};

  /**
     * @brief Called after client connects.
     * @param [in] pClient A pointer to the connecting client object.
     */
  virtual void onConnect(BLEClient *pClient);

  /**
     * @brief Called when a connection attempt fails.
     * @param [in] pClient A pointer to the connecting client object.
     * @param [in] reason Contains the reason code for the connection failure.
     */
  virtual void onConnectFail(BLEClient *pClient, int reason);

  /**
     * @brief Called when disconnected from the server.
     * @param [in] pClient A pointer to the calling client object.
     * @param [in] reason Contains the reason code for the disconnection.
     */
  virtual void onDisconnect(BLEClient *pClient, int reason);

  /**
     * @brief Called when server requests to update the connection parameters.
     * @param [in] pClient A pointer to the calling client object.
     * @param [in] params A pointer to the struct containing the connection parameters requested.
     * @return True to accept the parameters.
     */
  virtual bool onConnParamsUpdateRequest(BLEClient *pClient, const ble_gap_upd_params *params);

  /**
     * @brief Called when server requests a passkey for pairing.
     * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
     */
  virtual void onPassKeyEntry(BLEConnInfo &connInfo);

  /**
     * @brief Called when the pairing procedure is complete.
     * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.\n
     * This can be used to check the status of the connection encryption/pairing.
     */
  virtual void onAuthenticationComplete(BLEConnInfo &connInfo);

  /**
     * @brief Called when using numeric comparision for pairing.
     * @param [in] connInfo A reference to a BLEConnInfo instance containing the peer info.
     * @param [in] pin The pin to compare with the server.
     */
  virtual void onConfirmPasskey(BLEConnInfo &connInfo, uint32_t pin);

  /**
     * @brief Called when the peer identity address is resolved.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     */
  virtual void onIdentity(BLEConnInfo &connInfo);

  /**
     * @brief Called when the connection MTU changes.
     * @param [in] pClient A pointer to the client that the MTU change is associated with.
     * @param [in] MTU The new MTU value.
     * about the peer connection parameters.
     */
  virtual void onMTUChange(BLEClient *pClient, uint16_t MTU);

#if CONFIG_BT_NIMBLE_EXT_ADV
  /**
     * @brief Called when the PHY update procedure is complete.
     * @param [in] pClient A pointer to the client whose PHY was updated.
     * about the peer connection parameters.
     * @param [in] txPhy The transmit PHY.
     * @param [in] rxPhy The receive PHY.
     * Possible values:
     * * BLE_GAP_LE_PHY_1M
     * * BLE_GAP_LE_PHY_2M
     * * BLE_GAP_LE_PHY_CODED
     */
  virtual void onPhyUpdate(BLEClient *pClient, uint8_t txPhy, uint8_t rxPhy);
#endif
};

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
#endif /* NIMBLE_CPP_CLIENT_H_ */
