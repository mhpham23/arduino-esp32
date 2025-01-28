
/**
 *  BLE_Async_client Demo:
 *
 *  Demonstrates asynchronous client operations.
 *
 *  Created: on November 4, 2024
 *      Author: H2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>

static constexpr uint32_t scanTimeMs = 5 * 1000;

class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient *pClient) override {
    Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
  }

  void onDisconnect(BLEClient *pClient, int reason) override {
    Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
    BLEDevice::getScan()->start(scanTimeMs);
  }
} clientCallbacks;

class ScanCallbacks : public BLEScanCallbacks {
  void onResult(const BLEAdvertisedDevice *advertisedDevice) override {
    Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
    if (advertisedDevice->haveName() && advertisedDevice->getName() == "BLE-Server") {
      Serial.printf("Found Our Device\n");

      /** Async connections can be made directly in the scan callbacks */
      auto pClient = BLEDevice::getDisconnectedClient();
      if (!pClient) {
        pClient = BLEDevice::createClient(advertisedDevice->getAddress());
        if (!pClient) {
          Serial.printf("Failed to create client\n");
          return;
        }
      }

      pClient->setClientCallbacks(&clientCallbacks, false);
      if (!pClient->connect(true, true, false)) {  // delete attributes, async connect, no MTU exchange
        BLEDevice::deleteClient(pClient);
        Serial.printf("Failed to connect\n");
        return;
      }
    }
  }

  void onScanEnd(const BLEScanResults &results, int reason) override {
    Serial.printf("Scan Ended\n");
    BLEDevice::getScan()->start(scanTimeMs);
  }
} scanCallbacks;

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting BLE Async Client\n");
  BLEDevice::init("Async-Client");
  BLEDevice::setPower(3); /** +3db */

  BLEScan *pScan = BLEDevice::getScan();
  pScan->setScanCallbacks(&scanCallbacks);
  pScan->setInterval(45);
  pScan->setWindow(45);
  pScan->setActiveScan(true);
  pScan->start(scanTimeMs);
}

void loop() {
  delay(1000);
  auto pClients = BLEDevice::getConnectedClients();
  if (!pClients.size()) {
    return;
  }

  for (auto &pClient : pClients) {
    Serial.printf("%s\n", pClient->toString().c_str());
    BLEDevice::deleteClient(pClient);
  }

  BLEDevice::getScan()->start(scanTimeMs);
}
