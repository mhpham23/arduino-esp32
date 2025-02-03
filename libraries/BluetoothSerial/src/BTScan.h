/*
 * BTScan.h
 *
 *  Created on: Feb 5, 2021
 *      Author: Thomas M. (ArcticSnowSky)
 */

#ifndef __BTSCAN_H__
#define __BTSCAN_H__

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

#include <map>
#include <string>
#include <Print.h>
#include "BTAddress.h"
#include "BTAdvertisedDevice.h"

class BTAdvertisedDevice;
class BTAdvertisedDeviceSet;

class [[deprecated("BluetoothSerial/Bluedroid support will be removed in 4.0.0")]] BTScanResults {
public:
  virtual ~BTScanResults() = default;

  virtual void dump(Print *print = nullptr) = 0;
  virtual int getCount() = 0;
  virtual BTAdvertisedDevice *getDevice(int i) = 0;
};

class [[deprecated("BluetoothSerial/Bluedroid support will be removed in 4.0.0")]] BTScanResultsSet : public BTScanResults {
public:
  void dump(Print *print = nullptr);
  int getCount();
  BTAdvertisedDevice *getDevice(int i);

  bool add(BTAdvertisedDeviceSet advertisedDevice, bool unique = true);
  void clear();

  std::map<std::string, BTAdvertisedDeviceSet> m_vectorAdvertisedDevices;
};

#endif
#endif
