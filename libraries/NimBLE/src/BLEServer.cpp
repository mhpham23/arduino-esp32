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

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BLEServer.h"
#include "BLEDevice.h"

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#include "BLEClient.h"
#endif

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define NIMBLE_SERVER_GET_PEER_NAME_ON_CONNECT_CB 0
#define NIMBLE_SERVER_GET_PEER_NAME_ON_AUTH_CB    1

static const char *LOG_TAG = "BLEServer";
static BLEServerCallbacks defaultCallbacks;

/**
 * @brief Construct a BLE Server
 *
 * This class is not designed to be individually instantiated.
 * Instead it should be created the BLEDevice API.
 */
BLEServer::BLEServer()
  : m_gattsStarted{false}, m_svcChanged{false}, m_deleteCallbacks{false},
#if !CONFIG_BT_NIMBLE_EXT_ADV
    m_advertiseOnDisconnect{false},
#endif
    m_pServerCallbacks{&defaultCallbacks}, m_svcVec{} {
  m_connectedPeers.fill(BLE_HS_CONN_HANDLE_NONE);
}  // BLEServer

/**
 * @brief Destructor: frees all resources / attributes created.
 */
BLEServer::~BLEServer() {
  for (const auto &svc : m_svcVec) {
    delete svc;
  }

  if (m_deleteCallbacks) {
    delete m_pServerCallbacks;
  }

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
  if (m_pClient != nullptr) {
    delete m_pClient;
  }
#endif
}

/**
 * @brief Create a BLE Service.
 * @param [in] uuid The UUID of the new service.
 * @return A pointer to the new service object.
 */
BLEService *BLEServer::createService(const char *uuid) {
  return createService(BLEUUID(uuid));
}  // createService

/**
 * @brief Create a BLE Service.
 * @param [in] uuid The UUID of the new service.
 * @return A pointer to the new service object.
 */
BLEService *BLEServer::createService(const BLEUUID &uuid) {
  BLEService *pService = new BLEService(uuid);
  m_svcVec.push_back(pService);
  serviceChanged();

  return pService;
}  // createService

/**
 * @brief Get a BLE Service by its UUID
 * @param [in] uuid The UUID of the service.
 * @param instanceId The index of the service to return (used when multiple services have the same UUID).
 * @return A pointer to the service object or nullptr if not found.
 */
BLEService *BLEServer::getServiceByUUID(const char *uuid, uint16_t instanceId) const {
  return getServiceByUUID(BLEUUID(uuid), instanceId);
}  // getServiceByUUID

/**
 * @brief Get a BLE Service by its UUID
 * @param [in] uuid The UUID of the service.
 * @param instanceId The index of the service to return (used when multiple services have the same UUID).
 * @return A pointer to the service object or nullptr if not found.
 */
BLEService *BLEServer::getServiceByUUID(const BLEUUID &uuid, uint16_t instanceId) const {
  uint16_t position = 0;
  for (const auto &svc : m_svcVec) {
    if (svc->getUUID() == uuid) {
      if (position == instanceId) {
        return svc;
      }
      position++;
    }
  }

  return nullptr;
}  // getServiceByUUID

/**
 * @brief Get a BLE Service by its handle
 * @param handle The handle of the service.
 * @return A pointer to the service object or nullptr if not found.
 */
BLEService *BLEServer::getServiceByHandle(uint16_t handle) const {
  for (const auto &svc : m_svcVec) {
    if (svc->getHandle() == handle) {
      return svc;
    }
  }

  return nullptr;
}

#if CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Retrieve the advertising object that can be used to advertise the existence of the server.
 * @return A pinter to an advertising object.
 */
BLEExtAdvertising *BLEServer::getAdvertising() const {
  return BLEDevice::getAdvertising();
}  // getAdvertising
#endif

#if !CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Retrieve the advertising object that can be used to advertise the existence of the server.
 * @return A pointer to an advertising object.
 */
BLEAdvertising *BLEServer::getAdvertising() const {
  return BLEDevice::getAdvertising();
}  // getAdvertising
#endif

/**
 * @brief Called when the services are added/removed and sets a flag to indicate they should be reloaded.
 * @details This has no effect if the GATT server was not already started.
 */
void BLEServer::serviceChanged() {
  if (m_gattsStarted) {
    m_svcChanged = true;
  }
}  // serviceChanged

