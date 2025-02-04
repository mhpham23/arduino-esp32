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

#ifndef COMPONENTS_CPP_UTILS_BLEREMOTECHARACTERISTIC_H_
#define COMPONENTS_CPP_UTILS_BLEREMOTECHARACTERISTIC_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLERemoteValueAttribute.h"
#include <vector>
#include <functional>

class BLEUUID;
class BLERemoteService;
class BLERemoteDescriptor;
struct BLEDescriptorFilter;

/**
 * @brief A model of a remote BLE characteristic.
 */
class BLERemoteCharacteristic : public BLERemoteValueAttribute {
public:
  std::string toString() const;
  const BLERemoteService *getRemoteService() const;
  void deleteDescriptors() const;
  size_t deleteDescriptor(const BLEUUID &uuid) const;
  bool canBroadcast() const;
  bool canRead() const;
  bool canWriteNoResponse() const;
  bool canWrite() const;
  bool canNotify() const;
  bool canIndicate() const;
  bool canWriteSigned() const;
  bool hasExtendedProps() const;
  BLEClient *getClient() const override;

  typedef std::function<void(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)> notify_callback;

  bool subscribe(bool notifications = true, const notify_callback notifyCallback = nullptr, bool response = true) const;
  bool unsubscribe(bool response = true) const;

  std::vector<BLERemoteDescriptor *>::iterator begin() const;
  std::vector<BLERemoteDescriptor *>::iterator end() const;
  BLERemoteDescriptor *getDescriptor(const BLEUUID &uuid) const;
  const std::vector<BLERemoteDescriptor *> &getDescriptors(bool refresh = false) const;

private:
  friend class BLEClient;
  friend class BLERemoteService;

  BLERemoteCharacteristic(const BLERemoteService *pRemoteService, const ble_gatt_chr *chr);
  ~BLERemoteCharacteristic();

  bool setNotify(uint16_t val, notify_callback notifyCallback = nullptr, bool response = true) const;
  bool retrieveDescriptors(BLEDescriptorFilter *filter = nullptr) const;

  static int descriptorDiscCB(uint16_t connHandle, const ble_gatt_error *error, uint16_t chrHandle, const ble_gatt_dsc *dsc, void *arg);

  const BLERemoteService *m_pRemoteService{nullptr};
  uint8_t m_properties{0};
  mutable notify_callback m_notifyCallback{nullptr};
  mutable std::vector<BLERemoteDescriptor *> m_vDescriptors{};

};  // BLERemoteCharacteristic

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLERemoteCharacteristic.h
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#include <functional>

#include <esp_gattc_api.h>

#include "BLERemoteService.h"
#include "BLERemoteDescriptor.h"
#include "BLEUUID.h"
#include "RTOS.h"

class BLERemoteService;
class BLERemoteDescriptor;
typedef std::function<void(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)> notify_callback;
/**
 * @brief A model of a remote %BLE characteristic.
 */
class BLERemoteCharacteristic {
public:
  ~BLERemoteCharacteristic();

  // Public member functions
  bool canBroadcast();
  bool canIndicate();
  bool canNotify();
  bool canRead();
  bool canWrite();
  bool canWriteNoResponse();
  BLERemoteDescriptor *getDescriptor(BLEUUID uuid);
  std::map<std::string, BLERemoteDescriptor *> *getDescriptors();
  BLERemoteService *getRemoteService();
  uint16_t getHandle();
  BLEUUID getUUID();
  String readValue();
  uint8_t readUInt8();
  uint16_t readUInt16();
  uint32_t readUInt32();
  float readFloat();
  void registerForNotify(notify_callback _callback, bool notifications = true, bool descriptorRequiresRegistration = true);
  void writeValue(uint8_t *data, size_t length, bool response = false);
  void writeValue(String newValue, bool response = false);
  void writeValue(uint8_t newValue, bool response = false);
  String toString();
  uint8_t *readRawData();
  void setAuth(esp_gatt_auth_req_t auth);

private:
  BLERemoteCharacteristic(uint16_t handle, BLEUUID uuid, esp_gatt_char_prop_t charProp, BLERemoteService *pRemoteService);
  friend class BLEClient;
  friend class BLERemoteService;
  friend class BLERemoteDescriptor;

  // Private member functions
  void gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *evtParam);

  void removeDescriptors();
  void retrieveDescriptors();

  // Private properties
  BLEUUID m_uuid;
  esp_gatt_char_prop_t m_charProp;
  esp_gatt_auth_req_t m_auth;
  uint16_t m_handle;
  BLERemoteService *m_pRemoteService;
  FreeRTOS::Semaphore m_semaphoreReadCharEvt = FreeRTOS::Semaphore("ReadCharEvt");
  FreeRTOS::Semaphore m_semaphoreRegForNotifyEvt = FreeRTOS::Semaphore("RegForNotifyEvt");
  FreeRTOS::Semaphore m_semaphoreWriteCharEvt = FreeRTOS::Semaphore("WriteCharEvt");
  String m_value;
  uint8_t *m_rawData;
  notify_callback m_notifyCallback;

  // We maintain a map of descriptors owned by this characteristic keyed by a string representation of the UUID.
  std::map<std::string, BLERemoteDescriptor *> m_descriptorMap;
};  // BLERemoteCharacteristic

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTECHARACTERISTIC_H_ */
