#include <M5Core2.h>
#include <WiFi.h>
#include <TJpg_Decoder.h>
#include <ESPmDNS.h>

#include <EEPROM.h>
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASSWORD_ADDR 100

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_task_wdt.h"

// #define MY_DEBUG
#define TIMEOUT_DURATION 2000

const int serverPort = 40111;  // Server port to listen on

// save wifi
const char* ssidPath = "/ssid.txt";
const char* passwordPath = "/password.txt";

// NTP RTC
RTC_TimeTypeDef TimeStruct;
RTC_DateTypeDef DateStruct;

// sleep time
uint16_t sleep_time = 0;

// FreeRTOS
TaskHandle_t shakeTaskHandle = NULL;
TaskHandle_t networkingTaskHandle = NULL;
TaskHandle_t decodeImageTaskHandle = NULL;

// read EEPROM String
String readEEPROMString(int addr) {
  String value;
  char ch;
  for (int i = addr; i < EEPROM_SIZE; ++i) {
    ch = EEPROM.read(i);
    if (ch == 0) break;
    value += ch;
  }
  return value;
}

// write EEPROM String
void writeEEPROMString(int addr, const String &value) {
  for (int i = 0; i < value.length(); ++i) {
    EEPROM.write(addr + i, value[i]);
  }
  EEPROM.write(addr + value.length(), 0);
  EEPROM.commit();
}

// save wifi credentials
void saveWiFiCredentials(const String &ssid, const String &password) {
  writeEEPROMString(SSID_ADDR, ssid);
  writeEEPROMString(PASSWORD_ADDR, password);
  Serial.println("WiFi credentials saved to EEPROM.");
}

// connect to NTP service
void update_time() {
  // NTP RTC
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 0;
  const int daylightOffset_sec = 10 * 3600;

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.printf("Failed to obtain time!!");
    return;
  }

  // Set RTC
  TimeStruct.Hours = timeinfo.tm_hour;
  TimeStruct.Minutes = timeinfo.tm_min;
  TimeStruct.Seconds = timeinfo.tm_sec;
  M5.Rtc.SetTime(&TimeStruct);

  DateStruct.WeekDay = timeinfo.tm_wday + 1;
  DateStruct.Month = timeinfo.tm_mon + 1;
  DateStruct.Date = timeinfo.tm_mday;
  DateStruct.Year = timeinfo.tm_year + 1900;
  M5.Rtc.SetDate(&DateStruct);
}

// decode jpg and display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (!bitmap || y >= M5.Lcd.height()) return 0;
  // esp_task_wdt_reset();
  M5.Lcd.drawBitmap(x, y, w, h, bitmap);
  return 1;  // Continue decoding
}

