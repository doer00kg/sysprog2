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

static int write_reg(const mpu6050_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    return dev->bus->write(dev->bus->ctx, dev->addr, buf, 2);
}

static int read_regs(const mpu6050_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev->bus->write(dev->bus->ctx, dev->addr, &reg, 1) != 0) {
        return -1;
    }
    return dev->bus->read(dev->bus->ctx, dev->addr, buf, len);
}

mpu6050_status_t mpu6050_read_who_am_i(const mpu6050_t *dev, uint8_t *id)
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

    st = mpu6050_read_who_am_i(dev, &id);
    if (st != MPU6050_OK) {
        return st;
    }
    if (id != MPU6050_WHO_AM_I_VALUE) {
        return MPU6050_ERR_ID;
    }

    if (write_reg(dev, MPU6050_REG_PWR_MGMT_1, PWR_CLK_GYRO_X) != 0 ||
        write_reg(dev, MPU6050_REG_SMPLRT_DIV, SMPLRT_DIV_200HZ) != 0 ||
        write_reg(dev, MPU6050_REG_CONFIG, CONFIG_DLPF_44HZ) != 0 ||
        write_reg(dev, MPU6050_REG_GYRO_CONFIG, 0x00U) != 0 ||
        write_reg(dev, MPU6050_REG_ACCEL_CONFIG, 0x00U) != 0) {
        return MPU6050_ERR_BUS;
    }
    return MPU6050_OK;
}

static int16_t be16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

mpu6050_status_t mpu6050_read_raw(const mpu6050_t *dev, mpu6050_raw_t *raw)
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
