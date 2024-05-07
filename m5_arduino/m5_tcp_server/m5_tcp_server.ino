#include <M5Core2.h>
#include <WiFi.h>
#include <TJpg_Decoder.h>

const char* ssid = "YYFH";
const char* password = "808877616";
const int serverPort = 40111;  // Server port to listen on

WiFiServer server(serverPort);  // Initialize the server on the specified port
WiFiClient client;

float start_time, end_time;

uint8_t image_buf[30000]; 

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  M5.Lcd.drawBitmap(x, y, w, h, bitmap);
  return 1;  // Continue decoding
}

void decodeImage(void* param) {
  uint8_t header[2] = {0};
  uint16_t frame_size = 0;
  size_t idx = 0;
  
  while (true) {
    // Check if a new client has connected
    if (!client || !client.connected()) {
      client = server.available();
      if (client) {
        Serial.println("New client connected");
      }
    }

    // Handle incoming data if the client is connected
    if (client.connected()) {
      start_time = millis();
      client.write("ok");  // 向上位机发送准备好接收数据的信号
      if (client.available()) {
        if (client.read(header, 2) == 2) {
          frame_size = header[0] | (header[1] << 8);

          idx = 0;
          while (idx < frame_size) {
            if (client.available()) {
              idx += client.read(&image_buf[idx], frame_size - idx);
            }
          }

          // Verify frame end marker
          if (idx == frame_size && image_buf[frame_size - 3] == 0xAA && image_buf[frame_size - 2] == 0xBB && image_buf[frame_size - 1] == 0xCC) {
            // Display the image
            TJpgDec.drawJpg(0, 0, image_buf, idx - 3);
          }

          end_time = millis();
          // Serial.printf("f_size: %d  ", frame_size);
          // Serial.print(1000 / (end_time - start_time), 2);
          // Serial.println(" Fps");
        }
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void networking(void* param) {

  //Init WiFi as Station, start SmartConfig
  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();

  //Wait for SmartConfig packet from mobile
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("SmartConfig received.");

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");

  // WiFi.begin(ssid, password);

  // // Wait for WiFi connection
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.println("Connecting to WiFi...");
  // }

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println("Port: "+String(serverPort));
  Serial.println("Connected to WiFi");

  server.begin();  // Start the server
  Serial.println("Server started and waiting for clients to connect");

  //xTaskCreatePinnedToCore(decodeImage, "ImageDecoder", 8192, nullptr, 1, nullptr, 0); //tskNO_AFFINITY

  while (true) {
    M5.update();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("The last change at %d ms /n",M5.BtnA.lastChange());
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


void setup() {
  // UART
  Serial.begin(115200);
  Serial.flush();
  delay(50);
  Serial.print("M5Core2 initializing...");

  // Lcd and rtc
  M5.Lcd.begin();
  M5.Lcd.fillScreen('#51247A');
  M5.Rtc.begin();

  // Initialize JPEG decoder
  TJpgDec.setJpgScale(0);
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(tft_output);

  xTaskCreatePinnedToCore(networking, "Networking", 8192, nullptr, 1, nullptr, 1); //tskNO_AFFINITY
}
void loop() {
  vTaskDelay(10 / portTICK_PERIOD_MS);
}