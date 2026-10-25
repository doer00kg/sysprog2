#include "test_framework.h"
#include "motion.h"

static void test_angle_to_us(void)
{
    CHECK_EQ(motion_angle_to_us(0.0f), 1500);
    CHECK_EQ(motion_angle_to_us(90.0f), 2000);
    CHECK_EQ(motion_angle_to_us(-90.0f), 1000);
    CHECK_EQ(motion_angle_to_us(45.0f), 1750);
    CHECK_EQ(motion_angle_to_us(200.0f), 2000);   /* обмеження зверху */
    CHECK_EQ(motion_angle_to_us(-200.0f), 1000);  /* обмеження знизу */
}

static void test_accel_angles(void)
{
    CHECK_NEAR(motion_accel_pitch_deg(0.0f, 0.0f, 1.0f), 0.0, 1e-3);
    CHECK_NEAR(motion_accel_roll_deg(0.0f, 1.0f), 0.0, 1e-3);
    /* нахил на 30 градусів: sin 30 = 0.5, cos 30 = 0.8660254 */
    CHECK_NEAR(motion_accel_pitch_deg(-0.5f, 0.0f, 0.8660254f), 30.0, 0.01);
    CHECK_NEAR(motion_accel_roll_deg(0.5f, 0.8660254f), 30.0, 0.01);
}

static void test_filter_first_sample_uses_accel(void)
{
    motion_filter_t f;
    motion_filter_init(&f, 0.98f);
    motion_filter_update(&f, -0.5f, 0.0f, 0.8660254f, 100.0f, 100.0f, 0.005f);
    CHECK_NEAR(f.pitch_deg, 30.0, 0.01);  /* гіроскоп на першому кроці ігнорується */
    CHECK_EQ(f.initialized, 1);
}

static void test_filter_gyro_integration(void)
{
    motion_filter_t f;
    motion_filter_init(&f, 0.98f);
    motion_filter_update(&f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.005f);
    /* 10 dps * 5 мс = 0.05 градуса, з вагою 0.98 -> 0.049 */
    motion_filter_update(&f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f, 0.005f);
    CHECK_NEAR(f.pitch_deg, 0.049, 1e-4);
    CHECK_NEAR(f.roll_deg, 0.049, 1e-4);
}

static void test_filter_converges_to_accel(void)
{
    motion_filter_t f;
    int i;
    motion_filter_init(&f, 0.98f);
    motion_filter_update(&f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.005f);
    for (i = 0; i < 1000; i++) {   /* 5 секунд при 200 Гц */
        motion_filter_update(&f, -0.5f, 0.0f, 0.8660254f, 0.0f, 0.0f, 0.005f);
    }
    CHECK_NEAR(f.pitch_deg, 30.0, 0.5);
}

static void test_gain_from_adc(void)
{
    CHECK_NEAR(motion_gain_from_adc(0), 0.5, 1e-6);
    CHECK_NEAR(motion_gain_from_adc(4095), 2.0, 1e-6);
    CHECK_NEAR(motion_gain_from_adc(2048), 1.25, 0.01);
    CHECK_NEAR(motion_gain_from_adc(60000), 2.0, 1e-6); /* значення вище діапазону */
}

static void test_servo_deadzone(void)
{
    servo_cmd_t c = {0.0f, 0, 0};
    CHECK_EQ(servo_cmd_update(&c, 0.0f), 1500);      /* перша команда проходить */
    CHECK_EQ(servo_cmd_update(&c, 1.0f), 1500);      /* 1.0 < 1.5 - ігноруємо */
    CHECK_EQ(servo_cmd_update(&c, -1.4f), 1500);
    CHECK_EQ(servo_cmd_update(&c, 2.0f), 1511);      /* 2 градуси - оновлюємо */
    CHECK_EQ(servo_cmd_update(&c, 3.0f), 1511);      /* від 2.0 різниця 1.0 - ігноруємо */
    CHECK_EQ(servo_cmd_update(&c, 120.0f), 2000);    /* обмеження 90 градусів */
}

int main(void)
{
    RUN(test_angle_to_us);
    RUN(test_accel_angles);
    RUN(test_filter_first_sample_uses_accel);
    RUN(test_filter_gyro_integration);
    RUN(test_filter_converges_to_accel);
    RUN(test_gain_from_adc);
    RUN(test_servo_deadzone);
    return TEST_REPORT();
}
