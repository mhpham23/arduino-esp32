/*
 * BLE Scan active/passive switching demo
 *
 * Demonstrates the use of the scan callbacks while alternating between passive and active scanning.
 */

#include <Arduino.h>
#include <BLEDevice.h>

int scanTime = 5 * 1000;  // In milliseconds, 0 = scan forever
BLEScan *pBLEScan;

bool active = false;

class ScanCallbacks : public BLEScanCallbacks {
  void onDiscovered(const BLEAdvertisedDevice *advertisedDevice) override {
    Serial.printf("Discovered Advertised Device: %s \n", advertisedDevice->toString().c_str());
  }

  void onResult(const BLEAdvertisedDevice *advertisedDevice) override {
    Serial.printf("Advertised Device Result: %s \n", advertisedDevice->toString().c_str());
  }

  void onScanEnd(const BLEScanResults &results, int reason) override {
    Serial.print("Scan Ended; reason = ");
    Serial.println(reason);
    active = !active;
    pBLEScan->setActiveScan(active);
    Serial.printf("scan start, active = %u\n", active);
    pBLEScan->start(scanTime);
  }
} scanCallbacks;

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("active-passive-scan");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setScanCallbacks(&scanCallbacks);
  pBLEScan->setActiveScan(active);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);
  pBLEScan->start(scanTime);
}

void loop() {}
