/*
 * BLE_Scan_Whitelist demo
 *
 * Created May 16, 2021
 * Author: h2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>

int scanTimeMs = 5 * 1000;  // In milliseconds, 0 = scan forever
BLEScan *pBLEScan;

class ScanCallbacks : public BLEScanCallbacks {
  void onResult(const BLEAdvertisedDevice *advertisedDevice) override {
    Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
    /*
         * Here we add the device scanned to the whitelist based on service data but any
         * advertised data can be used for your preferred data.
         */
    if (advertisedDevice->haveServiceData()) {
      /* If this is a device with data we want to capture, add it to the whitelist */
      if (advertisedDevice->getServiceData(BLEUUID("AABB")) != "") {
        Serial.printf("Adding %s to whitelist\n", std::string(advertisedDevice->getAddress()).c_str());
        BLEDevice::whiteListAdd(advertisedDevice->getAddress());
      }
    }
  }
} scanCallbacks;

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setScanCallbacks(&scanCallbacks);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
  pBLEScan->setWindow(99);
}

void loop() {
  BLEScanResults foundDevices = pBLEScan->getResults(scanTimeMs, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

  Serial.println("Whitelist contains:");
  for (auto i = 0; i < BLEDevice::getWhiteListCount(); ++i) {
    Serial.println(BLEDevice::getWhiteListAddress(i).toString().c_str());
  }

  /*
     * If we have addresses in the whitelist enable the filter unless no devices were found
     * then scan without so we can find any new devices that we want.
     */
  if (BLEDevice::getWhiteListCount() > 0) {
    if (foundDevices.getCount() == 0) {
      pBLEScan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
    } else {
      pBLEScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
    }
  }

  pBLEScan->clearResults();
  delay(2000);
}
