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

#ifndef COMPONENTS_CPP_UTILS_BLEREMOTEDESCRIPTOR_H_
#define COMPONENTS_CPP_UTILS_BLEREMOTEDESCRIPTOR_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "BLERemoteValueAttribute.h"

class BLERemoteCharacteristic;
class BLEClient;

/**
 * @brief A model of remote BLE descriptor.
 */
class BLERemoteDescriptor : public BLERemoteValueAttribute {
public:
  BLERemoteCharacteristic *getRemoteCharacteristic() const;
  std::string toString(void) const;
  BLEClient *getClient() const override;

private:
  friend class BLERemoteCharacteristic;

  BLERemoteDescriptor(const BLERemoteCharacteristic *pRemoteCharacteristic, const ble_gatt_dsc *dsc);
  ~BLERemoteDescriptor() = default;

  const BLERemoteCharacteristic *m_pRemoteCharacteristic;
};

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLERemoteDescriptor.h
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#include <string>

#include <esp_gattc_api.h>

#include "BLERemoteCharacteristic.h"
#include "BLEUUID.h"
#include "RTOS.h"

class BLERemoteCharacteristic;
/**
 * @brief A model of remote %BLE descriptor.
 */
class BLERemoteDescriptor {
public:
  uint16_t getHandle();
  BLERemoteCharacteristic *getRemoteCharacteristic();
  BLEUUID getUUID();
  String readValue(void);
  uint8_t readUInt8(void);
  uint16_t readUInt16(void);
  uint32_t readUInt32(void);
  String toString(void);
  void writeValue(uint8_t *data, size_t length, bool response = false);
  void writeValue(String newValue, bool response = false);
  void writeValue(uint8_t newValue, bool response = false);
  void setAuth(esp_gatt_auth_req_t auth);
  void gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *evtParam);

private:
  friend class BLERemoteCharacteristic;
  BLERemoteDescriptor(uint16_t handle, BLEUUID uuid, BLERemoteCharacteristic *pRemoteCharacteristic);
  uint16_t m_handle;                                 // Server handle of this descriptor.
  BLEUUID m_uuid;                                    // UUID of this descriptor.
  String m_value;                                    // Last received value of the descriptor.
  BLERemoteCharacteristic *m_pRemoteCharacteristic;  // Reference to the Remote characteristic of which this descriptor is associated.
  FreeRTOS::Semaphore m_semaphoreReadDescrEvt = FreeRTOS::Semaphore("ReadDescrEvt");
  FreeRTOS::Semaphore m_semaphoreWriteDescrEvt = FreeRTOS::Semaphore("WriteDescrEvt");
  esp_gatt_auth_req_t m_auth;
};

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTEDESCRIPTOR_H_ */
