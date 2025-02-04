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

#ifndef COMPONENTS_CPP_UTILS_BLESERVER_H_
#define COMPONENTS_CPP_UTILS_BLESERVER_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "host/ble_gap.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include <vector>
#include <array>

#define NIMBLE_ATT_REMOVE_HIDE   1
#define NIMBLE_ATT_REMOVE_DELETE 2

class BLEService;
class BLEServerCallbacks;
class BLEUUID;
class BLEConnInfo;
class BLEAddress;
class BLEService;
class BLECharacteristic;
#if CONFIG_BT_NIMBLE_EXT_ADV
class BLEExtAdvertising;
#else
class BLEAdvertising;
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
class BLEClient;
#endif

/**
 * @brief The model of a BLE server.
 */
class BLEServer {
public:
  void start();
  uint8_t getConnectedCount() const;
  bool disconnect(uint16_t connHandle, uint8_t reason = BLE_ERR_REM_USER_CONN_TERM) const;
  bool disconnect(const BLEConnInfo &connInfo, uint8_t reason = BLE_ERR_REM_USER_CONN_TERM) const;
  void setCallbacks(BLEServerCallbacks *pCallbacks, bool deleteCallbacks = true);
  void updateConnParams(uint16_t connHandle, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const;
  BLEService *createService(const char *uuid);
  BLEService *createService(const BLEUUID &uuid);
  BLEService *getServiceByUUID(const char *uuid, uint16_t instanceId = 0) const;
  BLEService *getServiceByUUID(const BLEUUID &uuid, uint16_t instanceId = 0) const;
  BLEService *getServiceByHandle(uint16_t handle) const;
  void removeService(BLEService *service, bool deleteSvc = false);
  void addService(BLEService *service);
  uint16_t getPeerMTU(uint16_t connHandle) const;
  std::vector<uint16_t> getPeerDevices() const;
  BLEConnInfo getPeerInfo(uint8_t index) const;
  BLEConnInfo getPeerInfo(const BLEAddress &address) const;
  BLEConnInfo getPeerInfoByHandle(uint16_t connHandle) const;
  void advertiseOnDisconnect(bool enable);
  void setDataLen(uint16_t connHandle, uint16_t tx_octets) const;

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  BLEClient *getClient(uint16_t connHandle);
  BLEClient *getClient(const BLEConnInfo &connInfo);
  void deleteClient();
#endif

#if CONFIG_BT_NIMBLE_EXT_ADV
  BLEExtAdvertising *getAdvertising() const;
  bool startAdvertising(uint8_t instanceId, int duration = 0, int maxEvents = 0) const;
  bool stopAdvertising(uint8_t instanceId) const;
  bool updatePhy(uint16_t connHandle, uint8_t txPhysMask, uint8_t rxPhysMask, uint16_t phyOptions);
  bool getPhy(uint16_t connHandle, uint8_t *txPhy, uint8_t *rxPhy);
#endif

#if !CONFIG_BT_NIMBLE_EXT_ADV
  BLEAdvertising *getAdvertising() const;
  bool startAdvertising(uint32_t duration = 0) const;
  bool stopAdvertising() const;
#endif

private:
  friend class BLEDevice;
  friend class BLEService;
  friend class BLECharacteristic;
#if CONFIG_BT_NIMBLE_EXT_ADV
  friend class BLEExtAdvertising;
#else
  friend class BLEAdvertising;
#endif

  BLEServer();
  ~BLEServer();

  bool m_gattsStarted    : 1;
  bool m_svcChanged      : 1;
  bool m_deleteCallbacks : 1;
#if !CONFIG_BT_NIMBLE_EXT_ADV
  bool m_advertiseOnDisconnect : 1;
#endif
  BLEServerCallbacks *m_pServerCallbacks;
  std::vector<BLEService *> m_svcVec;
  std::array<uint16_t, CONFIG_BT_NIMBLE_MAX_CONNECTIONS> m_connectedPeers;

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  BLEClient *m_pClient{nullptr};
#endif

  static int handleGapEvent(struct ble_gap_event *event, void *arg);
  static int handleGattEvent(uint16_t connHandle, uint16_t attrHandle, ble_gatt_access_ctxt *ctxt, void *arg);
  void serviceChanged();
  void resetGATT();

};  // BLEServer

/**
 * @brief Callbacks associated with the operation of a BLE server.
 */
class BLEServerCallbacks {
public:
  virtual ~BLEServerCallbacks(){};

