/*
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <stddef.h>

//LVGL
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <lvgl_input_device.h>
#include <stdlib.h>

#include "wifi.h"
#include "server.h"

#include "tjpgd.h"
#include <stdio.h>
#include <stdlib.h>


/************************************ DEFINES **********************************/
#define SERVER_TL_TYPE SERVER_PROTO_TCP
#define SERVER_PORT 4242
#define REGISTER_SERVICE true
#define SERVER_STACK_SIZE 4000

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define SERVER_THREAD_PRIO K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define SERVER_THREAD_PRIO K_PRIO_PREEMPT(8)
#endif

/************************************* DATA ***********************************/
static k_tid_t server_tid;
static K_THREAD_STACK_DEFINE(server_stack, SERVER_STACK_SIZE);
static struct k_thread server_td;

K_MSGQ_DEFINE(image_msgq, sizeof(image_message_t), 10, 4);

/******************************* MAIN ENTRY POINT *****************************/
int main(void)
{
    wifi_iface_t *wifi = NULL;
    server_t *server = NULL;

    /* Connect to Wifi */
    wifi = wifi_get();
    wifi->connect(SSID, PSK);
    wifi->status();

    /* Create server context */
    server = server_new(SERVER_TL_TYPE, SERVER_PORT, REGISTER_SERVICE);

    /* Spawn server */
    server_tid = k_thread_create(&server_td, server_stack,
                 K_THREAD_STACK_SIZEOF(server_stack),
                 server_start, server, NULL, NULL, SERVER_THREAD_PRIO, 0,
                 K_NO_WAIT);

    /* LVGL example */
    const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	// Create screen
	lv_obj_t *screen = lv_scr_act();

	// Set screen back ground
	lv_obj_set_style_bg_color(screen, lv_color_make(128, 222, 111), LV_PART_MAIN);

    // refreshing screen
	lv_task_handler();
	display_blanking_off(display_dev);

    image_message_t received_msg;

    for (;;)
        // if (k_msgq_get(&image_msgq, &received_msg, K_FOREVER) == 0) {
        //     printk("Received image data of size %u\n", received_msg.size);
        //     IODEV dev = {
        //         .data = received_msg.data_ptr,
        //         .size = received_msg.size,
        //         .index = 0
        //     };

        //     static uint8_t pool[4000];
        //     JDEC jd;
        //     JRESULT res;

        //     res = jd_prepare(&jd, in_func, pool, sizeof(pool), &dev);
        //     if (res != JDR_OK) {
        //         printk("Failed to prepare: %d\n", res);
        //     }

        //     res = jd_decomp(&jd, out_func, 0);
        //     if (res != JDR_OK) {
        //         printk("Failed to decompress: %d\n", res);
        //     }
        //     lv_task_handler();
        // }
        lv_task_handler();
		k_sleep(K_MSEC(100));

    return 0;
}
