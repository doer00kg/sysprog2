#include "motion.h"
#include <math.h>

#define RAD_TO_DEG 57.2957795f

float motion_accel_pitch_deg(float ax, float ay, float az)
{
    return atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
}

float motion_accel_roll_deg(float ay, float az)
{
    return atan2f(ay, az) * RAD_TO_DEG;
}

void motion_filter_init(motion_filter_t *f, float alpha)
{
    f->pitch_deg = 0.0f;
    f->roll_deg = 0.0f;
    f->alpha = alpha;
    f->initialized = 0;
}

void motion_filter_update(motion_filter_t *f, float ax, float ay, float az,
                          float gx_dps, float gy_dps, float dt_s)
{
    float acc_pitch = motion_accel_pitch_deg(ax, ay, az);
    float acc_roll = motion_accel_roll_deg(ay, az);

    if (!f->initialized) {
        f->pitch_deg = acc_pitch;
        f->roll_deg = acc_roll;
        f->initialized = 1;
        return;
    }
    f->pitch_deg = f->alpha * (f->pitch_deg + gy_dps * dt_s) + (1.0f - f->alpha) * acc_pitch;
    f->roll_deg = f->alpha * (f->roll_deg + gx_dps * dt_s) + (1.0f - f->alpha) * acc_roll;
}

float motion_gain_from_adc(uint16_t adc12)
{
    if (adc12 > 4095U) {
        adc12 = 4095U;
    }
    return 0.5f + 1.5f * ((float)adc12 / 4095.0f);
}

float motion_clamp_angle(float deg)
{
    if (deg > MOTION_ANGLE_LIMIT_DEG) {
        return MOTION_ANGLE_LIMIT_DEG;
    }
    if (deg < -MOTION_ANGLE_LIMIT_DEG) {
        return -MOTION_ANGLE_LIMIT_DEG;
    }
    return deg;
}

uint16_t motion_angle_to_us(float deg)
{
    float d = motion_clamp_angle(deg);
    /* -90..+90 -> 1000..2000 мкс, тобто 500/90 мкс на градус навколо 1500 */
    return (uint16_t)((float)MOTION_SERVO_MID_US + d * (500.0f / MOTION_ANGLE_LIMIT_DEG) + 0.5f);
}

uint16_t servo_cmd_update(servo_cmd_t *cmd, float deg)
{
    float d = motion_clamp_angle(deg);

    if (!cmd->valid || fabsf(d - cmd->last_deg) >= MOTION_DEADZONE_DEG) {
        cmd->last_deg = d;
        cmd->last_us = motion_angle_to_us(d);
        cmd->valid = 1;
    }
    return cmd->last_us;
}