/**
 * @brief Start the GATT server.
 * @details Required to be called after setup of all services and characteristics / descriptors
 * for the BLE host to register them.
 */
void BLEServer::start() {
  if (m_gattsStarted) {
    return;  // already started
  }

  int rc = ble_gatts_start();
  if (rc != 0) {
    log_e(LOG_TAG, "ble_gatts_start; rc=%d, %s", rc, BLEUtils::returnCodeToString(rc));
    return;
  }

#if CONFIG_NIMBLE_CPP_LOG_LEVEL >= 4
  ble_gatts_show_local();
#endif

  // Get the assigned service handles and build a vector of characteristics
  // with Notify / Indicate capabilities for event handling
  for (const auto &svc : m_svcVec) {
    if (svc->getRemoved() == 0) {
      rc = ble_gatts_find_svc(svc->getUUID().getBase(), &svc->m_handle);
      if (rc != 0) {
        log_w(
          LOG_TAG, "GATT Server started without service: %s, Service %s", svc->getUUID().toString().c_str(), svc->isStarted() ? "missing" : "not started"
        );
        continue;  // Skip this service as it was not started
      }
    }

    // Set the descriptor handles now as the stack does not set these when the service is started
    for (const auto &chr : svc->m_vChars) {
      for (auto &desc : chr->m_vDescriptors) {
        ble_gatts_find_dsc(svc->getUUID().getBase(), chr->getUUID().getBase(), desc->getUUID().getBase(), &desc->m_handle);
      }
    }
  }

  // If the services have changed indicate it now
  if (m_svcChanged) {
    m_svcChanged = false;
    ble_svc_gatt_changed(0x0001, 0xffff);
  }

  m_gattsStarted = true;
}  // start

/**
 * @brief Disconnect the specified client with optional reason.
 * @param [in] connHandle Connection handle of the client to disconnect.
 * @param [in] reason code for disconnecting.
 * @return True if successful.
 */
