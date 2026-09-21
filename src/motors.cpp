// Based on: https://github.com/espressif/esp-idf/blob/master/examples/peripherals/mcpwm/mcpwm_bdc_speed_control/main/mcpwm_bdc_control_example_main.c
// Author: Larry Qiu
// Created: Sep 20
// Last modified: Sep 20

// Imports
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "driver/pulse_cnt.h"
#include "driver/mcpwm_prelude.h"
#include "motors.h"

static const char *TAG = "MOTOR";

// GPIO Pins (L293D inputs M1/M2, quadrature encoder channels A/B)
#define LEFT_M1                       21
#define LEFT_M2                       38
#define LEFT_A                        41
#define LEFT_B                        42
#define RIGHT_M1                      39
#define RIGHT_M2                      40
#define RIGHT_A                       2
#define RIGHT_B                       1

#define BDC_ENCODER_PCNT_HIGH_LIMIT   1000
#define BDC_ENCODER_PCNT_LOW_LIMIT    -1000

// Pid loop update period
#define BDC_PID_LOOP_PERIOD_MS        10
#define BDC_PID_EXPECT_SPEED          400

// Holds single motor (typedef'd as bdc_motor_t in motors.h)
struct bdc_motor_t {
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t oper;
    mcpwm_cmpr_handle_t cmpa;
    mcpwm_cmpr_handle_t cmpb;
    mcpwm_gen_handle_t gena;
    mcpwm_gen_handle_t genb;
};

// Holes PID state
typedef struct {
    float kp;
    float ki;
    float kd;
    float max_output;
    float min_output;
    float previous_err1;
    float previous_err2;
    float last_output;
} pid_ctrl_t;

// Combines motor + PID
typedef struct {
    bdc_motor_t *motor;
    pcnt_unit_handle_t pcnt_encoder;
    pid_ctrl_t *pid_ctrl;
    int report_pulses;
} motor_control_context_t;

static esp_err_t bdc_motor_new_mcpwm_device(bdc_motor_t *motor, int group_id, uint32_t resolution_hz, uint32_t pwm_freq_hz, int pwma_gpio_num, int pwmb_gpio_num)
{
    // Basically copied from reference for configuring mcpwm device
    // Configs two PWM counters from clock source for given frequency and GPIO
    // PWM counter counts at resolution_hz
    mcpwm_timer_config_t timer_config = {
        .group_id = group_id,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = resolution_hz,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = resolution_hz / pwm_freq_hz,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_timer(&timer_config, &motor->timer), TAG, "create MCPWM timer failed");

    mcpwm_operator_config_t operator_config = {
        .group_id = group_id,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_operator(&operator_config, &motor->oper), TAG, "create MCPWM operator failed");
    ESP_RETURN_ON_ERROR(mcpwm_operator_connect_timer(motor->oper, motor->timer), TAG, "connect timer and operator failed");

    mcpwm_comparator_config_t comparator_config = {
        .flags = { .update_cmp_on_tez = true },
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(motor->oper, &comparator_config, &motor->cmpa), TAG, "create comparator failed");
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(motor->oper, &comparator_config, &motor->cmpb), TAG, "create comparator failed");
    mcpwm_comparator_set_compare_value(motor->cmpa, 0);
    mcpwm_comparator_set_compare_value(motor->cmpb, 0);

    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = pwma_gpio_num,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(motor->oper, &generator_config, &motor->gena), TAG, "create generator failed");
    generator_config.gen_gpio_num = pwmb_gpio_num;
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(motor->oper, &generator_config, &motor->genb), TAG, "create generator failed");

    mcpwm_generator_set_action_on_timer_event(motor->gena,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(motor->gena,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, motor->cmpa, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_timer_event(motor->genb,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(motor->genb,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, motor->cmpb, MCPWM_GEN_ACTION_LOW));
    return ESP_OK;
}

static esp_err_t bdc_motor_enable(bdc_motor_t *motor)
{
    // Starts PWM
    ESP_RETURN_ON_ERROR(mcpwm_timer_enable(motor->timer), TAG, "enable timer failed");
    ESP_RETURN_ON_ERROR(mcpwm_timer_start_stop(motor->timer, MCPWM_TIMER_START_NO_STOP), TAG, "start timer failed");
    return ESP_OK;
}

esp_err_t bdc_motor_set_speed(bdc_motor_t *motor, uint32_t speed)
{
    // Changes PWM duty. speed goes from 0 to BDC_MCPWM_DUTY_TICK_MAX - 1
    ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(motor->cmpa, speed), TAG, "set compare value failed");
    ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(motor->cmpb, speed), TAG, "set compare value failed");
    return ESP_OK;
}


esp_err_t bdc_motor_forward(bdc_motor_t *motor)
{
    // Makes channel A the pwm channel and B always low (forwards)
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->gena, -1, true), TAG, "disable force level for gena failed");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->genb, 0, true), TAG, "set force level for genb failed");
    return ESP_OK;
}