// get and decode jpg
void decodeImage(void* param) {
  WiFiServer server(serverPort);  // Initialize the server on the specified port
  WiFiClient client;

  server.begin();  // Start the server

  uint8_t image_buf[20480]; 
  uint8_t header[2] = {0};
  uint16_t frame_size = 0;
  size_t idx = 0;
  
  while (true) {
    // Check if a new client has connected
    if (!client || !client.connected()) {
      if (eTaskGetState(networkingTaskHandle) == eSuspended) {
        M5.Lcd.fillScreen(0x512f);
        sleep_time = 0;
        vTaskResume(networkingTaskHandle);
      }
      client = server.available();
      if (client) {
        M5.Lcd.wakeup();
        vTaskSuspend(shakeTaskHandle);
        vTaskSuspend(networkingTaskHandle);
        Serial.println("New client connected");
      }
    }

    // Handle incoming data if the client is connected
    if (client && client.connected()) {
      client.write("ok");
      if (client.available()) {
        unsigned long startMillis = millis();  // Store the start time
        if (client.read(header, 2) == 2) {
          frame_size = header[0] | (header[1] << 8);

          // Check buffer size
          if (frame_size > sizeof(image_buf)) {
            Serial.println("Frame size exceeds buffer capacity");
            continue; // Skip this frame
          }

          idx = 0;
          while (idx < frame_size) {
            if (client.available()) {
              idx += client.read(&image_buf[idx], frame_size - idx);
              startMillis = millis(); // Reset the start time after receiving data
            }

            // Check for timeout
            if (millis() - startMillis > TIMEOUT_DURATION) {
              Serial.println("Data receive timeout");
              break;  // Exit the loop on timeout
            }
          }

          // Verify frame Display the image
          if (idx == frame_size) {
            TJpgDec.drawJpg(0, 0, image_buf, idx);
          }
        }
      }
    }
    // esp_task_wdt_reset();
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// get BLE wifi
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String deviceName = advertisedDevice.getName().c_str();
    String deviceAddress = advertisedDevice.getAddress().toString().c_str();

    // String targetName = "ESP32_BLE";
    String targetAddress = "d8:16:ad:c5:7d:29";

    if (deviceAddress == targetAddress) {
      Serial.print("Filtered Device: ");
      Serial.print("Name: ");
      Serial.print(deviceName);
      
      Serial.print(", Address: ");
      Serial.println(deviceAddress);

      // get ssid and passw
      int delimiterIndex = deviceName.indexOf('-');
      if (delimiterIndex != -1) {
        String ssid = deviceName.substring(0, delimiterIndex);
        String password = deviceName.substring(delimiterIndex + 1);
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);

        saveWiFiCredentials(ssid, password);

        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
  }
};

// network and display info
void networking(void* param) {
  // get MAC
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String mdns = "M5AY-" + mac;
  const char* mdnsName = mdns.c_str();

  M5.Lcd.setTextColor(WHITE,0x512f);
  M5.Lcd.setTextDatum(MC_DATUM);

  M5.Lcd.drawString("M5AY", 160, 60, 4);
  M5.Lcd.drawString(mac, 160, 90, 4);

  M5.Lcd.drawString("Waiting for WiFi...", 160, 140, 2);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // read SSID passw
  String ssid = readEEPROMString(SSID_ADDR);
  String password = readEEPROMString(PASSWORD_ADDR);

  //WiFi
  WiFi.begin(ssid.c_str(), password.c_str());
  
  //Wait for WiFi to connect to AP
  uint8_t connect_time = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    connect_time++;
    if (connect_time >= 20) {
      Serial.print("Wifi connection failed");

      BLEDevice::init("");

      BLEScan* pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
      pBLEScan->setActiveScan(true);
      pBLEScan->start(30);
      
    }
  }
  
  // disp wifi 
  M5.Lcd.drawString(WiFi.SSID(), 160, 120, 2);
  M5.Lcd.drawString(WiFi.localIP().toString() + ':', 160, 140, 4);
  M5.Lcd.drawString(String(serverPort), 160, 170, 4);

  // disp time
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Rtc.GetTime(&TimeStruct);
  M5.Lcd.printf("%02d:%02d:%02d",TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds);

  // BatVoltage
  M5.Lcd.setCursor(200, 0);
  M5.Lcd.printf("Bat: %.2fV", M5.Axp.GetBatVoltage());

  // connect NTP server
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Connecting NTP server...");
  update_time();

  // mDNS
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.printf("Add ayServer to mDNS...");
  if (!MDNS.begin(mdnsName)) {
    M5.Lcd.printf("Error starting mDNS");
  }
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.printf("mDNS responder started");
  MDNS.addService("ayServer", "tcp", serverPort); // add ayServer

  M5.Lcd.setCursor(0, 220);
  M5.Lcd.printf("Start the ayServer...");
  xTaskCreatePinnedToCore(decodeImage, "ImageDecoder", 30720, NULL, 6, &decodeImageTaskHandle, 0);

  M5.Lcd.setCursor(0, 220);
  M5.Lcd.printf("                             ");
  while (true) {
    // wifi server info
    M5.Lcd.setTextColor(WHITE,0x512f);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("M5AY", 160, 60, 4);
    M5.Lcd.drawString(mac, 160, 90, 4);

    M5.Lcd.drawString(WiFi.SSID(), 160, 120, 2);
    M5.Lcd.drawString(WiFi.localIP().toString() + ':', 160, 140, 4);
    M5.Lcd.drawString(String(serverPort), 160, 170, 4);

    // time
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Rtc.GetTime(&TimeStruct);
    M5.Lcd.printf("%02d:%02d:%02d",TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds);

    // BatVoltage
    M5.Lcd.setCursor(200, 0);
    M5.Lcd.printf("Bat: %.2fV", M5.Axp.GetBatVoltage());

    // info
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.printf("Waiting to connect...");

    // button
    M5.update();
    if (M5.BtnB.wasReleasefor(700)) {
      sleep_time = 0;
      M5.Lcd.setCursor(0, 220);
      M5.Lcd.printf("Receiving broadcast...        ");
      while (true) {
        BLEDevice::init("");

        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setActiveScan(true);
        pBLEScan->start(30);
      }
    }

    // Sleep
    sleep_time++;
    if (sleep_time>=2*20) {
      M5.Lcd.sleep(); 
      vTaskResume(shakeTaskHandle);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// shake shake Lcd wakeup
void shake(void* param) {
  float accX = 0.0F;
  float accY = 0.0F;
  float accZ = 0.0F;
  const float shakeThreshold = 1.5F;
  vTaskSuspend(shakeTaskHandle);
  while (true) {
    M5.IMU.getAccelData(&accX, &accY, &accZ);
    if (abs(accX) > shakeThreshold || abs(accY) > shakeThreshold || abs(accZ) > shakeThreshold) {
        
        M5.Lcd.wakeup(); // Wake up the LCD when a shake is detected.
        sleep_time = 0;
        vTaskSuspend(shakeTaskHandle);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// setup
void setup() {
  M5.begin(true, false, true, false, kMBusModeOutput, false);
  M5.Rtc.begin();
  M5.IMU.Init();

  M5.Axp.SetLcdVoltage(2900);
  M5.Lcd.fillScreen(0x512f);

  EEPROM.begin(EEPROM_SIZE);
  
  // Initialize JPEG decoder
  TJpgDec.setJpgScale(0);
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(tft_output);

  xTaskCreatePinnedToCore(networking, "Networking", 4096, NULL, 6, &networkingTaskHandle, tskNO_AFFINITY); //tskNO_AFFINITY
  xTaskCreatePinnedToCore(shake, "Shake", 4096, NULL, 6, &shakeTaskHandle, tskNO_AFFINITY);
}

// debug
#ifdef MY_DEBUG
  void printStackUsage(TaskHandle_t taskHandle, const char* taskName) {
      UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(taskHandle);
      Serial.print("Task ");
      Serial.print(taskName);
      Serial.print(" - Stack high water mark: ");
      Serial.println(uxHighWaterMark);
  }

  void loop() {
    // Monitor stack usage for each task
    printStackUsage(networkingTaskHandle, "Networking");
    printStackUsage(shakeTaskHandle, "Shake");
    printStackUsage(decodeImageTaskHandle, "ImageDecoder");
    printStackUsage(NULL, "loop");
    printStackUsage(xTimerGetTimerDaemonTaskHandle(), "tiT");
    Serial.println();

    // Add delay to avoid rapid polling
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
  #else
  void loop() {vTaskDelay(2000 / portTICK_PERIOD_MS);} // Suspend the loop task
#endif