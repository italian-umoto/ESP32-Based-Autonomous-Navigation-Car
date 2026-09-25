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
#include "motor_calibration.h"
#include <Arduino.h>

static const char *TAG = "MOTOR";

// GPIO Pins (L293D inputs M1/M2, quadrature encoder channels A/B)
#define LEFT_M1                       21
#define LEFT_M2                       4  // LARRY I changed this ****
#define LEFT_A                        41
#define LEFT_B                        42
#define RIGHT_M1                      39
#define RIGHT_M2                      40
#define RIGHT_A                       2
#define RIGHT_B                       1

#define BDC_ENCODER_PCNT_HIGH_LIMIT   1000
#define BDC_ENCODER_PCNT_LOW_LIMIT    -1000

// Pid loop update period
#define BDC_PID_LOOP_PERIOD_MS        50

// PID parameters
#define BDC_PID_KP 1
#define BDC_PID_KI 1
#define BDC_PID_KD 0.000 // 0.002

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
    float setpoint;
    bool enabled;
    float previous_err1;
    float previous_err2;
    float last_output;
    int last_encoder_count;
} pid_ctrl_t;

typedef struct {
    bdc_motor_t *motor;
    pcnt_unit_handle_t pcnt_encoder;
} motor_control_context_t;

bdc_motor_t left_motor;
pid_ctrl_t left_pid_ctrl;
motor_control_context_t left_motor_ctrl_ctx;


bdc_motor_t right_motor;
pid_ctrl_t right_pid_ctrl;
motor_control_context_t right_motor_ctrl_ctx;


// This is what the fsm can touch to talk to pid
static portMUX_TYPE cmd_mux = portMUX_INITIALIZER_UNLOCKED;
static struct {
    bool  pending;
    bool  stop;
    float left_cps;
    float right_cps;
} cmd_box = { false, true, 0.0f, 0.0f };

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


static float clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}


static float pid_compute(pid_ctrl_t *pid, float error)
{
    float output = (error - pid->previous_err1) * BDC_PID_KP +
                   (error - pid->previous_err1 - pid->previous_err1 + pid->previous_err2) * BDC_PID_KD +
                   error * BDC_PID_KI +
                   pid->last_output;
    output = clamp(output, -(BDC_MCPWM_DUTY_TICK_MAX - 1), BDC_MCPWM_DUTY_TICK_MAX - 1);

    pid->previous_err2 = pid->previous_err1;
    pid->previous_err1 = error;
    pid->last_output = output;
    return output;
}

static void pid_reset(pid_ctrl_t *pid, int encoder_count)
{
    pid->previous_err1      = 0.0f;
    pid->previous_err2      = 0.0f;
    pid->last_output        = 0.0f;
    pid->last_encoder_count = encoder_count;
}

static int signf(float x) { return (x > 0) - (x < 0); }

static void apply_wheel(pid_ctrl_t *pid, int (*count_fn)(), float setpoint)
{
    if (!pid->enabled || signf(setpoint) != signf(pid->setpoint)) {
        pid_reset(pid, count_fn());
        pid->enabled = true;
    }
    pid->setpoint = setpoint;
}



static void do_stop(void)
{
    left_pid_ctrl.enabled  = false;
    right_pid_ctrl.enabled = false;
    left_pid_ctrl.setpoint  = 0.0f;
    right_pid_ctrl.setpoint = 0.0f;
    bdc_motor_brake(&left_motor);    // or bdc_motor_coast()
    bdc_motor_brake(&right_motor);
}

static void pid_loop_cb(void *args) // args ignored
{
    // reads our fsm interface here and is muxed for safety :)
    bool pending, stop;
    float l, r;
    portENTER_CRITICAL(&cmd_mux);
    pending = cmd_box.pending;
    stop    = cmd_box.stop;
    l       = cmd_box.left_cps;
    r       = cmd_box.right_cps;
    cmd_box.pending = false;
    portEXIT_CRITICAL(&cmd_mux);

    if (pending) {
        if (stop) {
            do_stop();
            return;
        }
        apply_wheel(&left_pid_ctrl,  left_encoder_count,  l);
        apply_wheel(&right_pid_ctrl, right_encoder_count, r);
    }



    if(left_pid_ctrl.enabled) {
        int next_left_count = left_encoder_count();
        int count_delta = next_left_count - left_pid_ctrl.last_encoder_count;
        int cps = count_delta * (1000 / BDC_PID_LOOP_PERIOD_MS);
        float error = left_pid_ctrl.setpoint - cps;
        float feedback_left_speed = pid_compute(&left_pid_ctrl, error);
        float speed = feedforward_left_speed(left_pid_ctrl.setpoint) + feedback_left_speed;
        left_set_signed_speed(speed);
        left_pid_ctrl.last_encoder_count = next_left_count;
    }

    if(right_pid_ctrl.enabled) {
        int next_right_count = right_encoder_count();
        int count_delta = next_right_count - right_pid_ctrl.last_encoder_count;
        int cps = count_delta * (1000 / BDC_PID_LOOP_PERIOD_MS);
        float error = right_pid_ctrl.setpoint - cps;
        float feedback_right_speed = pid_compute(&right_pid_ctrl, error);
        float speed = feedforward_right_speed(right_pid_ctrl.setpoint) + feedback_right_speed;
        right_set_signed_speed(speed);
        right_pid_ctrl.last_encoder_count = next_right_count;
    }
    
}


