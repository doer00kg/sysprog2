/**
 * @file app.h
 * @brief Motion-Mirror: самоперевірка при старті та основний цикл.
 *        Викликається з Core/Src/main.c; про HAL не знає.
 */
#ifndef APP_H
#define APP_H

/**
 * Самоперевірка (bring-up). У UART друкується по рядку на кожну перевірку у форматі
 *   [SELFTEST] <назва> key=value ... result=OK|FAIL
 * і підсумок "[SELFTEST] summary pass=N fail=M". Повертає кількість невдалих перевірок.
 */
int app_init(void);

/** Один крок головного циклу: викликати якомога частіше, без блокувань. */
void app_step(void);

#endif /* APP_H */
