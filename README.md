# Athena‐yellow

We are team Athena‐yellow!

Our team members: Zhan Shi, Huantao Li

This is our timeline.

| time   | progress                           |
| ------ | ---------------------------------- |
| week9  | show image,   auto wifi connecting |
| week10 | Data                               |
| week11 | Data                               |
| week12 | Data                               |


## KPIs
- At least one sensor. Shaking on, shaking off. Fast screen switch.
- At least one actuator. Resolution, latency.
- Use non-trivial wireless networking. MQTT trans image.
- Auto wifi connecting, no password needed even for a encrypt wifi.
- Use of a web dashboard. Using tool Streamlit. Sync image on websit.
- Computer vision, image processing like beuring, filtering.



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


