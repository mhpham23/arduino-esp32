/**
 * BLE_Secure_Client Demo:
 *
 * This example demonstrates the secure passkey protected conenction and communication between an esp32 server and an
 * esp32 client. Please note that esp32 stores auth info in nvs memory. After a successful connection it is possible
 * that a passkey change will be ineffective. To avoid this clear the memory of the esp32's between security testings.
 * esptool.py is capable of this, example: esptool.py --port /dev/ttyUSB0 erase_flash.
 *
 *  Created: on Jan 08 2021
 *      Author: mblasee
 */

#include <Arduino.h>
#include <BLEDevice.h>

class ClientCallbacks : public BLEClientCallbacks {
  void onPassKeyEntry(BLEConnInfo &connInfo) override {
    Serial.printf("Server Passkey Entry\n");
    /**
         * This should prompt the user to enter the passkey displayed
         * on the peer device.
         */
    BLEDevice::injectPassKey(connInfo, 123456);
  }
} clientCallbacks;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client");

  BLEDevice::init("");
  BLEDevice::setPower(3);                               /** +3db */
  BLEDevice::setSecurityAuth(true, true, true);         /** bonding, MITM, BLE secure connections */
  BLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); /** passkey */
  BLEScan *pScan = BLEDevice::getScan();
  BLEScanResults results = pScan->getResults(5 * 1000);

  BLEUUID serviceUuid("ABCD");

  for (int i = 0; i < results.getCount(); i++) {
    const BLEAdvertisedDevice *device = results.getDevice(i);
    Serial.println(device->getName().c_str());

    if (device->isAdvertisingService(serviceUuid)) {
      BLEClient *pClient = BLEDevice::createClient();
      pClient->setClientCallbacks(&clientCallbacks, false);

      if (pClient->connect(&device)) {
        pClient->secureConnection();
        BLERemoteService *pService = pClient->getService(serviceUuid);
        if (pService != nullptr) {
          BLERemoteCharacteristic *pNonSecureCharacteristic = pService->getCharacteristic("1234");

          if (pNonSecureCharacteristic != nullptr) {
            // Testing to read a non secured characteristic, you should be able to read this even if you have mismatching passkeys.
            std::string value = pNonSecureCharacteristic->readValue();
            // print or do whatever you need with the value
            Serial.println(value.c_str());
          }

          BLERemoteCharacteristic *pSecureCharacteristic = pService->getCharacteristic("1235");

          if (pSecureCharacteristic != nullptr) {
            // Testing to read a secured characteristic, you should be able to read this only if you have
            // matching passkeys, otherwise you should get an error like this. E BLERemoteCharacteristic:
            // "<< readValue rc=261" This means you are trying to do something without the proper
            // permissions.
            std::string value = pSecureCharacteristic->readValue();
            // print or do whatever you need with the value
            Serial.println(value.c_str());
          }
        }
      } else {
        // failed to connect
        Serial.println("failed to connect");
      }

      BLEDevice::deleteClient(pClient);
    }
  }
}

void loop() {}
