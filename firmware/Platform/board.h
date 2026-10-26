/**
 * @file board.h
 * @brief Єдине місце, де записано зв'язок "функція - пін - периферія" для
 *        NUCLEO-C031C6. Відповідає таблиці docs/pinout.md.
 *
 *  Функція        Пін    AF    Периферія / сигнал     Роз'єм Arduino
 *  I2C SCL        PB8    AF6   I2C1_SCL               D15
 *  I2C SDA        PB9    AF6   I2C1_SDA               D14
 *  UART TX (VCP)  PA2    AF1   USART2_TX              D1
 *  UART RX (VCP)  PA3    AF1   USART2_RX              D0
 *  Потенціометр   PA0    -     ADC1_IN0               A0
 *  Серво (PWM)    PA6    AF1   TIM3_CH1               D12
 *  Кнопка B1      PC13   -     GPIO (вхід, pull-up)   -
 *  LED LD4        PA5    -     GPIO (вихід)           D13
 */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* PWM серво: тік таймера 1 мкс, період 20 мс (50 Гц) */
#define BOARD_SERVO_TIM_PRESCALER   47U      /* 48 МГц / (47 + 1) = 1 МГц */
#define BOARD_SERVO_TIM_PERIOD      19999U   /* 20000 тіків = 20 мс */
#define BOARD_SERVO_PIN             GPIO_PIN_6
#define BOARD_SERVO_PORT            GPIOA
#define BOARD_SERVO_AF              GPIO_AF1_TIM3

#define BOARD_POT_PIN               GPIO_PIN_0
#define BOARD_POT_PORT              GPIOA

/* Кнопка B1 у Wokwi дає низький рівень при натисканні (підтягнута до VDD) */
#define BOARD_BUTTON_ACTIVE_LEVEL   GPIO_PIN_RESET

/* Тайм-аут одного I2C-запиту, мс */
#define BOARD_I2C_TIMEOUT_MS        50U

#endif /* BOARD_H */
