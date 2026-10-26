#include "debounce.h"

void debounce_init(debounce_t *d, int initial_pressed, uint32_t now_ms)
{
    d->stable = initial_pressed ? 1 : 0;
    d->last_raw = d->stable;
    d->last_change_ms = now_ms;
}

int debounce_update(debounce_t *d, int raw_pressed, uint32_t now_ms)
{
    int raw = raw_pressed ? 1 : 0;

    if (raw != d->last_raw) {
        d->last_raw = raw;
        d->last_change_ms = now_ms;
        return 0;
    }
    if (raw != d->stable && (now_ms - d->last_change_ms) >= DEBOUNCE_MS) {
        d->stable = raw;
        return raw; /* подія лише при натисканні, не при відпусканні */
    }
    return 0;
}
