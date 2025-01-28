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
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLERemoteValueAttribute.h"
#include "BLEClient.h"
#include "BLEUtils.h"
#include "BLELog.h"

#include <climits>

static const char *LOG_TAG = "BLERemoteValueAttribute";

bool BLERemoteValueAttribute::writeValue(const uint8_t *data, size_t length, bool response) const {
  NIMBLE_LOGD(LOG_TAG, ">> writeValue()");

  const BLEClient *pClient = getClient();
  int retryCount = 1;
  int rc = 0;
  uint16_t mtu = pClient->getMTU() - 3;
  BLETaskData taskData(const_cast<BLERemoteValueAttribute *>(this));

  // Check if the data length is longer than we can write in one connection event.
  // If so we must do a long write which requires a response.
  if (length <= mtu && !response) {
    rc = ble_gattc_write_no_rsp_flat(pClient->getConnHandle(), getHandle(), data, length);
    goto Done;
  }

  do {
    if (length > mtu) {
      NIMBLE_LOGI(LOG_TAG, "writeValue: long write");
      os_mbuf *om = ble_hs_mbuf_from_flat(data, length);
      rc = ble_gattc_write_long(pClient->getConnHandle(), getHandle(), 0, om, BLERemoteValueAttribute::onWriteCB, &taskData);
    } else {
      rc = ble_gattc_write_flat(pClient->getConnHandle(), getHandle(), data, length, BLERemoteValueAttribute::onWriteCB, &taskData);
    }

    if (rc != 0) {
      goto Done;
    }

    BLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
    rc = taskData.m_flags;
    switch (rc) {
      case 0:
      case BLE_HS_EDONE: rc = 0; break;
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
        NIMBLE_LOGE(LOG_TAG, "Long write not supported by peer; Truncating length to %d", mtu);
        retryCount++;
        length = mtu;
        break;

      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
        if (retryCount && pClient->secureConnection()) {
          break;
        }
      /* Else falls through. */
      default: goto Done;
    }
  } while (rc != 0 && retryCount--);

Done:
  if (rc != 0) {
    NIMBLE_LOGE(LOG_TAG, "<< writeValue failed, rc: %d %s", rc, BLEUtils::returnCodeToString(rc));
  } else {
    NIMBLE_LOGD(LOG_TAG, "<< writeValue");
  }

  return (rc == 0);
}  // writeValue

/**
 * @brief Callback for characteristic write operation.
 * @return success == 0 or error code.
 */
int BLERemoteValueAttribute::onWriteCB(uint16_t conn_handle, const ble_gatt_error *error, ble_gatt_attr *attr, void *arg) {
  auto pTaskData = static_cast<BLETaskData *>(arg);
  const auto pAtt = static_cast<BLERemoteValueAttribute *>(pTaskData->m_pInstance);

  if (error->status == BLE_HS_ENOTCONN) {
    NIMBLE_LOGE(LOG_TAG, "<< Write complete; Not connected");
    BLEUtils::taskRelease(*pTaskData, error->status);
    return error->status;
  }

  if (pAtt->getClient()->getConnHandle() != conn_handle) {
    return 0;
  }

  NIMBLE_LOGI(LOG_TAG, "Write complete; status=%d", error->status);
  BLEUtils::taskRelease(*pTaskData, error->status);
  return 0;
}

/**
 * @brief Read the value of the remote characteristic.
 * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
 * @return The value of the remote characteristic.
 */
BLEAttValue BLERemoteValueAttribute::readValue(time_t *timestamp) const {
  NIMBLE_LOGD(LOG_TAG, ">> readValue()");

  BLEAttValue value{};
  const BLEClient *pClient = getClient();
  int rc = 0;
  int retryCount = 1;
  BLETaskData taskData(const_cast<BLERemoteValueAttribute *>(this), 0, &value);

  do {
    rc = ble_gattc_read_long(pClient->getConnHandle(), getHandle(), 0, BLERemoteValueAttribute::onReadCB, &taskData);
    if (rc != 0) {
      goto Done;
    }

    BLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
    rc = taskData.m_flags;
    switch (rc) {
      case 0:
      case BLE_HS_EDONE: rc = 0; break;
      // Characteristic is not long-readable, return with what we have.
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
        NIMBLE_LOGI(LOG_TAG, "Attribute not long");
        rc = ble_gattc_read(pClient->getConnHandle(), getHandle(), BLERemoteValueAttribute::onReadCB, &taskData);
        if (rc != 0) {
          goto Done;
        }
        retryCount++;
        break;
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
      case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
        if (retryCount && pClient->secureConnection()) {
          break;
        }
      /* Else falls through. */
      default: goto Done;
    }
  } while (rc != 0 && retryCount--);

  value.setTimeStamp();
  m_value = value;
  if (timestamp != nullptr) {
    *timestamp = value.getTimeStamp();
  }

Done:
  if (rc != 0) {
    NIMBLE_LOGE(LOG_TAG, "<< readValue failed rc=%d, %s", rc, BLEUtils::returnCodeToString(rc));
  } else {
    NIMBLE_LOGD(LOG_TAG, "<< readValue");
  }

  return value;
}  // readValue

/**
 * @brief Callback for characteristic read operation.
 * @return success == 0 or error code.
 */
int BLERemoteValueAttribute::onReadCB(uint16_t conn_handle, const ble_gatt_error *error, ble_gatt_attr *attr, void *arg) {
  auto pTaskData = static_cast<BLETaskData *>(arg);
  const auto pAtt = static_cast<BLERemoteValueAttribute *>(pTaskData->m_pInstance);

  if (error->status == BLE_HS_ENOTCONN) {
    NIMBLE_LOGE(LOG_TAG, "<< Read complete; Not connected");
    BLEUtils::taskRelease(*pTaskData, error->status);
    return error->status;
  }

  if (pAtt->getClient()->getConnHandle() != conn_handle) {
    return 0;
  }

  int rc = error->status;
  NIMBLE_LOGI(LOG_TAG, "Read complete; status=%d", rc);

  if (rc == 0) {
    if (attr) {
      auto valBuf = static_cast<BLEAttValue *>(pTaskData->m_pBuf);
      uint16_t data_len = OS_MBUF_PKTLEN(attr->om);
      if ((valBuf->size() + data_len) > BLE_ATT_ATTR_MAX_LEN) {
        rc = BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
      } else {
        NIMBLE_LOGD(LOG_TAG, "Got %u bytes", data_len);
        valBuf->append(attr->om->om_data, data_len);
        return 0;
      }
    }
  }

  BLEUtils::taskRelease(*pTaskData, rc);
  return rc;
}  // onReadCB

#endif  // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL
