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

#ifndef NIMBLE_CPP_UUID_H_
#define NIMBLE_CPP_UUID_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "host/ble_uuid.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include <string>
#include <cstring>

/**
 * @brief A model of a %BLE UUID.
 */
class BLEUUID {
public:
  /**
   * @brief Created a blank UUID.
   */
  BLEUUID() = default;
  BLEUUID(const ble_uuid_any_t &uuid);
  BLEUUID(const std::string &uuid);
  BLEUUID(uint16_t uuid);
  BLEUUID(uint32_t uuid);
  BLEUUID(const ble_uuid128_t *uuid);
  BLEUUID(const uint8_t *pData, size_t size);
  BLEUUID(uint32_t first, uint16_t second, uint16_t third, uint64_t fourth);

  uint8_t bitSize() const;
  const uint8_t *getValue() const;
  const ble_uuid_t *getBase() const;
  bool equals(const BLEUUID &uuid) const;
  std::string toString() const;
  static BLEUUID fromString(const std::string &uuid);
  const BLEUUID &to128();
  const BLEUUID &to16();
  const BLEUUID &reverseByteOrder();

  bool operator==(const BLEUUID &rhs) const;
  bool operator!=(const BLEUUID &rhs) const;
  operator std::string() const;

private:
  ble_uuid_any_t m_uuid{};
};  // BLEUUID

#endif /* CONFIG_BT_ENABLED */
#endif /* NIMBLE_CPP_UUID_H_ */
