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

#ifndef COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_
#define COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BLELocalValueAttribute.h"
#include <string>

class BLECharacteristic;
class BLEDescriptorCallbacks;

/**
 * @brief A model of a BLE descriptor.
 */
class BLEDescriptor : public BLELocalValueAttribute {
public:
  BLEDescriptor(const char *uuid, uint16_t properties, uint16_t maxLen, BLECharacteristic *pCharacteristic = nullptr);

  BLEDescriptor(const BLEUUID &uuid, uint16_t properties, uint16_t maxLen, BLECharacteristic *pCharacteristic = nullptr);
  ~BLEDescriptor() = default;

  std::string toString() const;
  void setCallbacks(BLEDescriptorCallbacks *pCallbacks);
  BLECharacteristic *getCharacteristic() const;

private:
  friend class BLECharacteristic;
  friend class BLEService;

  void setCharacteristic(BLECharacteristic *pChar);
  void readEvent(BLEConnInfo &connInfo) override;
  void writeEvent(const uint8_t *val, uint16_t len, BLEConnInfo &connInfo) override;

  BLEDescriptorCallbacks *m_pCallbacks{nullptr};
  BLECharacteristic *m_pCharacteristic{nullptr};
};  // BLEDescriptor

/**
 * @brief Callbacks that can be associated with a %BLE descriptors to inform of events.
 *
 * When a server application creates a %BLE descriptor, we may wish to be informed when there is either
 * a read or write request to the descriptors value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLEDescriptorCallbacks {
public:
  virtual ~BLEDescriptorCallbacks() = default;
  virtual void onRead(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo);
  virtual void onWrite(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo);
};

#include "BLE2904.h"

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEDescriptor.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#include <string>
#include "BLEUUID.h"
#include "BLECharacteristic.h"
#include <esp_gatts_api.h>
#include "RTOS.h"

class BLEService;
class BLECharacteristic;
class BLEDescriptorCallbacks;

/**
 * @brief A model of a %BLE descriptor.
 */
class BLEDescriptor {
public:
  BLEDescriptor(const char *uuid, uint16_t max_len = 100);
  BLEDescriptor(BLEUUID uuid, uint16_t max_len = 100);
  virtual ~BLEDescriptor();

  uint16_t getHandle();  // Get the handle of the descriptor.
  size_t getLength();    // Get the length of the value of the descriptor.
  BLEUUID getUUID();     // Get the UUID of the descriptor.
  uint8_t *getValue();   // Get a pointer to the value of the descriptor.
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

  void setAccessPermissions(esp_gatt_perm_t perm);        // Set the permissions of the descriptor.
  void setCallbacks(BLEDescriptorCallbacks *pCallbacks);  // Set callbacks to be invoked for the descriptor.
  void setValue(uint8_t *data, size_t size);              // Set the value of the descriptor as a pointer to data.
  void setValue(String value);                            // Set the value of the descriptor as a data buffer.

  String toString();  // Convert the descriptor to a string representation.

private:
  friend class BLEDescriptorMap;
  friend class BLECharacteristic;
  BLEUUID m_bleUUID;
  uint16_t m_handle;
  BLEDescriptorCallbacks *m_pCallback;
  BLECharacteristic *m_pCharacteristic;
  esp_gatt_perm_t m_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
  FreeRTOS::Semaphore m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
  esp_attr_value_t m_value;

  void executeCreate(BLECharacteristic *pCharacteristic);
  void setHandle(uint16_t handle);
};  // BLEDescriptor

/**
 * @brief Callbacks that can be associated with a %BLE descriptors to inform of events.
 *
 * When a server application creates a %BLE descriptor, we may wish to be informed when there is either
 * a read or write request to the descriptors value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLEDescriptorCallbacks {
public:
  virtual ~BLEDescriptorCallbacks();
  virtual void onRead(BLEDescriptor *pDescriptor);
  virtual void onWrite(BLEDescriptor *pDescriptor);
};

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_ */
