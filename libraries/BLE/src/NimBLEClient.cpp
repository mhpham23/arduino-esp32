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

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLEClient.h"
#include "BLERemoteService.h"
#include "BLERemoteCharacteristic.h"
#include "BLEDevice.h"

#include "nimble/nimble_port.h"

#include <climits>

static const char *LOG_TAG = "BLEClient";
static BLEClientCallbacks defaultCallbacks;

/*
 * Design
 * ------
 * When we perform a getService() request, we are asking the BLE server to return each of the services
 * that it exposes.  For each service, we receive a callback which contains details
 * of the exposed service including its UUID.
 *
 * The objects we will invent for a BLEClient will be as follows:
 * * BLERemoteService - A model of a remote service.
 * * BLERemoteCharacteristic - A model of a remote characteristic
 * * BLERemoteDescriptor - A model of a remote descriptor.
 *
 * Since there is a hierarchical relationship here, we will have the idea that from a BLERemoteService will own
 * zero or more remote characteristics and a BLERemoteCharacteristic will own zero or more BLERemoteDescriptors.
 *
 * We will assume that a BLERemoteService contains a vector of owned characteristics
 * and that a BLERemoteCharacteristic contains a vector of owned descriptors.
 */

/**
 * @brief Constructor, private - only callable by BLEDevice::createClient
 * to ensure proper handling of the list of client objects.
 */
BLEClient::BLEClient(const BLEAddress &peerAddress)
  : m_peerAddress(peerAddress), m_lastErr{0}, m_connectTimeout{30000}, m_pTaskData{nullptr}, m_svcVec{}, m_pClientCallbacks{&defaultCallbacks},
    m_connHandle{BLE_HS_CONN_HANDLE_NONE}, m_terminateFailCount{0}, m_asyncSecureAttempt{0}, m_config{},
#if CONFIG_BT_NIMBLE_EXT_ADV
    m_phyMask{BLE_GAP_LE_PHY_1M_MASK | BLE_GAP_LE_PHY_2M_MASK | BLE_GAP_LE_PHY_CODED_MASK},
#endif
    m_connParams{
      16,
      16,
      BLE_GAP_INITIAL_CONN_ITVL_MIN,
      BLE_GAP_INITIAL_CONN_ITVL_MAX,
      BLE_GAP_INITIAL_CONN_LATENCY,
      BLE_GAP_INITIAL_SUPERVISION_TIMEOUT,
      BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
      BLE_GAP_INITIAL_CONN_MAX_CE_LEN
    } {
}  // BLEClient

/**
 * @brief Destructor, private - only callable by BLEDevice::deleteClient
 * to ensure proper disconnect and removal from device list.
 */
BLEClient::~BLEClient() {
  // We may have allocated service references associated with this client.
  // Before we are finished with the client, we must release resources.
  deleteServices();

  if (m_config.deleteCallbacks) {
    delete m_pClientCallbacks;
  }
}  // ~BLEClient

/**
 * @brief Delete all service objects created by this client and clear the vector.
 */
void BLEClient::deleteServices() {
  // Delete all the services.
  for (auto &it : m_svcVec) {
    delete it;
  }

  std::vector<BLERemoteService *>().swap(m_svcVec);
}  // deleteServices

/**
 * @brief Delete a service by UUID from the local database to free resources.
 * @param [in] uuid The UUID of the service to be deleted.
 * @return Number of services left.
 */
size_t BLEClient::deleteService(const BLEUUID &uuid) {
  // Delete the requested service.
  for (auto it = m_svcVec.begin(); it != m_svcVec.end(); ++it) {
    if ((*it)->getUUID() == uuid) {
      delete *it;
      m_svcVec.erase(it);
      break;
    }
  }

  return m_svcVec.size();
}  // deleteService

