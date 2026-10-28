#include "test_framework.h"
#include "fake_i2c.h"
#include "mpu6050.h"
#include <string.h>

static fake_i2c_t fake;
static i2c_bus_t bus;
static mpu6050_t dev;

static void setup(void)
{
    fake_i2c_reset(&fake, MPU6050_ADDR_AD0_LOW);
    fake.regs[MPU6050_REG_WHO_AM_I] = 0x68;
    fake.regs[MPU6050_REG_PWR_MGMT_1] = 0x40; /* стан після скидання: чип спить */
    bus = fake_i2c_bus(&fake);
    memset(&dev, 0, sizeof dev);
}

static void test_init_ok_and_sequence(void)
{
    setup();
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_OK);
    CHECK_EQ(dev.retries, 0);
    CHECK_EQ(dev.fail_step, MPU6050_STEP_NONE);

    /* Спочатку читається WHO_AM_I: запис адреси регістра, потім читання 1 байта */
    CHECK_EQ(fake.log[0].is_read, 0);
    CHECK_EQ(fake.log[0].first_byte, MPU6050_REG_WHO_AM_I);
    CHECK_EQ(fake.log[1].is_read, 1);
    CHECK_EQ(fake.log[1].len, 1);
    /* Лише після цього чип виводиться зі сну */
    CHECK_EQ(fake.log[2].is_read, 0);
    CHECK_EQ(fake.log[2].first_byte, MPU6050_REG_PWR_MGMT_1);

    CHECK_EQ(fake.regs[MPU6050_REG_PWR_MGMT_1], 0x01);
    CHECK_EQ(fake.regs[MPU6050_REG_SMPLRT_DIV], 0x04);
    CHECK_EQ(fake.regs[MPU6050_REG_CONFIG], 0x03);
    CHECK_EQ(fake.regs[MPU6050_REG_GYRO_CONFIG], 0x00);
    CHECK_EQ(fake.regs[MPU6050_REG_ACCEL_CONFIG], 0x00);
}

static void test_init_wrong_id_does_not_wake(void)
{
    setup();
    fake.regs[MPU6050_REG_WHO_AM_I] = 0x70;
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_ERR_ID);
    /* Чужий чип не чіпаємо: PWR_MGMT_1 лишається без змін, записів не було */
    CHECK_EQ(fake.regs[MPU6050_REG_PWR_MGMT_1], 0x40);
    CHECK_EQ(fake.log_count, 2);
}

static void test_init_nack_wrong_address(void)
{
    setup();
    /* Пристрій на 0x69, а драйвер звертається до 0x68 */
    fake.dev_addr = 0x69;
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_ERR_BUS);
    CHECK_EQ(dev.fail_step, MPU6050_STEP_WHO_AM_I);
    CHECK_EQ(dev.retries, 2);                /* обидві спроби отримали NACK */
}

static void test_init_bus_error_on_read_phase(void)
{
    setup();
    fake.fail_from = 1; /* постійний збій, починаючи з читання WHO_AM_I */
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_ERR_BUS);
    CHECK_EQ(dev.fail_step, MPU6050_STEP_WHO_AM_I);
}

static void test_init_bus_error_on_config_write(void)
{
    setup();
    fake.fail_from = 4; /* постійний збій на запису CONFIG (транзакція №4) */
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_ERR_BUS);
    CHECK_EQ(dev.fail_step, MPU6050_STEP_CONFIG);
}

static void test_init_recovers_from_transient_error(void)
{
    setup();
    fake.fail_at = 3; /* разовий збій першої спроби запису SMPLRT_DIV */
    CHECK_EQ(mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW), MPU6050_OK);
    CHECK_EQ(dev.retries, 1);
    CHECK_EQ(dev.fail_step, MPU6050_STEP_NONE);
    CHECK_EQ(fake.regs[MPU6050_REG_SMPLRT_DIV], 0x04);
    CHECK_EQ(fake.regs[MPU6050_REG_ACCEL_CONFIG], 0x00);
}

static void test_params_null(void)
{
    mpu6050_raw_t raw;
    uint8_t id;
    setup();
    CHECK_EQ(mpu6050_init(NULL, &bus, 0x68), MPU6050_ERR_PARAM);
    CHECK_EQ(mpu6050_init(&dev, NULL, 0x68), MPU6050_ERR_PARAM);
    CHECK_EQ(mpu6050_read_raw(NULL, &raw), MPU6050_ERR_PARAM);
    CHECK_EQ(mpu6050_read_who_am_i(NULL, &id), MPU6050_ERR_PARAM);
}

static void test_read_who_am_i(void)
{
    uint8_t id = 0;
    setup();
    dev.bus = &bus;
    dev.addr = MPU6050_ADDR_AD0_LOW;
    CHECK_EQ(mpu6050_read_who_am_i(&dev, &id), MPU6050_OK);
    CHECK_EQ(id, 0x68);
}

