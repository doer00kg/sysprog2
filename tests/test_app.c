#include "test_framework.h"
#include "fake_i2c.h"
#include "fake_platform.h"
#include "mpu6050.h"
#include "app.h"
#include <string.h>

static fake_i2c_t imu;

static void set_accel_raw(int16_t ax, int16_t ay, int16_t az)
{
    uint8_t *r = &imu.regs[MPU6050_REG_ACCEL_XOUT_H];
    r[0] = (uint8_t)((uint16_t)ax >> 8); r[1] = (uint8_t)ax;
    r[2] = (uint8_t)((uint16_t)ay >> 8); r[3] = (uint8_t)ay;
    r[4] = (uint8_t)((uint16_t)az >> 8); r[5] = (uint8_t)az;
}

static void setup(void)
{
    fake_i2c_reset(&imu, MPU6050_ADDR_AD0_LOW);
    imu.regs[MPU6050_REG_WHO_AM_I] = 0x68;
    set_accel_raw(0, 0, 16384);             /* плата лежить рівно: 1 g по Z */
    fake_plat_reset(&imu);
}

static void run_steps(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        fake_plat_millis += 5;
        app_step();
    }
}

static void test_selftest_passes_with_healthy_hardware(void)
{
    setup();
    CHECK_EQ(app_init(), 0);
    CHECK_EQ(fake_plat_servo_us, 1500);     /* після самоперевірки серво в нейтралі */
}

static void test_sensor_missing_keeps_servo_neutral(void)
{
    setup();
    imu.regs[MPU6050_REG_WHO_AM_I] = 0x00;  /* чужий або відсутній чип */
    CHECK(app_init() > 0);
    fake_plat_servo_calls = 0;
    set_accel_raw(-8192, 0, 14189);
    run_steps(50);
    CHECK_EQ(fake_plat_servo_calls, 0);     /* FR-02: серво не рухається */
}

static void test_tilt_moves_servo_with_gain_one(void)
{
    setup();
    CHECK_EQ(app_init(), 0);
    set_accel_raw(-8192, 0, 14189);         /* нахил приблизно 30 градусів */
    run_steps(40);
    /* 30 градусів * 1.0 -> 1500 + 30 * 500/90 = 1667 мкс */
    CHECK_NEAR(fake_plat_servo_us, 1667, 4);
}

static void test_gain_potentiometer_scales_angle(void)
{
    setup();
    fake_plat_adc = 4095;                   /* підсилення 2.0 */
    CHECK_EQ(app_init(), 0);
    set_accel_raw(-8192, 0, 14189);
    run_steps(60);                          /* 300 мс: підсилення встигає оновитись */
    /* 30 * 2.0 = 60 градусів -> 1500 + 60 * 500/90 = 1833 мкс */
    CHECK_NEAR(fake_plat_servo_us, 1833, 5);
}

static void test_button_hold_freezes_servo(void)
{
    setup();
    CHECK_EQ(app_init(), 0);
    set_accel_raw(-8192, 0, 14189);
    run_steps(40);
    {
        uint16_t before = fake_plat_servo_us;
        fake_plat_button = 1;               /* натискаємо B1 і тримаємо */
        run_steps(10);                      /* 50 мс > 20 мс антидребезгу -> HOLD */
        fake_plat_button = 0;
        set_accel_raw(0, 0, 16384);         /* плата вирівнялась, але серво заморожене */
        run_steps(40);
        CHECK_EQ(fake_plat_servo_us, before);
    }
}

static void test_led_and_log_run_without_blocking(void)
{
    setup();
    CHECK_EQ(app_init(), 0);
    run_steps(250);                         /* 1.25 с */
    CHECK(fake_plat_led_toggles >= 2);
}

static void test_read_error_is_counted_not_fatal(void)
{
    setup();
    CHECK_EQ(app_init(), 0);
    imu.dev_addr = 0x70;                    /* чип "зник" з шини під час роботи */
    fake_plat_servo_calls = 0;
    run_steps(20);
    CHECK_EQ(fake_plat_servo_calls, 0);
}

int main(void)
{
    RUN(test_selftest_passes_with_healthy_hardware);
    RUN(test_sensor_missing_keeps_servo_neutral);
    RUN(test_tilt_moves_servo_with_gain_one);
    RUN(test_gain_potentiometer_scales_angle);
    RUN(test_button_hold_freezes_servo);
    RUN(test_led_and_log_run_without_blocking);
    RUN(test_read_error_is_counted_not_fatal);
    return TEST_REPORT();
}
