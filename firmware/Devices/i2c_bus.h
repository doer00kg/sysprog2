/**
 * @file i2c_bus.h
 * @brief Абстрактний інтерфейс I2C-шини для драйверів пристроїв.
 *
 * Драйвер сенсора не знає про HAL: він отримує вказівник на цю структуру.
 * На платі функції реалізує шар Platform (поверх HAL), у хостових тестах -
 * підроблена шина (tests/fakes/fake_i2c.c).
 *
 * Усі функції повертають 0 при успіху і від'ємне число при помилці
 * (NACK, timeout, збій шини).
 */
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>

typedef struct i2c_bus {
    /** Записати len байтів пристрою з 7-бітною адресою addr7 (START..STOP). */
    int (*write)(void *ctx, uint8_t addr7, const uint8_t *data, uint16_t len);
    /** Прочитати len байтів із пристрою з 7-бітною адресою addr7 (START..STOP). */
    int (*read)(void *ctx, uint8_t addr7, uint8_t *data, uint16_t len);
    /** Довільний контекст, який передається у write/read (на платі - хендл I2C). */
    void *ctx;
} i2c_bus_t;

#endif /* I2C_BUS_H */