void motors_command(float left_cps, float right_cps)
{
    portENTER_CRITICAL(&cmd_mux);
    cmd_box.left_cps  = left_cps;
    cmd_box.right_cps = right_cps;
    cmd_box.stop      = false;
    cmd_box.pending   = true;
    portEXIT_CRITICAL(&cmd_mux);
}

void motors_stop(void)
{
    portENTER_CRITICAL(&cmd_mux);
    cmd_box.stop    = true;
    cmd_box.pending = true;
    portEXIT_CRITICAL(&cmd_mux);
}



void print_pid_debug() {
    Serial.printf("L: en=%d set=%.0f cps=%.0f err=%.0f fb=%.1f count=%d\n",
                  left_pid_ctrl.enabled,
                  left_pid_ctrl.setpoint,
                  left_pid_ctrl.setpoint - left_pid_ctrl.previous_err1,
                  left_pid_ctrl.previous_err1,
                  left_pid_ctrl.last_output,
                  left_encoder_count());
    Serial.printf("R: en=%d set=%.0f cps=%.0f err=%.0f fb=%.1f count=%d\n",
                  right_pid_ctrl.enabled,
                  right_pid_ctrl.setpoint,
                  right_pid_ctrl.setpoint - right_pid_ctrl.previous_err1,
                  right_pid_ctrl.previous_err1,
                  right_pid_ctrl.last_output,
                  right_encoder_count());
}

void create_one_motor_with_encoder(bdc_motor_t * motor, motor_control_context_t * motor_ctrl_ctx, int group_id, int motor_M1, int motor_M2, int encoder_A, int encoder_B) {
    ESP_LOGI(TAG, "Create DC motor");
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(motor, group_id, BDC_MCPWM_TIMER_RESOLUTION_HZ, BDC_MCPWM_FREQ_HZ, motor_M1, motor_M2));

    motor_ctrl_ctx -> motor = motor;

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
    


    ESP_LOGI(TAG, "Enable motor");
    ESP_ERROR_CHECK(bdc_motor_enable(motor));
    ESP_LOGI(TAG, "Starting at brake");
    ESP_ERROR_CHECK(bdc_motor_brake(motor)); 
}





void setup_motors() {
    create_one_motor_with_encoder(&left_motor,  &left_motor_ctrl_ctx,  0, RIGHT_M1, RIGHT_M2, RIGHT_A, RIGHT_B);
    create_one_motor_with_encoder(&right_motor, &right_motor_ctrl_ctx, 1, LEFT_M1,  LEFT_M2,  LEFT_A,  LEFT_B);

    
    // setup single shared PID  loop

    ESP_LOGI(TAG, "Create PID loop");

    esp_timer_create_args_t periodic_timer_args = {};
    periodic_timer_args.callback = pid_loop_cb;
    periodic_timer_args.name = "pid_loop";
    esp_timer_handle_t pid_loop_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pid_loop_timer));

    ESP_LOGI(TAG, "Start motor speed loop");
    ESP_ERROR_CHECK(esp_timer_start_periodic(pid_loop_timer, BDC_PID_LOOP_PERIOD_MS * 1000));
}



int left_encoder_count() {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(left_motor_ctrl_ctx.pcnt_encoder, &count));
    return -count;
}

int right_encoder_count() {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(right_motor_ctrl_ctx.pcnt_encoder, &count));
    return -count;
}

void left_set_signed_speed (int speed) {
    speed = clamp(speed, -(BDC_MCPWM_DUTY_TICK_MAX - 1), BDC_MCPWM_DUTY_TICK_MAX - 1);
    if (speed > 0) {
        bdc_motor_forward(&left_motor);
        bdc_motor_set_speed(&left_motor, speed);

    } else {
        bdc_motor_reverse(&left_motor);
        bdc_motor_set_speed(&left_motor, -speed);
    }
}

void right_set_signed_speed (int speed) {
    speed = clamp(speed, -(BDC_MCPWM_DUTY_TICK_MAX - 1), BDC_MCPWM_DUTY_TICK_MAX - 1);
    if (speed > 0) {
        bdc_motor_forward(&right_motor);
        bdc_motor_set_speed(&right_motor, speed);

    } else {
        bdc_motor_reverse(&right_motor);
        bdc_motor_set_speed(&right_motor, -speed);
    }
}


// I took these out for now because the speed will be covered by drive component

//void enable_pid() {
//    left_pid_ctrl.last_encoder_count = left_encoder_count();
//    right_pid_ctrl.last_encoder_count = right_encoder_count();
//
//    left_pid_ctrl.enabled = true;
//    right_pid_ctrl.enabled = true;
//}


//void left_set_speed_pid(int speed){
//    if(!left_pid_ctrl.enabled) enable_pid();
//    left_pid_ctrl.setpoint=speed;   
//}

//void right_set_speed_pid(int speed){
//    if(!right_pid_ctrl.enabled) enable_pid();
//    right_pid_ctrl.setpoint=speed;
//}
