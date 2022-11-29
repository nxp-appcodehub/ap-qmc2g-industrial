/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: MIT
 * The auto-generated can only be used on NXP devices
 */

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "guider_fonts.h"

typedef struct
{
	lv_obj_t *screen;
	lv_obj_t *screen_label_warning;
	lv_obj_t *screen_label_motor_1;
	lv_obj_t *screen_cont_motor_status_1;
	lv_obj_t *screen_value_state_1;
	lv_obj_t *screen_label_state_1;
	lv_obj_t *screen_label_temperature_1;
	lv_obj_t *screen_value_temperature_1;
	lv_obj_t *screen_label_cont_en_1;
	lv_obj_t *screen_value_cont_en_1;
	lv_obj_t *screen_label_speed_1;
	lv_obj_t *screen_value_speed_1;
	lv_obj_t *screen_label_position_1;
	lv_obj_t *screen_value_position_1;
	lv_obj_t *screen_label_fault_1;
	lv_obj_t *screen_value_fault_1;
	lv_obj_t *screen_label_phase_a_curr_1;
	lv_obj_t *screen_value_phase_a_curr_1;
	lv_obj_t *screen_label_phase_b_curr_1;
	lv_obj_t *screen_value_phase_b_curr_1;
	lv_obj_t *screen_label_dc_bus_v_1;
	lv_obj_t *screen_label_phase_c_curr_1;
	lv_obj_t *screen_value_phase_c_curr_1;
	lv_obj_t *screen_label_alpha_v_1;
	lv_obj_t *screen_value_alpha_v_1;
	lv_obj_t *screen_label_beta_v_1;
	lv_obj_t *screen_value_beta_v_1;
	lv_obj_t *screen_value_dc_bus_v_1;
	lv_obj_t *screen_label_motor_2;
	lv_obj_t *screen_cont_motor_status_2;
	lv_obj_t *screen_label_state_2;
	lv_obj_t *screen_value_state_2;
	lv_obj_t *screen_label_temperature_2;
	lv_obj_t *screen_value_temperature_2;
	lv_obj_t *screen_label_cont_en_2;
	lv_obj_t *screen_value_cont_en_2;
	lv_obj_t *screen_label_speed_2;
	lv_obj_t *screen_value_speed_2;
	lv_obj_t *screen_label_position_2;
	lv_obj_t *screen_value_position_2;
	lv_obj_t *screen_label_fault_2;
	lv_obj_t *screen_value_fault_2;
	lv_obj_t *screen_label_phase_a_curr_2;
	lv_obj_t *screen_value_phase_a_curr_2;
	lv_obj_t *screen_label_phase_b_curr_2;
	lv_obj_t *screen_value_phase_b_curr_2;
	lv_obj_t *screen_label_dc_bus_v_2;
	lv_obj_t *screen_label_phase_c_curr_2;
	lv_obj_t *screen_value_phase_c_curr_2;
	lv_obj_t *screen_label_alpha_v_2;
	lv_obj_t *screen_value_alpha_v_2;
	lv_obj_t *screen_label_beta_v_2;
	lv_obj_t *screen_value_beta_v_2;
	lv_obj_t *screen_value_dc_bus_v_2;
	lv_obj_t *screen_label_motor_3;
	lv_obj_t *screen_cont_motor_status_3;
	lv_obj_t *screen_label_state_3;
	lv_obj_t *screen_value_state_3;
	lv_obj_t *screen_label_temperature_3;
	lv_obj_t *screen_value_temperature_3;
	lv_obj_t *screen_label_cont_en_3;
	lv_obj_t *screen_value_cont_en_3;
	lv_obj_t *screen_label_speed_3;
	lv_obj_t *screen_value_speed_3;
	lv_obj_t *screen_label_position_3;
	lv_obj_t *screen_value_position_3;
	lv_obj_t *screen_label_fault_3;
	lv_obj_t *screen_value_fault_3;
	lv_obj_t *screen_label_phase_a_curr_3;
	lv_obj_t *screen_value_phase_a_curr_3;
	lv_obj_t *screen_label_phase_b_curr_3;
	lv_obj_t *screen_value_phase_b_curr_3;
	lv_obj_t *screen_label_phase_c_curr_3;
	lv_obj_t *screen_value_phase_c_curr_3;
	lv_obj_t *screen_label_alpha_v_3;
	lv_obj_t *screen_value_alpha_v_3;
	lv_obj_t *screen_label_beta_v_3;
	lv_obj_t *screen_value_beta_v_3;
	lv_obj_t *screen_label_dc_bus_v_3;
	lv_obj_t *screen_value_dc_bus_v_3;
	lv_obj_t *screen_label_motor_4;
	lv_obj_t *screen_cont_motor_status_4;
	lv_obj_t *screen_label_state_4;
	lv_obj_t *screen_value_state_4;
	lv_obj_t *screen_label_temperature_4;
	lv_obj_t *screen_value_temperature_4;
	lv_obj_t *screen_label_cont_en_4;
	lv_obj_t *screen_value_cont_en_4;
	lv_obj_t *screen_label_speed_4;
	lv_obj_t *screen_value_speed_4;
	lv_obj_t *screen_label_position_4;
	lv_obj_t *screen_value_position_4;
	lv_obj_t *screen_label_fault_4;
	lv_obj_t *screen_value_fault_4;
	lv_obj_t *screen_label_phase_a_curr_4;
	lv_obj_t *screen_value_phase_a_curr_4;
	lv_obj_t *screen_label_phase_b_curr_4;
	lv_obj_t *screen_value_phase_b_curr_4;
	lv_obj_t *screen_label_phase_c_curr_4;
	lv_obj_t *screen_value_phase_c_curr_4;
	lv_obj_t *screen_label_alpha_v_4;
	lv_obj_t *screen_value_alpha_v_4;
	lv_obj_t *screen_label_beta_v_4;
	lv_obj_t *screen_value_beta_v_4;
	lv_obj_t *screen_label_dc_bus_v_4;
	lv_obj_t *screen_value_dc_bus_v_4;
	lv_obj_t *screen_cont_status_icons;
	lv_obj_t *screen_img_log;
	lv_obj_t *screen_img_network;
	lv_obj_t *screen_img_memory;
	lv_obj_t *screen_img_shutdown;
	lv_obj_t *screen_img_anomaly_detection;
	lv_obj_t *screen_img_configuration;
	lv_obj_t *screen_img_fw_update;
	lv_obj_t *screen_img_fault;
	lv_obj_t *screen_img_maintenance;
	lv_obj_t *screen_img_error;
	lv_obj_t *screen_img_operational;
	lv_obj_t *screen_cont_log;
	lv_obj_t *screen_label_log_datetime_3;
	lv_obj_t *screen_label_log_datetime_2;
	lv_obj_t *screen_label_log_datetime_1;
	lv_obj_t *screen_label_log_1;
	lv_obj_t *screen_label_log_2;
	lv_obj_t *screen_label_log_3;
}lv_ui;

void setup_ui(lv_ui *ui);
extern lv_ui guider_ui;
void setup_scr_screen(lv_ui *ui);
LV_IMG_DECLARE(_img_anomaly_detection_46x46);
LV_IMG_DECLARE(_img_fault_46x46);
LV_IMG_DECLARE(_img_error_46x46);
LV_IMG_DECLARE(_img_configuration_46x46);
LV_IMG_DECLARE(_img_log_46x46);
LV_IMG_DECLARE(_img_memory_46x46);
LV_IMG_DECLARE(_img_operational_46x46);
LV_IMG_DECLARE(_img_network_46x46);
LV_IMG_DECLARE(_img_shutdown_46x46);
LV_IMG_DECLARE(_img_maintenance_46x46);
LV_IMG_DECLARE(_img_fw_update_46x46);

#ifdef __cplusplus
}
#endif
#endif
