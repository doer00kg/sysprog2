/**
 * @file platform.c
 * @brief Реалізація platform.h поверх STM32 HAL. Єдиний файл (поза Core/),
 *        який знає про HAL.
 */
#include "platform.h"
#include "board.h"

extern I2C_HandleTypeDef hi2c1; /* створюється кодом CubeMX у Core/Src/main.c */

static ADC_HandleTypeDef hadc1;
static TIM_HandleTypeDef htim3;
static i2c_bus_t s_bus;

/* ---- I2C ------------------------------------------------------------- */

/* Якщо після невдалої транзакції шина лишилась "зайнятою" (BUSY), HAL відмовляється
 * починати нову. Перезапуск периферії I2C1 (DeInit/Init) скидає її автомат. */
static void i2c_recover_if_busy(I2C_HandleTypeDef *h)
{
    if (__HAL_I2C_GET_FLAG(h, I2C_FLAG_BUSY)) {
        (void)HAL_I2C_DeInit(h);
        (void)HAL_I2C_Init(h);
        (void)HAL_I2CEx_ConfigAnalogFilter(h, I2C_ANALOGFILTER_ENABLE);
        (void)HAL_I2CEx_ConfigDigitalFilter(h, 0);
    }
}

static int bus_write(void *ctx, uint8_t addr7, const uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)ctx;
    HAL_StatusTypeDef r = HAL_I2C_Master_Transmit(h, (uint16_t)(addr7 << 1),
                                                  (uint8_t *)data, len, BOARD_I2C_TIMEOUT_MS);
    if (r != HAL_OK) {
        i2c_recover_if_busy(h);
        return -1;
    }
    return 0;
}

static int bus_read(void *ctx, uint8_t addr7, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)ctx;
    HAL_StatusTypeDef r = HAL_I2C_Master_Receive(h, (uint16_t)(addr7 << 1),
                                                 data, len, BOARD_I2C_TIMEOUT_MS);
    if (r != HAL_OK) {
        i2c_recover_if_busy(h);
        return -1;
    }
    return 0;
}

const i2c_bus_t *platform_i2c_bus(void)
{
    s_bus.write = bus_write;
    s_bus.read = bus_read;
    s_bus.ctx = &hi2c1;
    return &s_bus;
}

int platform_i2c_probe(uint8_t addr7)
{
    return HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr7 << 1), 1, 5) == HAL_OK;
}

/* ---- ADC1 IN0 (PA0) --------------------------------------------------- */

static int adc_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    ADC_ChannelConfTypeDef ch = {0};

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = BOARD_POT_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BOARD_POT_PORT, &gpio);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_12CYCLES_5;
    hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_12CYCLES_5;
    hadc1.Init.OversamplingMode = DISABLE;
    hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return -1;
    }

    ch.Channel = ADC_CHANNEL_0;
    ch.Rank = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    return (HAL_ADC_ConfigChannel(&hadc1, &ch) == HAL_OK) ? 0 : -1;
}

uint16_t platform_adc_read(void)
{
    uint16_t v = 0;
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK) {
            v = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }
        (void)HAL_ADC_Stop(&hadc1);
    }
    return v;
}

/* ---- TIM3 CH1 (PA6): PWM для серво ------------------------------------ */

static int pwm_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef oc = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = BOARD_SERVO_PIN;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = BOARD_SERVO_AF;
    HAL_GPIO_Init(BOARD_SERVO_PORT, &gpio);

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = BOARD_SERVO_TIM_PRESCALER;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = BOARD_SERVO_TIM_PERIOD;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        return -1;
    }

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 1500;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1) != HAL_OK) {
        return -1;
    }
    return (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) == HAL_OK) ? 0 : -1;
}

void platform_servo_set_us(uint16_t us)
{
    if (us < 500U) {
        us = 500U;
    }
    if (us > 2500U) {
        us = 2500U;
    }
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, us); /* тік = 1 мкс */
}

/* ---- Кнопка, LED, час ------------------------------------------------- */

int platform_button_pressed(void)
{
    return HAL_GPIO_ReadPin(User_Button_GPIO_Port, User_Button_Pin) == BOARD_BUTTON_ACTIVE_LEVEL;
}

void platform_led_toggle(void)
{
    HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
}

uint32_t platform_millis(void)
{
    return HAL_GetTick();
}

int platform_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    int err = 0;

    /* Кнопка B1 читається опитуванням (антидребезг у шарі App), тому вхід із pull-up */
    gpio.Pin = User_Button_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(User_Button_GPIO_Port, &gpio);

    if (adc_init() != 0) {
        err |= PLATFORM_ERR_ADC;
    }
    if (pwm_init() != 0) {
        err |= PLATFORM_ERR_PWM;
    }
    return err;
}