bool BLEServer::disconnect(uint16_t connHandle, uint8_t reason) const {
  int rc = ble_gap_terminate(connHandle, reason);
  if (rc != 0 && rc != BLE_HS_ENOTCONN && rc != BLE_HS_EALREADY) {
    log_e(LOG_TAG, "ble_gap_terminate failed: rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
    return false;
  }

  return true;
}  // disconnect

/**
 * @brief Disconnect the specified client with optional reason.
 * @param [in] connInfo Connection of the client to disconnect.
 * @param [in] reason code for disconnecting.
 * @return BLE host return code.
 */
bool BLEServer::disconnect(const BLEConnInfo &connInfo, uint8_t reason) const {
  return disconnect(connInfo.getConnHandle(), reason);
}  // disconnect

#if !CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Set the server to automatically start advertising when a client disconnects.
 * @param [in] enable true == advertise, false == don't advertise.
 */
void BLEServer::advertiseOnDisconnect(bool enable) {
  m_advertiseOnDisconnect = enable;
}  // advertiseOnDisconnect
#endif

/**
 * @brief Return the number of connected clients.
 * @return The number of connected clients.
 */
uint8_t BLEServer::getConnectedCount() const {
  size_t count = 0;
  for (const auto &peer : m_connectedPeers) {
    if (peer != BLE_HS_CONN_HANDLE_NONE) {
      count++;
    }
  }

  return count;
}  // getConnectedCount

/**
 * @brief Get a vector of the connected client handles.
 * @return A vector of the connected client handles.
 */
std::vector<uint16_t> BLEServer::getPeerDevices() const {
  std::vector<uint16_t> peers{};
  for (const auto &peer : m_connectedPeers) {
    if (peer != BLE_HS_CONN_HANDLE_NONE) {
      peers.push_back(peer);
    }
  }

  return peers;
}  // getPeerDevices

/**
 * @brief Get the connection information of a connected peer by vector index.
 * @param [in] index The vector index of the peer.
 * @return A BLEConnInfo instance with the peer connection information, or an empty instance if not found.
 */
BLEConnInfo BLEServer::getPeerInfo(uint8_t index) const {
  if (index >= m_connectedPeers.size()) {
    log_e(LOG_TAG, "Invalid index %u", index);
    return BLEConnInfo{};
  }

  auto count = 0;
  for (const auto &peer : m_connectedPeers) {
    if (peer != BLE_HS_CONN_HANDLE_NONE) {
      if (count == index) {
        return getPeerInfoByHandle(m_connectedPeers[count]);
      }
      count++;
    }
  }

  return BLEConnInfo{};
}  // getPeerInfo

/**
 * @brief Get the connection information of a connected peer by address.
 * @param [in] address The address of the peer.
 * @return A BLEConnInfo instance with the peer connection information, or an empty instance if not found.
 */
BLEConnInfo BLEServer::getPeerInfo(const BLEAddress &address) const {
  BLEConnInfo peerInfo{};
  if (ble_gap_conn_find_by_addr(address.getBase(), &peerInfo.m_desc) != 0) {
    log_e(LOG_TAG, "Peer info not found");
  }

  return peerInfo;
}  // getPeerInfo

/**
 * @brief Get the connection information of a connected peer by connection handle.
 * @param [in] connHandle The connection handle of the peer.
 * @return A BLEConnInfo instance with the peer connection information, or an empty instance if not found.
 */
BLEConnInfo BLEServer::getPeerInfoByHandle(uint16_t connHandle) const {
  BLEConnInfo peerInfo{};
  if (ble_gap_conn_find(connHandle, &peerInfo.m_desc) != 0) {
    log_e(LOG_TAG, "Peer info not found");
  }

  return peerInfo;
}  // getPeerIDInfo

/**
 * @brief Gap event handler.
 */
int BLEServer::handleGapEvent(ble_gap_event *event, void *arg) {
  log_d(LOG_TAG, ">> handleGapEvent: %s", BLEUtils::gapEventToString(event->type));

  int rc = 0;
  BLEConnInfo peerInfo{};
  BLEServer *pServer = BLEDevice::getServer();

  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
    {
      if (event->connect.status != 0) {
        log_e(LOG_TAG, "Connection failed");
#if !CONFIG_BT_NIMBLE_EXT_ADV
        BLEDevice::startAdvertising();
#endif
      } else {
        rc = ble_gap_conn_find(event->connect.conn_handle, &peerInfo.m_desc);
        if (rc != 0) {
          return 0;
        }

        for (auto &peer : pServer->m_connectedPeers) {
          if (peer == BLE_HS_CONN_HANDLE_NONE) {
            peer = event->connect.conn_handle;
            break;
          }
        }

        pServer->m_pServerCallbacks->onConnect(pServer, peerInfo);
      }

      break;
    }  // BLE_GAP_EVENT_CONNECT

    case BLE_GAP_EVENT_DISCONNECT:
    {
      // If Host reset tell the device now before returning to prevent
      // any errors caused by calling host functions before resync.
      switch (event->disconnect.reason) {
        case BLE_HS_ETIMEOUT_HCI:
        case BLE_HS_EOS:
        case BLE_HS_ECONTROLLER:
        case BLE_HS_ENOTSYNCED:
          log_e(LOG_TAG, "Disconnect - host reset, rc=%d", event->disconnect.reason);
          BLEDevice::onReset(event->disconnect.reason);
          break;
        default: break;
      }

      for (auto &peer : pServer->m_connectedPeers) {
        if (peer == event->disconnect.conn.conn_handle) {
          peer = BLE_HS_CONN_HANDLE_NONE;
          break;
        }
      }

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
      if (pServer->m_pClient && pServer->m_pClient->m_connHandle == event->disconnect.conn.conn_handle) {
        // If this was also the client make sure it's flagged as disconnected.
        pServer->m_pClient->m_connHandle = BLE_HS_CONN_HANDLE_NONE;
      }
#endif

      if (pServer->m_svcChanged) {
        pServer->resetGATT();
      }

      peerInfo.m_desc = event->disconnect.conn;
      pServer->m_pServerCallbacks->onDisconnect(pServer, peerInfo, event->disconnect.reason);
#if !CONFIG_BT_NIMBLE_EXT_ADV
      if (pServer->m_advertiseOnDisconnect) {
        pServer->startAdvertising();
      }
#endif
      break;
    }  // BLE_GAP_EVENT_DISCONNECT

    case BLE_GAP_EVENT_SUBSCRIBE:
    {
      log_i(
        LOG_TAG, "subscribe event; attr_handle=%d, subscribed: %s", event->subscribe.attr_handle,
        ((event->subscribe.cur_notify || event->subscribe.cur_indicate) ? "true" : "false")
      );

      for (const auto &svc : pServer->m_svcVec) {
        for (const auto &chr : svc->m_vChars) {
          if (chr->getHandle() == event->subscribe.attr_handle) {
            rc = ble_gap_conn_find(event->subscribe.conn_handle, &peerInfo.m_desc);
            if (rc != 0) {
              break;
            }

            auto chrProps = chr->getProperties();
            if (!peerInfo.isEncrypted()
                && (chrProps & BLE_GATT_CHR_F_READ_AUTHEN || chrProps & BLE_GATT_CHR_F_READ_AUTHOR || chrProps & BLE_GATT_CHR_F_READ_ENC)) {
              BLEDevice::startSecurity(event->subscribe.conn_handle);
            }

            chr->m_pCallbacks->onSubscribe(chr, peerInfo, event->subscribe.cur_notify + event->subscribe.cur_indicate);
          }
        }
      }

      break;
    }  // BLE_GAP_EVENT_SUBSCRIBE

    case BLE_GAP_EVENT_MTU:
    {
      log_i(LOG_TAG, "mtu update event; conn_handle=%d mtu=%d", event->mtu.conn_handle, event->mtu.value);
      if (ble_gap_conn_find(event->mtu.conn_handle, &peerInfo.m_desc) == 0) {
        pServer->m_pServerCallbacks->onMTUChange(event->mtu.value, peerInfo);
      }

      break;
    }  // BLE_GAP_EVENT_MTU

    case BLE_GAP_EVENT_NOTIFY_TX:
    {
      BLECharacteristic *pChar = nullptr;

      for (const auto &svc : pServer->m_svcVec) {
        for (auto &chr : svc->m_vChars) {
          if (chr->getHandle() == event->notify_tx.attr_handle) {
            pChar = chr;
          }
        }
      }

      if (pChar == nullptr) {
        return 0;
      }

      if (event->notify_tx.indication) {
        if (event->notify_tx.status == 0) {
          return 0;  // Indication sent but not yet acknowledged.
        }
      }

      pChar->m_pCallbacks->onStatus(pChar, event->notify_tx.status);
      break;
    }  // BLE_GAP_EVENT_NOTIFY_TX

    case BLE_GAP_EVENT_ADV_COMPLETE:
    {
#if CONFIG_BT_NIMBLE_EXT_ADV
      case BLE_GAP_EVENT_SCAN_REQ_RCVD: return BLEExtAdvertising::handleGapEvent(event, arg);
#else
      return BLEAdvertising::handleGapEvent(event, arg);
#endif
    }  // BLE_GAP_EVENT_ADV_COMPLETE | BLE_GAP_EVENT_SCAN_REQ_RCVD

    case BLE_GAP_EVENT_CONN_UPDATE:
    {
      if (ble_gap_conn_find(event->connect.conn_handle, &peerInfo.m_desc) == 0) {
        pServer->m_pServerCallbacks->onConnParamsUpdate(peerInfo);
      }

      break;
    }  // BLE_GAP_EVENT_CONN_UPDATE

    case BLE_GAP_EVENT_REPEAT_PAIRING:
    {
      /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */

      /* Delete the old bond. */
      rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        return BLE_GAP_REPEAT_PAIRING_IGNORE;
      }

      ble_store_util_delete_peer(&peerInfo.m_desc.peer_id_addr);

      /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
             * continue with the pairing operation.
             */
      return BLE_GAP_REPEAT_PAIRING_RETRY;
    }  // BLE_GAP_EVENT_REPEAT_PAIRING

    case BLE_GAP_EVENT_ENC_CHANGE:
    {
      rc = ble_gap_conn_find(event->enc_change.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        return BLE_ATT_ERR_INVALID_HANDLE;
      }

      pServer->m_pServerCallbacks->onAuthenticationComplete(peerInfo);
      break;
    }  // BLE_GAP_EVENT_ENC_CHANGE

    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
    {
      rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        return BLE_ATT_ERR_INVALID_HANDLE;
      }

      pServer->m_pServerCallbacks->onIdentity(peerInfo);
      break;
    }  // BLE_GAP_EVENT_IDENTITY_RESOLVED

