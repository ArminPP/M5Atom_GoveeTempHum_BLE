/**
 * @file     M5Atom_GoveeBLE.cpp
 * @author   Armin P Pressler (app@gmx.at)
 * @brief    M5Atom test for reading BLE broadcast from a Govee 5075 temperature and humidity sensor
 * @version  0.1
 * @date     2023.06.13
 *
 * @copyright Copyright (c) 2023
 *
 * 
 * with help from : https://gist.github.com/tchen/65d6b29a20dd1ef01b210538143c0bf4
 *                : https://esp32.com/viewtopic.php?t=17552
 *                : BLE_scan.ino example
 */

#include <Arduino.h>
#include <M5Atom.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
BLEScan *pBLEScan;

// returns the elapsed time since startup of the ESP
void ElapsedRuntime(uint16_t &dd, byte &hh, byte &mm, byte &ss, uint16_t &ms)
{
  unsigned long now = millis();
  int nowSeconds = now / 1000;

  dd = nowSeconds / 60 / 60 / 24;
  hh = (nowSeconds / 60 / 60) % 24;
  mm = (nowSeconds / 60) % 60;
  ss = nowSeconds % 60;
  ms = now % 1000;
}

// helper to print hex values
void printHex(uint8_t num)
{
  char hexCar[3] = {'\0'};
  snprintf(hexCar, sizeof(hexCar), "%02X", num);
  Serial.print(hexCar);
}

// BLE callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {

    uint16_t dd = 0;
    byte hh = 0;
    byte ss = 0;
    byte mm = 0;
    uint16_t ms = 0;
    ElapsedRuntime(dd, hh, mm, ss, ms);

    // GVH5075_xxxx   (xxxx are the last two bytes of the MAC Address of the device)
    if (strncmp(advertisedDevice.getName().c_str(), "GVH5075_", 8) == 0)
    {
      size_t payloadLength = advertisedDevice.getPayloadLength(); // No of bytes

      // debug ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Serial.print("Payload Length (bytes):"
      // Serial.println(payloadLength);

      // Serial.print("Payload: ");
      // for (int i = 0; i < pll; i++)
      // {
      //   Serial.printf(" (%i):", i);
      //   printHex(payload[i]);
      // }
      // Serial.print("\n");

      // Serial.print("TempHum: ");
      // for (int i = 26; i < 29; i++)
      // {
      //   printHex(payload[i]);
      // }
      // Serial.print("\n");
      // debug ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

      if (payloadLength == 31) // payload length in bytes
      {
        uint8_t *payload = advertisedDevice.getPayload(); // byte array

        // Length 31  ||  0D09475648353037355F43424431030388EC02010509FF88EC000410C66400
        //                                                            FF88EC00 0410C6 64 00
        //                                                                     ^      ^
        //                                                                     |      Batt (Byte 29)
        //                                                                     Temp + Hum  (Byte 26 - Byte 28)

        uint32_t TempHum; // Temperature and Humidity is stored in 3 bytes...

        TempHum = (uint32_t)payload[26] << 16; // converts 3 bytes into int
        TempHum |= (uint32_t)payload[27] << 8; // put the result of bitwise OR
        TempHum |= (uint32_t)payload[28];      // put the result of bitwise OR

        float Temp = (float)(TempHum / 10000.0);        // extract Temperature as float
        uint8_t Hum = (uint8_t)((TempHum % 1000) / 10); // extract Humidity as integer
        uint8_t Batt = payload[29];                     // Battery is still percentage

        Serial.printf("%05d|%02i:%02i:%02i:%03i ||  Device: %s (RSSI: %i) |  Temperature: %.2f C  |  Humidity: %i %%  |  Battery: %i %%\n",
                      dd,
                      hh,
                      mm,
                      ss,
                      ms,
                      advertisedDevice.getName().c_str(),
                      advertisedDevice.getRSSI(),
                      Temp,
                      Hum,
                      Batt);
      }
      else
      {
        Serial.printf("%05d|%02i:%02i:%02i:%03i || Device: %s length [%i]advertisement...no sensor data! \n",
                      dd,
                      hh,
                      mm,
                      ss,
                      ms,
                      advertisedDevice.getName().c_str(),
                      payloadLength);
      }
    }
  }
};

void setup()
{
  M5.begin(false, true, true);
  Serial.begin(115200);
  M5.dis.drawpix(0, CRGB::Red); // red

  Serial.printf("Welcome to %s", __FILENAME__);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true /* wantDuplicates ? yes */);
  pBLEScan->setActiveScan(false); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{
  int scanTime = 10; // In seconds
  // Serial.printf("\nStarting scan...\n");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  delay(500);
}
