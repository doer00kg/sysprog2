/**
 * @file debounce.h
 * @brief Антидребезг кнопки (FR-08): стан вважається змінним, якщо сире значення
 *        не міняється щонайменше DEBOUNCE_MS мс.
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

#define DEBOUNCE_MS 20U

typedef struct {
    int      stable;          /* підтверджений стан: 1 = натиснуто */
    int      last_raw;        /* останнє сире значення */
    uint32_t last_change_ms;  /* коли сире значення востаннє змінилося */
} debounce_t;

void debounce_init(debounce_t *d, int initial_pressed, uint32_t now_ms);

/** Повертає 1 рівно один раз на підтверджене натискання (перехід 0 -> 1). */
int debounce_update(debounce_t *d, int raw_pressed, uint32_t now_ms);

#endif /* DEBOUNCE_H */