#if CONFIG_BT_NIMBLE_EXT_ADV
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
    {
      rc = ble_gap_conn_find(event->phy_updated.conn_handle, &peerInfo.m_desc);
      if (rc != 0) {
        return BLE_ATT_ERR_INVALID_HANDLE;
      }

      pServer->m_pServerCallbacks->onPhyUpdate(peerInfo, event->phy_updated.tx_phy, event->phy_updated.rx_phy);
      return 0;
    }  // BLE_GAP_EVENT_PHY_UPDATE_COMPLETE
#endif

    case BLE_GAP_EVENT_PASSKEY_ACTION:
    {
      struct ble_sm_io pkey = {0, 0};

      if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
        pkey.action = event->passkey.params.action;
        // backward compatibility
        pkey.passkey = BLEDevice::getSecurityPasskey();  // This is the passkey to be entered on peer
        // if the (static)passkey is the default, check the callback for custom value
        // both values default to the same.
        if (pkey.passkey == 123456) {
          pkey.passkey = pServer->m_pServerCallbacks->onPassKeyDisplay();
        }
        rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
        log_d(LOG_TAG, "BLE_SM_IOACT_DISP; ble_sm_inject_io result: %d", rc);

      } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
        log_d(LOG_TAG, "Passkey on device's display: %" PRIu32, event->passkey.params.numcmp);

        rc = ble_gap_conn_find(event->passkey.conn_handle, &peerInfo.m_desc);
        if (rc != 0) {
          return BLE_ATT_ERR_INVALID_HANDLE;
        }

        pServer->m_pServerCallbacks->onConfirmPassKey(peerInfo, event->passkey.params.numcmp);
      } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
        // TODO: Handle out of band pairing
        // static uint8_t tem_oob[16] = {0};
        // pkey.action                = event->passkey.params.action;
        // for (int i = 0; i < 16; i++) {
        //     pkey.oob[i] = tem_oob[i];
        // }
        // rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
        // log_d(LOG_TAG, "BLE_SM_IOACT_OOB; ble_sm_inject_io result: %d", rc);
      } else if (event->passkey.params.action == BLE_SM_IOACT_NONE) {
        log_d(LOG_TAG, "No passkey action required");
      }

      break;
    }  // BLE_GAP_EVENT_PASSKEY_ACTION

    default: break;
  }

  log_d(LOG_TAG, "<< handleGapEvent");
  return 0;
}  // handleGapEvent

