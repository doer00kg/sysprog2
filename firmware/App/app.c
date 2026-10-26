/**
 * @file app.c
 * @brief Склейка Motion-Mirror: IMU -> фільтр -> підсилення -> серво.
 *
 * Лог пишеться цілими числами (key=value): printf з newlib-nano не друкує float,
 * а такий формат ще й зручно розбирати скриптом.
 *   _mg   - тисячні частки g,   _cdeg - соті частки градуса.
 */
#include "app.h"
#include <stdio.h>
#include <math.h>
#include "platform.h"
#include "mpu6050.h"
#include "motion.h"
#include "debounce.h"

#define SAMPLE_PERIOD_MS   5U     /* 200 Гц (FR-03) */
#define ADC_PERIOD_MS      100U
#define LOG_PERIOD_MS      1000U
#define LED_PERIOD_MS      500U
#define FILTER_ALPHA       0.98f
#define SERVO_NEUTRAL_US   1500U
#define ACCEL_MIN_MG       900    /* у спокої |a| = 1 g з точністю 10 % */
#define ACCEL_MAX_MG       1100

static mpu6050_t      s_imu;
static motion_filter_t s_filter;
static servo_cmd_t    s_servo;
static debounce_t     s_button;
static int            s_sensor_ok;
static int            s_hold;
static float          s_gain = 1.0f;
static uint16_t       s_servo_us = SERVO_NEUTRAL_US;
static uint32_t       s_last_sample, s_last_adc, s_last_log, s_last_led;
static uint32_t       s_read_errors;
static int            s_pass, s_fail;

static void report(int ok)
{
    printf("result=%s\r\n", ok ? "OK" : "FAIL");
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
    }
}

static long to_mg(float g)
{
    return lroundf(g * 1000.0f);
}

int app_init(void)
{
    uint8_t addr;
    uint8_t id = 0;
    int found = 0;
    int plat_err;
    mpu6050_status_t st;

    s_pass = 0;
    s_fail = 0;
    s_sensor_ok = 0;
    s_hold = 0;
    s_read_errors = 0;

    printf("\r\n[SELFTEST] begin\r\n");

    printf("[SELFTEST] uart baud=115200 ");
    report(1);

    plat_err = platform_init();
    printf("[SELFTEST] platform adc_pwm_init err=0x%02X ", plat_err);
    report(plat_err == 0);

    /* Сканування шини: 7-бітні адреси 0x08 ... 0x77 */
    printf("[SELFTEST] i2c_scan found=");
    for (addr = 0x08; addr < 0x78; addr++) {
        if (platform_i2c_probe(addr)) {
            printf("0x%02X,", addr);
            if (addr == MPU6050_ADDR_AD0_LOW) {
                found = 1;
            }
        }
    }
    printf(" ");
    report(found);

    st = mpu6050_init(&s_imu, platform_i2c_bus(), MPU6050_ADDR_AD0_LOW);
    (void)mpu6050_read_who_am_i(&s_imu, &id);
    printf("[SELFTEST] mpu6050 status=%d who_am_i=0x%02X ", (int)st, id);
    s_sensor_ok = (st == MPU6050_OK);
    report(s_sensor_ok);

    if (s_sensor_ok) {
        mpu6050_raw_t raw;
        mpu6050_data_t d;
        if (mpu6050_read_raw(&s_imu, &raw) == MPU6050_OK) {
            long mag;
            mpu6050_convert(&raw, &d);
            mag = to_mg(sqrtf(d.ax_g * d.ax_g + d.ay_g * d.ay_g + d.az_g * d.az_g));
            printf("[SELFTEST] imu_plausibility accel_mag_mg=%ld temp_cdeg=%ld ", mag,
                   lroundf(d.temp_c * 100.0f));
            report(mag >= ACCEL_MIN_MG && mag <= ACCEL_MAX_MG);
        } else {
            printf("[SELFTEST] imu_plausibility read_error ");
            report(0);
        }
    }

    {
        uint16_t adc = platform_adc_read();
        printf("[SELFTEST] adc raw=%u gain_x100=%ld ", adc, lroundf(motion_gain_from_adc(adc) * 100.0f));
        report(adc <= 4095U);
    }

    platform_servo_set_us(SERVO_NEUTRAL_US);
    printf("[SELFTEST] servo pulse_us=%u ", SERVO_NEUTRAL_US);
    report(1);

    printf("[SELFTEST] button pressed=%d ", platform_button_pressed());
    report(1);

    printf("[SELFTEST] summary pass=%d fail=%d\r\n", s_pass, s_fail);

    motion_filter_init(&s_filter, FILTER_ALPHA);
    s_servo.valid = 0;
    s_servo.last_deg = 0.0f;
    s_servo.last_us = SERVO_NEUTRAL_US;
    s_servo_us = SERVO_NEUTRAL_US;
    debounce_init(&s_button, platform_button_pressed(), platform_millis());
    s_last_sample = s_last_adc = s_last_log = s_last_led = platform_millis();
    return s_fail;
}

static void sample_imu(uint32_t now)
{
    mpu6050_raw_t raw;
    mpu6050_data_t d;
    float dt = (float)(now - s_last_sample) / 1000.0f;

    s_last_sample = now;
    if (!s_sensor_ok) {
        return; /* FR-02: при помилці датчика серво лишається в нейтралі */
    }
    if (mpu6050_read_raw(&s_imu, &raw) != MPU6050_OK) {
        s_read_errors++;
        return;
    }
    mpu6050_convert(&raw, &d);
    motion_filter_update(&s_filter, d.ax_g, d.ay_g, d.az_g, d.gx_dps, d.gy_dps, dt);

    if (!s_hold) {
        s_servo_us = servo_cmd_update(&s_servo, s_filter.pitch_deg * s_gain);
        platform_servo_set_us(s_servo_us);
    }
}

void app_step(void)
{
    uint32_t now = platform_millis();

    if (debounce_update(&s_button, platform_button_pressed(), now)) {
        s_hold = !s_hold;
        printf("[EVENT] t_ms=%lu mode=%s\r\n", (unsigned long)now, s_hold ? "HOLD" : "LIVE");
    }
    if (now - s_last_sample >= SAMPLE_PERIOD_MS) {
        sample_imu(now);
    }
    if (now - s_last_adc >= ADC_PERIOD_MS) {
        s_last_adc = now;
        s_gain = motion_gain_from_adc(platform_adc_read());
    }
    if (now - s_last_led >= LED_PERIOD_MS) {
        s_last_led = now;
        platform_led_toggle();
    }
    if (now - s_last_log >= LOG_PERIOD_MS) {
        s_last_log = now;
        printf("[LOG] t_ms=%lu pitch_cdeg=%ld roll_cdeg=%ld gain_x100=%ld servo_us=%u mode=%s read_err=%lu\r\n",
               (unsigned long)now, lroundf(s_filter.pitch_deg * 100.0f),
               lroundf(s_filter.roll_deg * 100.0f), lroundf(s_gain * 100.0f), s_servo_us,
               s_hold ? "HOLD" : "LIVE", (unsigned long)s_read_errors);
    }
}
