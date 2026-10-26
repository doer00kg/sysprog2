#include "test_framework.h"
#include "debounce.h"

static void test_bounce_is_ignored(void)
{
    debounce_t d;
    debounce_init(&d, 0, 0);
    /* дребезг: стан міняється кожні 5 мс, стабільності немає */
    CHECK_EQ(debounce_update(&d, 1, 5), 0);
    CHECK_EQ(debounce_update(&d, 0, 10), 0);
    CHECK_EQ(debounce_update(&d, 1, 15), 0);
    CHECK_EQ(debounce_update(&d, 1, 30), 0);  /* лише 15 мс стабільності */
    CHECK_EQ(d.stable, 0);
}

static void test_stable_press_gives_single_event(void)
{
    debounce_t d;
    debounce_init(&d, 0, 0);
    CHECK_EQ(debounce_update(&d, 1, 100), 0);
    CHECK_EQ(debounce_update(&d, 1, 119), 0);   /* 19 мс - ще рано */
    CHECK_EQ(debounce_update(&d, 1, 120), 1);   /* 20 мс - подія */
    CHECK_EQ(debounce_update(&d, 1, 200), 0);   /* утримання не повторює подію */
    CHECK_EQ(d.stable, 1);
}

static void test_release_gives_no_event_then_second_press(void)
{
    debounce_t d;
    debounce_init(&d, 0, 0);
    (void)debounce_update(&d, 1, 100);
    CHECK_EQ(debounce_update(&d, 1, 130), 1);
    CHECK_EQ(debounce_update(&d, 0, 200), 0);
    CHECK_EQ(debounce_update(&d, 0, 230), 0);   /* відпускання без події */
    CHECK_EQ(d.stable, 0);
    CHECK_EQ(debounce_update(&d, 1, 300), 0);
    CHECK_EQ(debounce_update(&d, 1, 330), 1);   /* друге натискання */
}

static void test_initial_pressed_state(void)
{
    debounce_t d;
    debounce_init(&d, 1, 0);
    CHECK_EQ(d.stable, 1);
    CHECK_EQ(debounce_update(&d, 1, 500), 0);   /* уже натиснута при старті - без події */
}

int main(void)
{
    RUN(test_bounce_is_ignored);
    RUN(test_stable_press_gives_single_event);
    RUN(test_release_gives_no_event_then_second_press);
    RUN(test_initial_pressed_state);
    return TEST_REPORT();
}