/* Тестовий вектор: 14 байтів у порядку регістрів 0x3B..0x48.
 * ax=0x0000, ay=0x0000, az=0x4000 (=16384, тобто 1 g), temp=0x0000,
 * gx=0x0083 (=131, тобто 1 dps), gy=0xFF7D (=-131), gz=0x0000.
 * Пізніше сюди додається вектор із запису логічного аналізатора. */
static const uint8_t VECTOR_REST[14] = {
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x00, 0x83, 0xFF, 0x7D, 0x00, 0x00
};

static void test_read_raw_multibyte_sequence(void)
{
    mpu6050_raw_t raw;
    setup();
    dev.bus = &bus;
    dev.addr = MPU6050_ADDR_AD0_LOW;
    memcpy(&fake.regs[MPU6050_REG_ACCEL_XOUT_H], VECTOR_REST, sizeof VECTOR_REST);

    CHECK_EQ(mpu6050_read_raw(&dev, &raw), MPU6050_OK);

    /* Одне багатобайтове читання: запис 0x3B, потім читання рівно 14 байтів */
    CHECK_EQ(fake.log_count, 2);
    CHECK_EQ(fake.log[0].first_byte, MPU6050_REG_ACCEL_XOUT_H);
    CHECK_EQ(fake.log[1].is_read, 1);
    CHECK_EQ(fake.log[1].len, 14);

    CHECK_EQ(raw.ax, 0);
    CHECK_EQ(raw.az, 16384);
    CHECK_EQ(raw.gx, 131);
    CHECK_EQ(raw.gy, -131);
}

static void test_read_raw_bus_error(void)
{
    mpu6050_raw_t raw;
    setup();
    dev.bus = &bus;
    dev.addr = MPU6050_ADDR_AD0_LOW;
    fake.fail_from = 1;
    CHECK_EQ(mpu6050_read_raw(&dev, &raw), MPU6050_ERR_BUS);
    setup();
    dev.bus = &bus;
    fake.fail_from = 0;
    CHECK_EQ(mpu6050_read_raw(&dev, &raw), MPU6050_ERR_BUS);
}

static void test_read_raw_transient_error_is_retried(void)
{
    mpu6050_raw_t raw;
    setup();
    dev.bus = &bus;
    dev.addr = MPU6050_ADDR_AD0_LOW;
    memcpy(&fake.regs[MPU6050_REG_ACCEL_XOUT_H], VECTOR_REST, sizeof VECTOR_REST);
    fake.fail_at = 1; /* перше читання 14 байтів збоїть, друга спроба проходить */
    CHECK_EQ(mpu6050_read_raw(&dev, &raw), MPU6050_OK);
    CHECK_EQ(dev.retries, 1);
    CHECK_EQ(raw.az, 16384);
}

static void test_convert_rest_is_one_g(void)
{
    mpu6050_raw_t raw;
    mpu6050_data_t d;
    float mag;
    memset(&raw, 0, sizeof raw);
    raw.az = 16384;
    raw.gx = 131;
    raw.gy = -131;
    mpu6050_convert(&raw, &d);

    CHECK_NEAR(d.az_g, 1.0, 1e-6);
    CHECK_NEAR(d.gx_dps, 1.0, 1e-6);
    CHECK_NEAR(d.gy_dps, -1.0, 1e-6);
    /* У спокої модуль прискорення дорівнює 1 g з точністю 1 % */
    mag = sqrtf(d.ax_g * d.ax_g + d.ay_g * d.ay_g + d.az_g * d.az_g);
    CHECK_NEAR(mag, 1.0, 0.01);
}

static void test_convert_temperature_formula(void)
{
    mpu6050_raw_t raw;
    mpu6050_data_t d;
    memset(&raw, 0, sizeof raw);
    mpu6050_convert(&raw, &d);
    CHECK_NEAR(d.temp_c, 36.53, 1e-4);   /* TEMP_OUT = 0 -> 36.53 C */
    raw.temp = -521;
    mpu6050_convert(&raw, &d);
    CHECK_NEAR(d.temp_c, 35.0, 0.01);    /* -521/340 + 36.53 */
}

int main(void)
{
    RUN(test_init_ok_and_sequence);
    RUN(test_init_wrong_id_does_not_wake);
    RUN(test_init_nack_wrong_address);
    RUN(test_init_bus_error_on_read_phase);
    RUN(test_init_bus_error_on_config_write);
    RUN(test_init_recovers_from_transient_error);
    RUN(test_params_null);
    RUN(test_read_who_am_i);
    RUN(test_read_raw_multibyte_sequence);
    RUN(test_read_raw_bus_error);
    RUN(test_read_raw_transient_error_is_retried);
    RUN(test_convert_rest_is_one_g);
    RUN(test_convert_temperature_formula);
    return TEST_REPORT();
}
