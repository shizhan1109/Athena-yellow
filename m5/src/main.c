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

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

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

	// refreshing screen
	lv_task_handler();
	display_blanking_off(display_dev);
	while (1)
	{
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}

void create_hello_world_button(void)
{
	lv_obj_t *hello_world_button;
	lv_obj_t *hello_world_label;
	// Create button
	hello_world_button = lv_btn_create(lv_scr_act());
	lv_obj_align(hello_world_button, LV_ALIGN_CENTER, 0, -15);
	lv_obj_add_event_cb(hello_world_button, NULL, LV_EVENT_CLICKED,
						NULL);
	// Create label within button
	hello_world_label = lv_label_create(hello_world_button);
	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);
}

void create_slider(void)
{
	/*Create a slider in the center of the display*/
	lv_obj_t *slider = lv_slider_create(lv_scr_act());
	lv_obj_t *label;
	lv_obj_set_width(slider, 200);									 /*Set the width*/
	lv_obj_center(slider);											 /*Align to the center of the parent (screen)*/
	lv_obj_add_event_cb(slider, NULL, LV_EVENT_VALUE_CHANGED, NULL); /*Assign an event function*/

	/*Create a label above the slider*/
	label = lv_label_create(lv_scr_act());
	lv_label_set_text(label, "0");
	lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15); /*Align top of the slider*/
}