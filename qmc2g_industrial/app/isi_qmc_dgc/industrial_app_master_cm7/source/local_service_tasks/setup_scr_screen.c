/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: MIT
 * The auto-generated can only be used on NXP devices
 */

#include "lvgl/lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "qmc_features_config.h"

void setup_scr_screen(lv_ui *ui){

	//Write codes screen
	ui->screen = lv_obj_create(NULL);

	//Write style state: LV_STATE_DEFAULT for style_screen_main_main_default
	static lv_style_t style_screen_main_main_default;
	if (style_screen_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_main_main_default);
	else
		lv_style_init(&style_screen_main_main_default);
	lv_style_set_bg_color(&style_screen_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_opa(&style_screen_main_main_default, 255);
	lv_obj_add_style(ui->screen, &style_screen_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_warning
	ui->screen_label_warning = lv_label_create(ui->screen);
	lv_obj_set_pos(ui->screen_label_warning, 10, 20);
	lv_obj_set_size(ui->screen_label_warning, 700, 35);
	lv_label_set_text(ui->screen_label_warning, "Any interaction with this device will be logged");
	lv_label_set_long_mode(ui->screen_label_warning, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_warning, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_warning_main_main_default
	static lv_style_t style_screen_label_warning_main_main_default;
	if (style_screen_label_warning_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_warning_main_main_default);
	else
		lv_style_init(&style_screen_label_warning_main_main_default);
	lv_style_set_radius(&style_screen_label_warning_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_warning_main_main_default, lv_color_make(COLOR_MAINTENANCE));
	lv_style_set_bg_grad_color(&style_screen_label_warning_main_main_default, lv_color_make(COLOR_MAINTENANCE));
	lv_style_set_bg_grad_dir(&style_screen_label_warning_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_warning_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_warning_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_warning_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_warning_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_warning_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_warning_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_warning_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_warning_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_warning, &style_screen_label_warning_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_motor_1
	ui->screen_label_motor_1 = lv_label_create(ui->screen);
	lv_obj_set_pos(ui->screen_label_motor_1, 10, 65);
	lv_obj_set_size(ui->screen_label_motor_1, 345, 33);
	lv_label_set_text(ui->screen_label_motor_1, "Motor 1");
	lv_label_set_long_mode(ui->screen_label_motor_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_motor_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_motor_1_main_main_default
	static lv_style_t style_screen_label_motor_1_main_main_default;
	if (style_screen_label_motor_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_motor_1_main_main_default);
	else
		lv_style_init(&style_screen_label_motor_1_main_main_default);
	lv_style_set_radius(&style_screen_label_motor_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_motor_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_motor_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_motor_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_motor_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_motor_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_motor_1_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_motor_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_motor_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_motor_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_motor_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_motor_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_motor_1, &style_screen_label_motor_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_cont_motor_status_1
	ui->screen_cont_motor_status_1 = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_motor_status_1, 10, 99);
	lv_obj_set_size(ui->screen_cont_motor_status_1, 345, 454);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_motor_status_1_main_main_default
	static lv_style_t style_screen_cont_motor_status_1_main_main_default;
	if (style_screen_cont_motor_status_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_motor_status_1_main_main_default);
	else
		lv_style_init(&style_screen_cont_motor_status_1_main_main_default);
	lv_style_set_radius(&style_screen_cont_motor_status_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_motor_status_1_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_color(&style_screen_cont_motor_status_1_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_dir(&style_screen_cont_motor_status_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_motor_status_1_main_main_default, 0);
	lv_style_set_border_color(&style_screen_cont_motor_status_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_border_width(&style_screen_cont_motor_status_1_main_main_default, 2);
	lv_style_set_border_opa(&style_screen_cont_motor_status_1_main_main_default, 255);
	lv_style_set_pad_left(&style_screen_cont_motor_status_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_motor_status_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_motor_status_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_motor_status_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_motor_status_1, &style_screen_cont_motor_status_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_state_1
	ui->screen_value_state_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_state_1, 243, 5);
	lv_obj_set_size(ui->screen_value_state_1, 97, 35);
	lv_label_set_text(ui->screen_value_state_1, "default");
	lv_label_set_long_mode(ui->screen_value_state_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_state_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_state_1_main_main_default
	static lv_style_t style_screen_value_state_1_main_main_default;
	if (style_screen_value_state_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_state_1_main_main_default);
	else
		lv_style_init(&style_screen_value_state_1_main_main_default);
	lv_style_set_radius(&style_screen_value_state_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_state_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_state_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_state_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_state_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_state_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_state_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_state_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_state_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_state_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_state_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_state_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_state_1, &style_screen_value_state_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_state_1
	ui->screen_label_state_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_state_1, 5, 4);
	lv_obj_set_size(ui->screen_label_state_1, 233, 35);
	lv_label_set_text(ui->screen_label_state_1, "State:");
	lv_label_set_long_mode(ui->screen_label_state_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_state_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_state_1_main_main_default
	static lv_style_t style_screen_label_state_1_main_main_default;
	if (style_screen_label_state_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_state_1_main_main_default);
	else
		lv_style_init(&style_screen_label_state_1_main_main_default);
	lv_style_set_radius(&style_screen_label_state_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_state_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_state_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_state_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_state_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_state_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_state_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_state_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_state_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_state_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_state_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_state_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_state_1, &style_screen_label_state_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_cont_en_1
	ui->screen_label_cont_en_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_cont_en_1, 5, 42);
	lv_obj_set_size(ui->screen_label_cont_en_1, 233, 35);
	lv_label_set_text(ui->screen_label_cont_en_1, "Control Enabled:");
	lv_label_set_long_mode(ui->screen_label_cont_en_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_cont_en_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_cont_en_1_main_main_default
	static lv_style_t style_screen_label_cont_en_1_main_main_default;
	if (style_screen_label_cont_en_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_cont_en_1_main_main_default);
	else
		lv_style_init(&style_screen_label_cont_en_1_main_main_default);
	lv_style_set_radius(&style_screen_label_cont_en_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_cont_en_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_cont_en_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_cont_en_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_cont_en_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_cont_en_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_cont_en_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_cont_en_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_cont_en_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_cont_en_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_cont_en_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_cont_en_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_cont_en_1, &style_screen_label_cont_en_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_cont_en_1
	ui->screen_value_cont_en_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_cont_en_1, 243, 42);
	lv_obj_set_size(ui->screen_value_cont_en_1, 97, 35);
	lv_label_set_text(ui->screen_value_cont_en_1, "default");
	lv_label_set_long_mode(ui->screen_value_cont_en_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_cont_en_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_cont_en_1_main_main_default
	static lv_style_t style_screen_value_cont_en_1_main_main_default;
	if (style_screen_value_cont_en_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_cont_en_1_main_main_default);
	else
		lv_style_init(&style_screen_value_cont_en_1_main_main_default);
	lv_style_set_radius(&style_screen_value_cont_en_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_cont_en_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_cont_en_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_cont_en_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_cont_en_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_cont_en_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_cont_en_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_cont_en_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_cont_en_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_cont_en_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_cont_en_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_cont_en_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_cont_en_1, &style_screen_value_cont_en_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_speed_1
	ui->screen_label_speed_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_speed_1, 5, 79);
	lv_obj_set_size(ui->screen_label_speed_1, 233, 35);
	lv_label_set_text(ui->screen_label_speed_1, "Speed:");
	lv_label_set_long_mode(ui->screen_label_speed_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_speed_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_speed_1_main_main_default
	static lv_style_t style_screen_label_speed_1_main_main_default;
	if (style_screen_label_speed_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_speed_1_main_main_default);
	else
		lv_style_init(&style_screen_label_speed_1_main_main_default);
	lv_style_set_radius(&style_screen_label_speed_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_speed_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_speed_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_speed_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_speed_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_speed_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_speed_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_speed_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_speed_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_speed_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_speed_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_speed_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_speed_1, &style_screen_label_speed_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_speed_1
	ui->screen_value_speed_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_speed_1, 243, 79);
	lv_obj_set_size(ui->screen_value_speed_1, 97, 35);
	lv_label_set_text(ui->screen_value_speed_1, "default");
	lv_label_set_long_mode(ui->screen_value_speed_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_speed_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_speed_1_main_main_default
	static lv_style_t style_screen_value_speed_1_main_main_default;
	if (style_screen_value_speed_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_speed_1_main_main_default);
	else
		lv_style_init(&style_screen_value_speed_1_main_main_default);
	lv_style_set_radius(&style_screen_value_speed_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_speed_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_speed_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_speed_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_speed_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_speed_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_speed_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_speed_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_speed_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_speed_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_speed_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_speed_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_speed_1, &style_screen_value_speed_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_position_1
	ui->screen_label_position_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_position_1, 5, 116);
	lv_obj_set_size(ui->screen_label_position_1, 233, 35);
	lv_label_set_text(ui->screen_label_position_1, "Position:");
	lv_label_set_long_mode(ui->screen_label_position_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_position_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_position_1_main_main_default
	static lv_style_t style_screen_label_position_1_main_main_default;
	if (style_screen_label_position_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_position_1_main_main_default);
	else
		lv_style_init(&style_screen_label_position_1_main_main_default);
	lv_style_set_radius(&style_screen_label_position_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_position_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_position_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_position_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_position_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_position_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_position_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_position_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_position_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_position_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_position_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_position_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_position_1, &style_screen_label_position_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_position_1
	ui->screen_value_position_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_position_1, 243, 116);
	lv_obj_set_size(ui->screen_value_position_1, 97, 35);
	lv_label_set_text(ui->screen_value_position_1, "default");
	lv_label_set_long_mode(ui->screen_value_position_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_position_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_position_1_main_main_default
	static lv_style_t style_screen_value_position_1_main_main_default;
	if (style_screen_value_position_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_position_1_main_main_default);
	else
		lv_style_init(&style_screen_value_position_1_main_main_default);
	lv_style_set_radius(&style_screen_value_position_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_position_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_position_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_position_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_position_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_position_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_position_1_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_position_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_position_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_position_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_position_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_position_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_position_1, &style_screen_value_position_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_temperature_1
	ui->screen_label_temperature_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_temperature_1, 5, 153);
	lv_obj_set_size(ui->screen_label_temperature_1, 233, 35);
	lv_label_set_text(ui->screen_label_temperature_1, "Temperature (C):");
	lv_label_set_long_mode(ui->screen_label_temperature_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_temperature_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_temperature_1_main_main_default
	static lv_style_t style_screen_label_temperature_1_main_main_default;
	if (style_screen_label_temperature_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_temperature_1_main_main_default);
	else
		lv_style_init(&style_screen_label_temperature_1_main_main_default);
	lv_style_set_radius(&style_screen_label_temperature_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_temperature_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_temperature_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_temperature_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_temperature_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_temperature_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_temperature_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_temperature_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_temperature_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_temperature_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_temperature_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_temperature_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_temperature_1, &style_screen_label_temperature_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_temperature_1
	ui->screen_value_temperature_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_temperature_1, 243, 153);
	lv_obj_set_size(ui->screen_value_temperature_1, 97, 35);
	lv_label_set_text(ui->screen_value_temperature_1, "default");
	lv_label_set_long_mode(ui->screen_value_temperature_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_temperature_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_temperature_1_main_main_default
	static lv_style_t style_screen_value_temperature_1_main_main_default;
	if (style_screen_value_temperature_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_temperature_1_main_main_default);
	else
		lv_style_init(&style_screen_value_temperature_1_main_main_default);
	lv_style_set_radius(&style_screen_value_temperature_1_main_main_default, 0);
	lv_style_set_bg_color(&style_screen_value_temperature_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_temperature_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_temperature_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_temperature_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_temperature_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_temperature_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_temperature_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_temperature_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_temperature_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_temperature_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_temperature_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_temperature_1, &style_screen_value_temperature_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_fault_1
	ui->screen_label_fault_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_fault_1, 5, 190);
	lv_obj_set_size(ui->screen_label_fault_1, 233, 35);
	lv_label_set_text(ui->screen_label_fault_1, "Fault:");
	lv_label_set_long_mode(ui->screen_label_fault_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_fault_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_fault_1_main_main_default
	static lv_style_t style_screen_label_fault_1_main_main_default;
	if (style_screen_label_fault_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_fault_1_main_main_default);
	else
		lv_style_init(&style_screen_label_fault_1_main_main_default);
	lv_style_set_radius(&style_screen_label_fault_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_fault_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_fault_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_fault_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_fault_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_fault_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_fault_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_fault_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_fault_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_fault_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_fault_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_fault_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_fault_1, &style_screen_label_fault_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_fault_1
	ui->screen_value_fault_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_fault_1, 243, 190);
	lv_obj_set_size(ui->screen_value_fault_1, 97, 35);
	lv_label_set_text(ui->screen_value_fault_1, "default");
	lv_label_set_long_mode(ui->screen_value_fault_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_fault_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_fault_1_main_main_default
	static lv_style_t style_screen_value_fault_1_main_main_default;
	if (style_screen_value_fault_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_fault_1_main_main_default);
	else
		lv_style_init(&style_screen_value_fault_1_main_main_default);
	lv_style_set_radius(&style_screen_value_fault_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_fault_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_fault_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_fault_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_fault_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_fault_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_fault_1_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_fault_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_fault_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_fault_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_fault_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_fault_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_fault_1, &style_screen_value_fault_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_a_curr_1
	ui->screen_label_phase_a_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_phase_a_curr_1, 5, 227);
	lv_obj_set_size(ui->screen_label_phase_a_curr_1, 233, 35);
	lv_label_set_text(ui->screen_label_phase_a_curr_1, "Phase A Current:");
	lv_label_set_long_mode(ui->screen_label_phase_a_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_a_curr_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_a_curr_1_main_main_default
	static lv_style_t style_screen_label_phase_a_curr_1_main_main_default;
	if (style_screen_label_phase_a_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_a_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_a_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_a_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_a_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_a_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_a_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_a_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_a_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_a_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_phase_a_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_a_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_a_curr_1, &style_screen_label_phase_a_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_a_curr_1
	ui->screen_value_phase_a_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_phase_a_curr_1, 243, 227);
	lv_obj_set_size(ui->screen_value_phase_a_curr_1, 97, 35);
	lv_label_set_text(ui->screen_value_phase_a_curr_1, "default");
	lv_label_set_long_mode(ui->screen_value_phase_a_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_a_curr_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_a_curr_1_main_main_default
	static lv_style_t style_screen_value_phase_a_curr_1_main_main_default;
	if (style_screen_value_phase_a_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_a_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_a_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_a_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_a_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_a_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_a_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_a_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_a_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_a_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_phase_a_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_a_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_a_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_a_curr_1, &style_screen_value_phase_a_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_b_curr_1
	ui->screen_label_phase_b_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_phase_b_curr_1, 5, 264);
	lv_obj_set_size(ui->screen_label_phase_b_curr_1, 233, 35);
	lv_label_set_text(ui->screen_label_phase_b_curr_1, "Phase B Current:");
	lv_label_set_long_mode(ui->screen_label_phase_b_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_b_curr_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_b_curr_1_main_main_default
	static lv_style_t style_screen_label_phase_b_curr_1_main_main_default;
	if (style_screen_label_phase_b_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_b_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_b_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_b_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_b_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_b_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_b_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_b_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_b_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_b_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_phase_b_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_b_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_b_curr_1, &style_screen_label_phase_b_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_b_curr_1
	ui->screen_value_phase_b_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_phase_b_curr_1, 243, 264);
	lv_obj_set_size(ui->screen_value_phase_b_curr_1, 97, 35);
	lv_label_set_text(ui->screen_value_phase_b_curr_1, "default");
	lv_label_set_long_mode(ui->screen_value_phase_b_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_b_curr_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_b_curr_1_main_main_default
	static lv_style_t style_screen_value_phase_b_curr_1_main_main_default;
	if (style_screen_value_phase_b_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_b_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_b_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_b_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_b_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_b_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_b_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_b_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_b_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_b_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_phase_b_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_b_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_b_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_b_curr_1, &style_screen_value_phase_b_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_dc_bus_v_1
	ui->screen_label_dc_bus_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_dc_bus_v_1, 5, 412);
	lv_obj_set_size(ui->screen_label_dc_bus_v_1, 233, 35);
	lv_label_set_text(ui->screen_label_dc_bus_v_1, "DC Bus Voltage:");
	lv_label_set_long_mode(ui->screen_label_dc_bus_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_dc_bus_v_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_dc_bus_v_1_main_main_default
	static lv_style_t style_screen_label_dc_bus_v_1_main_main_default;
	if (style_screen_label_dc_bus_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_dc_bus_v_1_main_main_default);
	else
		lv_style_init(&style_screen_label_dc_bus_v_1_main_main_default);
	lv_style_set_radius(&style_screen_label_dc_bus_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_dc_bus_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_dc_bus_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_dc_bus_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_dc_bus_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_dc_bus_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_dc_bus_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_dc_bus_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_dc_bus_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_dc_bus_v_1, &style_screen_label_dc_bus_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_c_curr_1
	ui->screen_label_phase_c_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_phase_c_curr_1, 5, 301);
	lv_obj_set_size(ui->screen_label_phase_c_curr_1, 233, 35);
	lv_label_set_text(ui->screen_label_phase_c_curr_1, "Phase C Current: ");
	lv_label_set_long_mode(ui->screen_label_phase_c_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_c_curr_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_c_curr_1_main_main_default
	static lv_style_t style_screen_label_phase_c_curr_1_main_main_default;
	if (style_screen_label_phase_c_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_c_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_c_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_c_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_c_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_c_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_c_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_c_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_c_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_c_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_phase_c_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_c_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_c_curr_1, &style_screen_label_phase_c_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_c_curr_1
	ui->screen_value_phase_c_curr_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_phase_c_curr_1, 243, 301);
	lv_obj_set_size(ui->screen_value_phase_c_curr_1, 97, 35);
	lv_label_set_text(ui->screen_value_phase_c_curr_1, "default");
	lv_label_set_long_mode(ui->screen_value_phase_c_curr_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_c_curr_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_c_curr_1_main_main_default
	static lv_style_t style_screen_value_phase_c_curr_1_main_main_default;
	if (style_screen_value_phase_c_curr_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_c_curr_1_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_c_curr_1_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_c_curr_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_c_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_c_curr_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_c_curr_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_c_curr_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_c_curr_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_c_curr_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_phase_c_curr_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_c_curr_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_c_curr_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_c_curr_1, &style_screen_value_phase_c_curr_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_alpha_v_1
	ui->screen_label_alpha_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_alpha_v_1, 5, 338);
	lv_obj_set_size(ui->screen_label_alpha_v_1, 233, 35);
	lv_label_set_text(ui->screen_label_alpha_v_1, "Alpha Voltage:");
	lv_label_set_long_mode(ui->screen_label_alpha_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_alpha_v_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_alpha_v_1_main_main_default
	static lv_style_t style_screen_label_alpha_v_1_main_main_default;
	if (style_screen_label_alpha_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_alpha_v_1_main_main_default);
	else
		lv_style_init(&style_screen_label_alpha_v_1_main_main_default);
	lv_style_set_radius(&style_screen_label_alpha_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_alpha_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_alpha_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_alpha_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_alpha_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_alpha_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_alpha_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_alpha_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_alpha_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_alpha_v_1, &style_screen_label_alpha_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_alpha_v_1
	ui->screen_value_alpha_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_alpha_v_1, 243, 338);
	lv_obj_set_size(ui->screen_value_alpha_v_1, 97, 35);
	lv_label_set_text(ui->screen_value_alpha_v_1, "default");
	lv_label_set_long_mode(ui->screen_value_alpha_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_alpha_v_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_alpha_v_1_main_main_default
	static lv_style_t style_screen_value_alpha_v_1_main_main_default;
	if (style_screen_value_alpha_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_alpha_v_1_main_main_default);
	else
		lv_style_init(&style_screen_value_alpha_v_1_main_main_default);
	lv_style_set_radius(&style_screen_value_alpha_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_alpha_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_alpha_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_alpha_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_alpha_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_alpha_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_alpha_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_alpha_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_alpha_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_alpha_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_alpha_v_1, &style_screen_value_alpha_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_beta_v_1
	ui->screen_label_beta_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_label_beta_v_1, 5, 375);
	lv_obj_set_size(ui->screen_label_beta_v_1, 233, 35);
	lv_label_set_text(ui->screen_label_beta_v_1, "Beta Voltage:");
	lv_label_set_long_mode(ui->screen_label_beta_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_beta_v_1, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_beta_v_1_main_main_default
	static lv_style_t style_screen_label_beta_v_1_main_main_default;
	if (style_screen_label_beta_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_beta_v_1_main_main_default);
	else
		lv_style_init(&style_screen_label_beta_v_1_main_main_default);
	lv_style_set_radius(&style_screen_label_beta_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_beta_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_beta_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_beta_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_beta_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_beta_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_beta_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_label_beta_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_beta_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_beta_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_beta_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_beta_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_beta_v_1, &style_screen_label_beta_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_beta_v_1
	ui->screen_value_beta_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_beta_v_1, 243, 375);
	lv_obj_set_size(ui->screen_value_beta_v_1, 97, 35);
	lv_label_set_text(ui->screen_value_beta_v_1, "default");
	lv_label_set_long_mode(ui->screen_value_beta_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_beta_v_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_beta_v_1_main_main_default
	static lv_style_t style_screen_value_beta_v_1_main_main_default;
	if (style_screen_value_beta_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_beta_v_1_main_main_default);
	else
		lv_style_init(&style_screen_value_beta_v_1_main_main_default);
	lv_style_set_radius(&style_screen_value_beta_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_beta_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_beta_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_beta_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_beta_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_beta_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_beta_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_beta_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_beta_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_beta_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_beta_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_beta_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_beta_v_1, &style_screen_value_beta_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_dc_bus_v_1
	ui->screen_value_dc_bus_v_1 = lv_label_create(ui->screen_cont_motor_status_1);
	lv_obj_set_pos(ui->screen_value_dc_bus_v_1, 243, 412);
	lv_obj_set_size(ui->screen_value_dc_bus_v_1, 97, 35);
	lv_label_set_text(ui->screen_value_dc_bus_v_1, "default");
	lv_label_set_long_mode(ui->screen_value_dc_bus_v_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_dc_bus_v_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_dc_bus_v_1_main_main_default
	static lv_style_t style_screen_value_dc_bus_v_1_main_main_default;
	if (style_screen_value_dc_bus_v_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_dc_bus_v_1_main_main_default);
	else
		lv_style_init(&style_screen_value_dc_bus_v_1_main_main_default);
	lv_style_set_radius(&style_screen_value_dc_bus_v_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_dc_bus_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_dc_bus_v_1_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_dc_bus_v_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_dc_bus_v_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_dc_bus_v_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_dc_bus_v_1_main_main_default, &lv_font_arial_26);
	lv_style_set_text_letter_space(&style_screen_value_dc_bus_v_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_dc_bus_v_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_dc_bus_v_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_dc_bus_v_1, &style_screen_value_dc_bus_v_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_motor_2
	ui->screen_label_motor_2 = lv_label_create(ui->screen);
	lv_obj_set_pos(ui->screen_label_motor_2, 365, 65);
	lv_obj_set_size(ui->screen_label_motor_2, 345, 33);
	lv_label_set_text(ui->screen_label_motor_2, "Motor 2");
	lv_label_set_long_mode(ui->screen_label_motor_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_motor_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_motor_2_main_main_default
	static lv_style_t style_screen_label_motor_2_main_main_default;
	if (style_screen_label_motor_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_motor_2_main_main_default);
	else
		lv_style_init(&style_screen_label_motor_2_main_main_default);
	lv_style_set_radius(&style_screen_label_motor_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_motor_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_motor_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_motor_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_motor_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_motor_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_motor_2_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_motor_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_motor_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_motor_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_motor_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_motor_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_motor_2, &style_screen_label_motor_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_cont_motor_status_2
	ui->screen_cont_motor_status_2 = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_motor_status_2, 365, 99);
	lv_obj_set_size(ui->screen_cont_motor_status_2, 345, 454);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_motor_status_2_main_main_default
	static lv_style_t style_screen_cont_motor_status_2_main_main_default;
	if (style_screen_cont_motor_status_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_motor_status_2_main_main_default);
	else
		lv_style_init(&style_screen_cont_motor_status_2_main_main_default);
	lv_style_set_radius(&style_screen_cont_motor_status_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_motor_status_2_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_color(&style_screen_cont_motor_status_2_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_dir(&style_screen_cont_motor_status_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_motor_status_2_main_main_default, 0);
	lv_style_set_border_color(&style_screen_cont_motor_status_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_border_width(&style_screen_cont_motor_status_2_main_main_default, 2);
	lv_style_set_border_opa(&style_screen_cont_motor_status_2_main_main_default, 255);
	lv_style_set_pad_left(&style_screen_cont_motor_status_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_motor_status_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_motor_status_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_motor_status_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_motor_status_2, &style_screen_cont_motor_status_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_state_2
	ui->screen_label_state_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_state_2, 5, 5);
	lv_obj_set_size(ui->screen_label_state_2, 233, 35);
	lv_label_set_text(ui->screen_label_state_2, "State:");
	lv_label_set_long_mode(ui->screen_label_state_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_state_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_state_2_main_main_default
	static lv_style_t style_screen_label_state_2_main_main_default;
	if (style_screen_label_state_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_state_2_main_main_default);
	else
		lv_style_init(&style_screen_label_state_2_main_main_default);
	lv_style_set_radius(&style_screen_label_state_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_state_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_state_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_state_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_state_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_state_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_state_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_state_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_state_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_state_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_state_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_state_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_state_2, &style_screen_label_state_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_state_2
	ui->screen_value_state_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_state_2, 243, 5);
	lv_obj_set_size(ui->screen_value_state_2, 97, 35);
	lv_label_set_text(ui->screen_value_state_2, "default");
	lv_label_set_long_mode(ui->screen_value_state_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_state_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_state_2_main_main_default
	static lv_style_t style_screen_value_state_2_main_main_default;
	if (style_screen_value_state_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_state_2_main_main_default);
	else
		lv_style_init(&style_screen_value_state_2_main_main_default);
	lv_style_set_radius(&style_screen_value_state_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_state_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_state_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_state_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_state_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_state_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_state_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_state_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_state_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_state_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_state_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_state_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_state_2, &style_screen_value_state_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_cont_en_2
	ui->screen_label_cont_en_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_cont_en_2, 5, 42);
	lv_obj_set_size(ui->screen_label_cont_en_2, 233, 35);
	lv_label_set_text(ui->screen_label_cont_en_2, "Control Enabled:");
	lv_label_set_long_mode(ui->screen_label_cont_en_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_cont_en_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_cont_en_2_main_main_default
	static lv_style_t style_screen_label_cont_en_2_main_main_default;
	if (style_screen_label_cont_en_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_cont_en_2_main_main_default);
	else
		lv_style_init(&style_screen_label_cont_en_2_main_main_default);
	lv_style_set_radius(&style_screen_label_cont_en_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_cont_en_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_cont_en_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_cont_en_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_cont_en_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_cont_en_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_cont_en_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_cont_en_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_cont_en_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_cont_en_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_cont_en_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_cont_en_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_cont_en_2, &style_screen_label_cont_en_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_cont_en_2
	ui->screen_value_cont_en_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_cont_en_2, 243, 42);
	lv_obj_set_size(ui->screen_value_cont_en_2, 97, 35);
	lv_label_set_text(ui->screen_value_cont_en_2, "default");
	lv_label_set_long_mode(ui->screen_value_cont_en_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_cont_en_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_cont_en_2_main_main_default
	static lv_style_t style_screen_value_cont_en_2_main_main_default;
	if (style_screen_value_cont_en_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_cont_en_2_main_main_default);
	else
		lv_style_init(&style_screen_value_cont_en_2_main_main_default);
	lv_style_set_radius(&style_screen_value_cont_en_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_cont_en_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_cont_en_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_cont_en_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_cont_en_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_cont_en_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_cont_en_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_cont_en_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_cont_en_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_cont_en_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_cont_en_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_cont_en_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_cont_en_2, &style_screen_value_cont_en_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_speed_2
	ui->screen_label_speed_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_speed_2, 5, 79);
	lv_obj_set_size(ui->screen_label_speed_2, 233, 35);
	lv_label_set_text(ui->screen_label_speed_2, "Speed:");
	lv_label_set_long_mode(ui->screen_label_speed_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_speed_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_speed_2_main_main_default
	static lv_style_t style_screen_label_speed_2_main_main_default;
	if (style_screen_label_speed_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_speed_2_main_main_default);
	else
		lv_style_init(&style_screen_label_speed_2_main_main_default);
	lv_style_set_radius(&style_screen_label_speed_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_speed_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_speed_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_speed_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_speed_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_speed_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_speed_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_speed_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_speed_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_speed_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_speed_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_speed_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_speed_2, &style_screen_label_speed_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_speed_2
	ui->screen_value_speed_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_speed_2, 243, 79);
	lv_obj_set_size(ui->screen_value_speed_2, 97, 35);
	lv_label_set_text(ui->screen_value_speed_2, "default");
	lv_label_set_long_mode(ui->screen_value_speed_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_speed_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_speed_2_main_main_default
	static lv_style_t style_screen_value_speed_2_main_main_default;
	if (style_screen_value_speed_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_speed_2_main_main_default);
	else
		lv_style_init(&style_screen_value_speed_2_main_main_default);
	lv_style_set_radius(&style_screen_value_speed_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_speed_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_speed_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_speed_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_speed_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_speed_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_speed_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_speed_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_speed_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_speed_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_speed_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_speed_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_speed_2, &style_screen_value_speed_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_position_2
	ui->screen_label_position_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_position_2, 5, 116);
	lv_obj_set_size(ui->screen_label_position_2, 233, 35);
	lv_label_set_text(ui->screen_label_position_2, "Position:");
	lv_label_set_long_mode(ui->screen_label_position_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_position_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_position_2_main_main_default
	static lv_style_t style_screen_label_position_2_main_main_default;
	if (style_screen_label_position_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_position_2_main_main_default);
	else
		lv_style_init(&style_screen_label_position_2_main_main_default);
	lv_style_set_radius(&style_screen_label_position_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_position_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_position_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_position_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_position_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_position_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_position_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_position_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_position_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_position_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_position_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_position_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_position_2, &style_screen_label_position_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_position_2
	ui->screen_value_position_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_position_2, 243, 116);
	lv_obj_set_size(ui->screen_value_position_2, 97, 35);
	lv_label_set_text(ui->screen_value_position_2, "default");
	lv_label_set_long_mode(ui->screen_value_position_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_position_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_position_2_main_main_default
	static lv_style_t style_screen_value_position_2_main_main_default;
	if (style_screen_value_position_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_position_2_main_main_default);
	else
		lv_style_init(&style_screen_value_position_2_main_main_default);
	lv_style_set_radius(&style_screen_value_position_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_position_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_position_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_position_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_position_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_position_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_position_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_position_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_position_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_position_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_position_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_position_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_position_2, &style_screen_value_position_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_temperature_2
	ui->screen_label_temperature_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_temperature_2, 5, 153);
	lv_obj_set_size(ui->screen_label_temperature_2, 233, 35);
	lv_label_set_text(ui->screen_label_temperature_2, "Temperature (C):");
	lv_label_set_long_mode(ui->screen_label_temperature_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_temperature_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_temperature_2_main_main_default
	static lv_style_t style_screen_label_temperature_2_main_main_default;
	if (style_screen_label_temperature_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_temperature_2_main_main_default);
	else
		lv_style_init(&style_screen_label_temperature_2_main_main_default);
	lv_style_set_radius(&style_screen_label_temperature_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_temperature_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_temperature_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_temperature_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_temperature_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_temperature_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_temperature_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_temperature_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_temperature_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_temperature_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_temperature_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_temperature_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_temperature_2, &style_screen_label_temperature_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_temperature_2
	ui->screen_value_temperature_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_temperature_2, 243, 153);
	lv_obj_set_size(ui->screen_value_temperature_2, 97, 35);
	lv_label_set_text(ui->screen_value_temperature_2, "default");
	lv_label_set_long_mode(ui->screen_value_temperature_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_temperature_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_temperature_2_main_main_default
	static lv_style_t style_screen_value_temperature_2_main_main_default;
	if (style_screen_value_temperature_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_temperature_2_main_main_default);
	else
		lv_style_init(&style_screen_value_temperature_2_main_main_default);
	lv_style_set_radius(&style_screen_value_temperature_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_temperature_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_temperature_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_temperature_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_temperature_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_temperature_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_temperature_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_temperature_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_temperature_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_temperature_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_temperature_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_temperature_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_temperature_2, &style_screen_value_temperature_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_fault_2
	ui->screen_label_fault_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_fault_2, 5, 190);
	lv_obj_set_size(ui->screen_label_fault_2, 233, 35);
	lv_label_set_text(ui->screen_label_fault_2, "Fault:");
	lv_label_set_long_mode(ui->screen_label_fault_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_fault_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_fault_2_main_main_default
	static lv_style_t style_screen_label_fault_2_main_main_default;
	if (style_screen_label_fault_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_fault_2_main_main_default);
	else
		lv_style_init(&style_screen_label_fault_2_main_main_default);
	lv_style_set_radius(&style_screen_label_fault_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_fault_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_fault_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_fault_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_fault_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_fault_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_fault_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_fault_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_fault_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_fault_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_fault_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_fault_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_fault_2, &style_screen_label_fault_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_fault_2
	ui->screen_value_fault_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_fault_2, 243, 190);
	lv_obj_set_size(ui->screen_value_fault_2, 97, 35);
	lv_label_set_text(ui->screen_value_fault_2, "default");
	lv_label_set_long_mode(ui->screen_value_fault_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_fault_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_fault_2_main_main_default
	static lv_style_t style_screen_value_fault_2_main_main_default;
	if (style_screen_value_fault_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_fault_2_main_main_default);
	else
		lv_style_init(&style_screen_value_fault_2_main_main_default);
	lv_style_set_radius(&style_screen_value_fault_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_fault_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_fault_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_fault_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_fault_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_fault_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_fault_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_fault_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_fault_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_fault_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_fault_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_fault_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_fault_2, &style_screen_value_fault_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_a_curr_2
	ui->screen_label_phase_a_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_phase_a_curr_2, 5, 227);
	lv_obj_set_size(ui->screen_label_phase_a_curr_2, 233, 35);
	lv_label_set_text(ui->screen_label_phase_a_curr_2, "Phase A Current:");
	lv_label_set_long_mode(ui->screen_label_phase_a_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_a_curr_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_a_curr_2_main_main_default
	static lv_style_t style_screen_label_phase_a_curr_2_main_main_default;
	if (style_screen_label_phase_a_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_a_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_a_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_a_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_a_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_a_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_a_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_a_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_a_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_a_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_a_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_a_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_a_curr_2, &style_screen_label_phase_a_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_a_curr_2
	ui->screen_value_phase_a_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_phase_a_curr_2, 243, 227);
	lv_obj_set_size(ui->screen_value_phase_a_curr_2, 97, 35);
	lv_label_set_text(ui->screen_value_phase_a_curr_2, "default");
	lv_label_set_long_mode(ui->screen_value_phase_a_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_a_curr_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_a_curr_2_main_main_default
	static lv_style_t style_screen_value_phase_a_curr_2_main_main_default;
	if (style_screen_value_phase_a_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_a_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_a_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_a_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_a_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_a_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_a_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_a_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_a_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_a_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_a_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_a_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_a_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_a_curr_2, &style_screen_value_phase_a_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_b_curr_2
	ui->screen_label_phase_b_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_phase_b_curr_2, 5, 264);
	lv_obj_set_size(ui->screen_label_phase_b_curr_2, 233, 35);
	lv_label_set_text(ui->screen_label_phase_b_curr_2, "Phase B Current:");
	lv_label_set_long_mode(ui->screen_label_phase_b_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_b_curr_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_b_curr_2_main_main_default
	static lv_style_t style_screen_label_phase_b_curr_2_main_main_default;
	if (style_screen_label_phase_b_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_b_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_b_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_b_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_b_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_b_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_b_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_b_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_b_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_b_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_b_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_b_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_b_curr_2, &style_screen_label_phase_b_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_b_curr_2
	ui->screen_value_phase_b_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_phase_b_curr_2, 243, 264);
	lv_obj_set_size(ui->screen_value_phase_b_curr_2, 97, 35);
	lv_label_set_text(ui->screen_value_phase_b_curr_2, "default");
	lv_label_set_long_mode(ui->screen_value_phase_b_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_b_curr_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_b_curr_2_main_main_default
	static lv_style_t style_screen_value_phase_b_curr_2_main_main_default;
	if (style_screen_value_phase_b_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_b_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_b_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_b_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_b_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_b_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_b_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_b_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_b_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_b_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_b_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_b_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_b_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_b_curr_2, &style_screen_value_phase_b_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_dc_bus_v_2
	ui->screen_label_dc_bus_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_dc_bus_v_2, 5, 412);
	lv_obj_set_size(ui->screen_label_dc_bus_v_2, 233, 35);
	lv_label_set_text(ui->screen_label_dc_bus_v_2, "DC Bus Voltage:");
	lv_label_set_long_mode(ui->screen_label_dc_bus_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_dc_bus_v_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_dc_bus_v_2_main_main_default
	static lv_style_t style_screen_label_dc_bus_v_2_main_main_default;
	if (style_screen_label_dc_bus_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_dc_bus_v_2_main_main_default);
	else
		lv_style_init(&style_screen_label_dc_bus_v_2_main_main_default);
	lv_style_set_radius(&style_screen_label_dc_bus_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_dc_bus_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_dc_bus_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_dc_bus_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_dc_bus_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_dc_bus_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_dc_bus_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_dc_bus_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_dc_bus_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_dc_bus_v_2, &style_screen_label_dc_bus_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_c_curr_2
	ui->screen_label_phase_c_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_phase_c_curr_2, 5, 301);
	lv_obj_set_size(ui->screen_label_phase_c_curr_2, 233, 35);
	lv_label_set_text(ui->screen_label_phase_c_curr_2, "Phase C Current: ");
	lv_label_set_long_mode(ui->screen_label_phase_c_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_c_curr_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_c_curr_2_main_main_default
	static lv_style_t style_screen_label_phase_c_curr_2_main_main_default;
	if (style_screen_label_phase_c_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_c_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_c_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_c_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_c_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_c_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_c_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_c_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_c_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_c_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_c_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_c_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_c_curr_2, &style_screen_label_phase_c_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_c_curr_2
	ui->screen_value_phase_c_curr_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_phase_c_curr_2, 243, 301);
	lv_obj_set_size(ui->screen_value_phase_c_curr_2, 97, 35);
	lv_label_set_text(ui->screen_value_phase_c_curr_2, "default");
	lv_label_set_long_mode(ui->screen_value_phase_c_curr_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_c_curr_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_c_curr_2_main_main_default
	static lv_style_t style_screen_value_phase_c_curr_2_main_main_default;
	if (style_screen_value_phase_c_curr_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_c_curr_2_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_c_curr_2_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_c_curr_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_c_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_c_curr_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_c_curr_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_c_curr_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_c_curr_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_c_curr_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_c_curr_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_c_curr_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_c_curr_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_c_curr_2, &style_screen_value_phase_c_curr_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_alpha_v_2
	ui->screen_label_alpha_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_alpha_v_2, 5, 338);
	lv_obj_set_size(ui->screen_label_alpha_v_2, 233, 35);
	lv_label_set_text(ui->screen_label_alpha_v_2, "Alpha Voltage:");
	lv_label_set_long_mode(ui->screen_label_alpha_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_alpha_v_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_alpha_v_2_main_main_default
	static lv_style_t style_screen_label_alpha_v_2_main_main_default;
	if (style_screen_label_alpha_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_alpha_v_2_main_main_default);
	else
		lv_style_init(&style_screen_label_alpha_v_2_main_main_default);
	lv_style_set_radius(&style_screen_label_alpha_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_alpha_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_alpha_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_alpha_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_alpha_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_alpha_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_alpha_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_alpha_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_alpha_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_alpha_v_2, &style_screen_label_alpha_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_alpha_v_2
	ui->screen_value_alpha_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_alpha_v_2, 243, 338);
	lv_obj_set_size(ui->screen_value_alpha_v_2, 97, 35);
	lv_label_set_text(ui->screen_value_alpha_v_2, "default");
	lv_label_set_long_mode(ui->screen_value_alpha_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_alpha_v_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_alpha_v_2_main_main_default
	static lv_style_t style_screen_value_alpha_v_2_main_main_default;
	if (style_screen_value_alpha_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_alpha_v_2_main_main_default);
	else
		lv_style_init(&style_screen_value_alpha_v_2_main_main_default);
	lv_style_set_radius(&style_screen_value_alpha_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_alpha_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_alpha_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_alpha_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_alpha_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_alpha_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_alpha_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_alpha_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_alpha_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_alpha_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_alpha_v_2, &style_screen_value_alpha_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_beta_v_2
	ui->screen_label_beta_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_label_beta_v_2, 5, 375);
	lv_obj_set_size(ui->screen_label_beta_v_2, 233, 35);
	lv_label_set_text(ui->screen_label_beta_v_2, "Beta Voltage:");
	lv_label_set_long_mode(ui->screen_label_beta_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_beta_v_2, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_beta_v_2_main_main_default
	static lv_style_t style_screen_label_beta_v_2_main_main_default;
	if (style_screen_label_beta_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_beta_v_2_main_main_default);
	else
		lv_style_init(&style_screen_label_beta_v_2_main_main_default);
	lv_style_set_radius(&style_screen_label_beta_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_beta_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_beta_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_beta_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_beta_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_beta_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_beta_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_beta_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_beta_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_beta_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_beta_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_beta_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_beta_v_2, &style_screen_label_beta_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_beta_v_2
	ui->screen_value_beta_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_beta_v_2, 243, 375);
	lv_obj_set_size(ui->screen_value_beta_v_2, 97, 35);
	lv_label_set_text(ui->screen_value_beta_v_2, "default");
	lv_label_set_long_mode(ui->screen_value_beta_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_beta_v_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_beta_v_2_main_main_default
	static lv_style_t style_screen_value_beta_v_2_main_main_default;
	if (style_screen_value_beta_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_beta_v_2_main_main_default);
	else
		lv_style_init(&style_screen_value_beta_v_2_main_main_default);
	lv_style_set_radius(&style_screen_value_beta_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_beta_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_beta_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_beta_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_beta_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_beta_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_beta_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_beta_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_beta_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_beta_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_beta_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_beta_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_beta_v_2, &style_screen_value_beta_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_dc_bus_v_2
	ui->screen_value_dc_bus_v_2 = lv_label_create(ui->screen_cont_motor_status_2);
	lv_obj_set_pos(ui->screen_value_dc_bus_v_2, 243, 412);
	lv_obj_set_size(ui->screen_value_dc_bus_v_2, 97, 35);
	lv_label_set_text(ui->screen_value_dc_bus_v_2, "default");
	lv_label_set_long_mode(ui->screen_value_dc_bus_v_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_dc_bus_v_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_dc_bus_v_2_main_main_default
	static lv_style_t style_screen_value_dc_bus_v_2_main_main_default;
	if (style_screen_value_dc_bus_v_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_dc_bus_v_2_main_main_default);
	else
		lv_style_init(&style_screen_value_dc_bus_v_2_main_main_default);
	lv_style_set_radius(&style_screen_value_dc_bus_v_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_dc_bus_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_dc_bus_v_2_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_dc_bus_v_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_dc_bus_v_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_dc_bus_v_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_dc_bus_v_2_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_dc_bus_v_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_dc_bus_v_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_dc_bus_v_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_dc_bus_v_2, &style_screen_value_dc_bus_v_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_motor_3
	ui->screen_label_motor_3 = lv_label_create(ui->screen);
	lv_obj_set_pos(ui->screen_label_motor_3, 10, 563);
	lv_obj_set_size(ui->screen_label_motor_3, 345, 33);
	lv_label_set_text(ui->screen_label_motor_3, "Motor 3");
	lv_label_set_long_mode(ui->screen_label_motor_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_motor_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_motor_3_main_main_default
	static lv_style_t style_screen_label_motor_3_main_main_default;
	if (style_screen_label_motor_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_motor_3_main_main_default);
	else
		lv_style_init(&style_screen_label_motor_3_main_main_default);
	lv_style_set_radius(&style_screen_label_motor_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_motor_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_motor_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_motor_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_motor_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_motor_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_motor_3_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_motor_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_motor_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_motor_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_motor_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_motor_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_motor_3, &style_screen_label_motor_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_cont_motor_status_3
	ui->screen_cont_motor_status_3 = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_motor_status_3, 10, 597);
	lv_obj_set_size(ui->screen_cont_motor_status_3, 345, 454);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_motor_status_3_main_main_default
	static lv_style_t style_screen_cont_motor_status_3_main_main_default;
	if (style_screen_cont_motor_status_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_motor_status_3_main_main_default);
	else
		lv_style_init(&style_screen_cont_motor_status_3_main_main_default);
	lv_style_set_radius(&style_screen_cont_motor_status_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_motor_status_3_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_color(&style_screen_cont_motor_status_3_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_dir(&style_screen_cont_motor_status_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_motor_status_3_main_main_default, 0);
	lv_style_set_border_color(&style_screen_cont_motor_status_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_border_width(&style_screen_cont_motor_status_3_main_main_default, 2);
	lv_style_set_border_opa(&style_screen_cont_motor_status_3_main_main_default, 255);
	lv_style_set_pad_left(&style_screen_cont_motor_status_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_motor_status_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_motor_status_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_motor_status_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_motor_status_3, &style_screen_cont_motor_status_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_state_3
	ui->screen_label_state_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_state_3, 5, 4);
	lv_obj_set_size(ui->screen_label_state_3, 233, 35);
	lv_label_set_text(ui->screen_label_state_3, "State:");
	lv_label_set_long_mode(ui->screen_label_state_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_state_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_state_3_main_main_default
	static lv_style_t style_screen_label_state_3_main_main_default;
	if (style_screen_label_state_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_state_3_main_main_default);
	else
		lv_style_init(&style_screen_label_state_3_main_main_default);
	lv_style_set_radius(&style_screen_label_state_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_state_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_state_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_state_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_state_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_state_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_state_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_state_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_state_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_state_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_state_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_state_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_state_3, &style_screen_label_state_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_state_3
	ui->screen_value_state_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_state_3, 243, 5);
	lv_obj_set_size(ui->screen_value_state_3, 97, 35);
	lv_label_set_text(ui->screen_value_state_3, "default");
	lv_label_set_long_mode(ui->screen_value_state_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_state_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_state_3_main_main_default
	static lv_style_t style_screen_value_state_3_main_main_default;
	if (style_screen_value_state_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_state_3_main_main_default);
	else
		lv_style_init(&style_screen_value_state_3_main_main_default);
	lv_style_set_radius(&style_screen_value_state_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_state_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_state_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_state_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_state_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_state_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_state_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_state_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_state_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_state_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_state_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_state_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_state_3, &style_screen_value_state_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_cont_en_3
	ui->screen_label_cont_en_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_cont_en_3, 5, 42);
	lv_obj_set_size(ui->screen_label_cont_en_3, 233, 35);
	lv_label_set_text(ui->screen_label_cont_en_3, "Control Enabled:");
	lv_label_set_long_mode(ui->screen_label_cont_en_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_cont_en_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_cont_en_3_main_main_default
	static lv_style_t style_screen_label_cont_en_3_main_main_default;
	if (style_screen_label_cont_en_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_cont_en_3_main_main_default);
	else
		lv_style_init(&style_screen_label_cont_en_3_main_main_default);
	lv_style_set_radius(&style_screen_label_cont_en_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_cont_en_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_cont_en_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_cont_en_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_cont_en_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_cont_en_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_cont_en_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_cont_en_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_cont_en_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_cont_en_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_cont_en_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_cont_en_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_cont_en_3, &style_screen_label_cont_en_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_cont_en_3
	ui->screen_value_cont_en_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_cont_en_3, 243, 42);
	lv_obj_set_size(ui->screen_value_cont_en_3, 97, 35);
	lv_label_set_text(ui->screen_value_cont_en_3, "default");
	lv_label_set_long_mode(ui->screen_value_cont_en_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_cont_en_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_cont_en_3_main_main_default
	static lv_style_t style_screen_value_cont_en_3_main_main_default;
	if (style_screen_value_cont_en_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_cont_en_3_main_main_default);
	else
		lv_style_init(&style_screen_value_cont_en_3_main_main_default);
	lv_style_set_radius(&style_screen_value_cont_en_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_cont_en_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_cont_en_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_cont_en_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_cont_en_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_cont_en_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_cont_en_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_cont_en_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_cont_en_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_cont_en_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_cont_en_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_cont_en_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_cont_en_3, &style_screen_value_cont_en_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_speed_3
	ui->screen_label_speed_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_speed_3, 5, 79);
	lv_obj_set_size(ui->screen_label_speed_3, 233, 35);
	lv_label_set_text(ui->screen_label_speed_3, "Speed:");
	lv_label_set_long_mode(ui->screen_label_speed_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_speed_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_speed_3_main_main_default
	static lv_style_t style_screen_label_speed_3_main_main_default;
	if (style_screen_label_speed_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_speed_3_main_main_default);
	else
		lv_style_init(&style_screen_label_speed_3_main_main_default);
	lv_style_set_radius(&style_screen_label_speed_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_speed_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_speed_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_speed_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_speed_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_speed_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_speed_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_speed_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_speed_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_speed_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_speed_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_speed_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_speed_3, &style_screen_label_speed_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_speed_3
	ui->screen_value_speed_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_speed_3, 243, 79);
	lv_obj_set_size(ui->screen_value_speed_3, 97, 35);
	lv_label_set_text(ui->screen_value_speed_3, "default");
	lv_label_set_long_mode(ui->screen_value_speed_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_speed_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_speed_3_main_main_default
	static lv_style_t style_screen_value_speed_3_main_main_default;
	if (style_screen_value_speed_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_speed_3_main_main_default);
	else
		lv_style_init(&style_screen_value_speed_3_main_main_default);
	lv_style_set_radius(&style_screen_value_speed_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_speed_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_speed_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_speed_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_speed_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_speed_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_speed_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_speed_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_speed_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_speed_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_speed_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_speed_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_speed_3, &style_screen_value_speed_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_position_3
	ui->screen_label_position_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_position_3, 5, 116);
	lv_obj_set_size(ui->screen_label_position_3, 233, 35);
	lv_label_set_text(ui->screen_label_position_3, "Position:");
	lv_label_set_long_mode(ui->screen_label_position_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_position_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_position_3_main_main_default
	static lv_style_t style_screen_label_position_3_main_main_default;
	if (style_screen_label_position_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_position_3_main_main_default);
	else
		lv_style_init(&style_screen_label_position_3_main_main_default);
	lv_style_set_radius(&style_screen_label_position_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_position_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_position_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_position_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_position_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_position_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_position_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_position_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_position_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_position_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_position_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_position_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_position_3, &style_screen_label_position_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_position_3
	ui->screen_value_position_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_position_3, 243, 116);
	lv_obj_set_size(ui->screen_value_position_3, 97, 35);
	lv_label_set_text(ui->screen_value_position_3, "default");
	lv_label_set_long_mode(ui->screen_value_position_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_position_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_position_3_main_main_default
	static lv_style_t style_screen_value_position_3_main_main_default;
	if (style_screen_value_position_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_position_3_main_main_default);
	else
		lv_style_init(&style_screen_value_position_3_main_main_default);
	lv_style_set_radius(&style_screen_value_position_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_position_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_position_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_position_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_position_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_position_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_position_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_position_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_position_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_position_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_position_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_position_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_position_3, &style_screen_value_position_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_temperature_3
	ui->screen_label_temperature_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_temperature_3, 5, 153);
	lv_obj_set_size(ui->screen_label_temperature_3, 233, 35);
	lv_label_set_text(ui->screen_label_temperature_3, "Temperature (C):");
	lv_label_set_long_mode(ui->screen_label_temperature_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_temperature_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_temperature_3_main_main_default
	static lv_style_t style_screen_label_temperature_3_main_main_default;
	if (style_screen_label_temperature_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_temperature_3_main_main_default);
	else
		lv_style_init(&style_screen_label_temperature_3_main_main_default);
	lv_style_set_radius(&style_screen_label_temperature_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_temperature_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_temperature_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_temperature_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_temperature_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_temperature_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_temperature_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_temperature_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_temperature_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_temperature_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_temperature_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_temperature_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_temperature_3, &style_screen_label_temperature_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_temperature_3
	ui->screen_value_temperature_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_temperature_3, 243, 153);
	lv_obj_set_size(ui->screen_value_temperature_3, 97, 35);
	lv_label_set_text(ui->screen_value_temperature_3, "default");
	lv_label_set_long_mode(ui->screen_value_temperature_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_temperature_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_temperature_3_main_main_default
	static lv_style_t style_screen_value_temperature_3_main_main_default;
	if (style_screen_value_temperature_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_temperature_3_main_main_default);
	else
		lv_style_init(&style_screen_value_temperature_3_main_main_default);
	lv_style_set_radius(&style_screen_value_temperature_3_main_main_default, 0);
	lv_style_set_bg_color(&style_screen_value_temperature_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_temperature_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_temperature_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_temperature_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_temperature_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_temperature_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_temperature_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_temperature_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_temperature_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_temperature_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_temperature_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_temperature_3, &style_screen_value_temperature_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_fault_3
	ui->screen_label_fault_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_fault_3, 5, 190);
	lv_obj_set_size(ui->screen_label_fault_3, 233, 35);
	lv_label_set_text(ui->screen_label_fault_3, "Fault:");
	lv_label_set_long_mode(ui->screen_label_fault_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_fault_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_fault_3_main_main_default
	static lv_style_t style_screen_label_fault_3_main_main_default;
	if (style_screen_label_fault_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_fault_3_main_main_default);
	else
		lv_style_init(&style_screen_label_fault_3_main_main_default);
	lv_style_set_radius(&style_screen_label_fault_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_fault_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_fault_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_fault_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_fault_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_fault_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_fault_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_fault_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_fault_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_fault_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_fault_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_fault_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_fault_3, &style_screen_label_fault_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_fault_3
	ui->screen_value_fault_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_fault_3, 243, 190);
	lv_obj_set_size(ui->screen_value_fault_3, 97, 35);
	lv_label_set_text(ui->screen_value_fault_3, "default");
	lv_label_set_long_mode(ui->screen_value_fault_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_fault_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_fault_3_main_main_default
	static lv_style_t style_screen_value_fault_3_main_main_default;
	if (style_screen_value_fault_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_fault_3_main_main_default);
	else
		lv_style_init(&style_screen_value_fault_3_main_main_default);
	lv_style_set_radius(&style_screen_value_fault_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_fault_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_fault_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_fault_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_fault_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_fault_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_fault_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_fault_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_fault_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_fault_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_fault_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_fault_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_fault_3, &style_screen_value_fault_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_a_curr_3
	ui->screen_label_phase_a_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_phase_a_curr_3, 5, 227);
	lv_obj_set_size(ui->screen_label_phase_a_curr_3, 233, 35);
	lv_label_set_text(ui->screen_label_phase_a_curr_3, "Phase A Current:");
	lv_label_set_long_mode(ui->screen_label_phase_a_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_a_curr_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_a_curr_3_main_main_default
	static lv_style_t style_screen_label_phase_a_curr_3_main_main_default;
	if (style_screen_label_phase_a_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_a_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_a_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_a_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_a_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_a_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_a_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_a_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_a_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_a_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_a_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_a_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_a_curr_3, &style_screen_label_phase_a_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_a_curr_3
	ui->screen_value_phase_a_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_phase_a_curr_3, 243, 227);
	lv_obj_set_size(ui->screen_value_phase_a_curr_3, 97, 35);
	lv_label_set_text(ui->screen_value_phase_a_curr_3, "default");
	lv_label_set_long_mode(ui->screen_value_phase_a_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_a_curr_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_a_curr_3_main_main_default
	static lv_style_t style_screen_value_phase_a_curr_3_main_main_default;
	if (style_screen_value_phase_a_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_a_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_a_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_a_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_a_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_a_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_a_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_a_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_a_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_a_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_a_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_a_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_a_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_a_curr_3, &style_screen_value_phase_a_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_b_curr_3
	ui->screen_label_phase_b_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_phase_b_curr_3, 5, 264);
	lv_obj_set_size(ui->screen_label_phase_b_curr_3, 233, 35);
	lv_label_set_text(ui->screen_label_phase_b_curr_3, "Phase B Current:");
	lv_label_set_long_mode(ui->screen_label_phase_b_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_b_curr_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_b_curr_3_main_main_default
	static lv_style_t style_screen_label_phase_b_curr_3_main_main_default;
	if (style_screen_label_phase_b_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_b_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_b_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_b_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_b_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_b_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_b_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_b_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_b_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_b_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_b_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_b_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_b_curr_3, &style_screen_label_phase_b_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_b_curr_3
	ui->screen_value_phase_b_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_phase_b_curr_3, 243, 264);
	lv_obj_set_size(ui->screen_value_phase_b_curr_3, 97, 35);
	lv_label_set_text(ui->screen_value_phase_b_curr_3, "default");
	lv_label_set_long_mode(ui->screen_value_phase_b_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_b_curr_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_b_curr_3_main_main_default
	static lv_style_t style_screen_value_phase_b_curr_3_main_main_default;
	if (style_screen_value_phase_b_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_b_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_b_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_b_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_b_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_b_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_b_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_b_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_b_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_b_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_b_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_b_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_b_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_b_curr_3, &style_screen_value_phase_b_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_c_curr_3
	ui->screen_label_phase_c_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_phase_c_curr_3, 5, 301);
	lv_obj_set_size(ui->screen_label_phase_c_curr_3, 233, 35);
	lv_label_set_text(ui->screen_label_phase_c_curr_3, "Phase C Current: ");
	lv_label_set_long_mode(ui->screen_label_phase_c_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_c_curr_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_c_curr_3_main_main_default
	static lv_style_t style_screen_label_phase_c_curr_3_main_main_default;
	if (style_screen_label_phase_c_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_c_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_c_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_c_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_c_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_c_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_c_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_c_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_c_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_c_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_c_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_c_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_c_curr_3, &style_screen_label_phase_c_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_c_curr_3
	ui->screen_value_phase_c_curr_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_phase_c_curr_3, 243, 301);
	lv_obj_set_size(ui->screen_value_phase_c_curr_3, 97, 35);
	lv_label_set_text(ui->screen_value_phase_c_curr_3, "default");
	lv_label_set_long_mode(ui->screen_value_phase_c_curr_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_c_curr_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_c_curr_3_main_main_default
	static lv_style_t style_screen_value_phase_c_curr_3_main_main_default;
	if (style_screen_value_phase_c_curr_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_c_curr_3_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_c_curr_3_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_c_curr_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_c_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_c_curr_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_c_curr_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_c_curr_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_c_curr_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_c_curr_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_c_curr_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_c_curr_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_c_curr_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_c_curr_3, &style_screen_value_phase_c_curr_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_alpha_v_3
	ui->screen_label_alpha_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_alpha_v_3, 5, 338);
	lv_obj_set_size(ui->screen_label_alpha_v_3, 233, 35);
	lv_label_set_text(ui->screen_label_alpha_v_3, "Alpha Voltage:");
	lv_label_set_long_mode(ui->screen_label_alpha_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_alpha_v_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_alpha_v_3_main_main_default
	static lv_style_t style_screen_label_alpha_v_3_main_main_default;
	if (style_screen_label_alpha_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_alpha_v_3_main_main_default);
	else
		lv_style_init(&style_screen_label_alpha_v_3_main_main_default);
	lv_style_set_radius(&style_screen_label_alpha_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_alpha_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_alpha_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_alpha_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_alpha_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_alpha_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_alpha_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_alpha_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_alpha_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_alpha_v_3, &style_screen_label_alpha_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_alpha_v_3
	ui->screen_value_alpha_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_alpha_v_3, 243, 338);
	lv_obj_set_size(ui->screen_value_alpha_v_3, 97, 35);
	lv_label_set_text(ui->screen_value_alpha_v_3, "default");
	lv_label_set_long_mode(ui->screen_value_alpha_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_alpha_v_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_alpha_v_3_main_main_default
	static lv_style_t style_screen_value_alpha_v_3_main_main_default;
	if (style_screen_value_alpha_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_alpha_v_3_main_main_default);
	else
		lv_style_init(&style_screen_value_alpha_v_3_main_main_default);
	lv_style_set_radius(&style_screen_value_alpha_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_alpha_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_alpha_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_alpha_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_alpha_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_alpha_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_alpha_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_alpha_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_alpha_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_alpha_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_alpha_v_3, &style_screen_value_alpha_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_beta_v_3
	ui->screen_label_beta_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_beta_v_3, 5, 375);
	lv_obj_set_size(ui->screen_label_beta_v_3, 233, 35);
	lv_label_set_text(ui->screen_label_beta_v_3, "Beta Voltage:");
	lv_label_set_long_mode(ui->screen_label_beta_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_beta_v_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_beta_v_3_main_main_default
	static lv_style_t style_screen_label_beta_v_3_main_main_default;
	if (style_screen_label_beta_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_beta_v_3_main_main_default);
	else
		lv_style_init(&style_screen_label_beta_v_3_main_main_default);
	lv_style_set_radius(&style_screen_label_beta_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_beta_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_beta_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_beta_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_beta_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_beta_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_beta_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_beta_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_beta_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_beta_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_beta_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_beta_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_beta_v_3, &style_screen_label_beta_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_beta_v_3
	ui->screen_value_beta_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_beta_v_3, 243, 375);
	lv_obj_set_size(ui->screen_value_beta_v_3, 97, 35);
	lv_label_set_text(ui->screen_value_beta_v_3, "default");
	lv_label_set_long_mode(ui->screen_value_beta_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_beta_v_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_beta_v_3_main_main_default
	static lv_style_t style_screen_value_beta_v_3_main_main_default;
	if (style_screen_value_beta_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_beta_v_3_main_main_default);
	else
		lv_style_init(&style_screen_value_beta_v_3_main_main_default);
	lv_style_set_radius(&style_screen_value_beta_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_beta_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_beta_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_beta_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_beta_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_beta_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_beta_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_beta_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_beta_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_beta_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_beta_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_beta_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_beta_v_3, &style_screen_value_beta_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_dc_bus_v_3
	ui->screen_label_dc_bus_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_label_dc_bus_v_3, 5, 412);
	lv_obj_set_size(ui->screen_label_dc_bus_v_3, 233, 35);
	lv_label_set_text(ui->screen_label_dc_bus_v_3, "DC Bus Voltage:");
	lv_label_set_long_mode(ui->screen_label_dc_bus_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_dc_bus_v_3, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_dc_bus_v_3_main_main_default
	static lv_style_t style_screen_label_dc_bus_v_3_main_main_default;
	if (style_screen_label_dc_bus_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_dc_bus_v_3_main_main_default);
	else
		lv_style_init(&style_screen_label_dc_bus_v_3_main_main_default);
	lv_style_set_radius(&style_screen_label_dc_bus_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_dc_bus_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_dc_bus_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_dc_bus_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_dc_bus_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_dc_bus_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_dc_bus_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_dc_bus_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_dc_bus_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_dc_bus_v_3, &style_screen_label_dc_bus_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_dc_bus_v_3
	ui->screen_value_dc_bus_v_3 = lv_label_create(ui->screen_cont_motor_status_3);
	lv_obj_set_pos(ui->screen_value_dc_bus_v_3, 243, 412);
	lv_obj_set_size(ui->screen_value_dc_bus_v_3, 97, 35);
	lv_label_set_text(ui->screen_value_dc_bus_v_3, "default");
	lv_label_set_long_mode(ui->screen_value_dc_bus_v_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_dc_bus_v_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_dc_bus_v_3_main_main_default
	static lv_style_t style_screen_value_dc_bus_v_3_main_main_default;
	if (style_screen_value_dc_bus_v_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_dc_bus_v_3_main_main_default);
	else
		lv_style_init(&style_screen_value_dc_bus_v_3_main_main_default);
	lv_style_set_radius(&style_screen_value_dc_bus_v_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_dc_bus_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_dc_bus_v_3_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_dc_bus_v_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_dc_bus_v_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_dc_bus_v_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_dc_bus_v_3_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_dc_bus_v_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_dc_bus_v_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_dc_bus_v_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_dc_bus_v_3, &style_screen_value_dc_bus_v_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_motor_4
	ui->screen_label_motor_4 = lv_label_create(ui->screen);
	lv_obj_set_pos(ui->screen_label_motor_4, 365, 563);
	lv_obj_set_size(ui->screen_label_motor_4, 345, 33);
	lv_label_set_text(ui->screen_label_motor_4, "Motor 4");
	lv_label_set_long_mode(ui->screen_label_motor_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_motor_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_motor_4_main_main_default
	static lv_style_t style_screen_label_motor_4_main_main_default;
	if (style_screen_label_motor_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_motor_4_main_main_default);
	else
		lv_style_init(&style_screen_label_motor_4_main_main_default);
	lv_style_set_radius(&style_screen_label_motor_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_motor_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_motor_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_motor_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_motor_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_motor_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_motor_4_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_motor_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_motor_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_motor_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_motor_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_motor_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_motor_4, &style_screen_label_motor_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_cont_motor_status_4
	ui->screen_cont_motor_status_4 = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_motor_status_4, 365, 597);
	lv_obj_set_size(ui->screen_cont_motor_status_4, 345, 454);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_motor_status_4_main_main_default
	static lv_style_t style_screen_cont_motor_status_4_main_main_default;
	if (style_screen_cont_motor_status_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_motor_status_4_main_main_default);
	else
		lv_style_init(&style_screen_cont_motor_status_4_main_main_default);
	lv_style_set_radius(&style_screen_cont_motor_status_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_motor_status_4_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_color(&style_screen_cont_motor_status_4_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_dir(&style_screen_cont_motor_status_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_motor_status_4_main_main_default, 0);
	lv_style_set_border_color(&style_screen_cont_motor_status_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_border_width(&style_screen_cont_motor_status_4_main_main_default, 2);
	lv_style_set_border_opa(&style_screen_cont_motor_status_4_main_main_default, 255);
	lv_style_set_pad_left(&style_screen_cont_motor_status_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_motor_status_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_motor_status_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_motor_status_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_motor_status_4, &style_screen_cont_motor_status_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_state_4
	ui->screen_label_state_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_state_4, 5, 4);
	lv_obj_set_size(ui->screen_label_state_4, 233, 35);
	lv_label_set_text(ui->screen_label_state_4, "State:");
	lv_label_set_long_mode(ui->screen_label_state_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_state_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_state_4_main_main_default
	static lv_style_t style_screen_label_state_4_main_main_default;
	if (style_screen_label_state_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_state_4_main_main_default);
	else
		lv_style_init(&style_screen_label_state_4_main_main_default);
	lv_style_set_radius(&style_screen_label_state_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_state_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_state_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_state_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_state_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_state_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_state_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_state_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_state_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_state_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_state_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_state_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_state_4, &style_screen_label_state_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_state_4
	ui->screen_value_state_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_state_4, 243, 5);
	lv_obj_set_size(ui->screen_value_state_4, 97, 35);
	lv_label_set_text(ui->screen_value_state_4, "default");
	lv_label_set_long_mode(ui->screen_value_state_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_state_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_state_4_main_main_default
	static lv_style_t style_screen_value_state_4_main_main_default;
	if (style_screen_value_state_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_state_4_main_main_default);
	else
		lv_style_init(&style_screen_value_state_4_main_main_default);
	lv_style_set_radius(&style_screen_value_state_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_state_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_state_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_state_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_state_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_state_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_state_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_state_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_state_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_state_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_state_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_state_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_state_4, &style_screen_value_state_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_cont_en_4
	ui->screen_label_cont_en_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_cont_en_4, 5, 42);
	lv_obj_set_size(ui->screen_label_cont_en_4, 233, 35);
	lv_label_set_text(ui->screen_label_cont_en_4, "Control Enabled:");
	lv_label_set_long_mode(ui->screen_label_cont_en_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_cont_en_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_cont_en_4_main_main_default
	static lv_style_t style_screen_label_cont_en_4_main_main_default;
	if (style_screen_label_cont_en_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_cont_en_4_main_main_default);
	else
		lv_style_init(&style_screen_label_cont_en_4_main_main_default);
	lv_style_set_radius(&style_screen_label_cont_en_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_cont_en_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_cont_en_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_cont_en_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_cont_en_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_cont_en_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_cont_en_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_cont_en_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_cont_en_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_cont_en_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_cont_en_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_cont_en_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_cont_en_4, &style_screen_label_cont_en_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_cont_en_4
	ui->screen_value_cont_en_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_cont_en_4, 243, 42);
	lv_obj_set_size(ui->screen_value_cont_en_4, 97, 35);
	lv_label_set_text(ui->screen_value_cont_en_4, "default");
	lv_label_set_long_mode(ui->screen_value_cont_en_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_cont_en_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_cont_en_4_main_main_default
	static lv_style_t style_screen_value_cont_en_4_main_main_default;
	if (style_screen_value_cont_en_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_cont_en_4_main_main_default);
	else
		lv_style_init(&style_screen_value_cont_en_4_main_main_default);
	lv_style_set_radius(&style_screen_value_cont_en_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_cont_en_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_cont_en_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_cont_en_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_cont_en_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_cont_en_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_cont_en_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_cont_en_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_cont_en_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_cont_en_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_cont_en_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_cont_en_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_cont_en_4, &style_screen_value_cont_en_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_speed_4
	ui->screen_label_speed_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_speed_4, 5, 79);
	lv_obj_set_size(ui->screen_label_speed_4, 233, 35);
	lv_label_set_text(ui->screen_label_speed_4, "Speed:");
	lv_label_set_long_mode(ui->screen_label_speed_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_speed_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_speed_4_main_main_default
	static lv_style_t style_screen_label_speed_4_main_main_default;
	if (style_screen_label_speed_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_speed_4_main_main_default);
	else
		lv_style_init(&style_screen_label_speed_4_main_main_default);
	lv_style_set_radius(&style_screen_label_speed_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_speed_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_speed_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_speed_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_speed_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_speed_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_speed_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_speed_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_speed_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_speed_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_speed_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_speed_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_speed_4, &style_screen_label_speed_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_speed_4
	ui->screen_value_speed_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_speed_4, 243, 79);
	lv_obj_set_size(ui->screen_value_speed_4, 97, 35);
	lv_label_set_text(ui->screen_value_speed_4, "default");
	lv_label_set_long_mode(ui->screen_value_speed_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_speed_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_speed_4_main_main_default
	static lv_style_t style_screen_value_speed_4_main_main_default;
	if (style_screen_value_speed_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_speed_4_main_main_default);
	else
		lv_style_init(&style_screen_value_speed_4_main_main_default);
	lv_style_set_radius(&style_screen_value_speed_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_speed_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_speed_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_speed_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_speed_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_speed_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_speed_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_speed_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_speed_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_speed_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_speed_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_speed_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_speed_4, &style_screen_value_speed_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_position_4
	ui->screen_label_position_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_position_4, 5, 116);
	lv_obj_set_size(ui->screen_label_position_4, 233, 35);
	lv_label_set_text(ui->screen_label_position_4, "Position:");
	lv_label_set_long_mode(ui->screen_label_position_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_position_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_position_4_main_main_default
	static lv_style_t style_screen_label_position_4_main_main_default;
	if (style_screen_label_position_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_position_4_main_main_default);
	else
		lv_style_init(&style_screen_label_position_4_main_main_default);
	lv_style_set_radius(&style_screen_label_position_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_position_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_position_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_position_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_position_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_position_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_position_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_position_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_position_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_position_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_position_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_position_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_position_4, &style_screen_label_position_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_position_4
	ui->screen_value_position_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_position_4, 243, 116);
	lv_obj_set_size(ui->screen_value_position_4, 97, 35);
	lv_label_set_text(ui->screen_value_position_4, "default");
	lv_label_set_long_mode(ui->screen_value_position_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_position_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_position_4_main_main_default
	static lv_style_t style_screen_value_position_4_main_main_default;
	if (style_screen_value_position_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_position_4_main_main_default);
	else
		lv_style_init(&style_screen_value_position_4_main_main_default);
	lv_style_set_radius(&style_screen_value_position_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_position_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_position_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_position_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_position_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_position_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_position_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_position_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_position_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_position_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_position_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_position_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_position_4, &style_screen_value_position_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_temperature_4
	ui->screen_label_temperature_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_temperature_4, 5, 153);
	lv_obj_set_size(ui->screen_label_temperature_4, 233, 35);
	lv_label_set_text(ui->screen_label_temperature_4, "Temperature (C):");
	lv_label_set_long_mode(ui->screen_label_temperature_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_temperature_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_temperature_4_main_main_default
	static lv_style_t style_screen_label_temperature_4_main_main_default;
	if (style_screen_label_temperature_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_temperature_4_main_main_default);
	else
		lv_style_init(&style_screen_label_temperature_4_main_main_default);
	lv_style_set_radius(&style_screen_label_temperature_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_temperature_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_temperature_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_temperature_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_temperature_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_temperature_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_temperature_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_temperature_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_temperature_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_temperature_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_temperature_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_temperature_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_temperature_4, &style_screen_label_temperature_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_temperature_4
	ui->screen_value_temperature_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_temperature_4, 243, 153);
	lv_obj_set_size(ui->screen_value_temperature_4, 97, 35);
	lv_label_set_text(ui->screen_value_temperature_4, "default");
	lv_label_set_long_mode(ui->screen_value_temperature_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_temperature_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_temperature_4_main_main_default
	static lv_style_t style_screen_value_temperature_4_main_main_default;
	if (style_screen_value_temperature_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_temperature_4_main_main_default);
	else
		lv_style_init(&style_screen_value_temperature_4_main_main_default);
	lv_style_set_radius(&style_screen_value_temperature_4_main_main_default, 0);
	lv_style_set_bg_color(&style_screen_value_temperature_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_temperature_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_temperature_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_temperature_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_temperature_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_temperature_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_temperature_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_temperature_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_temperature_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_temperature_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_temperature_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_temperature_4, &style_screen_value_temperature_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_fault_4
	ui->screen_label_fault_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_fault_4, 5, 190);
	lv_obj_set_size(ui->screen_label_fault_4, 233, 35);
	lv_label_set_text(ui->screen_label_fault_4, "Fault:");
	lv_label_set_long_mode(ui->screen_label_fault_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_fault_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_fault_4_main_main_default
	static lv_style_t style_screen_label_fault_4_main_main_default;
	if (style_screen_label_fault_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_fault_4_main_main_default);
	else
		lv_style_init(&style_screen_label_fault_4_main_main_default);
	lv_style_set_radius(&style_screen_label_fault_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_fault_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_fault_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_fault_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_fault_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_fault_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_fault_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_fault_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_fault_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_fault_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_fault_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_fault_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_fault_4, &style_screen_label_fault_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_fault_4
	ui->screen_value_fault_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_fault_4, 243, 190);
	lv_obj_set_size(ui->screen_value_fault_4, 97, 35);
	lv_label_set_text(ui->screen_value_fault_4, "default");
	lv_label_set_long_mode(ui->screen_value_fault_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_fault_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_fault_4_main_main_default
	static lv_style_t style_screen_value_fault_4_main_main_default;
	if (style_screen_value_fault_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_fault_4_main_main_default);
	else
		lv_style_init(&style_screen_value_fault_4_main_main_default);
	lv_style_set_radius(&style_screen_value_fault_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_fault_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_fault_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_fault_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_fault_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_fault_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_fault_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_fault_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_fault_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_fault_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_fault_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_fault_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_fault_4, &style_screen_value_fault_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_a_curr_4
	ui->screen_label_phase_a_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_phase_a_curr_4, 5, 227);
	lv_obj_set_size(ui->screen_label_phase_a_curr_4, 233, 35);
	lv_label_set_text(ui->screen_label_phase_a_curr_4, "Phase A Current:");
	lv_label_set_long_mode(ui->screen_label_phase_a_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_a_curr_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_a_curr_4_main_main_default
	static lv_style_t style_screen_label_phase_a_curr_4_main_main_default;
	if (style_screen_label_phase_a_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_a_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_a_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_a_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_a_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_a_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_a_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_a_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_a_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_a_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_a_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_a_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_a_curr_4, &style_screen_label_phase_a_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_a_curr_4
	ui->screen_value_phase_a_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_phase_a_curr_4, 243, 227);
	lv_obj_set_size(ui->screen_value_phase_a_curr_4, 97, 35);
	lv_label_set_text(ui->screen_value_phase_a_curr_4, "default");
	lv_label_set_long_mode(ui->screen_value_phase_a_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_a_curr_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_a_curr_4_main_main_default
	static lv_style_t style_screen_value_phase_a_curr_4_main_main_default;
	if (style_screen_value_phase_a_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_a_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_a_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_a_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_a_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_a_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_a_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_a_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_a_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_a_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_a_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_a_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_a_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_a_curr_4, &style_screen_value_phase_a_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_b_curr_4
	ui->screen_label_phase_b_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_phase_b_curr_4, 5, 264);
	lv_obj_set_size(ui->screen_label_phase_b_curr_4, 233, 35);
	lv_label_set_text(ui->screen_label_phase_b_curr_4, "Phase B Current:");
	lv_label_set_long_mode(ui->screen_label_phase_b_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_b_curr_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_b_curr_4_main_main_default
	static lv_style_t style_screen_label_phase_b_curr_4_main_main_default;
	if (style_screen_label_phase_b_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_b_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_b_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_b_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_b_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_b_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_b_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_b_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_b_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_b_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_b_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_b_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_b_curr_4, &style_screen_label_phase_b_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_b_curr_4
	ui->screen_value_phase_b_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_phase_b_curr_4, 243, 264);
	lv_obj_set_size(ui->screen_value_phase_b_curr_4, 97, 35);
	lv_label_set_text(ui->screen_value_phase_b_curr_4, "default");
	lv_label_set_long_mode(ui->screen_value_phase_b_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_b_curr_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_b_curr_4_main_main_default
	static lv_style_t style_screen_value_phase_b_curr_4_main_main_default;
	if (style_screen_value_phase_b_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_b_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_b_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_b_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_b_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_b_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_b_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_b_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_b_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_b_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_b_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_b_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_b_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_b_curr_4, &style_screen_value_phase_b_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_phase_c_curr_4
	ui->screen_label_phase_c_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_phase_c_curr_4, 5, 301);
	lv_obj_set_size(ui->screen_label_phase_c_curr_4, 233, 35);
	lv_label_set_text(ui->screen_label_phase_c_curr_4, "Phase C Current: ");
	lv_label_set_long_mode(ui->screen_label_phase_c_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_phase_c_curr_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_phase_c_curr_4_main_main_default
	static lv_style_t style_screen_label_phase_c_curr_4_main_main_default;
	if (style_screen_label_phase_c_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_phase_c_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_label_phase_c_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_label_phase_c_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_phase_c_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_phase_c_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_phase_c_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_phase_c_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_phase_c_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_phase_c_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_phase_c_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_phase_c_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_phase_c_curr_4, &style_screen_label_phase_c_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_phase_c_curr_4
	ui->screen_value_phase_c_curr_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_phase_c_curr_4, 243, 301);
	lv_obj_set_size(ui->screen_value_phase_c_curr_4, 97, 35);
	lv_label_set_text(ui->screen_value_phase_c_curr_4, "default");
	lv_label_set_long_mode(ui->screen_value_phase_c_curr_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_phase_c_curr_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_phase_c_curr_4_main_main_default
	static lv_style_t style_screen_value_phase_c_curr_4_main_main_default;
	if (style_screen_value_phase_c_curr_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_phase_c_curr_4_main_main_default);
	else
		lv_style_init(&style_screen_value_phase_c_curr_4_main_main_default);
	lv_style_set_radius(&style_screen_value_phase_c_curr_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_phase_c_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_phase_c_curr_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_phase_c_curr_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_phase_c_curr_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_phase_c_curr_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_phase_c_curr_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_phase_c_curr_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_phase_c_curr_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_phase_c_curr_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_phase_c_curr_4, &style_screen_value_phase_c_curr_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_alpha_v_4
	ui->screen_label_alpha_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_alpha_v_4, 5, 338);
	lv_obj_set_size(ui->screen_label_alpha_v_4, 233, 35);
	lv_label_set_text(ui->screen_label_alpha_v_4, "Alpha Voltage:");
	lv_label_set_long_mode(ui->screen_label_alpha_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_alpha_v_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_alpha_v_4_main_main_default
	static lv_style_t style_screen_label_alpha_v_4_main_main_default;
	if (style_screen_label_alpha_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_alpha_v_4_main_main_default);
	else
		lv_style_init(&style_screen_label_alpha_v_4_main_main_default);
	lv_style_set_radius(&style_screen_label_alpha_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_alpha_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_alpha_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_alpha_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_alpha_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_alpha_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_alpha_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_alpha_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_alpha_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_alpha_v_4, &style_screen_label_alpha_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_alpha_v_4
	ui->screen_value_alpha_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_alpha_v_4, 243, 338);
	lv_obj_set_size(ui->screen_value_alpha_v_4, 97, 35);
	lv_label_set_text(ui->screen_value_alpha_v_4, "default");
	lv_label_set_long_mode(ui->screen_value_alpha_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_alpha_v_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_alpha_v_4_main_main_default
	static lv_style_t style_screen_value_alpha_v_4_main_main_default;
	if (style_screen_value_alpha_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_alpha_v_4_main_main_default);
	else
		lv_style_init(&style_screen_value_alpha_v_4_main_main_default);
	lv_style_set_radius(&style_screen_value_alpha_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_alpha_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_alpha_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_alpha_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_alpha_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_alpha_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_alpha_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_alpha_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_alpha_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_alpha_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_alpha_v_4, &style_screen_value_alpha_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_beta_v_4
	ui->screen_label_beta_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_beta_v_4, 5, 375);
	lv_obj_set_size(ui->screen_label_beta_v_4, 233, 35);
	lv_label_set_text(ui->screen_label_beta_v_4, "Beta Voltage:");
	lv_label_set_long_mode(ui->screen_label_beta_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_beta_v_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_beta_v_4_main_main_default
	static lv_style_t style_screen_label_beta_v_4_main_main_default;
	if (style_screen_label_beta_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_beta_v_4_main_main_default);
	else
		lv_style_init(&style_screen_label_beta_v_4_main_main_default);
	lv_style_set_radius(&style_screen_label_beta_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_beta_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_beta_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_beta_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_beta_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_beta_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_beta_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_beta_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_beta_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_beta_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_beta_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_beta_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_beta_v_4, &style_screen_label_beta_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_beta_v_4
	ui->screen_value_beta_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_beta_v_4, 243, 375);
	lv_obj_set_size(ui->screen_value_beta_v_4, 97, 35);
	lv_label_set_text(ui->screen_value_beta_v_4, "default");
	lv_label_set_long_mode(ui->screen_value_beta_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_beta_v_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_beta_v_4_main_main_default
	static lv_style_t style_screen_value_beta_v_4_main_main_default;
	if (style_screen_value_beta_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_beta_v_4_main_main_default);
	else
		lv_style_init(&style_screen_value_beta_v_4_main_main_default);
	lv_style_set_radius(&style_screen_value_beta_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_beta_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_beta_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_beta_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_beta_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_beta_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_beta_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_beta_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_beta_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_beta_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_beta_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_beta_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_beta_v_4, &style_screen_value_beta_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_dc_bus_v_4
	ui->screen_label_dc_bus_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_label_dc_bus_v_4, 5, 412);
	lv_obj_set_size(ui->screen_label_dc_bus_v_4, 233, 35);
	lv_label_set_text(ui->screen_label_dc_bus_v_4, "DC Bus Voltage:");
	lv_label_set_long_mode(ui->screen_label_dc_bus_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_dc_bus_v_4, LV_TEXT_ALIGN_LEFT, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_dc_bus_v_4_main_main_default
	static lv_style_t style_screen_label_dc_bus_v_4_main_main_default;
	if (style_screen_label_dc_bus_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_dc_bus_v_4_main_main_default);
	else
		lv_style_init(&style_screen_label_dc_bus_v_4_main_main_default);
	lv_style_set_radius(&style_screen_label_dc_bus_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_dc_bus_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_label_dc_bus_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_label_dc_bus_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_dc_bus_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_dc_bus_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_label_dc_bus_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_label_dc_bus_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_dc_bus_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_dc_bus_v_4, &style_screen_label_dc_bus_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_value_dc_bus_v_4
	ui->screen_value_dc_bus_v_4 = lv_label_create(ui->screen_cont_motor_status_4);
	lv_obj_set_pos(ui->screen_value_dc_bus_v_4, 243, 412);
	lv_obj_set_size(ui->screen_value_dc_bus_v_4, 97, 35);
	lv_label_set_text(ui->screen_value_dc_bus_v_4, "default");
	lv_label_set_long_mode(ui->screen_value_dc_bus_v_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_value_dc_bus_v_4, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_value_dc_bus_v_4_main_main_default
	static lv_style_t style_screen_value_dc_bus_v_4_main_main_default;
	if (style_screen_value_dc_bus_v_4_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_value_dc_bus_v_4_main_main_default);
	else
		lv_style_init(&style_screen_value_dc_bus_v_4_main_main_default);
	lv_style_set_radius(&style_screen_value_dc_bus_v_4_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_value_dc_bus_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_value_dc_bus_v_4_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_value_dc_bus_v_4_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_value_dc_bus_v_4_main_main_default, 255);
	lv_style_set_text_color(&style_screen_value_dc_bus_v_4_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_text_font(&style_screen_value_dc_bus_v_4_main_main_default, &lv_font_arial_24);
	lv_style_set_text_letter_space(&style_screen_value_dc_bus_v_4_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_value_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_value_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_value_dc_bus_v_4_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_value_dc_bus_v_4_main_main_default, 0);
	lv_obj_add_style(ui->screen_value_dc_bus_v_4, &style_screen_value_dc_bus_v_4_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_cont_status_icons
	ui->screen_cont_status_icons = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_status_icons, 10, 1210);
	lv_obj_set_size(ui->screen_cont_status_icons, 700, 50);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_status_icons_main_main_default
	static lv_style_t style_screen_cont_status_icons_main_main_default;
	if (style_screen_cont_status_icons_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_status_icons_main_main_default);
	else
		lv_style_init(&style_screen_cont_status_icons_main_main_default);
	lv_style_set_radius(&style_screen_cont_status_icons_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_status_icons_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_color(&style_screen_cont_status_icons_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_bg_grad_dir(&style_screen_cont_status_icons_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_status_icons_main_main_default, 255);
	lv_style_set_border_color(&style_screen_cont_status_icons_main_main_default, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&style_screen_cont_status_icons_main_main_default, 0);
	lv_style_set_border_opa(&style_screen_cont_status_icons_main_main_default, 0);
	lv_style_set_pad_left(&style_screen_cont_status_icons_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_status_icons_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_status_icons_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_status_icons_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_status_icons, &style_screen_cont_status_icons_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_img_log
	ui->screen_img_log = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_log, 652, 2);
	lv_obj_set_size(ui->screen_img_log, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_log_main_main_default
	static lv_style_t style_screen_img_log_main_main_default;
	if (style_screen_img_log_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_log_main_main_default);
	else
		lv_style_init(&style_screen_img_log_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_log_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_log_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_log_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_log, &style_screen_img_log_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_log, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_log,&_img_log_46x46);
	lv_img_set_pivot(ui->screen_img_log, 0,0);
	lv_img_set_angle(ui->screen_img_log, 0);

	//Write codes screen_img_network
	ui->screen_img_network = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_network, 587, 2);
	lv_obj_set_size(ui->screen_img_network, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_network_main_main_default
	static lv_style_t style_screen_img_network_main_main_default;
	if (style_screen_img_network_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_network_main_main_default);
	else
		lv_style_init(&style_screen_img_network_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_network_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_network_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_network_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_network, &style_screen_img_network_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_network, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_network,&_img_network_46x46);
	lv_img_set_pivot(ui->screen_img_network, 0,0);
	lv_img_set_angle(ui->screen_img_network, 0);

	//Write codes screen_img_memory
	ui->screen_img_memory = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_memory, 522, 2);
	lv_obj_set_size(ui->screen_img_memory, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_memory_main_main_default
	static lv_style_t style_screen_img_memory_main_main_default;
	if (style_screen_img_memory_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_memory_main_main_default);
	else
		lv_style_init(&style_screen_img_memory_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_memory_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_memory_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_memory_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_memory, &style_screen_img_memory_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_memory, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_memory,&_img_memory_46x46);
	lv_img_set_pivot(ui->screen_img_memory, 0,0);
	lv_img_set_angle(ui->screen_img_memory, 0);

	//Write codes screen_img_shutdown
	ui->screen_img_shutdown = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_shutdown, 457, 2);
	lv_obj_set_size(ui->screen_img_shutdown, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_shutdown_main_main_default
	static lv_style_t style_screen_img_shutdown_main_main_default;
	if (style_screen_img_shutdown_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_shutdown_main_main_default);
	else
		lv_style_init(&style_screen_img_shutdown_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_shutdown_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_shutdown_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_shutdown_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_shutdown, &style_screen_img_shutdown_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_shutdown, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_shutdown,&_img_shutdown_46x46);
	lv_img_set_pivot(ui->screen_img_shutdown, 0,0);
	lv_img_set_angle(ui->screen_img_shutdown, 0);

	//Write codes screen_img_anomaly_detection
	ui->screen_img_anomaly_detection = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_anomaly_detection, 392, 2);
	lv_obj_set_size(ui->screen_img_anomaly_detection, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_anomaly_detection_main_main_default
	static lv_style_t style_screen_img_anomaly_detection_main_main_default;
	if (style_screen_img_anomaly_detection_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_anomaly_detection_main_main_default);
	else
		lv_style_init(&style_screen_img_anomaly_detection_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_anomaly_detection_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_anomaly_detection_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_anomaly_detection_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_anomaly_detection, &style_screen_img_anomaly_detection_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_anomaly_detection, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_anomaly_detection,&_img_anomaly_detection_46x46);
	lv_img_set_pivot(ui->screen_img_anomaly_detection, 0,0);
	lv_img_set_angle(ui->screen_img_anomaly_detection, 0);

	//Write codes screen_img_configuration
	ui->screen_img_configuration = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_configuration, 327, 2);
	lv_obj_set_size(ui->screen_img_configuration, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_configuration_main_main_default
	static lv_style_t style_screen_img_configuration_main_main_default;
	if (style_screen_img_configuration_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_configuration_main_main_default);
	else
		lv_style_init(&style_screen_img_configuration_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_configuration_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_configuration_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_configuration_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_configuration, &style_screen_img_configuration_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_configuration, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_configuration,&_img_configuration_46x46);
	lv_img_set_pivot(ui->screen_img_configuration, 0,0);
	lv_img_set_angle(ui->screen_img_configuration, 0);

	//Write codes screen_img_fw_update
	ui->screen_img_fw_update = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_fw_update, 262, 2);
	lv_obj_set_size(ui->screen_img_fw_update, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_fw_update_main_main_default
	static lv_style_t style_screen_img_fw_update_main_main_default;
	if (style_screen_img_fw_update_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_fw_update_main_main_default);
	else
		lv_style_init(&style_screen_img_fw_update_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_fw_update_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_fw_update_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_fw_update_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_fw_update, &style_screen_img_fw_update_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_fw_update, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_fw_update,&_img_fw_update_46x46);
	lv_img_set_pivot(ui->screen_img_fw_update, 0,0);
	lv_img_set_angle(ui->screen_img_fw_update, 0);

	//Write codes screen_img_fault
	ui->screen_img_fault = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_fault, 197, 2);
	lv_obj_set_size(ui->screen_img_fault, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_fault_main_main_default
	static lv_style_t style_screen_img_fault_main_main_default;
	if (style_screen_img_fault_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_fault_main_main_default);
	else
		lv_style_init(&style_screen_img_fault_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_fault_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_fault_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_fault_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_fault, &style_screen_img_fault_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_fault, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_fault,&_img_fault_46x46);
	lv_img_set_pivot(ui->screen_img_fault, 0,0);
	lv_img_set_angle(ui->screen_img_fault, 0);

	//Write codes screen_img_maintenance
	ui->screen_img_maintenance = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_maintenance, 132, 2);
	lv_obj_set_size(ui->screen_img_maintenance, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_maintenance_main_main_default
	static lv_style_t style_screen_img_maintenance_main_main_default;
	if (style_screen_img_maintenance_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_maintenance_main_main_default);
	else
		lv_style_init(&style_screen_img_maintenance_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_maintenance_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_maintenance_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_maintenance_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_maintenance, &style_screen_img_maintenance_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_maintenance, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_maintenance,&_img_maintenance_46x46);
	lv_img_set_pivot(ui->screen_img_maintenance, 0,0);
	lv_img_set_angle(ui->screen_img_maintenance, 0);

	//Write codes screen_img_error
	ui->screen_img_error = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_error, 67, 2);
	lv_obj_set_size(ui->screen_img_error, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_error_main_main_default
	static lv_style_t style_screen_img_error_main_main_default;
	if (style_screen_img_error_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_error_main_main_default);
	else
		lv_style_init(&style_screen_img_error_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_error_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_error_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_error_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_error, &style_screen_img_error_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_error, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_error,&_img_error_46x46);
	lv_img_set_pivot(ui->screen_img_error, 0,0);
	lv_img_set_angle(ui->screen_img_error, 0);

	//Write codes screen_img_operational
	ui->screen_img_operational = lv_img_create(ui->screen_cont_status_icons);
	lv_obj_set_pos(ui->screen_img_operational, 2, 2);
	lv_obj_set_size(ui->screen_img_operational, 46, 46);

	//Write style state: LV_STATE_DEFAULT for style_screen_img_operational_main_main_default
	static lv_style_t style_screen_img_operational_main_main_default;
	if (style_screen_img_operational_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_img_operational_main_main_default);
	else
		lv_style_init(&style_screen_img_operational_main_main_default);
	lv_style_set_img_recolor(&style_screen_img_operational_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_img_recolor_opa(&style_screen_img_operational_main_main_default, 0);
	lv_style_set_img_opa(&style_screen_img_operational_main_main_default, 255);
	lv_obj_add_style(ui->screen_img_operational, &style_screen_img_operational_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->screen_img_operational, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_operational,&_img_operational_46x46);
	lv_img_set_pivot(ui->screen_img_operational, 0,0);
	lv_img_set_angle(ui->screen_img_operational, 0);

	//Write codes screen_cont_log
	ui->screen_cont_log = lv_obj_create(ui->screen);
	lv_obj_set_pos(ui->screen_cont_log, 10, 1071);
	lv_obj_set_size(ui->screen_cont_log, 700, 121);

	//Write style state: LV_STATE_DEFAULT for style_screen_cont_log_main_main_default
	static lv_style_t style_screen_cont_log_main_main_default;
	if (style_screen_cont_log_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_cont_log_main_main_default);
	else
		lv_style_init(&style_screen_cont_log_main_main_default);
	lv_style_set_radius(&style_screen_cont_log_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_cont_log_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_color(&style_screen_cont_log_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_bg_grad_dir(&style_screen_cont_log_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_cont_log_main_main_default, 0);
	lv_style_set_border_color(&style_screen_cont_log_main_main_default, lv_color_make(COLOR_OPERATIONAL));
	lv_style_set_border_width(&style_screen_cont_log_main_main_default, 2);
	lv_style_set_border_opa(&style_screen_cont_log_main_main_default, 255);
	lv_style_set_pad_left(&style_screen_cont_log_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_cont_log_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_cont_log_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_cont_log_main_main_default, 0);
	lv_obj_add_style(ui->screen_cont_log, &style_screen_cont_log_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_datetime_3
	ui->screen_label_log_datetime_3 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_datetime_3, 5, 81);
	lv_obj_set_size(ui->screen_label_log_datetime_3, 150, 35);
	lv_label_set_text(ui->screen_label_log_datetime_3, "");
	lv_label_set_long_mode(ui->screen_label_log_datetime_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_datetime_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_datetime_3_main_main_default
	static lv_style_t style_screen_label_log_datetime_3_main_main_default;
	if (style_screen_label_log_datetime_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_datetime_3_main_main_default);
	else
		lv_style_init(&style_screen_label_log_datetime_3_main_main_default);
	lv_style_set_radius(&style_screen_label_log_datetime_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_datetime_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_datetime_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_datetime_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_datetime_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_datetime_3_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_datetime_3_main_main_default, &lv_font_arial_16);
	lv_style_set_text_letter_space(&style_screen_label_log_datetime_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_datetime_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_datetime_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_datetime_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_datetime_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_datetime_3, &style_screen_label_log_datetime_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_datetime_2
	ui->screen_label_log_datetime_2 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_datetime_2, 5, 43);
	lv_obj_set_size(ui->screen_label_log_datetime_2, 150, 35);
	lv_label_set_text(ui->screen_label_log_datetime_2, "");
	lv_label_set_long_mode(ui->screen_label_log_datetime_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_datetime_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_datetime_2_main_main_default
	static lv_style_t style_screen_label_log_datetime_2_main_main_default;
	if (style_screen_label_log_datetime_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_datetime_2_main_main_default);
	else
		lv_style_init(&style_screen_label_log_datetime_2_main_main_default);
	lv_style_set_radius(&style_screen_label_log_datetime_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_datetime_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_datetime_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_datetime_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_datetime_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_datetime_2_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_datetime_2_main_main_default, &lv_font_arial_16);
	lv_style_set_text_letter_space(&style_screen_label_log_datetime_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_datetime_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_datetime_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_datetime_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_datetime_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_datetime_2, &style_screen_label_log_datetime_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_datetime_1
	ui->screen_label_log_datetime_1 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_datetime_1, 5, 5);
	lv_obj_set_size(ui->screen_label_log_datetime_1, 150, 35);
	lv_label_set_text(ui->screen_label_log_datetime_1, "");
	lv_label_set_long_mode(ui->screen_label_log_datetime_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_datetime_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_datetime_1_main_main_default
	static lv_style_t style_screen_label_log_datetime_1_main_main_default;
	if (style_screen_label_log_datetime_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_datetime_1_main_main_default);
	else
		lv_style_init(&style_screen_label_log_datetime_1_main_main_default);
	lv_style_set_radius(&style_screen_label_log_datetime_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_datetime_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_datetime_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_datetime_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_datetime_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_datetime_1_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_datetime_1_main_main_default, &lv_font_arial_16);
	lv_style_set_text_letter_space(&style_screen_label_log_datetime_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_datetime_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_datetime_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_datetime_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_datetime_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_datetime_1, &style_screen_label_log_datetime_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_1
	ui->screen_label_log_1 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_1, 160, 5);
	lv_obj_set_size(ui->screen_label_log_1, 535, 35);
	lv_label_set_text(ui->screen_label_log_1, "");
	lv_label_set_long_mode(ui->screen_label_log_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_1, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_1_main_main_default
	static lv_style_t style_screen_label_log_1_main_main_default;
	if (style_screen_label_log_1_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_1_main_main_default);
	else
		lv_style_init(&style_screen_label_log_1_main_main_default);
	lv_style_set_radius(&style_screen_label_log_1_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_1_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_1_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_1_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_1_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_1_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_log_1_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_1_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_1_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_1_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_1, &style_screen_label_log_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_2
	ui->screen_label_log_2 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_2, 160, 43);
	lv_obj_set_size(ui->screen_label_log_2, 535, 35);
	lv_label_set_text(ui->screen_label_log_2, "");
	lv_label_set_long_mode(ui->screen_label_log_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_2, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_2_main_main_default
	static lv_style_t style_screen_label_log_2_main_main_default;
	if (style_screen_label_log_2_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_2_main_main_default);
	else
		lv_style_init(&style_screen_label_log_2_main_main_default);
	lv_style_set_radius(&style_screen_label_log_2_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_2_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_2_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_2_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_2_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_2_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_log_2_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_2_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_2_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_2_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_2_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_2, &style_screen_label_log_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_log_3
	ui->screen_label_log_3 = lv_label_create(ui->screen_cont_log);
	lv_obj_set_pos(ui->screen_label_log_3, 160, 81);
	lv_obj_set_size(ui->screen_label_log_3, 535, 35);
	lv_label_set_text(ui->screen_label_log_3, "");
	lv_label_set_long_mode(ui->screen_label_log_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ui->screen_label_log_3, LV_TEXT_ALIGN_CENTER, 0);

	//Write style state: LV_STATE_DEFAULT for style_screen_label_log_3_main_main_default
	static lv_style_t style_screen_label_log_3_main_main_default;
	if (style_screen_label_log_3_main_main_default.prop_cnt > 1)
		lv_style_reset(&style_screen_label_log_3_main_main_default);
	else
		lv_style_init(&style_screen_label_log_3_main_main_default);
	lv_style_set_radius(&style_screen_label_log_3_main_main_default, 5);
	lv_style_set_bg_color(&style_screen_label_log_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_color(&style_screen_label_log_3_main_main_default, lv_color_make(COLOR_MAIN_LIGHT));
	lv_style_set_bg_grad_dir(&style_screen_label_log_3_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_label_log_3_main_main_default, 255);
	lv_style_set_text_color(&style_screen_label_log_3_main_main_default, lv_color_make(COLOR_MAIN_DARK));
	lv_style_set_text_font(&style_screen_label_log_3_main_main_default, &lv_font_arial_29);
	lv_style_set_text_letter_space(&style_screen_label_log_3_main_main_default, 2);
	lv_style_set_pad_left(&style_screen_label_log_3_main_main_default, 0);
	lv_style_set_pad_right(&style_screen_label_log_3_main_main_default, 0);
	lv_style_set_pad_top(&style_screen_label_log_3_main_main_default, 0);
	lv_style_set_pad_bottom(&style_screen_label_log_3_main_main_default, 0);
	lv_obj_add_style(ui->screen_label_log_3, &style_screen_label_log_3_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
}
