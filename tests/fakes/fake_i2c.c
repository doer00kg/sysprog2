#include "fake_i2c.h"
#include <string.h>

static int log_tx(fake_i2c_t *f, int is_read, uint8_t addr, uint16_t len, uint8_t first)
{
    int idx = f->log_count;
    if (idx < FAKE_I2C_LOG_MAX) {
        f->log[idx].is_read = is_read;
        f->log[idx].addr = addr;
        f->log[idx].len = len;
        f->log[idx].first_byte = first;
        f->log_count++;
    }
    return idx;
}

static int fake_write(void *ctx, uint8_t addr7, const uint8_t *data, uint16_t len)
{
    fake_i2c_t *f = (fake_i2c_t *)ctx;
    uint16_t i;
    int idx = log_tx(f, 0, addr7, len, len ? data[0] : 0);

    if (idx == f->fail_at || addr7 != f->dev_addr) {
        return -1; /* NACK */
    }
    if (len == 0) {
        return 0;
    }
    f->ptr = data[0];
    for (i = 1; i < len; i++) {
        f->regs[f->ptr++] = data[i];
    }
    return 0;
}

static int fake_read(void *ctx, uint8_t addr7, uint8_t *data, uint16_t len)
{
    fake_i2c_t *f = (fake_i2c_t *)ctx;
    uint16_t i;
    int idx = log_tx(f, 1, addr7, len, 0);

    if (idx == f->fail_at || addr7 != f->dev_addr) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        data[i] = f->regs[f->ptr++];
    }
    return 0;
}

void fake_i2c_reset(fake_i2c_t *f, uint8_t dev_addr)
{
    memset(f, 0, sizeof *f);
    f->dev_addr = dev_addr;
    f->fail_at = -1;
}

i2c_bus_t fake_i2c_bus(fake_i2c_t *f)
{
    i2c_bus_t bus;
    bus.write = fake_write;
    bus.read = fake_read;
    bus.ctx = f;
    return bus;
}
