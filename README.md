# Athena‐yellow

We are team Athena‐yellow!

Our team members: Zhan Shi, Huantao Li

This is our timeline.

| time   | progress                           |
| ------ | ---------------------------------- |
| week9  | show image,   auto wifi connecting |
| week10 | MQTT ip addr, TCP trans one display sever, TCP trans one display client, display sjpg, show ip ,connect wifi    |
| week11 | Data                               |
| week12 | Data                               |


## KPIs

- **MotionTracking sensor**. Shaking on, shaking off. A fast way to switch on and off screen.
- **Performance**. High resolution, 960x720 pixels, and low display latency.
- **Apply MQTT**. Translate image from website to displays through MQTT.
- **Auto wifi connecting**. Connect wifi without inputting password,even for a encrypt wifi.
- **Fancy web dashboard**. Using tool Streamlit. Synchronize with uploaded image from websit.
- **Computer vision**. Integrate image processing technic like image filtering and edge detection.



## Team member’s involvement

Zhan Shi:

Huantao Li:

Our Team photo:





## logging

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