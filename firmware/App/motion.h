/**
 * @file motion.h
 * @brief Прикладна логіка Motion-Mirror: кути нахилу з IMU, комплементарний
 *        фільтр, підсилення, мертва зона та перетворення кута в ширину
 *        PWM-імпульсу серво. Чиста математика, жодних залежностей від HAL.
 */
#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>

#define MOTION_SERVO_MIN_US   1000U  /* -90 градусів */
#define MOTION_SERVO_MID_US   1500U  /*   0 градусів */
#define MOTION_SERVO_MAX_US   2000U  /* +90 градусів */
#define MOTION_ANGLE_LIMIT_DEG 90.0f
#define MOTION_DEADZONE_DEG    1.5f  /* FR-06: зміни менші за 1,5 градуса ігноруємо */

typedef struct {
    float pitch_deg;
    float roll_deg;
    float alpha;        /* вага гіроскопа у фільтрі, типово 0.98 */
    int   initialized;
} motion_filter_t;

typedef struct {
    float    last_deg;
    uint16_t last_us;
    int      valid;
} servo_cmd_t;

/** Кут тангажа за акселерометром, градуси (ax, ay, az у g). */
float motion_accel_pitch_deg(float ax, float ay, float az);
/** Кут крену за акселерометром, градуси. */
float motion_accel_roll_deg(float ay, float az);

void motion_filter_init(motion_filter_t *f, float alpha);

/**
 * Оновити фільтр. Перший виклик бере кути з акселерометра, далі
 * angle = alpha * (angle + gyro * dt) + (1 - alpha) * accel_angle.
 * gx_dps обертає навколо X (крен), gy_dps навколо Y (тангаж).
 */
void motion_filter_update(motion_filter_t *f, float ax, float ay, float az,
                          float gx_dps, float gy_dps, float dt_s);

/** Підсилення 0.5 ... 2.0 за 12-бітним значенням АЦП потенціометра (FR-07). */
float motion_gain_from_adc(uint16_t adc12);

/** Кут -90 ... +90 у ширину імпульсу 1000 ... 2000 мкс (з обмеженням). */
uint16_t motion_angle_to_us(float deg);

/** Обмежити кут діапазоном +-90. */
float motion_clamp_angle(float deg);

/**
 * Нова команда для серво з мертвою зоною: якщо кут змінився менше ніж на
 * MOTION_DEADZONE_DEG, повертається попередня ширина імпульсу.
 */
uint16_t servo_cmd_update(servo_cmd_t *cmd, float deg);

#endif /* MOTION_H */