/**
 * @brief GATT event handler.
 */
int BLEServer::handleGattEvent(uint16_t connHandle, uint16_t attrHandle, ble_gatt_access_ctxt *ctxt, void *arg) {
  log_d(LOG_TAG, "Gatt %s event", (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR || ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) ? "Read" : "Write");
  auto pAtt = static_cast<BLELocalValueAttribute *>(arg);
  const BLEAttValue &val = pAtt->getAttVal();

  BLEConnInfo peerInfo{};
  ble_gap_conn_find(connHandle, &peerInfo.m_desc);

  switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_DSC:
    case BLE_GATT_ACCESS_OP_READ_CHR:
    {
      // Don't call readEvent if this is an internal read (handle is NONE)
      if (connHandle != BLE_HS_CONN_HANDLE_NONE) {
        // If the packet header is only 8 bytes then this is a follow up of a long read
        // so we don't want to call the onRead() callback again.
        if (ctxt->om->om_pkthdr_len > 8 || val.size() <= (ble_att_mtu(connHandle) - 3)) {
          pAtt->readEvent(peerInfo);
        }
      }

      ble_npl_hw_enter_critical();
      int rc = os_mbuf_append(ctxt->om, val.data(), val.size());
      ble_npl_hw_exit_critical(0);
      return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
    {
      uint16_t maxLen = val.max_size();
      if (ctxt->om->om_len > maxLen) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
      }

      uint8_t buf[maxLen];
      uint16_t len = ctxt->om->om_len;
      memcpy(buf, ctxt->om->om_data, len);

      os_mbuf *next;
      next = SLIST_NEXT(ctxt->om, om_next);
      while (next != NULL) {
        if ((len + next->om_len) > maxLen) {
          return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        memcpy(&buf[len], next->om_data, next->om_len);
        len += next->om_len;
        next = SLIST_NEXT(next, om_next);
      }

      pAtt->writeEvent(buf, len, peerInfo);
      return 0;
    }

    default: break;
  }

  return BLE_ATT_ERR_UNLIKELY;
}  // handleGattEvent

