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

#ifndef NIMBLE_CPP_ATTRIBUTE_H_
#define NIMBLE_CPP_ATTRIBUTE_H_

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && (defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL) || defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL))

#include "BLEUUID.h"

/**
 * @brief A base class for BLE attributes.
 */
class BLEAttribute {
public:
  /**
     * @brief Get the UUID of the attribute.
     * @return The UUID.
     */
  const BLEUUID &getUUID() const {
    return m_uuid;
  }

  /**
     * @brief Get the handle of the attribute.
     */
  uint16_t getHandle() const {
    return m_handle;
  };

protected:
  /**
     * @brief Construct a new BLEAttribute object.
     * @param [in] handle The handle of the attribute.
     * @param [in] uuid The UUID of the attribute.
     */
  BLEAttribute(const BLEUUID &uuid, uint16_t handle) : m_uuid{uuid}, m_handle{handle} {}

  /**
     * @brief Destroy the BLEAttribute object.
     */
  ~BLEAttribute() = default;

  const BLEUUID m_uuid{};
  uint16_t m_handle{0};
};

#endif  // CONFIG_BT_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif  // NIMBLE_CPP_ATTRIBUTE_H_