  /**
     * @brief Handle a client connection.
     * This is called when a client connects.
     * @param [in] pServer A pointer to the BLE server that received the client connection.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information.
     * about the peer connection parameters.
     */
  virtual void onConnect(BLEServer *pServer, BLEConnInfo &connInfo);

  /**
     * @brief Handle a client disconnection.
     * This is called when a client disconnects.
     * @param [in] pServer A pointer to the BLE server that received the client disconnection.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     * about the peer connection parameters.
     * @param [in] reason The reason code for the disconnection.
     */
  virtual void onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason);

  /**
     * @brief Called when the connection MTU changes.
     * @param [in] MTU The new MTU value.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     * about the peer connection parameters.
     */
  virtual void onMTUChange(uint16_t MTU, BLEConnInfo &connInfo);

  /**
     * @brief Called when a client requests a passkey for pairing (display).
     * @return The passkey to be sent to the client.
     */
  virtual uint32_t onPassKeyDisplay();

  /**
     * @brief Called when using numeric comparision for pairing.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     * Should be passed back to BLEDevice::injectConfirmPasskey
     * @param [in] pin The pin to compare with the client.
     */
  virtual void onConfirmPassKey(BLEConnInfo &connInfo, uint32_t pin);

  /**
     * @brief Called when the pairing procedure is complete.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     * about the peer connection parameters.
     */
  virtual void onAuthenticationComplete(BLEConnInfo &connInfo);

  /**
     * @brief Called when the peer identity address is resolved.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     */
  virtual void onIdentity(BLEConnInfo &connInfo);

  /**
     * @brief Called when connection parameters are updated following a request to
     * update via BLEServer::updateConnParams
     * @param [in] connInfo A reference to a BLEConnInfo instance containing the
     * updated connection parameters.
     */
  virtual void onConnParamsUpdate(BLEConnInfo &connInfo);

#if CONFIG_BT_NIMBLE_EXT_ADV
  /**
     * @brief Called when the PHY update procedure is complete.
     * @param [in] connInfo A reference to a BLEConnInfo instance with information
     * about the peer connection parameters.
     * @param [in] txPhy The transmit PHY.
     * @param [in] rxPhy The receive PHY.
     * Possible values:
     * * BLE_GAP_LE_PHY_1M
     * * BLE_GAP_LE_PHY_2M
     * * BLE_GAP_LE_PHY_CODED
     */
  virtual void onPhyUpdate(BLEConnInfo &connInfo, uint8_t txPhy, uint8_t rxPhy);
#endif
};  // BLEServerCallbacks

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEServer.h
 *
 *  Created on: Apr 16, 2017
 *      Author: kolban
 */

#include <esp_gatts_api.h>

#include <string>
#include <string.h>
// #include "BLEDevice.h"

#include "BLEUUID.h"
#include "BLEAdvertising.h"
#include "BLECharacteristic.h"
#include "BLEService.h"
#include "BLESecurity.h"
#include "RTOS.h"
#include "BLEAddress.h"

class BLEServerCallbacks;
/* TODO possibly refactor this struct */
typedef struct {
  void *peer_device;  // peer device BLEClient or BLEServer - maybe its better to have 2 structures or union here
  bool connected;     // do we need it?
  uint16_t mtu;       // every peer device negotiate own mtu
} conn_status_t;

/**
 * @brief A data structure that manages the %BLE servers owned by a BLE server.
 */