/**
 * @brief Set the server callbacks.
 *
 * As a BLE server operates, it will generate server level events such as a new client connecting or a previous
 * client disconnecting.  This function can be called to register a callback handler that will be invoked when these
 * events are detected.
 *
 * @param [in] pCallbacks The callbacks to be invoked.
 * @param [in] deleteCallbacks if true callback class will be deleted when server is destructed.
 */
void BLEServer::setCallbacks(BLEServerCallbacks *pCallbacks, bool deleteCallbacks) {
  if (pCallbacks != nullptr) {
    m_pServerCallbacks = pCallbacks;
    m_deleteCallbacks = deleteCallbacks;
  } else {
    m_pServerCallbacks = &defaultCallbacks;
    m_deleteCallbacks = false;
  }
}  // setCallbacks

/**
 * @brief Remove a service from the server.
 *
 * @details Immediately removes access to the service by clients, sends a service changed indication,
 * and removes the service (if applicable) from the advertisements.
 * The service is not deleted unless the deleteSvc parameter is true, otherwise the service remains
 * available and can be re-added in the future. If desired a removed but not deleted service can
 * be deleted later by calling this method with deleteSvc set to true.
 *
 * @note The service will not be removed from the database until all open connections are closed
 * as it requires resetting the GATT server. In the interim the service will have it's visibility disabled.
 *
 * @note Advertising will need to be restarted by the user after calling this as we must stop
 * advertising in order to remove the service.
 *
 * @param [in] service The service object to remove.
 * @param [in] deleteSvc true if the service should be deleted.
 */
void BLEServer::removeService(BLEService *service, bool deleteSvc) {
  // Check if the service was already removed and if so check if this
  // is being called to delete the object and do so if requested.
  // Otherwise, ignore the call and return.
  if (service->getRemoved() > 0) {
    if (deleteSvc) {
      for (auto it = m_svcVec.begin(); it != m_svcVec.end(); ++it) {
        if ((*it) == service) {
          delete *it;
          m_svcVec.erase(it);
          break;
        }
      }
    }

    return;
  }

  int rc = ble_gatts_svc_set_visibility(service->getHandle(), 0);
  if (rc != 0) {
    return;
  }

  service->setRemoved(deleteSvc ? NIMBLE_ATT_REMOVE_DELETE : NIMBLE_ATT_REMOVE_HIDE);
  serviceChanged();
#if !CONFIG_BT_NIMBLE_EXT_ADV
  BLEDevice::getAdvertising()->removeServiceUUID(service->getUUID());
#endif
}  // removeService

/**
 * @brief Adds a service which was either already created but removed from availability,\n
 * or created and later added to services list.
 * @param [in] service The service object to add.
 * @note If it is desired to advertise the service it must be added by
 * calling BLEAdvertising::addServiceUUID.
 */
void BLEServer::addService(BLEService *service) {
  // Check that a service with the supplied UUID does not already exist.
  if (getServiceByUUID(service->getUUID()) != nullptr) {
    log_w(LOG_TAG, "Warning creating a duplicate service UUID: %s", std::string(service->getUUID()).c_str());
  }

  // If adding a service that was not removed add it and return.
  // Else reset GATT and send service changed notification.
  if (service->getRemoved() == 0) {
    m_svcVec.push_back(service);
    return;
  }

  service->setRemoved(0);
  serviceChanged();
}  // addService

/**
 * @brief Resets the GATT server, used when services are added/removed after initialization.
 */
void BLEServer::resetGATT() {
  if (getConnectedCount() > 0) {
    return;
  }

  BLEDevice::stopAdvertising();
  ble_gatts_reset();
  ble_svc_gap_init();
  ble_svc_gatt_init();

  for (auto it = m_svcVec.begin(); it != m_svcVec.end();) {
    if ((*it)->getRemoved() > 0) {
      if ((*it)->getRemoved() == NIMBLE_ATT_REMOVE_DELETE) {
        delete *it;
        it = m_svcVec.erase(it);
      } else {
        ++it;
      }
      continue;
    }

    (*it)->start();
    ++it;
  }

  m_gattsStarted = false;
}  // resetGATT

