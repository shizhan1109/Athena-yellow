/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>
#include "lv_example_anim_3.h"


#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

// static void anim_x_cb(void * var, int32_t v)
// {
//     lv_obj_set_x(var, v);
// }

// static void sw_event_cb(lv_event_t * e)
// {
//     lv_obj_t * sw = lv_event_get_target(e);
//     lv_obj_t * label = lv_event_get_user_data(e);

//     if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
//         lv_anim_t a;
//         lv_anim_init(&a);
//         lv_anim_set_var(&a, label);
//         lv_anim_set_values(&a, lv_obj_get_x(label), 100);
//         lv_anim_set_time(&a, 500);
//         lv_anim_set_exec_cb(&a, anim_x_cb);
//         lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
//         lv_anim_start(&a);
//     }
//     else {
//         lv_anim_t a;
//         lv_anim_init(&a);
//         lv_anim_set_var(&a, label);
//         lv_anim_set_values(&a, lv_obj_get_x(label), -lv_obj_get_width(label));
//         lv_anim_set_time(&a, 500);
//         lv_anim_set_exec_cb(&a, anim_x_cb);
//         lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
//         lv_anim_start(&a);
//     }

// }

// /**
//  * Start animation on an event
//  */
// void lv_example_anim_1(void)
// {
//     lv_obj_t * label = lv_label_create(lv_scr_act());
//     lv_label_set_text(label, "Hello animations!");
//     lv_obj_set_pos(label, 100, 10);


//     lv_obj_t * sw = lv_switch_create(lv_scr_act());
//     lv_obj_center(sw);
//     lv_obj_add_state(sw, LV_STATE_CHECKED);
//     lv_obj_add_event_cb(sw, sw_event_cb, LV_EVENT_VALUE_CHANGED, label);
// }



// static void anim_x_cb(void * var, int32_t v)
// {
//     lv_obj_set_x(var, v);
// }

// static void anim_size_cb(void * var, int32_t v)
// {
//     lv_obj_set_size(var, v, v);
// }

// /**
//  * Create a playback animation
//  */
// void lv_example_anim_2(void)
// {

//     lv_obj_t * obj = lv_obj_create(lv_scr_act());
//     lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_RED), 0);
//     lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);

//     lv_obj_align(obj, LV_ALIGN_LEFT_MID, 10, 0);

//     lv_anim_t a;
//     lv_anim_init(&a);
//     lv_anim_set_var(&a, obj);
//     lv_anim_set_values(&a, 10, 50);
//     lv_anim_set_time(&a, 1000);
//     lv_anim_set_playback_delay(&a, 100);
//     lv_anim_set_playback_time(&a, 300);
//     lv_anim_set_repeat_delay(&a, 500);
//     lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
//     lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

//     lv_anim_set_exec_cb(&a, anim_size_cb);
//     lv_anim_start(&a);
//     lv_anim_set_exec_cb(&a, anim_x_cb);
//     lv_anim_set_values(&a, 10, 240);
//     lv_anim_start(&a);
// }

int main(void)
{
	const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	// Create screen
	lv_obj_t *screen = lv_scr_act();

	// Set screen back ground
	lv_obj_set_style_bg_color(screen, lv_color_make(128, 222, 111), LV_PART_MAIN);

	// Add widget here
	lv_example_anim_3();

	// refreshing screen
	lv_task_handler();
	display_blanking_off(display_dev);
	while (1)
	{
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}