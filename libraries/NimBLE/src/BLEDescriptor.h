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

#ifndef NIMBLE_CPP_DESCRIPTOR_H_
#define NIMBLE_CPP_DESCRIPTOR_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

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

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
#endif /* NIMBLE_CPP_DESCRIPTOR_H_ */
