# Athena‐yellow

We are team Athena‐yellow!

Our wiki <https://github.com/shizhan1109/Athena-yellow/wiki/Index>

Our team members: Zhan Shi, Huantao Li


## Scenario Description

## Team Member List and Roles

48323626, Zhan Shi : Develop the blue board software. Set up the Bluetooth communication protocol. Coordinate software development tasks. Spearheaded the development of software for the M5 board using zephyr.

46435244, Huantao Li : Develop the m5 board software. Implement GUI features. Integrate sensor functionalities. Modify the overall design when developing m5 by zephyr. Test and debug the m5 board.

Our Team photo:

## This is our timeline.

| time   | progress                                                                          |
| ------ | --------------------------------------------------------------------------------- |
| week9  | Show image and animation on one display                                           |
| week10 | Use TCP trans from client to sever(display), Show ip on display, Connect to Wi-Fi |
| week11 | Show image and screen casting on 9 displays                                       |
| week12 | Fixing bugs and preparing demo                                                    |

## Devices

- 9 m5stack core2
- 1 nRF52840 DK(blue board)
- 1 server on PC

## KPIs

- **MotionTracking sensor**. Shaking on, shaking off. A fast way to switch on and off screen.
- **Performance**. FPS > 1 which is low because compromises for the sake of time synchronization.
- **Auto wifi connecting**. Bluetooth advertising Wi-Fi information. No need input or fix configuration.
- **GUI**. Synchronized display 9 screens states.
- **Computer vision**. Integrate image processing technic like image filtering and edge detection.(planed but not done)
- **More than image**: We achieve screen casting.



## DIKW

![image](https://github.com/shizhan1109/Athena-yellow/assets/80838084/d1e2cfdc-3a25-4ee8-a864-81aba5bf2205)

## Overview

![image](https://github.com/shizhan1109/Athena-yellow/assets/80838084/856e87d5-0ac6-4b45-af45-918d94dd5734)


## Network

![image](https://github.com/shizhan1109/Athena-yellow/assets/80838084/8670f2df-11c8-4128-be67-93b0e6b9c85e)


## Deploy

**Initialize**

1. FLash the 9 m5core2s,
2. Connect WiFi lihongtao.

**Reach network**

1. Push reset button. M5core2s shows *waiting for Wi-Fi*. In the GUI, all the M5 icons turn red.
2. Input Wi-Fi configuration `wifi csse4011demo-csse4011` in the blue board. The blue board will advertising configuration.
3. The M5core2s connect to the new Wi-Fi, and show SSID, mac addr and IP on display. 
4. The M5core2s register IP to router, the GUI collects 9 IPs through mDNS protocol. In the GUI, click **search** button, the connected m5core2s' icons turn green. Place the m5 to the corresponding location.

**Let's play**

7. In the GUI, click **run**, start screen casting.
8. Show one image in 9 displays. Shake it to turn on/off display. The 9 displays synchronize by NTP server.
9. Play an animation and even a video.

## Sensor

MotionTracking MPU-6886

We utilize gravity accelerometers along the X, Y, and Z axes! Any axe changes will trun on/off display.

![image](https://github.com/shizhan1109/Athena-yellow/assets/80838084/649c4ccb-1c05-4762-be15-e990ea7cf896)
![image](https://github.com/shizhan1109/Athena-yellow/assets/80838084/390f1c4b-f714-478d-89f5-aba7e5102c82)


## Logging

<https://docs.zephyrproject.org/latest/boards/m5stack/m5stack_core2/doc/index.html>

west build -b m5stack_core2/esp32/procpu zephyr/samples/hello_world -p

west config build.board m5stack_core2/esp32/procpu
west build zephyr/samples/hello_world
west build zephyr/samples/subsys/display/lvgl
west build zephyr/samples/sensor/mpu6050 -- -DSHIELD=m5stack_core2_ext
west build Athena-yellow/m5

west espressif monitor

examples
modules/lib/gui/lvgl/examples/widgets/img/lv_example_img_1.c

LCD IPS TFT 2”, 320x240 px screen (ILI9342C)

### blue
west build Athena-yellow/blue --pristine
west build zephyr/tests/bluetooth/shell --pristine

minicom -c on -D /dev/ttyACM0

set once
*D8:16:AD:C5:7D:29*
bt id-create D8:16:AD:C5:7D:29
bt id-show

bt adv-create conn-scan identity
bt adv-info
*csse4011demo-csse4011*
bt adv-data 1609637373653430313164656d6f2d6373736534303131
bt adv-start

`wifi csse4011demo-csse4011`


rm -rf ~/cheese/Athena-yellow/.git;cp -r ~/Documents/Athena-yellow/.git ~/cheese/Athena-yellow/