/**
 * @brief Connect to the BLE Server using the address of the last connected device, or the address\n
 * passed to the constructor.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool BLEClient::connect(bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
  return connect(m_peerAddress, deleteAttributes, asyncConnect, exchangeMTU);
}  // connect

/**
 * @brief Connect to an advertising device.
 * @param [in] device The device to connect to.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool BLEClient::connect(const BLEAdvertisedDevice *device, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
  BLEAddress address(device->getAddress());
  return connect(address, deleteAttributes, asyncConnect, exchangeMTU);
}  // connect

/**
 * @brief Connect to a BLE Server by address.
 * @param [in] address The address of the server.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool BLEClient::connect(const BLEAddress &address, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
  log_d(LOG_TAG, ">> connect(%s)", address.toString().c_str());

  if (!BLEDevice::m_synced) {
    log_e(LOG_TAG, "Host reset, wait for sync.");
    return false;
  }

  if (isConnected()) {
    log_e(LOG_TAG, "Client already connected");
    return false;
  }

  const ble_addr_t *peerAddr = address.getBase();
  if (ble_gap_conn_find_by_addr(peerAddr, NULL) == 0) {
    log_e(LOG_TAG, "A connection to %s already exists", address.toString().c_str());
    return false;
  }

  if (address.isNull()) {
    log_e(LOG_TAG, "Invalid peer address; (NULL)");
    return false;
  } else {
    m_peerAddress = address;
  }

  if (deleteAttributes) {
    deleteServices();
  }

  int rc = 0;
  m_config.asyncConnect = asyncConnect;
  m_config.exchangeMTU = exchangeMTU;

  do {
#if CONFIG_BT_NIMBLE_EXT_ADV
    rc = ble_gap_ext_connect(
      BLEDevice::m_ownAddrType, peerAddr, m_connectTimeout, m_phyMask, &m_connParams, &m_connParams, &m_connParams, BLEClient::handleGapEvent, this
    );

#else
    rc = ble_gap_connect(BLEDevice::m_ownAddrType, peerAddr, m_connectTimeout, &m_connParams, BLEClient::handleGapEvent, this);
#endif
    switch (rc) {
      case 0: break;

      case BLE_HS_EBUSY:
        // Scan was active, stop it through the BLEScan API to release any tasks and call the callback.
        if (!BLEDevice::getScan()->stop()) {
          rc = BLE_HS_EUNKNOWN;
        }
        break;

      case BLE_HS_EDONE:
        // A connection to this device already exists, do not connect twice.
        log_e(LOG_TAG, "Already connected to device; addr=%s", std::string(m_peerAddress).c_str());
        break;

      case BLE_HS_EALREADY: log_e(LOG_TAG, "Already attempting to connect"); break;

      default: log_e(LOG_TAG, "Failed to connect to %s, rc=%d; %s", std::string(m_peerAddress).c_str(), rc, BLEUtils::returnCodeToString(rc)); break;
    }

  } while (rc == BLE_HS_EBUSY);

  if (rc != 0) {
    m_lastErr = rc;
    return false;
  }

  if (m_config.asyncConnect) {
    return true;
  }

  BLETaskData taskData(this);
  m_pTaskData = &taskData;

  // Wait for the connect timeout time +1 second for the connection to complete
  if (!BLEUtils::taskWait(taskData, m_connectTimeout + 1000)) {
    // If a connection was made but no response from MTU exchange proceed anyway
    if (isConnected()) {
      taskData.m_flags = 0;
    } else {
      // workaround; if the controller doesn't cancel the connection at the timeout, cancel it here.
      log_e(LOG_TAG, "Connect timeout - cancelling");
      ble_gap_conn_cancel();
      taskData.m_flags = BLE_HS_ETIMEOUT;
    }
  }

  m_pTaskData = nullptr;
  rc = taskData.m_flags;
  if (rc != 0) {
    log_e(LOG_TAG, "Connection failed; status=%d %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    if (m_config.deleteOnConnectFail) {
      BLEDevice::deleteClient(this);
    }
    return false;
  }

  m_pClientCallbacks->onConnect(this);
  log_d(LOG_TAG, "<< connect()");
  // Check if still connected before returning
  return isConnected();
}  // connect

/**
 * @brief Initiate a secure connection (pair/bond) with the server.\n
 * Called automatically when a characteristic or descriptor requires encryption or authentication to access it.
 * @param [in] async If true, the connection will be secured asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is secured or the client disconnects.
 * @return True on success.
 * @details If async=false, this function will block and should not be used in a callback.
 */
bool BLEClient::secureConnection(bool async) const {
  log_d(LOG_TAG, ">> secureConnection()");

  int rc = 0;
  if (async && !BLEDevice::startSecurity(m_connHandle, &rc)) {
    m_lastErr = rc;
    m_asyncSecureAttempt = 0;
    return false;
  }

  if (async) {
    m_asyncSecureAttempt++;
    return true;
  }

  BLETaskData taskData(const_cast<BLEClient *>(this), BLE_HS_ENOTCONN);
  m_pTaskData = &taskData;
  int retryCount = 1;
  do {
    if (BLEDevice::startSecurity(m_connHandle)) {
      BLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
    }
  } while (taskData.m_flags == (BLE_HS_ERR_HCI_BASE + BLE_ERR_PINKEY_MISSING) && retryCount--);

  m_pTaskData = nullptr;

  if (taskData.m_flags == 0) {
    log_d(LOG_TAG, "<< secureConnection: success");
    return true;
  }

  m_lastErr = taskData.m_flags;
  log_e(LOG_TAG, "secureConnection: failed rc=%d", taskData.m_flags);
  return false;

}  // secureConnection

/**
 * @brief Disconnect from the peer.
 * @return True if the command was successfully sent.
 */
