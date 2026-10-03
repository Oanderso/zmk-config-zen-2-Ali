#include <assert.h>
#include <stdio.h>
#include "../local-modules/tps43-release-inertia/src/cursor_glide.h"

static const struct cursor_glide_config cfg = {20, 80, 3, 1, 90};

static struct cursor_glide_state flick(void) {
    struct cursor_glide_state s = {0};
    cursor_glide_contact(&s, &cfg, 1, 100);
    cursor_glide_sample(&s, &cfg, 10, -5, 110);
    cursor_glide_sample(&s, &cfg, 10, -5, 130);
    return s;
}

int main(void) {
    int16_t dx, dy;
    struct cursor_glide_state s = flick();
    for (int i = 0; i < 100; i++) {
        assert(!cursor_glide_step(&s, &cfg, &dx, &dy));
        assert(dx == 0 && dy == 0);
    }
    cursor_glide_contact(&s, &cfg, 0, 140);
    assert(s.active);
    assert(cursor_glide_step(&s, &cfg, &dx, &dy));
    assert(dx == 9 && dy == -4);
    cursor_glide_contact(&s, &cfg, 1, 141);
    assert(!cursor_glide_step(&s, &cfg, &dx, &dy));
    cursor_glide_contact(&s, &cfg, 0, 142);
    assert(!s.active); /* Touch-to-brake cannot relaunch old momentum. */

    s = flick();
    cursor_glide_contact(&s, &cfg, 0, 211);
    assert(!s.active); /* Pause before lift. */
    s = flick();
    cursor_glide_sample(&s, &cfg, 0, 0, 150);
    cursor_glide_contact(&s, &cfg, 0, 160);
    assert(!s.active);

    s = flick();
    cursor_glide_block(&s); /* Click, drag or scroll. */
    cursor_glide_contact(&s, &cfg, 0, 140);
    assert(!s.active);
    for (int fingers = 2; fingers <= 255; fingers += 253) {
        s = flick();
        cursor_glide_contact(&s, &cfg, fingers, 135);
        cursor_glide_contact(&s, &cfg, 1, 136);
        cursor_glide_sample(&s, &cfg, 20, 20, 137);
        cursor_glide_contact(&s, &cfg, 0, 140);
        assert(!s.active); /* Multi-touch or palm cannot become a flick. */
    }

    s = (struct cursor_glide_state){0};
    cursor_glide_contact(&s, &cfg, 1, 100);
    cursor_glide_sample(&s, &cfg, 20, 0, 110);
    cursor_glide_contact(&s, &cfg, 0, 120);
    assert(!s.active); /* A single sample is not enough to infer speed. */

    s = flick();
    cursor_glide_sample(&s, &cfg, 32767, -32768, 131);
    cursor_glide_contact(&s, &cfg, 0, 132);
    assert(s.vx_q8 == 127 * 256 && s.vy_q8 == -127 * 256);
    int steps = 0;
    while (cursor_glide_step(&s, &cfg, &dx, &dy)) {
        assert(dx >= 0 && dy <= 0 && dx <= 127 && dy >= -127);
        assert(++steps < 100);
    }
    assert(!s.active);
    puts("Release-only cursor glide tests passed");
    return 0;
}
