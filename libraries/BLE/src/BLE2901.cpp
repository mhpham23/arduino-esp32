/*
   BLE2901.h

     GATT Descriptor 0x2901 Characteristic User Description

     The value of this description is a user-readable string
     describing the characteristic.

     The Characteristic User Description descriptor
     provides a textual user description for a characteristic
     value.
     If the Writable Auxiliary bit of the Characteristics
     Properties is set then this descriptor is written. Only one
     User Description descriptor exists in a characteristic
     definition.
*/

#include "soc/soc_caps.h"
#if SOC_BLE_SUPPORTED

#include "sdkconfig.h"
#if defined(CONFIG_BLUEDROID_ENABLED) || defined(CONFIG_NIMBLE_ENABLED)

#include "BLE2901.h"
#include "BLEUUID.h"

#ifdef CONFIG_NIMBLE_ENABLED
#include "host/ble_att.h" // for Nimble
#else
#include "esp_gatt_defs.h" // for Bluedroid
#define BLE_ATT_ATTR_MAX_LEN ESP_GATT_MAX_ATTR_LEN // for Bluedroid
#endif

BLE2901::BLE2901() : BLEDescriptor(BLEUUID((uint16_t)0x2901)) {}  // BLE2901

/**
 * @brief Set the Characteristic User Description
 */
void BLE2901::setDescription(String userDesc) {
  if (userDesc.length() > BLE_ATT_ATTR_MAX_LEN) {
    log_e("Size %d too large, must be no bigger than %d", userDesc.length(), BLE_ATT_ATTR_MAX_LEN);
    return;
  }
  setValue(userDesc);
}

#endif /* CONFIG_BLUEDROID_ENABLED || CONFIG_NIMBLE_ENABLED */
#endif /* SOC_BLE_SUPPORTED */
