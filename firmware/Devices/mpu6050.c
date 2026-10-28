/**
 * @file mpu6050.c
 * @brief Драйвер MPU-6050. Регістр читається двома транзакціями:
 *        запис адреси регістра (з STOP), потім читання. Це працює і на реальному
 *        чипі (він зберігає вказівник регістра), і в Wokwi, де модель I2C не
 *        підтримує repeated START у HAL_I2C_Mem_Read.
 */
#include "mpu6050.h"

#define SMPLRT_DIV_200HZ  0x04U /* 1 кГц / (1 + 4) = 200 Гц (при DLPF увімкненому) */
#define CONFIG_DLPF_44HZ  0x03U
#define PWR_CLK_GYRO_X    0x01U /* прокинутись, тактування від PLL гіроскопа X */

/* Транзакція з повтором: якщо шина дала помилку, платформа її відновлює, і друга
 * спроба зазвичай проходить (спостерігалось у Wokwi: разовий timeout I2C). */
static int write_reg(mpu6050_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    unsigned attempt;
    buf[0] = reg;
    buf[1] = value;
    for (attempt = 0; attempt <= MPU6050_BUS_RETRIES; attempt++) {
        if (dev->bus->write(dev->bus->ctx, dev->addr, buf, 2) == 0) {
            return 0;
        }
        dev->retries++;
    }
    return -1;
}

static int read_regs(mpu6050_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    unsigned attempt;
    for (attempt = 0; attempt <= MPU6050_BUS_RETRIES; attempt++) {
        if (dev->bus->write(dev->bus->ctx, dev->addr, &reg, 1) == 0 &&
            dev->bus->read(dev->bus->ctx, dev->addr, buf, len) == 0) {
            return 0;
        }
        dev->retries++;
    }
    return -1;
}

static int step_write(mpu6050_t *dev, mpu6050_step_t step, uint8_t reg, uint8_t value)
{
    dev->step = (uint8_t)step;
    if (write_reg(dev, reg, value) != 0) {
        dev->fail_step = (uint8_t)step;
        return -1;
    }
    return 0;
}

mpu6050_status_t mpu6050_read_who_am_i(mpu6050_t *dev, uint8_t *id)
{
    if (dev == NULL || id == NULL || dev->bus == NULL ||
        dev->bus->read == NULL || dev->bus->write == NULL) {
        return MPU6050_ERR_PARAM;
    }
    return (read_regs(dev, MPU6050_REG_WHO_AM_I, id, 1) == 0) ? MPU6050_OK
                                                                : MPU6050_ERR_BUS;
}

mpu6050_status_t mpu6050_init(mpu6050_t *dev, const i2c_bus_t *bus, uint8_t addr)
{
    uint8_t id = 0;
    mpu6050_status_t st;

    if (dev == NULL || bus == NULL) {
        return MPU6050_ERR_PARAM;
    }
    dev->bus = bus;
    dev->addr = addr;
    dev->step = MPU6050_STEP_WHO_AM_I;
    dev->fail_step = MPU6050_STEP_NONE;
    dev->retries = 0;

    st = mpu6050_read_who_am_i(dev, &id);
    if (st != MPU6050_OK) {
        dev->fail_step = MPU6050_STEP_WHO_AM_I;
        return st;
    }
    if (id != MPU6050_WHO_AM_I_VALUE) {
        return MPU6050_ERR_ID;
    }

    if (step_write(dev, MPU6050_STEP_PWR_MGMT_1, MPU6050_REG_PWR_MGMT_1, PWR_CLK_GYRO_X) != 0 ||
        step_write(dev, MPU6050_STEP_SMPLRT_DIV, MPU6050_REG_SMPLRT_DIV, SMPLRT_DIV_200HZ) != 0 ||
        step_write(dev, MPU6050_STEP_CONFIG, MPU6050_REG_CONFIG, CONFIG_DLPF_44HZ) != 0 ||
        step_write(dev, MPU6050_STEP_GYRO_CONFIG, MPU6050_REG_GYRO_CONFIG, 0x00U) != 0 ||
        step_write(dev, MPU6050_STEP_ACCEL_CONFIG, MPU6050_REG_ACCEL_CONFIG, 0x00U) != 0) {
        return MPU6050_ERR_BUS;
    }
    return MPU6050_OK;
}

static int16_t be16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

mpu6050_status_t mpu6050_read_raw(mpu6050_t *dev, mpu6050_raw_t *raw)
{
    uint8_t b[14];

    if (dev == NULL || raw == NULL || dev->bus == NULL ||
        dev->bus->read == NULL || dev->bus->write == NULL) {
        return MPU6050_ERR_PARAM;
    }
    if (read_regs(dev, MPU6050_REG_ACCEL_XOUT_H, b, sizeof b) != 0) {
        return MPU6050_ERR_BUS;
    }
    raw->ax   = be16(&b[0]);
    raw->ay   = be16(&b[2]);
    raw->az   = be16(&b[4]);
    raw->temp = be16(&b[6]);
    raw->gx   = be16(&b[8]);
    raw->gy   = be16(&b[10]);
    raw->gz   = be16(&b[12]);
    return MPU6050_OK;
}

void mpu6050_convert(const mpu6050_raw_t *raw, mpu6050_data_t *out)
{
    out->ax_g = (float)raw->ax / MPU6050_ACCEL_LSB_PER_G;
    out->ay_g = (float)raw->ay / MPU6050_ACCEL_LSB_PER_G;
    out->az_g = (float)raw->az / MPU6050_ACCEL_LSB_PER_G;
    /* Register Map: Temperature in degrees C = TEMP_OUT / 340 + 36.53 */
    out->temp_c = (float)raw->temp / 340.0f + 36.53f;
    out->gx_dps = (float)raw->gx / MPU6050_GYRO_LSB_PER_DPS;
    out->gy_dps = (float)raw->gy / MPU6050_GYRO_LSB_PER_DPS;
    out->gz_dps = (float)raw->gz / MPU6050_GYRO_LSB_PER_DPS;
}