class BLEServiceMap {
public:
  BLEService *getByHandle(uint16_t handle);
  BLEService *getByUUID(const char *uuid);
  BLEService *getByUUID(BLEUUID uuid, uint8_t inst_id = 0);
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
  void setByHandle(uint16_t handle, BLEService *service);
  void setByUUID(const char *uuid, BLEService *service);
  void setByUUID(BLEUUID uuid, BLEService *service);
  String toString();
  BLEService *getFirst();
  BLEService *getNext();
  void removeService(BLEService *service);
  int getRegisteredServiceCount();

private:
  std::map<uint16_t, BLEService *> m_handleMap;
  std::map<BLEService *, String> m_uuidMap;
  std::map<BLEService *, String>::iterator m_iterator;
};

/**
 * @brief The model of a %BLE server.
 */
class BLEServer {
public:
  uint32_t getConnectedCount();
  BLEService *createService(const char *uuid);
  BLEService *createService(BLEUUID uuid, uint32_t numHandles = 15, uint8_t inst_id = 0);
  BLEAdvertising *getAdvertising();
  void setCallbacks(BLEServerCallbacks *pCallbacks);
  void startAdvertising();
  void removeService(BLEService *service);
  BLEService *getServiceByUUID(const char *uuid);
  BLEService *getServiceByUUID(BLEUUID uuid);
  bool connect(BLEAddress address);
  void disconnect(uint16_t connId);
  uint16_t m_appId;
  void updateConnParams(esp_bd_addr_t remote_bda, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout);

  /* multi connection support */
  std::map<uint16_t, conn_status_t> getPeerDevices(bool client);
  void addPeerDevice(void *peer, bool is_client, uint16_t conn_id);
  bool removePeerDevice(uint16_t conn_id, bool client);
  BLEServer *getServerByConnId(uint16_t conn_id);
  void updatePeerMTU(uint16_t connId, uint16_t mtu);
  uint16_t getPeerMTU(uint16_t conn_id);
  uint16_t getConnId();

private:
  BLEServer();
  friend class BLEService;
  friend class BLECharacteristic;
  friend class BLEDevice;
  esp_ble_adv_data_t m_adv_data;
  // BLEAdvertising      m_bleAdvertising;
  uint16_t m_connId;
  uint32_t m_connectedCount;
  uint16_t m_gatts_if;
  std::map<uint16_t, conn_status_t> m_connectedServersMap;

  FreeRTOS::Semaphore m_semaphoreRegisterAppEvt = FreeRTOS::Semaphore("RegisterAppEvt");
  FreeRTOS::Semaphore m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
  FreeRTOS::Semaphore m_semaphoreOpenEvt = FreeRTOS::Semaphore("OpenEvt");
  BLEServiceMap m_serviceMap;
  BLEServerCallbacks *m_pServerCallbacks = nullptr;

  void createApp(uint16_t appId);
  uint16_t getGattsIf();
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
  void registerApp(uint16_t);
};  // BLEServer

/**
 * @brief Callbacks associated with the operation of a %BLE server.
 */
class BLEServerCallbacks {
public:
  virtual ~BLEServerCallbacks(){};
  /**
	 * @brief Handle a new client connection.
	 *
	 * When a new client connects, we are invoked.
	 *
	 * @param [in] pServer A reference to the %BLE server that received the client connection.
	 */
  virtual void onConnect(BLEServer *pServer);
  virtual void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param);
  /**
	 * @brief Handle an existing client disconnection.
	 *
	 * When an existing client disconnects, we are invoked.
	 *
	 * @param [in] pServer A reference to the %BLE server that received the existing client disconnection.
	 */
  virtual void onDisconnect(BLEServer *pServer);
  virtual void onDisconnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param);

  /**
	 * @brief Handle a new client connection.
	 *
	 * When the MTU changes this method is invoked.
	 *
	 * @param [in] pServer A reference to the %BLE server that received the client connection.
	 * @param [in] param A reference to esp_ble_gatts_cb_param_t.
	 */
  virtual void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param);
};  // BLEServerCallbacks

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLESERVER_H_ */
