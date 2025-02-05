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

#ifndef COMPONENTS_CPP_UTILS_BLECHARACTERISTIC_H_
#define COMPONENTS_CPP_UTILS_BLECHARACTERISTIC_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class BLECharacteristicCallbacks;
class BLEService;
class BLECharacteristic;
class BLEDescriptor;
class BLE2904;

#include "BLELocalValueAttribute.h"

#include <string>
#include <vector>

/**
 * @brief The model of a BLE Characteristic.
 *
 * A BLE Characteristic is an identified value container that manages a value. It is exposed by a BLE service and
 * can be read and written to by a BLE client.
 */
class BLECharacteristic : public BLELocalValueAttribute {
public:
  static const uint32_t PROPERTY_READ = BLE_GATT_CHR_F_READ;
  static const uint32_t PROPERTY_READ_ENC = BLE_GATT_CHR_F_READ_ENC;
  static const uint32_t PROPERTY_READ_AUTHEN = BLE_GATT_CHR_F_READ_AUTHEN;
  static const uint32_t PROPERTY_READ_AUTHOR = BLE_GATT_CHR_F_READ_AUTHOR;
  static const uint32_t PROPERTY_WRITE = BLE_GATT_CHR_F_WRITE;
  static const uint32_t PROPERTY_WRITE_NR = BLE_GATT_CHR_F_WRITE_NO_RSP;
  static const uint32_t PROPERTY_WRITE_ENC = BLE_GATT_CHR_F_WRITE_ENC;
  static const uint32_t PROPERTY_WRITE_AUTHEN = BLE_GATT_CHR_F_WRITE_AUTHEN;
  static const uint32_t PROPERTY_WRITE_AUTHOR = BLE_GATT_CHR_F_WRITE_AUTHOR;
  static const uint32_t PROPERTY_BROADCAST = BLE_GATT_CHR_F_BROADCAST;
  static const uint32_t PROPERTY_NOTIFY = BLE_GATT_CHR_F_NOTIFY;
  static const uint32_t PROPERTY_INDICATE = BLE_GATT_CHR_F_INDICATE;

  BLECharacteristic(
    const char *uuid, uint16_t properties = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, uint16_t maxLen = BLE_ATT_ATTR_MAX_LEN,
    BLEService *pService = nullptr
  );
  BLECharacteristic(
    const BLEUUID &uuid, uint16_t properties = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, uint16_t maxLen = BLE_ATT_ATTR_MAX_LEN,
    BLEService *pService = nullptr
  );

  ~BLECharacteristic();

