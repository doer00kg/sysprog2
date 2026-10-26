#include "fake_platform.h"
#include "platform.h"

fake_i2c_t *fake_plat_i2c;
uint32_t    fake_plat_millis;
uint16_t    fake_plat_adc;
int         fake_plat_button;
int         fake_plat_init_err;
uint16_t    fake_plat_servo_us;
int         fake_plat_servo_calls;
int         fake_plat_led_toggles;

static i2c_bus_t s_bus;

void fake_plat_reset(fake_i2c_t *i2c)
{
    fake_plat_i2c = i2c;
    fake_plat_millis = 1000;
    fake_plat_adc = 1365; /* підсилення 1.0 */
    fake_plat_button = 0;
    fake_plat_init_err = 0;
    fake_plat_servo_us = 0;
    fake_plat_servo_calls = 0;
    fake_plat_led_toggles = 0;
}

int platform_init(void) { return fake_plat_init_err; }
uint32_t platform_millis(void) { return fake_plat_millis; }

const i2c_bus_t *platform_i2c_bus(void)
{
    s_bus = fake_i2c_bus(fake_plat_i2c);
    return &s_bus;
}

int platform_i2c_probe(uint8_t addr7) { return addr7 == fake_plat_i2c->dev_addr; }
uint16_t platform_adc_read(void) { return fake_plat_adc; }

void platform_servo_set_us(uint16_t us)
{
    fake_plat_servo_us = us;
    fake_plat_servo_calls++;
}

int platform_button_pressed(void) { return fake_plat_button; }
void platform_led_toggle(void) { fake_plat_led_toggles++; }