bool BLEClient::disconnect(uint8_t reason) {
  int rc = ble_gap_terminate(m_connHandle, reason);
  if (rc != 0 && rc != BLE_HS_ENOTCONN && rc != BLE_HS_EALREADY) {
    log_e(LOG_TAG, "ble_gap_terminate failed: rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return false;
  }

  return true;
}  // disconnect

/**
 * @brief Cancel an ongoing connection attempt.
 * @return True if the command was successfully sent.
 */
bool BLEClient::cancelConnect() const {
  int rc = ble_gap_conn_cancel();
  if (rc != 0 && rc != BLE_HS_EALREADY) {
    log_e(LOG_TAG, "ble_gap_conn_cancel failed: rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return false;
  }

  return true;
}  // cancelConnect

/**
 * @brief Set or unset a flag to delete this client when disconnected or connection failed.
 * @param [in] deleteOnDisconnect Set the client to self delete when disconnected.
 * @param [in] deleteOnConnectFail Set the client to self delete when a connection attempt fails.
 */
void BLEClient::setSelfDelete(bool deleteOnDisconnect, bool deleteOnConnectFail) {
  m_config.deleteOnDisconnect = deleteOnDisconnect;
  m_config.deleteOnConnectFail = deleteOnConnectFail;
}  // setSelfDelete

/**
 * @brief Get a copy of the clients configuration.
 * @return A copy of the clients configuration.
 */
BLEClient::Config BLEClient::getConfig() const {
  return m_config;
}  // getConfig

/**
 * @brief Set the client configuration options.
 * @param [in] config The config options instance to set the client configuration to.
 */
void BLEClient::setConfig(BLEClient::Config config) {
  m_config = config;
}  // setConfig

#if CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Set the PHY types to use when connecting to a server.
 * @param [in] mask A bitmask indicating what PHYS to connect with.\n
 * The available bits are:
 * * 0x01 BLE_GAP_LE_PHY_1M_MASK
 * * 0x02 BLE_GAP_LE_PHY_2M_MASK
 * * 0x04 BLE_GAP_LE_PHY_CODED_MASK
 */
void BLEClient::setConnectPhy(uint8_t mask) {
  m_phyMask = mask;
}  // setConnectPhy

/**
 * @brief Request a change to the PHY used for this peer connection.
 * @param [in] txPhyMask TX PHY. Can be mask of following:
 * - BLE_GAP_LE_PHY_1M_MASK
 * - BLE_GAP_LE_PHY_2M_MASK
 * - BLE_GAP_LE_PHY_CODED_MASK
 * - BLE_GAP_LE_PHY_ANY_MASK
 * @param [in] rxPhyMask RX PHY. Can be mask of following:
 * - BLE_GAP_LE_PHY_1M_MASK
 * - BLE_GAP_LE_PHY_2M_MASK
 * - BLE_GAP_LE_PHY_CODED_MASK
 * - BLE_GAP_LE_PHY_ANY_MASK
 * @param phyOptions Additional PHY options. Valid values are:
 * - BLE_GAP_LE_PHY_CODED_ANY (default)
 * - BLE_GAP_LE_PHY_CODED_S2
 * - BLE_GAP_LE_PHY_CODED_S8
 * @return True if successful.
 */
bool BLEClient::updatePhy(uint8_t txPhyMask, uint8_t rxPhyMask, uint16_t phyOptions) {
  int rc = ble_gap_set_prefered_le_phy(m_connHandle, txPhyMask, rxPhyMask, phyOptions);
  if (rc != 0) {
    log_e(LOG_TAG, "Failed to update phy; rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
  }

  return rc == 0;
}  // updatePhy

/**
 * @brief Get the PHY used for this peer connection.
 * @param [out] txPhy The TX PHY.
 * @param [out] rxPhy The RX PHY.
 * @return True if successful.
 */
bool BLEClient::getPhy(uint8_t *txPhy, uint8_t *rxPhy) {
  int rc = ble_gap_read_le_phy(m_connHandle, txPhy, rxPhy);
  if (rc != 0) {
    log_e(LOG_TAG, "Failed to read phy; rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
  }

  return rc == 0;
}  // getPhy
#endif

/**
 * @brief Set the connection parameters to use when connecting to a server.
 * @param [in] minInterval The minimum connection interval in 1.25ms units.
 * @param [in] maxInterval The maximum connection interval in 1.25ms units.
 * @param [in] latency The number of packets allowed to skip (extends max interval).
 * @param [in] timeout The timeout time in 10ms units before disconnecting.
 * @param [in] scanInterval The scan interval to use when attempting to connect in 0.625ms units.
 * @param [in] scanWindow The scan window to use when attempting to connect in 0.625ms units.
 */
void BLEClient::setConnectionParams(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout, uint16_t scanInterval, uint16_t scanWindow)
/*, uint16_t minConnEvtTime, uint16_t maxConnEvtTime)*/
{
  m_connParams.itvl_min = minInterval;
  m_connParams.itvl_max = maxInterval;
  m_connParams.latency = latency;
  m_connParams.supervision_timeout = timeout;
  m_connParams.scan_itvl = scanInterval;
  m_connParams.scan_window = scanWindow;

  // These are not used by BLE at this time - Must leave at defaults
  // m_connParams.min_ce_len = minConnEvtTime;     // Minimum length of connection event in 0.625ms units
  // m_connParams.max_ce_len = maxConnEvtTime;     // Maximum length of connection event in 0.625ms units
}  // setConnectionParams

/**
 * @brief Update the connection parameters:
 * * Can only be used after a connection has been established.
 * @param [in] minInterval The minimum connection interval in 1.25ms units.
 * @param [in] maxInterval The maximum connection interval in 1.25ms units.
 * @param [in] latency The number of packets allowed to skip (extends max interval).
 * @param [in] timeout The timeout time in 10ms units before disconnecting.
 */
bool BLEClient::updateConnParams(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
  ble_gap_upd_params params{
    .itvl_min = minInterval,
    .itvl_max = maxInterval,
    .latency = latency,
    .supervision_timeout = timeout,
    // These are not used by BLE at this time - leave at defaults
    .min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
    .max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN
  };

  int rc = ble_gap_update_params(m_connHandle, &params);
  if (rc != 0) {
    log_e(LOG_TAG, "Update params error: %d, %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
  }

  return rc == 0;
}  // updateConnParams

/**
 * @brief Request an update of the data packet length.
 * * Can only be used after a connection has been established.
 * @details Sends a data length update request to the server the client is connected to.
 * The Data Length Extension (DLE) allows to increase the Data Channel Payload from 27 bytes to up to 251 bytes.
 * The server needs to support the Bluetooth 4.2 specifications, to be capable of DLE.
 * @param [in] txOctets The preferred number of payload octets to use (Range 0x001B-0x00FB).
 */
bool BLEClient::setDataLen(uint16_t txOctets) {
  uint16_t txTime = (txOctets + 14) * 8;
  int rc = ble_gap_set_data_len(m_connHandle, txOctets, txTime);
  if (rc != 0) {
    log_e(LOG_TAG, "Set data length error: %d, %s", rc, BLEUtils::returnCodeToString(rc));
  }

  return rc == 0;
}  // setDataLen

/**
 * @brief Get detailed information about the current peer connection.
 * @return A BLEConnInfo instance with the data, or a NULL instance if not found.
 */
BLEConnInfo BLEClient::getConnInfo() const {
  BLEConnInfo connInfo{};
  if (ble_gap_conn_find(m_connHandle, &connInfo.m_desc) != 0) {
    log_e(LOG_TAG, "Connection info not found");
  }

  return connInfo;
}  // getConnInfo

/**
 * @brief Set the timeout to wait for connection attempt to complete.
 * @param [in] time The number of milliseconds before timeout, default is 30 seconds.
 */
void BLEClient::setConnectTimeout(uint32_t time) {
  m_connectTimeout = time;
}  // setConnectTimeout

/**
 * @brief Get the connection handle for this client.
 * @return The connection handle.
 */
uint16_t BLEClient::getConnHandle() const {
  return m_connHandle;
}  // getConnHandle

/**
 * @brief Retrieve the address of the peer.
 * @return A BLEAddress instance with the peer address data.
 */
BLEAddress BLEClient::getPeerAddress() const {
  return m_peerAddress;
}  // getPeerAddress

/**
 * @brief Set the peer address.
 * @param [in] address The address of the peer that this client is connected or should connect to.
 * @return True if successful.
 */
bool BLEClient::setPeerAddress(const BLEAddress &address) {
  if (isConnected()) {
    log_e(LOG_TAG, "Cannot set peer address while connected");
    return false;
  }

  m_peerAddress = address;
  return true;
}  // setPeerAddress

/**
 * @brief Ask the BLE server for the RSSI value.
 * @return The RSSI value or 0 if there was an error.
 */
int BLEClient::getRssi() const {
  if (!isConnected()) {
    log_e(LOG_TAG, "getRssi(): Not connected");
    return 0;
  }

  int8_t rssi = 0;
  int rc = ble_gap_conn_rssi(m_connHandle, &rssi);
  if (rc != 0) {
    log_e(LOG_TAG, "Failed to read RSSI error code: %d, %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return 0;
  }

  return rssi;
}  // getRssi

/**
 * @brief Get iterator to the beginning of the vector of remote service pointers.
 * @return An iterator to the beginning of the vector of remote service pointers.
 */
std::vector<BLERemoteService *>::iterator BLEClient::begin() {
  return m_svcVec.begin();
}  // begin

/**
 * @brief Get iterator to the end of the vector of remote service pointers.
 * @return An iterator to the end of the vector of remote service pointers.
 */
std::vector<BLERemoteService *>::iterator BLEClient::end() {
  return m_svcVec.end();
}  // end

/**
 * @brief Get the service BLE Remote Service instance corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A pointer to the service or nullptr if not found.
 */
BLERemoteService *BLEClient::getService(const char *uuid) {
  return getService(BLEUUID(uuid));
}  // getService

/**
 * @brief Get the service object corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A pointer to the service or nullptr if not found.
 */
BLERemoteService *BLEClient::getService(const BLEUUID &uuid) {
  log_d(LOG_TAG, ">> getService: uuid: %s", uuid.toString().c_str());

  for (auto &it : m_svcVec) {
    if (it->getUUID() == uuid) {
      log_d(LOG_TAG, "<< getService: found the service with uuid: %s", uuid.toString().c_str());
      return it;
    }
  }

  size_t prevSize = m_svcVec.size();
  if (retrieveServices(&uuid)) {
    if (m_svcVec.size() > prevSize) {
      return m_svcVec.back();
    }

    // If the request was successful but 16/32 bit uuid not found
    // try again with the 128 bit uuid.
    if (uuid.bitSize() == BLE_UUID_TYPE_16 || uuid.bitSize() == BLE_UUID_TYPE_32) {
      BLEUUID uuid128(uuid);
      uuid128.to128();
      if (retrieveServices(&uuid128)) {
        if (m_svcVec.size() > prevSize) {
          return m_svcVec.back();
        }
      }
    } else {
      // If the request was successful but the 128 bit uuid not found
      // try again with the 16 bit uuid.
      BLEUUID uuid16(uuid);
      uuid16.to16();
      // if the uuid was 128 bit but not of the BLE base type this check will fail
      if (uuid16.bitSize() == BLE_UUID_TYPE_16) {
        if (retrieveServices(&uuid16)) {
          if (m_svcVec.size() > prevSize) {
            return m_svcVec.back();
          }
        }
      }
    }
  }

  log_d(LOG_TAG, "<< getService: not found");
  return nullptr;
}  // getService

/**
 * @brief Get a pointer to the vector of found services.
 * @param [in] refresh If true the current services vector will be cleared and\n
 * all services will be retrieved from the peripheral.\n
 * If false the vector will be returned with the currently stored services.
 * @return A pointer to the vector of available services.
 */
const std::vector<BLERemoteService *> &BLEClient::getServices(bool refresh) {
  if (refresh) {
    deleteServices();
    if (!retrieveServices()) {
      log_e(LOG_TAG, "Error: Failed to get services");
    } else {
      log_i(LOG_TAG, "Found %d services", m_svcVec.size());
    }
  }

  return m_svcVec;
}  // getServices

/**
 * @brief Retrieves the full database of attributes that the peripheral has available.
 * @return True if successful.
 */
bool BLEClient::discoverAttributes() {
  deleteServices();
  if (!retrieveServices()) {
    return false;
  }

  for (auto svc : m_svcVec) {
    if (!svc->retrieveCharacteristics()) {
      return false;
    }

    for (auto chr : svc->m_vChars) {
      if (!chr->retrieveDescriptors()) {
        return false;
      }
    }
  }

  return true;
}  // discoverAttributes

/**
 * @brief Ask the remote BLE server for its services.
 * * Here we ask the server for its set of services and wait until we have received them all.
 * @return true on success otherwise false if an error occurred
 */
bool BLEClient::retrieveServices(const BLEUUID *uuidFilter) {
  if (!isConnected()) {
    log_e(LOG_TAG, "Disconnected, could not retrieve services -aborting");
    return false;
  }

  int rc = 0;
  BLETaskData taskData(this);

  if (uuidFilter == nullptr) {
    rc = ble_gattc_disc_all_svcs(m_connHandle, BLEClient::serviceDiscoveredCB, &taskData);
  } else {
    rc = ble_gattc_disc_svc_by_uuid(m_connHandle, uuidFilter->getBase(), BLEClient::serviceDiscoveredCB, &taskData);
  }

  if (rc != 0) {
    log_e(LOG_TAG, "ble_gattc_disc_all_svcs: rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return false;
  }

  BLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
  rc = taskData.m_flags;
  if (rc == 0 || rc == BLE_HS_EDONE) {
    return true;
  }

  m_lastErr = rc;
  log_e(LOG_TAG, "Could not retrieve services, rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
  return false;
}  // getServices

/**
 * @brief Callback for the service discovery API function.
 * @details When a service is found or there is none left or there was an error
 * the API will call this and report findings.
 */
int BLEClient::serviceDiscoveredCB(uint16_t connHandle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg) {
  log_d(LOG_TAG, "Service Discovered >> status: %d handle: %d", error->status, (error->status == 0) ? service->start_handle : -1);

  BLETaskData *pTaskData = (BLETaskData *)arg;
  BLEClient *pClient = (BLEClient *)pTaskData->m_pInstance;

  if (error->status == BLE_HS_ENOTCONN) {
    log_e(LOG_TAG, "<< Service Discovered; Disconnected");
    BLEUtils::taskRelease(*pTaskData, error->status);
    return error->status;
  }

  // Make sure the service discovery is for this device
  if (pClient->getConnHandle() != connHandle) {
    return 0;
  }

  if (error->status == 0) {
    // Found a service - add it to the vector
    pClient->m_svcVec.push_back(new BLERemoteService(pClient, service));
    return 0;
  }

  BLEUtils::taskRelease(*pTaskData, error->status);
  log_d(LOG_TAG, "<< Service Discovered");
  return error->status;
}  // serviceDiscoveredCB

/**
 * @brief Get the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to read.
 * @returns characteristic value or an empty value if not found.
 */
BLEAttValue BLEClient::getValue(const BLEUUID &serviceUUID, const BLEUUID &characteristicUUID) {
  log_d(LOG_TAG, ">> getValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());

  BLEAttValue ret{};
  auto pService = getService(serviceUUID);
  if (pService != nullptr) {
    auto pChar = pService->getCharacteristic(characteristicUUID);
    if (pChar != nullptr) {
      ret = pChar->readValue();
    }
  }

  log_d(LOG_TAG, "<< getValue");
  return ret;
}  // getValue

/**
 * @brief Set the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to write.
 * @param [in] value The value to write to the characteristic.
 * @param [in] response If true, uses write with response operation.
 * @returns true if successful otherwise false
 */
bool BLEClient::setValue(const BLEUUID &serviceUUID, const BLEUUID &characteristicUUID, const BLEAttValue &value, bool response) {
  log_d(LOG_TAG, ">> setValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());

  bool ret = false;
  auto pService = getService(serviceUUID);
  if (pService != nullptr) {
    BLERemoteCharacteristic *pChar = pService->getCharacteristic(characteristicUUID);
    if (pChar != nullptr) {
      ret = pChar->writeValue(value, response);
    }
  }

  log_d(LOG_TAG, "<< setValue");
  return ret;
}  // setValue

/**
 * @brief Get the remote characteristic with the specified handle.
 * @param [in] handle The handle of the desired characteristic.
 * @returns The matching remote characteristic, nullptr otherwise.
 */
BLERemoteCharacteristic *BLEClient::getCharacteristic(uint16_t handle) {
  for (const auto &svc : m_svcVec) {
    if (svc->getStartHandle() <= handle && handle <= svc->getEndHandle()) {
      for (const auto &chr : svc->m_vChars) {
        if (chr->getHandle() == handle) {
          return chr;
        }
      }
    }
  }

  return nullptr;
}  // getCharacteristic

/**
 * @brief Get the current mtu of this connection.
 * @returns The MTU value.
 */
uint16_t BLEClient::getMTU() const {
  return ble_att_mtu(m_connHandle);
}  // getMTU

/**
 * @brief Callback for the MTU exchange API function.
 * @details When the MTU exchange is complete the API will call this and report the new MTU.
 */
int BLEClient::exchangeMTUCb(uint16_t conn_handle, const ble_gatt_error *error, uint16_t mtu, void *arg) {
  log_d(LOG_TAG, "exchangeMTUCb: status=%d, mtu=%d", error->status, mtu);

  BLEClient *pClient = (BLEClient *)arg;
  if (pClient->getConnHandle() != conn_handle) {
    return 0;
  }

  if (error->status != 0) {
    log_e(LOG_TAG, "exchangeMTUCb() rc=%d %s", error->status, BLEUtils::returnCodeToString(error->status));
    pClient->m_lastErr = error->status;
  }

  return 0;
}  // exchangeMTUCb

/**
 * @brief Begin the MTU exchange process with the server.
 * @returns true if the request was sent successfully.
 */
bool BLEClient::exchangeMTU() {
  int rc = ble_gattc_exchange_mtu(m_connHandle, BLEClient::exchangeMTUCb, this);
  if (rc != 0) {
    log_e(LOG_TAG, "MTU exchange error; rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return false;
  }

  return true;
}  // exchangeMTU

/**
 * @brief Handle a received GAP event.
 * @param [in] event The event structure sent by the BLE stack.
 * @param [in] arg A pointer to the client instance that registered for this callback.
 */
int BLEClient::handleGapEvent(struct ble_gap_event *event, void *arg) {
  BLEClient *pClient = (BLEClient *)arg;
  int rc = 0;
  BLETaskData *pTaskData = pClient->m_pTaskData;  // save a copy in case client is deleted

  log_d(LOG_TAG, ">> handleGapEvent %s", BLEUtils::gapEventToString(event->type));

  switch (event->type) {
    case BLE_GAP_EVENT_DISCONNECT:
    {
      // workaround for bug in BLE stack where disconnect event argument is not passed correctly
      pClient = BLEDevice::getClientByHandle(event->disconnect.conn.conn_handle);
      if (pClient == nullptr) {
        return 0;
      }

      rc = event->disconnect.reason;
      // If Host reset tell the device now before returning to prevent
      // any errors caused by calling host functions before re-syncing.
      switch (rc) {
        case BLE_HS_ECONTROLLER:
        case BLE_HS_ETIMEOUT_HCI:
        case BLE_HS_ENOTSYNCED:
        case BLE_HS_EOS:
          log_e(LOG_TAG, "Disconnect - host reset, rc=%d", rc);
          BLEDevice::onReset(rc);
          break;
        default: break;
      }

      log_d(LOG_TAG, "disconnect; reason=%d, %s", rc, BLEUtils::returnCodeToString(rc));

      pClient->m_terminateFailCount = 0;
      pClient->m_asyncSecureAttempt = 0;

      // Don't call the disconnect callback if we are waiting for a connection to complete and it fails
      if (rc != (BLE_HS_ERR_HCI_BASE + BLE_ERR_CONN_ESTABLISHMENT) || pClient->m_config.asyncConnect) {
        pClient->m_pClientCallbacks->onDisconnect(pClient, rc);
      }

      pClient->m_connHandle = BLE_HS_CONN_HANDLE_NONE;

      if (pClient->m_config.deleteOnDisconnect) {
        // If we are set to self delete on disconnect but we have a task waiting on the connection
        // completion we will set the flag to delete on connect fail instead of deleting here
        // to prevent segmentation faults or double deleting
        if (pTaskData != nullptr && rc == (BLE_HS_ERR_HCI_BASE + BLE_ERR_CONN_ESTABLISHMENT)) {
          pClient->m_config.deleteOnConnectFail = true;
          break;
        }
        BLEDevice::deleteClient(pClient);
      }

      break;
    }  // BLE_GAP_EVENT_DISCONNECT

    case BLE_GAP_EVENT_CONNECT:
    {
      // If we aren't waiting for this connection response we should drop the connection immediately.
      if (pClient->isConnected() || (!pClient->m_config.asyncConnect && pClient->m_pTaskData == nullptr)) {
        ble_gap_terminate(event->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
      }

      rc = event->connect.status;
      if (rc == 0) {
        pClient->m_connHandle = event->connect.conn_handle;

        if (pClient->m_config.asyncConnect) {
          pClient->m_pClientCallbacks->onConnect(pClient);
        }

        if (pClient->m_config.exchangeMTU) {
          if (!pClient->exchangeMTU()) {
            rc = pClient->m_lastErr;  // sets the error in the task data
            break;
          }

          return 0;  // return as we may have a task waiting for the MTU before releasing it.
        }
      } else {
        pClient->m_connHandle = BLE_HS_CONN_HANDLE_NONE;

        if (pClient->m_config.asyncConnect) {
          pClient->m_pClientCallbacks->onConnectFail(pClient, rc);
          if (pClient->m_config.deleteOnConnectFail) {
            BLEDevice::deleteClient(pClient);
          }
        }
      }

      break;
    }  // BLE_GAP_EVENT_CONNECT

    case BLE_GAP_EVENT_TERM_FAILURE:
    {
      if (pClient->m_connHandle != event->term_failure.conn_handle) {
        return 0;
      }

      log_e(LOG_TAG, "Connection termination failure; rc=%d - retrying", event->term_failure.status);
      if (++pClient->m_terminateFailCount > 2) {
        ble_hs_sched_reset(BLE_HS_ECONTROLLER);
      } else {
        ble_gap_terminate(event->term_failure.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      }
      return 0;
    }  // BLE_GAP_EVENT_TERM_FAILURE

    case BLE_GAP_EVENT_NOTIFY_RX:
    {
      if (pClient->m_connHandle != event->notify_rx.conn_handle) {
        return 0;
      }
      log_d(LOG_TAG, "Notify Received for handle: %d", event->notify_rx.attr_handle);

      for (const auto &svc : pClient->m_svcVec) {
        // Dont waste cycles searching services without this handle in its range
        if (svc->getEndHandle() < event->notify_rx.attr_handle) {
          continue;
        }

        log_d(LOG_TAG, "checking service %s for handle: %d", svc->getUUID().toString().c_str(), event->notify_rx.attr_handle);

        for (const auto &chr : svc->m_vChars) {
          if (chr->getHandle() == event->notify_rx.attr_handle) {
            log_d(LOG_TAG, "Got Notification for characteristic %s", chr->toString().c_str());

            uint32_t data_len = OS_MBUF_PKTLEN(event->notify_rx.om);
            chr->m_value.setValue(event->notify_rx.om->om_data, data_len);

            if (chr->m_notifyCallback != nullptr) {
              chr->m_notifyCallback(chr, event->notify_rx.om->om_data, data_len, !event->notify_rx.indication);
            }
            break;
          }
        }
      }

      return 0;
    }  // BLE_GAP_EVENT_NOTIFY_RX

    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
    {
      if (pClient->m_connHandle != event->conn_update_req.conn_handle) {
        return 0;
      }
      log_d(LOG_TAG, "Peer requesting to update connection parameters");
      log_d(
        LOG_TAG, "MinInterval: %d, MaxInterval: %d, Latency: %d, Timeout: %d", event->conn_update_req.peer_params->itvl_min,
        event->conn_update_req.peer_params->itvl_max, event->conn_update_req.peer_params->latency, event->conn_update_req.peer_params->supervision_timeout
      );

      rc = pClient->m_pClientCallbacks->onConnParamsUpdateRequest(pClient, event->conn_update_req.peer_params) ? 0 : BLE_ERR_CONN_PARMS;

      if (!rc && event->type == BLE_GAP_EVENT_CONN_UPDATE_REQ) {
        event->conn_update_req.self_params->itvl_min = pClient->m_connParams.itvl_min;
        event->conn_update_req.self_params->itvl_max = pClient->m_connParams.itvl_max;
        event->conn_update_req.self_params->latency = pClient->m_connParams.latency;
        event->conn_update_req.self_params->supervision_timeout = pClient->m_connParams.supervision_timeout;
      }

      log_d(LOG_TAG, "%s peer params", (rc == 0) ? "Accepted" : "Rejected");
      return rc;
    }  // BLE_GAP_EVENT_CONN_UPDATE_REQ, BLE_GAP_EVENT_L2CAP_UPDATE_REQ

    case BLE_GAP_EVENT_CONN_UPDATE:
    {
      if (pClient->m_connHandle != event->conn_update.conn_handle) {
        return 0;
      }
      if (event->conn_update.status == 0) {
        log_i(LOG_TAG, "Connection parameters updated.");
      } else {
        log_e(LOG_TAG, "Update connection parameters failed.");
      }
      return 0;
    }  // BLE_GAP_EVENT_CONN_UPDATE

    case BLE_GAP_EVENT_ENC_CHANGE:
    {
      if (pClient->m_connHandle != event->enc_change.conn_handle) {
        return 0;
      }

      if (event->enc_change.status == 0 || event->enc_change.status == (BLE_HS_ERR_HCI_BASE + BLE_ERR_PINKEY_MISSING)) {
        BLEConnInfo peerInfo;
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &peerInfo.m_desc);
        if (rc != 0) {
          rc = 0;
          break;
        }

        if (event->enc_change.status == (BLE_HS_ERR_HCI_BASE + BLE_ERR_PINKEY_MISSING)) {
          // Key is missing, try deleting.
          ble_store_util_delete_peer(&peerInfo.m_desc.peer_id_addr);
          // Attempt a retry if async secure failed.
          if (pClient->m_asyncSecureAttempt == 1) {
            pClient->secureConnection(true);
          }
        } else {
          pClient->m_asyncSecureAttempt = 0;
          pClient->m_pClientCallbacks->onAuthenticationComplete(peerInfo);
        }
      }

      rc = event->enc_change.status;
      break;
    }  // BLE_GAP_EVENT_ENC_CHANGE

    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
    {
      BLEConnInfo peerInfo;
      rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        rc = 0;
        break;
      }

      pClient->m_pClientCallbacks->onIdentity(peerInfo);
      break;
    }  // BLE_GAP_EVENT_IDENTITY_RESOLVED

#if CONFIG_BT_NIMBLE_EXT_ADV
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
    {
      BLEConnInfo peerInfo;
      rc = ble_gap_conn_find(event->phy_updated.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        return BLE_ATT_ERR_INVALID_HANDLE;
      }

      pClient->m_pClientCallbacks->onPhyUpdate(pClient, event->phy_updated.tx_phy, event->phy_updated.rx_phy);
      return 0;
    }  // BLE_GAP_EVENT_PHY_UPDATE_COMPLETE
#endif

    case BLE_GAP_EVENT_MTU:
    {
      if (pClient->m_connHandle != event->mtu.conn_handle) {
        return 0;
      }

      log_i(LOG_TAG, "mtu update: mtu=%d", event->mtu.value);
      pClient->m_pClientCallbacks->onMTUChange(pClient, event->mtu.value);
      rc = 0;
      break;
    }  // BLE_GAP_EVENT_MTU

    case BLE_GAP_EVENT_PASSKEY_ACTION:
    {
      if (pClient->m_connHandle != event->passkey.conn_handle) {
        return 0;
      }

      BLEConnInfo peerInfo;
      rc = ble_gap_conn_find(event->passkey.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        rc = 0;
        break;
      }

      if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
        log_d(LOG_TAG, "Passkey on device's display: %" PRIu32, event->passkey.params.numcmp);
        pClient->m_pClientCallbacks->onConfirmPasskey(peerInfo, event->passkey.params.numcmp);
      } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
        log_d(LOG_TAG, "OOB request received");
        // TODO: Handle out of band pairing
        // struct ble_sm_io pkey;
        // pkey.action = BLE_SM_IOACT_OOB;
        // pClient->onOobPairingRequest(pkey.oob);
        // rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
        // log_d(LOG_TAG, "ble_sm_inject_io result: %d", rc);
      } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
        log_d(LOG_TAG, "Enter the passkey");
        pClient->m_pClientCallbacks->onPassKeyEntry(peerInfo);
      } else if (event->passkey.params.action == BLE_SM_IOACT_NONE) {
        log_d(LOG_TAG, "No passkey action required");
      }

      return 0;
    }  // BLE_GAP_EVENT_PASSKEY_ACTION

    default:
    {
      return 0;
    }
  }  // Switch

  if (pTaskData != nullptr) {
    BLEUtils::taskRelease(*pTaskData, rc);
  }

  log_d(LOG_TAG, "<< handleGapEvent");
  return 0;
}  // handleGapEvent

/**
 * @brief Are we connected to a server?
 * @return True if we are connected and false if we are not connected.
 */
bool BLEClient::isConnected() const {
  return m_connHandle != BLE_HS_CONN_HANDLE_NONE;
}  // isConnected

/**
 * @brief Set the callbacks that will be invoked when events are received.
 * @param [in] pClientCallbacks A pointer to a class to receive the event callbacks.
 * @param [in] deleteCallbacks If true this will delete the callback class sent when the client is destructed.
 */
void BLEClient::setClientCallbacks(BLEClientCallbacks *pClientCallbacks, bool deleteCallbacks) {
  if (pClientCallbacks != nullptr) {
    m_pClientCallbacks = pClientCallbacks;
    m_config.deleteCallbacks = deleteCallbacks;
  } else {
    m_pClientCallbacks = &defaultCallbacks;
    m_config.deleteCallbacks = false;
  }
}  // setClientCallbacks

/**
 * @brief Return a string representation of this client.
 * @return A string representation of this client.
 */
std::string BLEClient::toString() const {
  std::string res = "peer address: " + m_peerAddress.toString();
  res += "\nServices:\n";

  for (const auto &it : m_svcVec) {
    res += it->toString() + "\n";
  }

  return res;
}  // toString

static const char *CB_TAG = "BLEClientCallbacks";

/**
 * @brief Get the last error code reported by the BLE host
 * @return int, the BLE error code.
 */
int BLEClient::getLastError() const {
  return m_lastErr;
}  // getLastError

void BLEClientCallbacks::onConnect(BLEClient *pClient) {
  log_d(CB_TAG, "onConnect: default");
}  // onConnect

void BLEClientCallbacks::onConnectFail(BLEClient *pClient, int reason) {
  log_d(CB_TAG, "onConnectFail: default, reason: %d", reason);
}  // onConnectFail

void BLEClientCallbacks::onDisconnect(BLEClient *pClient, int reason) {
  log_d(CB_TAG, "onDisconnect: default, reason: %d", reason);
}  // onDisconnect

bool BLEClientCallbacks::onConnParamsUpdateRequest(BLEClient *pClient, const ble_gap_upd_params *params) {
  log_d(CB_TAG, "onConnParamsUpdateRequest: default");
  return true;
}  // onConnParamsUpdateRequest

void BLEClientCallbacks::onPassKeyEntry(BLEConnInfo &connInfo) {
  log_d(CB_TAG, "onPassKeyEntry: default: 123456");
  BLEDevice::injectPassKey(connInfo, 123456);
}  // onPassKeyEntry

void BLEClientCallbacks::onAuthenticationComplete(BLEConnInfo &connInfo) {
  log_d(CB_TAG, "onAuthenticationComplete: default");
}  // onAuthenticationComplete

void BLEClientCallbacks::onIdentity(BLEConnInfo &connInfo) {
  log_d(CB_TAG, "onIdentity: default");
}  // onIdentity

void BLEClientCallbacks::onConfirmPasskey(BLEConnInfo &connInfo, uint32_t pin) {
  log_d(CB_TAG, "onConfirmPasskey: default: true");
  BLEDevice::injectConfirmPasskey(connInfo, true);
}  // onConfirmPasskey

void BLEClientCallbacks::onMTUChange(BLEClient *pClient, uint16_t mtu) {
  log_d(CB_TAG, "onMTUChange: default");
}  // onMTUChange

#if CONFIG_BT_NIMBLE_EXT_ADV
void BLEClientCallbacks::onPhyUpdate(BLEClient *pClient, uint8_t txPhy, uint8_t rxPhy) {
  log_d(CB_TAG, "onPhyUpdate: default, txPhy: %d, rxPhy: %d", txPhy, rxPhy);
}  // onPhyUpdate
#endif

#endif /* CONFIG_NIMBLE_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
