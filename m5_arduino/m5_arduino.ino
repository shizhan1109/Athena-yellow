#include <M5Core2.h>
#include <WiFi.h>
#include <TJpg_Decoder.h>

const char* ssid = "YYFH";
const char* password = "808877616";
const char* service_ip = "192.168.31.86";  // 上位机IP
const int httpPort = 40111;  // 上位机端口

WiFiClient client;

float start_time,end_time;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  M5.Lcd.drawBitmap(x, y, w, h, bitmap);
  return 1;  // 继续解码
}

void setup() {
  Serial.begin(115200);
  M5.Lcd.begin();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  if (client.connect(service_ip, httpPort)) {
    Serial.println("Connected to server");
  }

  // 初始化解码器
  TJpgDec.setJpgScale(0);
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(tft_output);
}

void loop() {
  if (client.connected()) {
    start_time=millis();
    client.write("ok");  // 向上位机发送准备好接收数据的信号
    if (client.available()) {
      uint8_t header[2];
      if (client.read(header, 2) == 2) {
        uint16_t frame_size = header[0] | (header[1] << 8);

        uint8_t* image_buf = new uint8_t[frame_size];
        size_t idx = 0;
        while (idx < frame_size) {
          if (client.available()) {
            idx += client.read(&image_buf[idx], frame_size - idx);
          }
        }

        // 检查帧结束标记
        if (idx == frame_size && image_buf[frame_size-3] == 0xAA && image_buf[frame_size-2] == 0xBB && image_buf[frame_size-1] == 0xCC) {
          // // 清除结束标记
          // image_buf[frame_size-3] = 0;
          // image_buf[frame_size-2] = 0;
          // image_buf[frame_size-1] = 0;
          
          // 显示图片
          TJpgDec.drawJpg(0, 0, image_buf, idx-3);
        }
        
        delete[] image_buf;
        end_time = millis();
        Serial.printf("f_size：%d  ",frame_size); Serial.print(1000 / (end_time - start_time), 2); Serial.println("Fps");
      }
    }
  } else {
    Serial.println("Server disconnected, reconnecting...");
    client.connect(service_ip, httpPort);
  }
}