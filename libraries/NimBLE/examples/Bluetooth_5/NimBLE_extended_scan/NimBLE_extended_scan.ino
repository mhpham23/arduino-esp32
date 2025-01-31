/**
 *  BLE Extended Scanner Demo:
 *
 *  Demonstrates the Bluetooth 5.x scanning capabilities of the BLE library.
 *
 *  Created: on November 28, 2024
 *      Author: H2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>
#if !CONFIG_BT_NIMBLE_EXT_ADV
#error Must enable extended advertising.
#endif

static uint32_t scanTime = 10 * 1000;  // In milliseconds, 0 = scan forever
static BLEScan::Phy scanPhy = BLEScan::Phy::SCAN_ALL;

/** Define a class to handle the callbacks when advertisements are received */
class ScanCallbacks : public BLEScanCallbacks {
  void onResult(const BLEAdvertisedDevice *advertisedDevice) {
    Serial.printf(
      "Advertised Device found: %s\n PHY1: %d\n PHY2: %d\n", advertisedDevice->toString().c_str(), advertisedDevice->getPrimaryPhy(),
      advertisedDevice->getSecondaryPhy()
    );
  }

  /** Callback to process the results of the completed scan or restart it */
  void onScanEnd(const BLEScanResults &scanResults, int reason) {
    Serial.printf("Scan Ended, reason: %d; found %d devices\n", reason, scanResults.getCount());

    /** Try Different PHY's */
    switch (scanPhy) {
      case BLEScan::Phy::SCAN_ALL:
        Serial.printf("Scanning only 1M PHY\n");
        scanPhy = BLEScan::Phy::SCAN_1M;
        break;
      case BLEScan::Phy::SCAN_1M:
        Serial.printf("Scanning only CODED PHY\n");
        scanPhy = BLEScan::Phy::SCAN_CODED;
        break;
      case BLEScan::Phy::SCAN_CODED:
        Serial.printf("Scanning all PHY's\n");
        scanPhy = BLEScan::Phy::SCAN_ALL;
        break;
    }

    BLEScan *pScan = BLEDevice::getScan();
    pScan->setPhy(scanPhy);
    pScan->start(scanTime);
  }
} scanCallbacks;

void setup() {
  Serial.printf("Starting Extended Scanner\n");

  /** Initialize BLE and set the device name */
  BLEDevice::init("BLE Extended Scanner");
  BLEScan *pScan = BLEDevice::getScan();

  /** Set the callbacks that the scanner will call on events. */
  pScan->setScanCallbacks(&scanCallbacks);

  /** Use active scanning to obtain scan response data from advertisers */
  pScan->setActiveScan(true);

  /** Set the initial PHY's to scan on, default is SCAN_ALL */
  pScan->setPhy(scanPhy);

  /** Start scanning for scanTime */
  pScan->start(scanTime);
  Serial.printf("Scanning for peripherals\n");
}

void loop() {}
