/**
 * @file mpu6050.h
 * @brief Драйвер IMU MPU-6050 (акселерометр + гіроскоп + температура) по I2C.
 *
 * Джерело: InvenSense MPU-6000/MPU-6050 Product Specification та Register Map.
 * Драйвер залежить лише від i2c_bus.h, HAL не підключає.
 */
#ifndef MPU6050_H
#define MPU6050_H

#include <stddef.h>
#include <stdint.h>
#include "i2c_bus.h"

/* 7-бітна адреса при AD0 = GND (при AD0 = VCC було б 0x69) */
#define MPU6050_ADDR_AD0_LOW   0x68U

/* Регістри (Register Map) */
#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_WHO_AM_I      0x75U

#define MPU6050_WHO_AM_I_VALUE    0x68U

/* Чутливості при діапазонах за замовчуванням: +-2 g та +-250 dps */
#define MPU6050_ACCEL_LSB_PER_G   16384.0f
#define MPU6050_GYRO_LSB_PER_DPS  131.0f

typedef enum {
    MPU6050_OK        =  0,
    MPU6050_ERR_BUS   = -1, /**< NACK, timeout або інша помилка шини */
    MPU6050_ERR_ID    = -2, /**< WHO_AM_I не дорівнює 0x68 */
    MPU6050_ERR_PARAM = -3  /**< порожній вказівник */
} mpu6050_status_t;

/** Кроки mpu6050_init (для діагностики: на якому кроці сталася помилка шини) */
typedef enum {
    MPU6050_STEP_NONE = 0,
    MPU6050_STEP_WHO_AM_I = 1,
    MPU6050_STEP_PWR_MGMT_1 = 2,
    MPU6050_STEP_SMPLRT_DIV = 3,
    MPU6050_STEP_CONFIG = 4,
    MPU6050_STEP_GYRO_CONFIG = 5,
    MPU6050_STEP_ACCEL_CONFIG = 6
} mpu6050_step_t;

/** Скільки разів повторюється транзакція після помилки шини. */
#define MPU6050_BUS_RETRIES 1U

typedef struct {
    const i2c_bus_t *bus;
    uint8_t addr;
    uint8_t step;       /**< поточний крок ініціалізації */
    uint8_t fail_step;  /**< крок, на якому ініціалізація остаточно не вдалась (0 = ні) */
    uint8_t retries;    /**< скільки повторних спроб довелось зробити (лічильник) */
} mpu6050_t;

/** Сирі значення з регістрів 0x3B..0x48 */
typedef struct {
    int16_t ax, ay, az;
    int16_t temp;
    int16_t gx, gy, gz;
} mpu6050_raw_t;

/** Значення у фізичних одиницях */
typedef struct {
    float ax_g, ay_g, az_g;
    float temp_c;
    float gx_dps, gy_dps, gz_dps;
} mpu6050_data_t;

/**
 * Перевірити WHO_AM_I, вивести чип зі сну (джерело тактування - PLL гіроскопа X),
 * налаштувати частоту вибірки 200 Гц (DLPF 44 Гц), діапазони +-2 g і +-250 dps.
 */
mpu6050_status_t mpu6050_init(mpu6050_t *dev, const i2c_bus_t *bus, uint8_t addr);

/** Прочитати WHO_AM_I (регістр 0x75). */
mpu6050_status_t mpu6050_read_who_am_i(mpu6050_t *dev, uint8_t *id);

/** Прочитати 14 байтів з 0x3B одним читанням. */
mpu6050_status_t mpu6050_read_raw(mpu6050_t *dev, mpu6050_raw_t *raw);

/** Перерахувати сирі значення у g, градуси за секунду і градуси Цельсія. */
void mpu6050_convert(const mpu6050_raw_t *raw, mpu6050_data_t *out);

#endif /* MPU6050_H */
