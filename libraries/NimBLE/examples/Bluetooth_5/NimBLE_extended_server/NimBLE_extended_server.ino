/**
 *  BLE Extended Server Demo:
 *
 *  Demonstrates the Bluetooth 5.x extended advertising capabilities.
 *
 *  This demo will advertise a long data string on the CODED and 1M Phy's and
 *  starts a server allowing connection over either PHY's. It will advertise for
 *  5 seconds then sleep for 20 seconds, if a client connects it will sleep once
 *  it has disconnected then repeats.
 *
 *  Created: on April 2 2022
 *      Author: H2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>
#if !CONFIG_BT_NIMBLE_EXT_ADV
#error Must enable extended advertising.
#endif

#include "esp_sleep.h"

#define SERVICE_UUID        "ABCD"
#define CHARACTERISTIC_UUID "1234"

/** Time in milliseconds to advertise */
static uint32_t advTime = 5000;

/** Time to sleep between advertisements */
static uint32_t sleepSeconds = 20;

/** Primary PHY used for advertising, can be one of BLE_HCI_LE_PHY_1M or BLE_HCI_LE_PHY_CODED */
static uint8_t primaryPhy = BLE_HCI_LE_PHY_CODED;

/**
 *  Secondary PHY used for advertising and connecting,
 *  can be one of BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_2M or BLE_HCI_LE_PHY_CODED
 */
static uint8_t secondaryPhy = BLE_HCI_LE_PHY_1M;

/** Handler class for server events */
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, BLEConnInfo &connInfo) override {
    Serial.printf("Client connected:: %s\n", connInfo.getAddress().toString().c_str());
  }

  void onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason) override {
    Serial.printf("Client disconnected - sleeping for %" PRIu32 " seconds\n", sleepSeconds);
    esp_deep_sleep_start();
  }
} serverCallbacks;

/** Callback class to handle advertising events */
class AdvertisingCallbacks : public BLEExtAdvertisingCallbacks {
  void onStopped(BLEExtAdvertising *pAdv, int reason, uint8_t instId) override {
    /* Check the reason advertising stopped, don't sleep if client is connecting */
    Serial.printf("Advertising instance %u stopped\n", instId);
    switch (reason) {
      case 0:               Serial.printf("Client connecting\n"); return;
      case BLE_HS_ETIMEOUT: Serial.printf("Time expired - sleeping for %" PRIu32 " seconds\n", sleepSeconds); break;
      default:              break;
    }
    esp_deep_sleep_start();
  }
} advertisingCallbacks;

void setup() {
  Serial.begin(115200);

  /** Initialize BLE and set the device name */
  BLEDevice::init("Extended advertiser");

  /** Create the server and add the services/characteristics/descriptors */
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pCharacteristic->setValue("Hello World");

  /** Start the service */
  pService->start();

  /**
     *  Create an extended advertisement with the instance ID 0 and set the PHY's.
     *  Multiple instances can be added as long as the instance ID is incremented.
     */
  BLEExtAdvertisement extAdv(primaryPhy, secondaryPhy);

  /** Set the advertisement as connectable */
  extAdv.setConnectable(true);

  /** As per Bluetooth specification, extended advertising cannot be both scannable and connectable */
  extAdv.setScannable(false);  // The default is false, set here for demonstration.

  /** Extended advertising allows for 251 bytes (minus header bytes ~20) in a single advertisement or up to 1650 if chained */
  extAdv.setServiceData(
    BLEUUID(SERVICE_UUID), std::string("Extended Advertising Demo.\r\n"
                                       "Extended advertising allows for "
                                       "251 bytes of data in a single advertisement,\r\n"
                                       "or up to 1650 bytes with chaining.\r\n"
                                       "This example message is 226 bytes long "
                                       "and is using CODED_PHY for long range.")
  );

  extAdv.setCompleteServices16({BLEUUID(SERVICE_UUID)});
  extAdv.setName("Extended advertiser");

  /** When extended advertising is enabled `BLEDevice::getAdvertising` returns a pointer to `BLEExtAdvertising */
  BLEExtAdvertising *pAdvertising = BLEDevice::getAdvertising();

  /** Set the callbacks for advertising events */
  pAdvertising->setCallbacks(&advertisingCallbacks);

  /**
     *  BLEExtAdvertising::setInstanceData takes the instance ID and
     *  a reference to a `BLEExtAdvertisement` object. This sets the data
     *  that will be advertised for this instance ID, returns true if successful.
     *
     *  Note: It is safe to create the advertisement as a local variable if setInstanceData
     *  is called before exiting the code block as the data will be copied.
     */
  if (pAdvertising->setInstanceData(0, extAdv)) {
    /**
         *  BLEExtAdvertising::start takes the advertisement instance ID to start
         *  and a duration in milliseconds or a max number of advertisements to send (or both).
         */
    if (pAdvertising->start(0, advTime)) {
      Serial.printf("Started advertising\n");
    } else {
      Serial.printf("Failed to start advertising\n");
    }
  } else {
    Serial.printf("Failed to register advertisement data\n");
  }
  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000);
}

void loop() {}