  std::string toString() const;
  void addDescriptor(BLEDescriptor *pDescriptor);
  void removeDescriptor(BLEDescriptor *pDescriptor, bool deleteDsc = false);
  uint16_t getProperties() const;
  void setCallbacks(BLECharacteristicCallbacks *pCallbacks);
  bool indicate(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
  bool indicate(const uint8_t *value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
  bool notify(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
  bool notify(const uint8_t *value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;

  BLEDescriptor *createDescriptor(
    const char *uuid, uint32_t properties = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, uint16_t maxLen = BLE_ATT_ATTR_MAX_LEN
  );
  BLEDescriptor *createDescriptor(
    const BLEUUID &uuid, uint32_t properties = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, uint16_t maxLen = BLE_ATT_ATTR_MAX_LEN
  );
  BLE2904 *create2904();
  BLEDescriptor *getDescriptorByUUID(const char *uuid) const;
  BLEDescriptor *getDescriptorByUUID(const BLEUUID &uuid) const;
  BLEDescriptor *getDescriptorByHandle(uint16_t handle) const;
  BLEService *getService() const;

  BLECharacteristicCallbacks *getCallbacks() const;

  /*********************** Template Functions ************************/

#if __cplusplus < 201703L
  /**
     * @brief Template to send a notification with a value from a struct or array.
     * @param [in] v The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     * @details <type\> size must be evaluatable by `sizeof()`.
     */
  template<typename T>
  typename std::enable_if<!std::is_pointer<T>::value && !Has_c_str_length<T>::value && !Has_data_size<T>::value, bool>::type
    notify(const T &v, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return notify(reinterpret_cast<const uint8_t *>(&v), sizeof(T), connHandle);
  }

  /**
     * @brief Template to send a notification with a value from a class that has a c_str() and length() method.
     * @param [in] s The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     */
  template<typename T>
  typename std::enable_if<Has_c_str_length<T>::value && !Has_data_size<T>::value, bool>::type
    notify(const T &s, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return notify(reinterpret_cast<const uint8_t *>(s.c_str()), s.length(), connHandle);
  }

  /**
     * @brief Template to send a notification with a value from a class that has a data() and size() method.
     * @param [in] v The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     */
  template<typename T> typename std::enable_if<Has_data_size<T>::value, bool>::type notify(const T &v, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return notify(reinterpret_cast<const uint8_t *>(v.data()), v.size(), connHandle);
  }

  /**
     * @brief Template to send an indication with a value from a struct or array.
     * @param [in] v The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     * @details <type\> size must be evaluatable by `sizeof()`.
     */
  template<typename T>
  typename std::enable_if<!std::is_pointer<T>::value && !Has_c_str_length<T>::value && !Has_data_size<T>::value, bool>::type
    indicate(const T &v, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return indicate(reinterpret_cast<const uint8_t *>(&v), sizeof(T), connHandle);
  }

  /**
     * @brief Template to send a indication with a value from a class that has a c_str() and length() method.
     * @param [in] s The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     */
  template<typename T>
  typename std::enable_if<Has_c_str_length<T>::value && !Has_data_size<T>::value, bool>::type
    indicate(const T &s, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return indicate(reinterpret_cast<const uint8_t *>(s.c_str()), s.length(), connHandle);
  }

  /**
     * @brief Template to send a indication with a value from a class that has a data() and size() method.
     * @param [in] v The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     */
  template<typename T> typename std::enable_if<Has_data_size<T>::value, bool>::type indicate(const T &v, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    return indicate(reinterpret_cast<const uint8_t *>(v.data()), v.size(), connHandle);
  }

#else

  /**
     * @brief Template to send a notification for classes which may have
     *        data()/size() or c_str()/length() methods. Falls back to sending
     *        the data by casting the first element of the array to a uint8_t
     *        pointer and getting the length of the array using sizeof.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @param[in] connHandle The connection handle to send the notification to.
     * @note This function is only available if the type T is not a pointer.
     */
  template<typename T>
  typename std::enable_if<!std::is_pointer<T>::value, bool>::type notify(const T &value, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    if constexpr (Has_data_size<T>::value) {
      return notify(reinterpret_cast<const uint8_t *>(value.data()), value.size(), connHandle);
    } else if constexpr (Has_c_str_length<T>::value) {
      return notify(reinterpret_cast<const uint8_t *>(value.c_str()), value.length(), connHandle);
    } else {
      return notify(reinterpret_cast<const uint8_t *>(&value), sizeof(value), connHandle);
    }
  }

  /**
     * @brief Template to send an indication for classes which may have
     *       data()/size() or c_str()/length() methods. Falls back to sending
     *       the data by casting the first element of the array to a uint8_t
     *       pointer and getting the length of the array using sizeof.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @param[in] connHandle The connection handle to send the indication to.
     * @note This function is only available if the type T is not a pointer.
     */
  template<typename T>
  typename std::enable_if<!std::is_pointer<T>::value, bool>::type indicate(const T &value, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
    if constexpr (Has_data_size<T>::value) {
      return indicate(reinterpret_cast<const uint8_t *>(value.data()), value.size(), connHandle);
    } else if constexpr (Has_c_str_length<T>::value) {
      return indicate(reinterpret_cast<const uint8_t *>(value.c_str()), value.length(), connHandle);
    } else {
      return indicate(reinterpret_cast<const uint8_t *>(&value), sizeof(value), connHandle);
    }
  }
#endif

private:
  friend class BLEServer;
  friend class BLEService;

  void setService(BLEService *pService);
  void readEvent(BLEConnInfo &connInfo) override;
  void writeEvent(const uint8_t *val, uint16_t len, BLEConnInfo &connInfo) override;
  bool sendValue(const uint8_t *value, size_t length, bool is_notification = true, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;

  BLECharacteristicCallbacks *m_pCallbacks{nullptr};
  BLEService *m_pService{nullptr};
  std::vector<BLEDescriptor *> m_vDescriptors{};
};  // BLECharacteristic

/**
 * @brief Callbacks that can be associated with a %BLE characteristic to inform of events.
 *
 * When a server application creates a %BLE characteristic, we may wish to be informed when there is either
 * a read or write request to the characteristic's value. An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLECharacteristicCallbacks {
public:
  virtual ~BLECharacteristicCallbacks() {}
  virtual void onRead(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo);
  virtual void onWrite(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo);
  virtual void onStatus(BLECharacteristic *pCharacteristic, int code);
  virtual void onSubscribe(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo, uint16_t subValue);
};

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLECharacteristic.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#include <string>
#include <map>
#include "BLEUUID.h"
#include <esp_gatts_api.h>
#include <esp_gap_ble_api.h>
#include "BLEDescriptor.h"
#include "BLEValue.h"
#include "RTOS.h"

class BLEService;
class BLEDescriptor;
class BLECharacteristicCallbacks;

/**
 * @brief A management structure for %BLE descriptors.
 */
class BLEDescriptorMap {
public:
  void setByUUID(const char *uuid, BLEDescriptor *pDescriptor);
  void setByUUID(BLEUUID uuid, BLEDescriptor *pDescriptor);
  void setByHandle(uint16_t handle, BLEDescriptor *pDescriptor);
  BLEDescriptor *getByUUID(const char *uuid);
  BLEDescriptor *getByUUID(BLEUUID uuid);
  BLEDescriptor *getByHandle(uint16_t handle);
  String toString();
  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
  BLEDescriptor *getFirst();
  BLEDescriptor *getNext();

private:
  std::map<BLEDescriptor *, String> m_uuidMap;
  std::map<uint16_t, BLEDescriptor *> m_handleMap;
  std::map<BLEDescriptor *, String>::iterator m_iterator;
};

/**
 * @brief The model of a %BLE Characteristic.
 *
 * A BLE Characteristic is an identified value container that manages a value. It is exposed by a BLE server and
 * can be read and written to by a %BLE client.
 */
class BLECharacteristic {
public:
  BLECharacteristic(const char *uuid, uint32_t properties = 0);
  BLECharacteristic(BLEUUID uuid, uint32_t properties = 0);
  virtual ~BLECharacteristic();

  void addDescriptor(BLEDescriptor *pDescriptor);
  BLEDescriptor *getDescriptorByUUID(const char *descriptorUUID);
  BLEDescriptor *getDescriptorByUUID(BLEUUID descriptorUUID);
  BLEUUID getUUID();
  String getValue();
  uint8_t *getData();
  size_t getLength();

  void indicate();
  void notify(bool is_notification = true);
  void setBroadcastProperty(bool value);
  void setCallbacks(BLECharacteristicCallbacks *pCallbacks);
  void setIndicateProperty(bool value);
  void setNotifyProperty(bool value);
  void setReadProperty(bool value);
  void setValue(uint8_t *data, size_t size);
  void setValue(String value);
  void setValue(uint16_t &data16);
  void setValue(uint32_t &data32);
  void setValue(int &data32);
  void setValue(float &data32);
  void setValue(double &data64);
  void setWriteProperty(bool value);
  void setWriteNoResponseProperty(bool value);
  String toString();
  uint16_t getHandle();
  void setAccessPermissions(esp_gatt_perm_t perm);

  static const uint32_t PROPERTY_READ = 1 << 0;
  static const uint32_t PROPERTY_WRITE = 1 << 1;
  static const uint32_t PROPERTY_NOTIFY = 1 << 2;
  static const uint32_t PROPERTY_BROADCAST = 1 << 3;
  static const uint32_t PROPERTY_INDICATE = 1 << 4;
  static const uint32_t PROPERTY_WRITE_NR = 1 << 5;

  static const uint32_t indicationTimeout = 1000;

private:
  friend class BLEServer;
  friend class BLEService;
  friend class BLEDescriptor;
  friend class BLECharacteristicMap;

  BLEUUID m_bleUUID;
  BLEDescriptorMap m_descriptorMap;
  uint16_t m_handle;
  esp_gatt_char_prop_t m_properties;
  BLECharacteristicCallbacks *m_pCallbacks;
  BLEService *m_pService;
  BLEValue m_value;
  esp_gatt_perm_t m_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
  bool m_writeEvt = false;  // If we have started a long write, this tells the commit code that we were the target

  void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

  void executeCreate(BLEService *pService);
  esp_gatt_char_prop_t getProperties();
  BLEService *getService();
  void setHandle(uint16_t handle);
  FreeRTOS::Semaphore m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
  FreeRTOS::Semaphore m_semaphoreConfEvt = FreeRTOS::Semaphore("ConfEvt");
  FreeRTOS::Semaphore m_semaphoreSetValue = FreeRTOS::Semaphore("SetValue");
};  // BLECharacteristic

/**
 * @brief Callbacks that can be associated with a %BLE characteristic to inform of events.
 *
 * When a server application creates a %BLE characteristic, we may wish to be informed when there is either
 * a read or write request to the characteristic's value. An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLECharacteristicCallbacks {
public:
  typedef enum {
    SUCCESS_INDICATE,
    SUCCESS_NOTIFY,
    ERROR_INDICATE_DISABLED,
    ERROR_NOTIFY_DISABLED,
    ERROR_GATT,
    ERROR_NO_CLIENT,
    ERROR_INDICATE_TIMEOUT,
    ERROR_INDICATE_FAILURE
  } Status;

  virtual ~BLECharacteristicCallbacks();

  /**
	 * @brief Callback function to support a read request.
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 * @param [in] param The BLE GATTS param. Use param->read.
	 */
  virtual void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param);
  /**
	 * @brief DEPRECATED! Callback function to support a read request. Called only if onRead(,) is not overridden
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 */
  virtual void onRead(BLECharacteristic *pCharacteristic);

  /**
	 * @brief Callback function to support a write request.
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 * @param [in] param The BLE GATTS param. Use param->write.
	 */
  virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param);
  /**
	 * @brief DEPRECATED! Callback function to support a write request. Called only if onWrite(,) is not overridden.
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 */
  virtual void onWrite(BLECharacteristic *pCharacteristic);

  /**
	 * @brief Callback function to support a Notify request.
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 */
  virtual void onNotify(BLECharacteristic *pCharacteristic);

  /**
	 * @brief Callback function to support a Notify/Indicate Status report.
	 * @param [in] pCharacteristic The characteristic that is the source of the event.
	 * @param [in] s Status of the notification/indication
	 * @param [in] code Additional code of underlying errors
	 */
  virtual void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code);
};

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLECHARACTERISTIC_H_ */