#if CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Start advertising.
 * @param [in] instId The extended advertisement instance ID to start.
 * @param [in] duration How long to advertise for in milliseconds, 0 = forever (default).
 * @param [in] maxEvents Maximum number of advertisement events to send, 0 = no limit (default).
 * @return True if advertising started successfully.
 * @details Start the server advertising its existence.  This is a convenience function and is equivalent to
 * retrieving the advertising object and invoking start upon it.
 */
bool BLEServer::startAdvertising(uint8_t instId, int duration, int maxEvents) const {
  return getAdvertising()->start(instId, duration, maxEvents);
}  // startAdvertising

/**
 * @brief Convenience function to stop advertising a data set.
 * @param [in] instId The extended advertisement instance ID to stop advertising.
 * @return True if advertising stopped successfully.
 */
bool BLEServer::stopAdvertising(uint8_t instId) const {
  return getAdvertising()->stop(instId);
}  // stopAdvertising

/**
 * @brief Request an update to the PHY used for a peer connection.
 * @param [in] connHandle the connection handle to the update the PHY for.
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
bool BLEServer::updatePhy(uint16_t connHandle, uint8_t txPhyMask, uint8_t rxPhyMask, uint16_t phyOptions) {
  int rc = ble_gap_set_prefered_le_phy(connHandle, txPhyMask, rxPhyMask, phyOptions);
  if (rc != 0) {
    log_e(LOG_TAG, "Failed to update phy; rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
  }

  return rc == 0;
}  // updatePhy

/**
 * @brief Get the PHY used for a peer connection.
 * @param [in] connHandle the connection handle to the get the PHY for.
 * @param [out] txPhy The TX PHY.
 * @param [out] rxPhy The RX PHY.
 * @return True if successful.
 */
bool BLEServer::getPhy(uint16_t connHandle, uint8_t *txPhy, uint8_t *rxPhy) {
  int rc = ble_gap_read_le_phy(connHandle, txPhy, rxPhy);
  if (rc != 0) {
    log_e(LOG_TAG, "Failed to read phy; rc=%d %s", rc, BLEUtils::returnCodeToString(rc));
  }

  return rc == 0;
}  // getPhy
#endif

#if !CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Start advertising.
 * @param [in] duration The duration in milliseconds to advertise for, default = forever.
 * @return True if advertising started successfully.
 * @details Start the server advertising its existence. This is a convenience function and is equivalent to
 * retrieving the advertising object and invoking start upon it.
 */
bool BLEServer::startAdvertising(uint32_t duration) const {
  return getAdvertising()->start(duration);
}  // startAdvertising

/**
 * @brief Stop advertising.
 * @return True if advertising stopped successfully.
 */
bool BLEServer::stopAdvertising() const {
  return getAdvertising()->stop();
}  // stopAdvertising
#endif

/**
 * @brief Get the MTU value of a client connection.
 * @param [in] connHandle The connection handle of the client to get the MTU value for.
 * @returns The MTU or 0 if not found/connected.
 */
uint16_t BLEServer::getPeerMTU(uint16_t connHandle) const {
  return ble_att_mtu(connHandle);
}  // getPeerMTU

/**
 * @brief Request an Update the connection parameters:
 * * Can only be used after a connection has been established.
 * @param [in] connHandle The connection handle of the peer to send the request to.
 * @param [in] minInterval The minimum connection interval in 1.25ms units.
 * @param [in] maxInterval The maximum connection interval in 1.25ms units.
 * @param [in] latency The number of packets allowed to skip (extends max interval).
 * @param [in] timeout The timeout time in 10ms units before disconnecting.
 */
