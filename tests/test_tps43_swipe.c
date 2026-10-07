/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "../local-modules/tps43-release-inertia/drivers/input/tps43_swipe.h"

static void start(struct tps43_swipe *s, struct tps43_point p[5], uint8_t mask) {
    *s = (struct tps43_swipe){0};
    for (unsigned i = 0; i < 5; i++) { p[i] = (struct tps43_point){500, 700}; }
    tps43_swipe_contact(s, 3);
    assert(tps43_swipe_sample(s, mask, p) == 0);
}
static void move(struct tps43_point p[5], int x, int y) {
    for (unsigned i = 0; i < 5; i++) { p[i].x += x; p[i].y += y; }
}
int main(void) {
    struct tps43_swipe s = {0};
    struct tps43_point p[5];
    start(&s, p, 0x15); /* Sparse slots, not the first three. */
    move(p, 0, -191);
    assert(tps43_swipe_sample(&s, 0x15, p) == 0);
    move(p, 0, -1);
    assert(tps43_swipe_sample(&s, 0x15, p) == -1);
    move(p, 0, -200);
    assert(tps43_swipe_sample(&s, 0x15, p) == 0);
    tps43_swipe_contact(&s, 2);
    assert(s.captured);
    tps43_swipe_contact(&s, 3);
    assert(tps43_swipe_sample(&s, 0x15, p) == 0);
    tps43_swipe_contact(&s, 0);
    assert(!s.captured && !s.finished);

    start(&s, p, 7);
    move(p, 10, 200);
    assert(tps43_swipe_sample(&s, 7, p) == 1);
    start(&s, p, 7);
    move(p, 300, 200);
    assert(tps43_swipe_sample(&s, 7, p) == 0); /* Horizontal/diagonal. */
    start(&s, p, 7);
    p[0].y += 700;
    assert(tps43_swipe_sample(&s, 7, p) == 0); /* One finger wandering. */
    start(&s, p, 7);
    move(p, 0, 250);
    p[2].y = 450;
    assert(tps43_swipe_sample(&s, 7, p) == 0); /* Opposing finger. */
    start(&s, p, 7);
    move(p, 0, 250);
    assert(tps43_swipe_sample(&s, 0x0B, p) == 0); /* Slot replacement. */
    assert(s.finished);
    for (unsigned n = 4; n <= 255; n += 251) {
        start(&s, p, 7);
        tps43_swipe_contact(&s, n);
        tps43_swipe_contact(&s, 3);
        move(p, 0, 250);
        assert(tps43_swipe_sample(&s, 7, p) == 0);
        assert(s.captured);
    }
    start(&s, p, 7);
    tps43_swipe_contact(&s, 1);
    tps43_swipe_contact(&s, 3);
    move(p, 0, 250);
    assert(tps43_swipe_sample(&s, 7, p) == 0);
    start(&s, p, 7);
    assert(tps43_swipe_sample(&s, 3, p) == 0); /* Invalid frame. */
    puts("TPS43 three-finger swipe tests passed");
    return 0;
}
