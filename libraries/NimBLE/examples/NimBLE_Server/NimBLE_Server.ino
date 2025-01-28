
/**
 *  BLE_Server Demo:
 *
 *  Demonstrates many of the available features of the BLE server library.
 *
 *  Created: on March 22 2020
 *      Author: H2zero
 */

#include <Arduino.h>
#include <BLEDevice.h>

static BLEServer *pServer;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, BLEConnInfo &connInfo) override {
    Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());

    /**
         *  We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments.
         */
    pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
  }

  void onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason) override {
    Serial.printf("Client disconnected - start advertising\n");
    BLEDevice::startAdvertising();
  }

  void onMTUChange(uint16_t MTU, BLEConnInfo &connInfo) override {
    Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
  }

  /********************* Security handled here *********************/
  uint32_t onPassKeyDisplay() override {
    Serial.printf("Server Passkey Display\n");
    /**
         * This should return a random 6 digit number for security
         *  or make your own static passkey as done here.
         */
    return 123456;
  }

  void onConfirmPassKey(BLEConnInfo &connInfo, uint32_t pass_key) override {
    Serial.printf("The passkey YES/NO number: %" PRIu32 "\n", pass_key);
    /** Inject false if passkeys don't match. */
    BLEDevice::injectConfirmPasskey(connInfo, true);
  }

  void onAuthenticationComplete(BLEConnInfo &connInfo) override {
    /** Check that encryption was successful, if not we disconnect the client */
    if (!connInfo.isEncrypted()) {
      BLEDevice::getServer()->disconnect(connInfo.getConnHandle());
      Serial.printf("Encrypt connection failed - disconnecting client\n");
      return;
    }

    Serial.printf("Secured connection to: %s\n", connInfo.getAddress().toString().c_str());
  }
} serverCallbacks;

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo) override {
    Serial.printf("%s : onRead(), value: %s\n", pCharacteristic->getUUID().toString().c_str(), pCharacteristic->getValue().c_str());
  }

  void onWrite(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo) override {
    Serial.printf("%s : onWrite(), value: %s\n", pCharacteristic->getUUID().toString().c_str(), pCharacteristic->getValue().c_str());
  }

  /**
     *  The value returned in code is the BLE host return code.
     */
  void onStatus(BLECharacteristic *pCharacteristic, int code) override {
    Serial.printf("Notification/Indication return code: %d, %s\n", code, BLEUtils::returnCodeToString(code));
  }

  /** Peer subscribed to notifications/indications */
  void onSubscribe(BLECharacteristic *pCharacteristic, BLEConnInfo &connInfo, uint16_t subValue) override {
    std::string str = "Client ID: ";
    str += connInfo.getConnHandle();
    str += " Address: ";
    str += connInfo.getAddress().toString();
    if (subValue == 0) {
      str += " Unsubscribed to ";
    } else if (subValue == 1) {
      str += " Subscribed to notifications for ";
    } else if (subValue == 2) {
      str += " Subscribed to indications for ";
    } else if (subValue == 3) {
      str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID());

    Serial.printf("%s\n", str.c_str());
  }
} chrCallbacks;

/** Handler class for descriptor actions */
class DescriptorCallbacks : public BLEDescriptorCallbacks {
  void onWrite(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo) override {
    std::string dscVal = pDescriptor->getValue();
    Serial.printf("Descriptor written value: %s\n", dscVal.c_str());
  }

  void onRead(BLEDescriptor *pDescriptor, BLEConnInfo &connInfo) override {
    Serial.printf("%s Descriptor read\n", pDescriptor->getUUID().toString().c_str());
  }
} dscCallbacks;

void setup(void) {
  Serial.begin(115200);
  Serial.printf("Starting BLE Server\n");

  /** Initialize BLE and set the device name */
  BLEDevice::init("BLE");

  /**
     * Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
  // BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
  // BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

  /**
     *  2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, BLE secure connections.
     *
     *  These are the default values, only shown here for demonstration.
     */
  // BLEDevice::setSecurityAuth(false, false, true);

  BLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  BLEService *pDeadService = pServer->createService("DEAD");
  BLECharacteristic *pBeefCharacteristic = pDeadService->createCharacteristic(
    "BEEF", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
              /** Require a secure connection for read and write access */
              NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
              NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
  );

  pBeefCharacteristic->setValue("Burger");
  pBeefCharacteristic->setCallbacks(&chrCallbacks);

  /**
     *  2902 and 2904 descriptors are a special case, when createDescriptor is called with
     *  either of those uuid's it will create the associated class with the correct properties
     *  and sizes. However we must cast the returned reference to the correct type as the method
     *  only returns a pointer to the base BLEDescriptor class.
     */
  BLE2904 *pBeef2904 = pBeefCharacteristic->create2904();
  pBeef2904->setFormat(BLE2904::FORMAT_UTF8);
  pBeef2904->setCallbacks(&dscCallbacks);

  BLEService *pBaadService = pServer->createService("BAAD");
  BLECharacteristic *pFoodCharacteristic = pBaadService->createCharacteristic("F00D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pFoodCharacteristic->setValue("Fries");
  pFoodCharacteristic->setCallbacks(&chrCallbacks);

  /** Custom descriptor: Arguments are UUID, Properties, max length of the value in bytes */
  BLEDescriptor *pC01Ddsc = pFoodCharacteristic->createDescriptor("C01D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC, 20);
  pC01Ddsc->setValue("Send it back!");
  pC01Ddsc->setCallbacks(&dscCallbacks);

  /** Start the services when finished creating all Characteristics and Descriptors */
  pDeadService->start();
  pBaadService->start();

  /** Create an advertising instance and add the services to the advertised data */
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setName("BLE-Server");
  pAdvertising->addServiceUUID(pDeadService->getUUID());
  pAdvertising->addServiceUUID(pBaadService->getUUID());
  /**
     *  If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();

  Serial.printf("Advertising Started\n");
}

void loop() {
  /** Loop here and send notifications to connected peers */
  delay(2000);

  if (pServer->getConnectedCount()) {
    BLEService *pSvc = pServer->getServiceByUUID("BAAD");
    if (pSvc) {
      BLECharacteristic *pChr = pSvc->getCharacteristic("F00D");
      if (pChr) {
        pChr->notify();
      }
    }
  }
}
