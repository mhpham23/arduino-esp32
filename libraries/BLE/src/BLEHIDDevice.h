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

#ifndef _BLEHIDDEVICE_H_
#define _BLEHIDDEVICE_H_

#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#define GENERIC_HID         0x03C0
#define HID_KEYBOARD        0x03C1
#define HID_MOUSE           0x03C2
#define HID_JOYSTICK        0x03C3
#define HID_GAMEPAD         0x03C4
#define HID_TABLET          0x03C5
#define HID_CARD_READER     0x03C6
#define HID_DIGITAL_PEN     0x03C7
#define HID_BARCODE         0x03C8
#define HID_BRAILLE_DISPLAY 0x03C9

#include "btconfig.h"
#if defined(CONFIG_NIMBLE_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include <stdint.h>
#include <string>

#define PNPVersionField(MajorVersion, MinorVersion, PatchVersion) ((MajorVersion << 16) & 0xFF00) | ((MinorVersion << 8) & 0x00F0) | (PatchVersion & 0x000F)

class BLEServer;
class BLEService;
class BLECharacteristic;

/**
 * @brief A model of a BLE Human Interface Device.
 */
class BLEHIDDevice {
public:
  BLEHIDDevice(BLEServer *server);

  void setReportMap(uint8_t *map, uint16_t);
  void startServices();
  bool setManufacturer(const std::string &name);
  void setPnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version);
  void setHidInfo(uint8_t country, uint8_t flags);
  void setBatteryLevel(uint8_t level, bool notify = false);
  BLECharacteristic *getBatteryLevel();
  BLECharacteristic *getReportMap();
  BLECharacteristic *getHidControl();
  BLECharacteristic *getInputReport(uint8_t reportId);
  BLECharacteristic *getOutputReport(uint8_t reportId);
  BLECharacteristic *getFeatureReport(uint8_t reportId);
  BLECharacteristic *getProtocolMode();
  BLECharacteristic *getBootInput();
  BLECharacteristic *getBootOutput();
  BLECharacteristic *getPnp();
  BLECharacteristic *getHidInfo();
  BLEService *getDeviceInfoService();
  BLEService *getHidService();
  BLEService *getBatteryService();

private:
  BLEService *m_deviceInfoSvc{nullptr};  // 0x180a
  BLEService *m_hidSvc{nullptr};         // 0x1812
  BLEService *m_batterySvc{nullptr};     // 0x180f

  BLECharacteristic *m_manufacturerChr{nullptr};  // 0x2a29
  BLECharacteristic *m_pnpChr{nullptr};           // 0x2a50
  BLECharacteristic *m_hidInfoChr{nullptr};       // 0x2a4a
  BLECharacteristic *m_reportMapChr{nullptr};     // 0x2a4b
  BLECharacteristic *m_hidControlChr{nullptr};    // 0x2a4c
  BLECharacteristic *m_protocolModeChr{nullptr};  // 0x2a4e
  BLECharacteristic *m_batteryLevelChr{nullptr};  // 0x2a19

  BLECharacteristic *locateReportCharacteristicByIdAndType(uint8_t reportId, uint8_t reportType);
};

#elif defined(CONFIG_BLUEDROID_ENABLED)

/*
 * BLEHIDDevice.h
 *
 *  Created on: Jan 03, 2018
 *      Author: chegewara
 */

#include "BLECharacteristic.h"
#include "BLEService.h"
#include "BLEDescriptor.h"
#include "BLE2902.h"
#include "HIDTypes.h"

class BLEHIDDevice {
public:
  BLEHIDDevice(BLEServer *);
  virtual ~BLEHIDDevice();

  void reportMap(uint8_t *map, uint16_t);
  void startServices();

  BLEService *deviceInfo();
  BLEService *hidService();
  BLEService *batteryService();

  BLECharacteristic *manufacturer();
  void manufacturer(String name);
  //BLECharacteristic* 	pnp();
  void pnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version);
  //BLECharacteristic*	hidInfo();
  void hidInfo(uint8_t country, uint8_t flags);
  //BLECharacteristic* 	batteryLevel();
  void setBatteryLevel(uint8_t level);

  //BLECharacteristic* 	reportMap();
  BLECharacteristic *hidControl();
  BLECharacteristic *inputReport(uint8_t reportID);
  BLECharacteristic *outputReport(uint8_t reportID);
  BLECharacteristic *featureReport(uint8_t reportID);
  BLECharacteristic *protocolMode();
  BLECharacteristic *bootInput();
  BLECharacteristic *bootOutput();

private:
  BLEService *m_deviceInfoService;   //0x180a
  BLEService *m_hidService;          //0x1812
  BLEService *m_batteryService = 0;  //0x180f

  BLECharacteristic *m_manufacturerCharacteristic;  //0x2a29
  BLECharacteristic *m_pnpCharacteristic;           //0x2a50
  BLECharacteristic *m_hidInfoCharacteristic;       //0x2a4a
  BLECharacteristic *m_reportMapCharacteristic;     //0x2a4b
  BLECharacteristic *m_hidControlCharacteristic;    //0x2a4c
  BLECharacteristic *m_protocolModeCharacteristic;  //0x2a4e
  BLECharacteristic *m_batteryLevelCharacteristic;  //0x2a19
};

#endif /* CONFIG_BLUEDROID_ENABLED */

#endif /* SOC_BLE_SUPPORTED */
#endif /* _BLEHIDDEVICE_H_ */
