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

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED)

#include "nimble/nimble_npl.h"

#include "BLEAttValue.h"

static const char *LOG_TAG = "BLEAttValue";

// Default constructor implementation.
BLEAttValue::BLEAttValue(uint16_t init_len, uint16_t max_len)
  : m_attr_value{static_cast<uint8_t *>(calloc(init_len + 1, 1))}, m_attr_max_len{std::min<uint16_t>(BLE_ATT_ATTR_MAX_LEN, max_len)}, m_attr_len{},
    m_capacity{init_len}
#if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
    ,
    m_timestamp{}
#endif
{
  if (m_attr_value == nullptr) {
    log_e(LOG_TAG, "Failed to calloc ctx");
  }
}

// Value constructor implementation.
BLEAttValue::BLEAttValue(const uint8_t *value, uint16_t len, uint16_t max_len) : BLEAttValue(len, max_len) {
  if (m_attr_value != nullptr) {
    memcpy(m_attr_value, value, len);
    m_attr_len = len;
  }
}

// Destructor implementation.
BLEAttValue::~BLEAttValue() {
  if (m_attr_value != nullptr) {
    free(m_attr_value);
  }
}

// Move assignment operator implementation.
BLEAttValue &BLEAttValue::operator=(BLEAttValue &&source) {
  if (this != &source) {
    free(m_attr_value);
    m_attr_value = source.m_attr_value;
    m_attr_max_len = source.m_attr_max_len;
    m_attr_len = source.m_attr_len;
    m_capacity = source.m_capacity;
    setTimeStamp(source.getTimeStamp());
    source.m_attr_value = nullptr;
  }

  return *this;
}

// Copy assignment implementation.
BLEAttValue &BLEAttValue::operator=(const BLEAttValue &source) {
  if (this != &source) {
    deepCopy(source);
  }
  return *this;
}

// Copy all the data from the source object to this object, including allocated space.
void BLEAttValue::deepCopy(const BLEAttValue &source) {
  uint8_t *res = static_cast<uint8_t *>(realloc(m_attr_value, source.m_capacity + 1));
  if (res == nullptr) {
    log_e(LOG_TAG, "Failed to realloc deepCopy");
    return;
  }

  ble_npl_hw_enter_critical();
  m_attr_value = res;
  m_attr_max_len = source.m_attr_max_len;
  m_attr_len = source.m_attr_len;
  m_capacity = source.m_capacity;
  setTimeStamp(source.getTimeStamp());
  memcpy(m_attr_value, source.m_attr_value, m_attr_len + 1);
  ble_npl_hw_exit_critical(0);
}

// Set the value of the attribute.
bool BLEAttValue::setValue(const uint8_t *value, uint16_t len) {
  m_attr_len = 0;  // Just set the value length to 0 and append instead of repeating code.
  append(value, len);
  return memcmp(m_attr_value, value, len) == 0 && m_attr_len == len;
}

// Append the new data, allocate as necessary.
BLEAttValue &BLEAttValue::append(const uint8_t *value, uint16_t len) {
  if (len == 0) {
    return *this;
  }

  if ((m_attr_len + len) > m_attr_max_len) {
    log_e(LOG_TAG, "val > max, len=%u, max=%u", len, m_attr_max_len);
    return *this;
  }

  uint8_t *res = m_attr_value;
  uint16_t new_len = m_attr_len + len;
  if (new_len > m_capacity) {
    res = static_cast<uint8_t *>(realloc(m_attr_value, (new_len + 1)));
    m_capacity = new_len;
  }
  if (res == nullptr) {
    log_e(LOG_TAG, "Failed to realloc append");
    return *this;
  }

#if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
  time_t t = time(nullptr);
#else
  time_t t = 0;
#endif

  ble_npl_hw_enter_critical();
  memcpy(res + m_attr_len, value, len);
  m_attr_value = res;
  m_attr_len = new_len;
  m_attr_value[m_attr_len] = '\0';
  setTimeStamp(t);
  ble_npl_hw_exit_critical(0);

  return *this;
}

uint8_t BLEAttValue::operator[](int pos) const {
  if (pos >= m_attr_len) {
    log_e(LOG_TAG, "pos >= len, pos=%u, len=%u", pos, m_attr_len);
    return 0;
  }
  return m_attr_value[pos];
}

#endif  // CONFIG_NIMBLE_ENABLED