void BLEServer::updateConnParams(uint16_t connHandle, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const {
  ble_gap_upd_params params = {
    .itvl_min = minInterval,
    .itvl_max = maxInterval,
    .latency = latency,
    .supervision_timeout = timeout,
    .min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
    .max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN
  };

  int rc = ble_gap_update_params(connHandle, &params);
  if (rc != 0) {
    log_e(LOG_TAG, "Update params error: %d, %s", rc, BLEUtils::returnCodeToString(rc));
  }
}  // updateConnParams

/**
 * @brief Request an update of the data packet length.
 * * Can only be used after a connection has been established.
 * @details Sends a data length update request to the peer.
 * The Data Length Extension (DLE) allows to increase the Data Channel Payload from 27 bytes to up to 251 bytes.
 * The peer needs to support the Bluetooth 4.2 specifications, to be capable of DLE.
 * @param [in] connHandle The connection handle of the peer to send the request to.
 * @param [in] octets The preferred number of payload octets to use (Range 0x001B-0x00FB).
 */
void BLEServer::setDataLen(uint16_t connHandle, uint16_t octets) const {
  uint16_t tx_time = (octets + 14) * 8;

  int rc = ble_gap_set_data_len(connHandle, octets, tx_time);
  if (rc != 0) {
    log_e(LOG_TAG, "Set data length error: %d, %s", rc, BLEUtils::returnCodeToString(rc));
  }
}  // setDataLen

#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)
/**
 * @brief Create a client instance from the connection handle.
 * @param [in] connHandle The connection handle to create a client instance from.
 * @return A pointer to the BLEClient instance or nullptr if there was an error.
 * @note Only one instance is supported subsequent calls will overwrite the previous
 * client connection information and data.
 */
BLEClient *BLEServer::getClient(uint16_t connHandle) {
  BLEConnInfo connInfo;
  int rc = ble_gap_conn_find(connHandle, &connInfo.m_desc);
  if (rc != 0) {
    log_e(LOG_TAG, "Client info not found");
    return nullptr;
  }

  return getClient(connInfo);
}  // getClient

/**
 * @brief Create a client instance from the BLEConnInfo reference.
 * @param [in] connInfo The connection info to create a client instance from.
 * @return A pointer to the BLEClient instance or nullptr if there was an error.
 * @note Only one instance is supported subsequent calls will overwrite the previous
 * client connection information and data.
 */
BLEClient *BLEServer::getClient(const BLEConnInfo &connInfo) {
  if (m_pClient == nullptr) {
    m_pClient = new BLEClient(connInfo.getAddress());
  }

  m_pClient->deleteServices();  // Changed peer connection delete the database.
  m_pClient->m_peerAddress = connInfo.getAddress();
  m_pClient->m_connHandle = connInfo.getConnHandle();
  return m_pClient;
}  // getClient

/**
 * @brief Delete the BLEClient instance that was created with `getClient()`
 */
void BLEServer::deleteClient() {
  if (m_pClient != nullptr) {
    delete m_pClient;
    m_pClient = nullptr;
  }
}  // deleteClient
#endif

/** Default callback handlers */
void BLEServerCallbacks::onConnect(BLEServer *pServer, BLEConnInfo &connInfo) {
  log_d("BLEServerCallbacks", "onConnect(): Default");
}  // onConnect

void BLEServerCallbacks::onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason) {
  log_d("BLEServerCallbacks", "onDisconnect(): Default");
}  // onDisconnect

void BLEServerCallbacks::onMTUChange(uint16_t MTU, BLEConnInfo &connInfo) {
  log_d("BLEServerCallbacks", "onMTUChange(): Default");
}  // onMTUChange

uint32_t BLEServerCallbacks::onPassKeyDisplay() {
  log_d("BLEServerCallbacks", "onPassKeyDisplay: default: 123456");
  return 123456;
}  // onPassKeyDisplay

void BLEServerCallbacks::onConfirmPassKey(BLEConnInfo &connInfo, uint32_t pin) {
  log_d("BLEServerCallbacks", "onConfirmPasskey: default: true");
  BLEDevice::injectConfirmPasskey(connInfo, true);
}  // onConfirmPasskey

void BLEServerCallbacks::onIdentity(BLEConnInfo &connInfo) {
  log_d("BLEServerCallbacks", "onIdentity: default");
}  // onIdentity

void BLEServerCallbacks::onAuthenticationComplete(BLEConnInfo &connInfo) {
  log_d("BLEServerCallbacks", "onAuthenticationComplete: default");
}  // onAuthenticationComplete

void BLEServerCallbacks::onConnParamsUpdate(BLEConnInfo &connInfo) {
  log_d("BLEServerCallbacks", "onConnParamsUpdate: default");
}  // onConnParamsUpdate

#if CONFIG_BT_NIMBLE_EXT_ADV
void BLEServerCallbacks::onPhyUpdate(BLEConnInfo &connInfo, uint8_t txPhy, uint8_t rxPhy) {
  log_d("BLEServerCallbacks", "onPhyUpdate: default, txPhy: %d, rxPhy: %d", txPhy, rxPhy);
}  // onPhyUpdate
#endif

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