esp_err_t bdc_motor_reverse(bdc_motor_t *motor)
{
    // Makes channel B the pwm channel and A always low (backwards)

    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->gena, 0, true), TAG, "disable force level for gena failed");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->genb, -1, true), TAG, "set force level for genb failed");
    return ESP_OK;
}

esp_err_t bdc_motor_coast(bdc_motor_t *motor)
{
    // Makes both always low (motors spin freely)
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->gena, 0, true), TAG, "disable force level for gena failed");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->genb, 0, true), TAG, "set force level for genb failed");
    return ESP_OK;
}

esp_err_t bdc_motor_brake(bdc_motor_t *motor)
{
    // Makes both always high (shorts windings, motor brakes)
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->gena, 1, true), TAG, "disable force level for gena failed");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_force_level(motor->genb, 1, true), TAG, "set force level for genb failed");
    return ESP_OK;
}

// Not using PID for now, measurement only.

// static float pid_clamp(float value, float min_value, float max_value)
// {
//     if (value < min_value) {
//         return min_value;
//     }
//     if (value > max_value) {
//         return max_value;
//     }
//     return value;
// }

// static void pid_new_control_block(pid_ctrl_t *pid, float kp, float ki, float kd, float max_output, float min_output)
// {
//     pid->kp = kp;
//     pid->ki = ki;
//     pid->kd = kd;
//     pid->max_output = max_output;
//     pid->min_output = min_output;
//     pid->previous_err1 = 0;
//     pid->previous_err2 = 0;
//     pid->last_output = 0;
// }

// static float pid_compute(pid_ctrl_t *pid, float error)
// {
//     float output = (error - pid->previous_err1) * pid->kp +
//                    (error - pid->previous_err1 - pid->previous_err1 + pid->previous_err2) * pid->kd +
//                    error * pid->ki +
//                    pid->last_output;
//     output = pid_clamp(output, pid->min_output, pid->max_output);

//     pid->previous_err2 = pid->previous_err1;
//     pid->previous_err1 = error;
//     pid->last_output = output;
//     return output;
// }

// static void pid_loop_cb(void *args)
// {
//     static int last_pulse_count = 0;
//     motor_control_context_t *ctx = (motor_control_context_t *)args;
//     pcnt_unit_handle_t pcnt_unit = ctx->pcnt_encoder;
//     pid_ctrl_t *pid_ctrl = ctx->pid_ctrl;
//     bdc_motor_t *motor = ctx->motor;

//     int cur_pulse_count = 0;
//     pcnt_unit_get_count(pcnt_unit, &cur_pulse_count);
//     int real_pulses = cur_pulse_count - last_pulse_count;
//     last_pulse_count = cur_pulse_count;
//     ctx->report_pulses = real_pulses;

//     float error = BDC_PID_EXPECT_SPEED - real_pulses;
//     float new_speed = pid_compute(pid_ctrl, error);
//     bdc_motor_set_speed(motor, (uint32_t)new_speed);
// }


void create_one_motor_with_encoder(bdc_motor_t * motor, pid_ctrl_t * pid_ctrl, motor_control_context_t * motor_ctrl_ctx, int group_id, int motor_M1, int motor_M2, int encoder_A, int encoder_B) {
    ESP_LOGI(TAG, "Create DC motor");
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(motor, group_id, BDC_MCPWM_TIMER_RESOLUTION_HZ, BDC_MCPWM_FREQ_HZ, motor_M1, motor_M2));

    motor_ctrl_ctx -> motor = motor;
    motor_ctrl_ctx -> pid_ctrl = pid_ctrl;

    ESP_LOGI(TAG, "Init pcnt driver to decode rotary signal");
    pcnt_unit_config_t unit_config = {
        .low_limit = BDC_ENCODER_PCNT_LOW_LIMIT,
        .high_limit = BDC_ENCODER_PCNT_HIGH_LIMIT,
        .flags = { .accum_count = true }, // output count accumulate above those limits
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = encoder_A,
        .level_gpio_num = encoder_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = encoder_B,
        .level_gpio_num = encoder_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_LOW_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    motor_ctrl_ctx -> pcnt_encoder = pcnt_unit;
    
    // ESP_LOGI(TAG, "Create PID control block");
    // // pid_new_control_block(&pid_ctrl, 0.6, 0.4, 0.2, BDC_MCPWM_DUTY_TICK_MAX - 1, 0);
    // motor_ctrl_ctx -> pid_ctrl = &pid_ctrl;

    // ESP_LOGI(TAG, "Create a timer to do PID calculation periodically");
    // esp_timer_create_args_t periodic_timer_args = {};
    // periodic_timer_args.callback = pid_loop_cb;
    // periodic_timer_args.arg = &motor_ctrl_ctx;
    // periodic_timer_args.name = "pid_loop";
    // esp_timer_handle_t pid_loop_timer = NULL;
    // ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pid_loop_timer));

    ESP_LOGI(TAG, "Enable motor");
    ESP_ERROR_CHECK(bdc_motor_enable(motor));
    ESP_LOGI(TAG, "Forward motor");
    ESP_ERROR_CHECK(bdc_motor_forward(motor));

    // ESP_LOGI(TAG, "Start motor speed loop");
    // ESP_ERROR_CHECK(esp_timer_start_periodic(pid_loop_timer, BDC_PID_LOOP_PERIOD_MS * 1000));


}


