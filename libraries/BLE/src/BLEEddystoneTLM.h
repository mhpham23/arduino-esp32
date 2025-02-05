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

#ifndef _BLEEddystoneTLM_H_
#define _BLEEddystoneTLM_H_
#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)

class BLEUUID;

#include <string>

#define EDDYSTONE_TLM_FRAME_TYPE 0x20

/**
 * @brief Representation of a beacon.
 * See:
 * * https://github.com/google/eddystone
 */
class BLEEddystoneTLM {
public:
  struct BeaconData {
    uint8_t frameType{EDDYSTONE_TLM_FRAME_TYPE};
    uint8_t version{0};
    uint16_t volt{3300};
    uint16_t temp{23 * 256};
    uint32_t advCount{0};
    uint32_t tmil{0};
  } __attribute__((packed));

  const BeaconData getData();
  BLEUUID getUUID();
  uint8_t getVersion();
  uint16_t getVolt();
  int16_t getTemp();
  uint32_t getCount();
  uint32_t getTime();
  std::string toString();
  void setData(const uint8_t *data, uint8_t length);
  void setData(const BeaconData &data);
  void setUUID(const BLEUUID &l_uuid);
  void setVersion(uint8_t version);
  void setVolt(uint16_t volt);
  void setTemp(int16_t temp);
  void setCount(uint32_t advCount);
  void setTime(uint32_t tmil);

private:
  uint16_t beaconUUID{0xFEAA};
  BeaconData m_eddystoneData;

};  // BLEEddystoneTLM

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEEddystoneTLM.cpp
 *
 *  Created on: Mar 12, 2018
 *      Author: pcbreflux
 */

#include "BLEUUID.h"
#include <BLEAdvertisedDevice.h>

#define EDDYSTONE_TLM_FRAME_TYPE               0x20
#define ENDIAN_CHANGE_U16(x)                   ((((x) & 0xFF00) >> 8) + (((x) & 0xFF) << 8))
#define ENDIAN_CHANGE_U32(x)                   ((((x) & 0xFF000000) >> 24) + (((x) & 0x00FF0000) >> 8)) + ((((x) & 0xFF00) << 8) + (((x) & 0xFF) << 24))
#define EDDYSTONE_TEMP_U16_TO_FLOAT(tempU16)   (((int16_t)ENDIAN_CHANGE_U16(tempU16)) / 256.0f)
#define EDDYSTONE_TEMP_FLOAT_TO_U16(tempFloat) (ENDIAN_CHANGE_U16(((int)((tempFloat) * 256))))

/**
 * @brief Representation of a beacon.
 * See:
 * * https://github.com/google/eddystone
 */
class BLEEddystoneTLM {
public:
  BLEEddystoneTLM();
  BLEEddystoneTLM(BLEAdvertisedDevice *advertisedDevice);
  String getData();
  BLEUUID getUUID();
  uint8_t getVersion();
  uint16_t getVolt();
  float getTemp();
  uint16_t getRawTemp();
  uint32_t getCount();
  uint32_t getTime();
  String toString();
  void setData(String data);
  void setUUID(BLEUUID l_uuid);
  void setVersion(uint8_t version);
  void setVolt(uint16_t volt);
  void setTemp(float temp);
  void setCount(uint32_t advCount);
  void setTime(uint32_t tmil);

private:
  BLEUUID beaconUUID;
  struct {
    uint8_t frameType;
    uint8_t version;
    uint16_t volt;
    uint16_t temp;
    uint32_t advCount;
    uint32_t tmil;
  } __attribute__((packed)) m_eddystoneData;
};  // BLEEddystoneTLM

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* _BLEEddystoneTLM_H_ */
