#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // 初始化BLE设备
    // BLEDevice::init("lihuantao-12345678");
    BLEDevice::init("YYFH-808877616");

    // 创建BLE服务器
    BLEServer *pServer = BLEDevice::createServer();

    // 获取广告对象
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // 自定义广播包内容
    // std::string customPayload = "YYFH-808877616";

    // 设置广告数据
    BLEAdvertisementData advertisementData;
    advertisementData.setName("ESP32_BLE");  // 设置设备名称
    // advertisementData.setManufacturerData(customPayload);

    // 设置广告数据
    pAdvertising->setAdvertisementData(advertisementData);

    // 开始广告
    pAdvertising->start();
    Serial.println("Advertising started!");
}

void loop() {
    // 主循环中可以添加其他逻辑
    delay(1000);
}
