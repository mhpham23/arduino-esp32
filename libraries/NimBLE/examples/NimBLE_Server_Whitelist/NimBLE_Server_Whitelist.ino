/*
 * BLE_Server_Whitelist demo
 *
 * Created May 17, 2021
 * Author: h2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>

BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, BLEConnInfo &connInfo) override {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason) override {
    // Peer disconnected, add them to the whitelist
    // This allows us to use the whitelist to filter connection attempts
    // which will minimize reconnection time.
    BLEDevice::whiteListAdd(connInfo.getAddress());
    deviceConnected = false;
  }
} serverCallbacks;

void onAdvComplete(BLEAdvertising *pAdvertising) {
  Serial.println("Advertising stopped");
  if (deviceConnected) {
    return;
  }
  // If advertising timed out without connection start advertising without whitelist filter
  pAdvertising->setScanFilter(false, false);
  pAdvertising->start();
}

void setup() {
  Serial.begin(115200);

  BLEDevice::init("Whitelist BLEServer");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);
  pServer->advertiseOnDisconnect(false);

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(false);
  pAdvertising->setAdvertisingCompleteCallback(onAdvComplete);
  pAdvertising->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
    value++;
  }

  if (!deviceConnected && oldDeviceConnected) {
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    if (BLEDevice::getWhiteListCount() > 0) {
      // Allow anyone to scan but only whitelisted can connect.
      pAdvertising->setScanFilter(false, true);
    }
    // advertise with whitelist for 30 seconds
    pAdvertising->start(30 * 1000);
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  delay(2000);
}
