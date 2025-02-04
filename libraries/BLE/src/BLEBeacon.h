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

#ifndef COMPONENTS_CPP_UTILS_BLEBEACON_H_
#define COMPONENTS_CPP_UTILS_BLEBEACON_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "sdkconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)

class BLEUUID;

#include <cstdint>

/**
 * @brief Representation of a beacon.
 * See:
 * * https://en.wikipedia.org/wiki/IBeacon
 */
class BLEBeacon {
public:
  struct BeaconData {
    uint16_t manufacturerId{0x4c00};
    uint8_t subType{0x02};
    uint8_t subTypeLength{0x15};
    uint8_t proximityUUID[16]{};
    uint16_t major{};
    uint16_t minor{};
    int8_t signalPower{};
  } __attribute__((packed));

  const BeaconData &getData();
  uint16_t getMajor();
  uint16_t getMinor();
  uint16_t getManufacturerId();
  BLEUUID getProximityUUID();
  int8_t getSignalPower();
  void setData(const uint8_t *data, uint8_t length);
  void setData(const BeaconData &data);
  void setMajor(uint16_t major);
  void setMinor(uint16_t minor);
  void setManufacturerId(uint16_t manufacturerId);
  void setProximityUUID(const BLEUUID &uuid);
  void setSignalPower(int8_t signalPower);

private:
  BeaconData m_beaconData;
};  // BLEBeacon

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEBeacon2.h
 *
 *  Created on: Jan 4, 2018
 *      Author: kolban
 */

#include "BLEUUID.h"
/**
 * @brief Representation of a beacon.
 * See:
 * * https://en.wikipedia.org/wiki/IBeacon
 */
class BLEBeacon {
private:
  struct {
    uint16_t manufacturerId;
    uint8_t subType;
    uint8_t subTypeLength;
    uint8_t proximityUUID[16];
    uint16_t major;
    uint16_t minor;
    int8_t signalPower;
  } __attribute__((packed)) m_beaconData;

public:
  BLEBeacon();
  String getData();
  uint16_t getMajor();
  uint16_t getMinor();
  uint16_t getManufacturerId();
  BLEUUID getProximityUUID();
  int8_t getSignalPower();
  void setData(String data);
  void setMajor(uint16_t major);
  void setMinor(uint16_t minor);
  void setManufacturerId(uint16_t manufacturerId);
  void setProximityUUID(BLEUUID uuid);
  void setSignalPower(int8_t signalPower);
};  // BLEBeacon

#endif  // CONFIG_BLUEDROID_ENABLED

#endif /* SOC_BLE_SUPPORTED */
#endif /* COMPONENTS_CPP_UTILS_BLEBEACON_H_ */
