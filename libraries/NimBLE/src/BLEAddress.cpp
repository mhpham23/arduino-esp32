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

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLEAddress.h"

#include <algorithm>

#ifdef CONFIG_NIMBLE_CPP_ADDR_FMT_EXCLUDE_DELIMITER
#define NIMBLE_CPP_ADDR_DELIMITER ""
#else
#define NIMBLE_CPP_ADDR_DELIMITER ":"
#endif

#ifdef CONFIG_NIMBLE_CPP_ADDR_FMT_UPPERCASE
#define NIMBLE_CPP_ADDR_FMT "%02X%s%02X%s%02X%s%02X%s%02X%s%02X"
#else
#define NIMBLE_CPP_ADDR_FMT "%02x%s%02x%s%02x%s%02x%s%02x%s%02x"
#endif

static const char *LOG_TAG = "BLEAddress";

/*************************************************
 * NOTE: BLE address bytes are in INVERSE ORDER!
 * We will accommodate that fact in these methods.
 *************************************************/

/**
 * @brief Create an address from the native BLE representation.
 * @param [in] address The native BLE address.
 */
BLEAddress::BLEAddress(ble_addr_t address) : ble_addr_t{address} {}

/**
 * @brief Create an address from a hex string.
 *
 * A hex string is of the format:
 * ```
 * 00:00:00:00:00:00
 * ```
 * which is 17 characters in length.
 * @param [in] addr The hex string representation of the address.
 * @param [in] type The type of the address, should be one of:
 * * BLE_ADDR_PUBLIC (0)
 * * BLE_ADDR_RANDOM (1)
 */
BLEAddress::BLEAddress(const std::string &addr, uint8_t type) {
  this->type = type;

  if (addr.length() == BLE_DEV_ADDR_LEN) {
    std::reverse_copy(addr.data(), addr.data() + BLE_DEV_ADDR_LEN, this->val);
    return;
  }

  if (addr.length() == 17) {
    std::string mac{addr};
    mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
    uint64_t address = std::stoull(mac, nullptr, 16);
    memcpy(this->val, &address, sizeof(this->val));
    return;
  }

  *this = BLEAddress{};
  log_e(LOG_TAG, "Invalid address '%s'", addr.c_str());
}  // BLEAddress

/**
 * @brief Constructor for compatibility with bluedroid esp library using native ESP representation.
 * @param [in] address A uint8_t[6] or esp_bd_addr_t containing the address.
 * @param [in] type The type of the address should be one of:
 * * BLE_ADDR_PUBLIC (0)
 * * BLE_ADDR_RANDOM (1)
 */
BLEAddress::BLEAddress(const uint8_t address[BLE_DEV_ADDR_LEN], uint8_t type) {
  std::reverse_copy(address, address + BLE_DEV_ADDR_LEN, this->val);
  this->type = type;
}  // BLEAddress

/**
 * @brief Constructor for address using a hex value.\n
 * Use the same byte order, so use 0xa4c1385def16 for "a4:c1:38:5d:ef:16"
 * @param [in] address uint64_t containing the address.
 * @param [in] type The type of the address should be one of:
 * * BLE_ADDR_PUBLIC (0)
 * * BLE_ADDR_RANDOM (1)
 */
BLEAddress::BLEAddress(const uint64_t &address, uint8_t type) {
  memcpy(this->val, &address, sizeof(this->val));
  this->type = type;
}  // BLEAddress

/**
 * @brief Determine if this address equals another.
 * @param [in] otherAddress The other address to compare against.
 * @return True if the addresses are equal.
 */
bool BLEAddress::equals(const BLEAddress &otherAddress) const {
  return *this == otherAddress;
}  // equals

/**
 * @brief Get the BLE base struct of the address.
 * @return A read only reference to the BLE base struct of the address.
 */
const ble_addr_t *BLEAddress::getBase() const {
  return reinterpret_cast<const ble_addr_t *>(this);
}  // getBase

/**
 * @brief Get the address type.
 * @return The address type.
 */
uint8_t BLEAddress::getType() const {
  return this->type;
}  // getType

/**
 * @brief Get the address value.
 * @return A read only reference to the address value.
 */
const uint8_t *BLEAddress::getVal() const {
  return this->val;
}  // getVal

/**
 * @brief Determine if this address is a Resolvable Private Address.
 * @return True if the address is a RPA.
 */
bool BLEAddress::isRpa() const {
  return BLE_ADDR_IS_RPA(this);
}  // isRpa

/**
 * @brief Determine if this address is a Non-Resolvable Private Address.
 * @return True if the address is a NRPA.
 */
bool BLEAddress::isNrpa() const {
  return BLE_ADDR_IS_NRPA(this);
}  // isNrpa

/**
 * @brief Determine if this address is a Static Address.
 * @return True if the address is a Static Address.
 */
bool BLEAddress::isStatic() const {
  return BLE_ADDR_IS_STATIC(this);
}  // isStatic

/**
 * @brief Determine if this address is a Public Address.
 * @return True if the address is a Public Address.
 */
bool BLEAddress::isPublic() const {
  return this->type == BLE_ADDR_PUBLIC;
}  // isPublic

/**
 * @brief Determine if this address is a NULL Address.
 * @return True if the address is a NULL Address.
 */
bool BLEAddress::isNull() const {
  return *this == BLEAddress{};
}  // isNull

/**
 * @brief Convert a BLE address to a string.
 * @return The string representation of the address.
 * @deprecated Use std::string() operator instead.
 */
std::string BLEAddress::toString() const {
  return std::string(*this);
}  // toString

/**
 * @brief Reverse the byte order of the address.
 * @return A reference to this address.
 */
const BLEAddress &BLEAddress::reverseByteOrder() {
  std::reverse(this->val, this->val + BLE_DEV_ADDR_LEN);
  return *this;
}  // reverseByteOrder

/**
 * @brief Convenience operator to check if this address is equal to another.
 */
bool BLEAddress::operator==(const BLEAddress &rhs) const {
  if (this->type != rhs.type) {
    return false;
  }

  return memcmp(rhs.val, this->val, sizeof(this->val)) == 0;
}  // operator ==

/**
 * @brief Convenience operator to check if this address is not equal to another.
 */
bool BLEAddress::operator!=(const BLEAddress &rhs) const {
  return !this->operator==(rhs);
}  // operator !=

/**
 * @brief Convenience operator to convert this address to string representation.
 * @details This allows passing BLEAddress to functions that accept std::string and/or it's methods as a parameter.
 */
BLEAddress::operator std::string() const {
  char buffer[18];
  snprintf(
    buffer, sizeof(buffer), NIMBLE_CPP_ADDR_FMT, this->val[5], NIMBLE_CPP_ADDR_DELIMITER, this->val[4], NIMBLE_CPP_ADDR_DELIMITER, this->val[3],
    NIMBLE_CPP_ADDR_DELIMITER, this->val[2], NIMBLE_CPP_ADDR_DELIMITER, this->val[1], NIMBLE_CPP_ADDR_DELIMITER, this->val[0]
  );
  return std::string{buffer};
}  // operator std::string

/**
 * @brief Convenience operator to convert the native address representation to uint_64.
 */
BLEAddress::operator uint64_t() const {
  uint64_t address = 0;
  memcpy(&address, this->val, sizeof(this->val));
  return address;
}  // operator uint64_t

#endif
