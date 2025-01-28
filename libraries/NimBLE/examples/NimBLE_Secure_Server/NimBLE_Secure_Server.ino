/**
 * BLE_Secure_Server Demo:
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

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server");
  BLEDevice::init("BLE");
  BLEDevice::setPower(3); /** +3db */

  BLEDevice::setSecurityAuth(true, true, true); /** bonding, MITM, BLE secure connections */
  BLEDevice::setSecurityPasskey(123456);
  BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("ABCD");
  BLECharacteristic *pNonSecureCharacteristic = pService->createCharacteristic("1234", NIMBLE_PROPERTY::READ);
  BLECharacteristic *pSecureCharacteristic =
    pService->createCharacteristic("1235", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

  pService->start();
  pNonSecureCharacteristic->setValue("Hello Non Secure BLE");
  pSecureCharacteristic->setValue("Hello Secure BLE");

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("ABCD");
  pAdvertising->start();
}

void loop() {}
