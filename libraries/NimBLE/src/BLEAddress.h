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

#ifndef NIMBLE_CPP_ADDRESS_H_
#define NIMBLE_CPP_ADDRESS_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimble/ble.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include <string>

/**
 * @brief A BLE device address.
 *
 * Every BLE device has a unique address which can be used to identify it and form connections.
 */
class BLEAddress : private ble_addr_t {
public:
  /**
     * @brief Create a blank address, i.e. 00:00:00:00:00:00, type 0.
     */
  BLEAddress() = default;
  BLEAddress(const ble_addr_t address);
  BLEAddress(const uint8_t address[BLE_DEV_ADDR_LEN], uint8_t type);
  BLEAddress(const std::string &stringAddress, uint8_t type);
  BLEAddress(const uint64_t &address, uint8_t type);

  bool isRpa() const;
  bool isNrpa() const;
  bool isStatic() const;
  bool isPublic() const;
  bool isNull() const;
  bool equals(const BLEAddress &otherAddress) const;
  const ble_addr_t *getBase() const;
  std::string toString() const;
  uint8_t getType() const;
  const uint8_t *getVal() const;
  const BLEAddress &reverseByteOrder();
  bool operator==(const BLEAddress &rhs) const;
  bool operator!=(const BLEAddress &rhs) const;
  operator std::string() const;
  operator uint64_t() const;
};

#endif /* CONFIG_BT_ENABLED */
#endif /* NIMBLE_CPP_ADDRESS_H_ */