bdc_motor_t left_motor;
pid_ctrl_t left_pid_ctrl;
motor_control_context_t left_motor_ctrl_ctx;


bdc_motor_t right_motor;
pid_ctrl_t right_pid_ctrl;
motor_control_context_t right_motor_ctrl_ctx;


void setup_motors() {
    create_one_motor_with_encoder(&left_motor,  &left_pid_ctrl,  &left_motor_ctrl_ctx,  0, LEFT_M1,  LEFT_M2,  LEFT_A,  LEFT_B);
    create_one_motor_with_encoder(&right_motor, &right_pid_ctrl, &right_motor_ctrl_ctx, 1, RIGHT_M1, RIGHT_M2, RIGHT_A, RIGHT_B);
    
}

void loop_motors() {

}

int left_encoder_count() {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(left_motor_ctrl_ctx.pcnt_encoder, &count));
    return count;
}

int right_encoder_count() {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(right_motor_ctrl_ctx.pcnt_encoder, &count));
    return count;
}

// void app_main(void)
// {
//     static bdc_motor_t motor = {};
//     static pid_ctrl_t pid_ctrl = {};
//     static motor_control_context_t motor_ctrl_ctx = {};

//     ESP_LOGI(TAG, "Create DC motor");
//     ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor, 0, BDC_MCPWM_TIMER_RESOLUTION_HZ, BDC_MCPWM_FREQ_HZ, BDC_MCPWM_GPIO_A, BDC_MCPWM_GPIO_B));
//     motor_ctrl_ctx.motor = &motor;

//     ESP_LOGI(TAG, "Init pcnt driver to decode rotary signal");
//     pcnt_unit_config_t unit_config = {
//         .low_limit = BDC_ENCODER_PCNT_LOW_LIMIT,
//         .high_limit = BDC_ENCODER_PCNT_HIGH_LIMIT,
//         .flags = { .accum_count = true },
//     };
//     pcnt_unit_handle_t pcnt_unit = NULL;
//     ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
//     pcnt_glitch_filter_config_t filter_config = {
//         .max_glitch_ns = 1000,
//     };
//     ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
//     pcnt_chan_config_t chan_a_config = {
//         .edge_gpio_num = BDC_ENCODER_GPIO_A,
//         .level_gpio_num = BDC_ENCODER_GPIO_B,
//     };
//     pcnt_channel_handle_t pcnt_chan_a = NULL;
//     ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
//     pcnt_chan_config_t chan_b_config = {
//         .edge_gpio_num = BDC_ENCODER_GPIO_B,
//         .level_gpio_num = BDC_ENCODER_GPIO_A,
//     };
//     pcnt_channel_handle_t pcnt_chan_b = NULL;
//     ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));
//     ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
//     ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
//     ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
//     ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
//     ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_HIGH_LIMIT));
//     ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_LOW_LIMIT));
//     ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
//     ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
//     ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
//     motor_ctrl_ctx.pcnt_encoder = pcnt_unit;

//     ESP_LOGI(TAG, "Create PID control block");
//     pid_new_control_block(&pid_ctrl, 0.6, 0.4, 0.2, BDC_MCPWM_DUTY_TICK_MAX - 1, 0);
//     motor_ctrl_ctx.pid_ctrl = &pid_ctrl;

//     ESP_LOGI(TAG, "Create a timer to do PID calculation periodically");
//     esp_timer_create_args_t periodic_timer_args = {};
//     periodic_timer_args.callback = pid_loop_cb;
//     periodic_timer_args.arg = &motor_ctrl_ctx;
//     periodic_timer_args.name = "pid_loop";
//     esp_timer_handle_t pid_loop_timer = NULL;
//     ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pid_loop_timer));

//     ESP_LOGI(TAG, "Enable motor");
//     ESP_ERROR_CHECK(bdc_motor_enable(&motor));
//     ESP_LOGI(TAG, "Forward motor");
//     ESP_ERROR_CHECK(bdc_motor_forward(&motor));

//     ESP_LOGI(TAG, "Start motor speed loop");
//     ESP_ERROR_CHECK(esp_timer_start_periodic(pid_loop_timer, BDC_PID_LOOP_PERIOD_MS * 1000));

//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }
