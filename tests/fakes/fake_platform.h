/** Підроблена реалізація platform.h для хостових тестів App. */
#ifndef FAKE_PLATFORM_H
#define FAKE_PLATFORM_H

#include <stdint.h>
#include "fake_i2c.h"

extern fake_i2c_t *fake_plat_i2c;     /* шина з пристроєм-IMU */
extern uint32_t    fake_plat_millis;  /* поточний час, мс */
extern uint16_t    fake_plat_adc;
extern int         fake_plat_button;  /* 1 = натиснуто */
extern int         fake_plat_init_err;
extern uint16_t    fake_plat_servo_us;
extern int         fake_plat_servo_calls;
extern int         fake_plat_led_toggles;

void fake_plat_reset(fake_i2c_t *i2c);

#endif
