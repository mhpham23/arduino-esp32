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

#ifndef NIMBLE_CPP_REMOTE_DESCRIPTOR_H_
#define NIMBLE_CPP_REMOTE_DESCRIPTOR_H_

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

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
#endif /* NIMBLE_CPP_REMOTE_DESCRIPTOR_H_ */
