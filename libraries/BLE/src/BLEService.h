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

#ifndef COMPONENTS_CPP_UTILS_BLESERVICE_H_
#define COMPONENTS_CPP_UTILS_BLESERVICE_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class BLEService;

#include "BLEAttribute.h"
#include "BLEServer.h"
#include "BLECharacteristic.h"

/**
 * @brief The model of a BLE service.
 *
 */
class BLEService : public BLELocalAttribute {
public:
  BLEService(const char *uuid);
  BLEService(const BLEUUID &uuid);
  ~BLEService();

  BLEServer *getServer() const;
  std::string toString() const;
  void dump() const;
  bool isStarted() const;
  bool start();
  BLECharacteristic *
    createCharacteristic(const char *uuid, uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);

  BLECharacteristic *
    createCharacteristic(const BLEUUID &uuid, uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);
  void addCharacteristic(BLECharacteristic *pCharacteristic);
  void removeCharacteristic(BLECharacteristic *pCharacteristic, bool deleteChr = false);
  BLECharacteristic *getCharacteristic(const char *uuid, uint16_t instanceId = 0) const;
  BLECharacteristic *getCharacteristic(const BLEUUID &uuid, uint16_t instanceId = 0) const;
  BLECharacteristic *getCharacteristicByHandle(uint16_t handle) const;

  const std::vector<BLECharacteristic *> &getCharacteristics() const;
  std::vector<BLECharacteristic *> getCharacteristics(const char *uuid) const;
  std::vector<BLECharacteristic *> getCharacteristics(const BLEUUID &uuid) const;

private:
  friend class BLEServer;

  std::vector<BLECharacteristic *> m_vChars{};
  // Nimble requires an array of services to be sent to the api
  // Since we are adding 1 at a time we create an array of 2 and set the type
  // of the second service to 0 to indicate the end of the array.
  ble_gatt_svc_def m_pSvcDef[2]{};
};  // BLEService

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEService.h
 *
 *  Created on: Mar 25, 2017
 *      Author: kolban
 */

#include <esp_gatts_api.h>

#include "BLECharacteristic.h"
#include "BLEServer.h"
#include "BLEUUID.h"
#include "RTOS.h"

class BLEServer;

/**
 * @brief A data mapping used to manage the set of %BLE characteristics known to the server.
 */
class BLECharacteristicMap {
public:
  void setByUUID(BLECharacteristic *pCharacteristic, const char *uuid);
  void setByUUID(BLECharacteristic *pCharacteristic, BLEUUID uuid);
  void setByHandle(uint16_t handle, BLECharacteristic *pCharacteristic);
  BLECharacteristic *getByUUID(const char *uuid);
  BLECharacteristic *getByUUID(BLEUUID uuid);
  BLECharacteristic *getByHandle(uint16_t handle);
  BLECharacteristic *getFirst();
  BLECharacteristic *getNext();
  String toString();
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

private:
  std::map<BLECharacteristic *, String> m_uuidMap;
  std::map<uint16_t, BLECharacteristic *> m_handleMap;
  std::map<BLECharacteristic *, String>::iterator m_iterator;
};

/**
 * @brief The model of a %BLE service.
 *
 */
class BLEService {
public:
  void addCharacteristic(BLECharacteristic *pCharacteristic);
  BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties);
  BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties);
  void dump();
  void executeCreate(BLEServer *pServer);
  void executeDelete();
  BLECharacteristic *getCharacteristic(const char *uuid);
  BLECharacteristic *getCharacteristic(BLEUUID uuid);
  BLEUUID getUUID();
  BLEServer *getServer();
  void start();
  void stop();
  String toString();
  uint16_t getHandle();
  uint8_t m_instId = 0;

private:
  BLEService(const char *uuid, uint16_t numHandles);
  BLEService(BLEUUID uuid, uint16_t numHandles);
  friend class BLEServer;
  friend class BLEServiceMap;
  friend class BLEDescriptor;
  friend class BLECharacteristic;
  friend class BLEDevice;

  BLECharacteristicMap m_characteristicMap;
  uint16_t m_handle;
  BLECharacteristic *m_lastCreatedCharacteristic = nullptr;
  BLEServer *m_pServer = nullptr;
  BLEUUID m_uuid;

  FreeRTOS::Semaphore m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
  FreeRTOS::Semaphore m_semaphoreDeleteEvt = FreeRTOS::Semaphore("DeleteEvt");
  FreeRTOS::Semaphore m_semaphoreStartEvt = FreeRTOS::Semaphore("StartEvt");
  FreeRTOS::Semaphore m_semaphoreStopEvt = FreeRTOS::Semaphore("StopEvt");

  uint16_t m_numHandles;

  BLECharacteristic *getLastCreatedCharacteristic();
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
  void setHandle(uint16_t handle);
  //void               setService(esp_gatt_srvc_id_t srvc_id);
};  // BLEService

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLESERVICE_H_ */
