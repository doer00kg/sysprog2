/**
 * @file platform.h
 * @brief Нейтральний інтерфейс до плати для прикладного шару.
 *
 * У цьому заголовку немає жодних типів HAL: App користується лише ним.
 * Реалізація (platform.c) викликає HAL; у хостових тестах її замінює
 * tests/fakes/fake_platform.c.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "i2c_bus.h"

#define PLATFORM_ERR_ADC 0x01
#define PLATFORM_ERR_PWM 0x02

/** Ініціалізувати ADC і PWM (I2C, UART, GPIO вже ініціалізовані кодом CubeMX). 0 = успіх. */
int platform_init(void);

uint32_t platform_millis(void);

/** I2C-шина до датчиків (для драйверів у шарі Devices). */
const i2c_bus_t *platform_i2c_bus(void);

/** 1, якщо пристрій з 7-бітною адресою відповів ACK (для сканування шини). */
int platform_i2c_probe(uint8_t addr7);

/** 12-бітне значення АЦП потенціометра (0 ... 4095). */
uint16_t platform_adc_read(void);

/** Ширина PWM-імпульсу серво в мікросекундах (обмежується 500 ... 2500). */
void platform_servo_set_us(uint16_t us);

/** 1, якщо кнопку натиснуто (сире значення, без антидребезгу). */
int platform_button_pressed(void);

void platform_led_toggle(void);

#endif /* PLATFORM_H */
