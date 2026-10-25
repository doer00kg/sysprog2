/**
 * @file fake_i2c.h
 * @brief Підроблена I2C-шина з одним пристроєм-"регістровою пам'яттю".
 *
 * Це fake (проста робоча заміна реальної шини): має власний стан, записує
 * журнал транзакцій, вміє імітувати NACK. Перевірка послідовності транзакцій
 * робиться через журнал.
 */
#ifndef FAKE_I2C_H
#define FAKE_I2C_H

#include <stdint.h>
#include "i2c_bus.h"

#define FAKE_I2C_LOG_MAX 32

typedef struct {
    int      is_read;
    uint8_t  addr;
    uint16_t len;
    uint8_t  first_byte; /* для запису - перший байт (адреса регістра) */
} fake_i2c_tx_t;

typedef struct {
    uint8_t       dev_addr;            /* на цій адресі "є" пристрій */
    uint8_t       regs[256];
    uint8_t       ptr;                 /* поточний вказівник регістра */
    int           fail_at;             /* номер транзакції з помилкою, -1 = немає */
    fake_i2c_tx_t log[FAKE_I2C_LOG_MAX];
    int           log_count;
} fake_i2c_t;

void fake_i2c_reset(fake_i2c_t *f, uint8_t dev_addr);
i2c_bus_t fake_i2c_bus(fake_i2c_t *f);

#endif /* FAKE_I2C_H */
