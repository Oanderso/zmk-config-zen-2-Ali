/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* IQS5xx tracks fingers in five stable, seven-byte slots. */
struct tps43_point { uint16_t x, y; };
struct tps43_swipe {
    struct tps43_point origin[5];
    uint8_t mask;
    bool captured, tracking, finished;
};

/* Capture the entire touch sequence, including staggered finger releases.
 * Errors/palms and four/five fingers cancel recognition until a full lift. */
static inline void tps43_swipe_contact(struct tps43_swipe *s, unsigned fingers) {
    if (!fingers) {
        *s = (struct tps43_swipe){0};
    } else if (fingers >= 3) {
        s->captured = true;
        if (fingers != 3) { s->finished = true; }
    } else if (s->captured) {
        s->finished = true;
    }
}

/* Returns -1 for up, +1 for down, zero otherwise. Sensor orientation is
 * already applied by XY_CONFIG_0, just as for ordinary cursor movement. */
static inline int tps43_swipe_sample(struct tps43_swipe *s, uint8_t mask,
                                     const struct tps43_point points[5]) {
    if (!s->captured || s->finished) { return 0; }
    unsigned count = 0;
    for (unsigned i = 0; i < 5; i++) { count += !!(mask & (1u << i)); }
    if (count != 3 || (s->tracking && mask != s->mask)) {
        s->finished = true;
        return 0;
    }
    if (!s->tracking) {
        for (unsigned i = 0; i < 5; i++) { s->origin[i] = points[i]; }
        s->mask = mask;
        s->tracking = true;
        return 0;
    }
    int sum_y = 0, sum_x = 0, dy[5] = {0};
    for (unsigned i = 0; i < 5; i++) {
        if (!(mask & (1u << i))) { continue; }
        int dx = (int)points[i].x - s->origin[i].x;
        dy[i] = (int)points[i].y - s->origin[i].y;
        sum_x += dx;
        sum_y += dy[i];
    }
    /* About 4 mm of deliberate vertical travel at the TPS43 default scale.
     * Require each finger to move with the group, not just one wandering. */
    if (abs(sum_y) < 3 * 192 || abs(sum_y) < 2 * abs(sum_x)) { return 0; }
    int direction = sum_y < 0 ? -1 : 1;
    for (unsigned i = 0; i < 5; i++) {
        if (!(mask & (1u << i))) { continue; }
        int dx = (int)points[i].x - s->origin[i].x;
        if (dy[i] * direction < 96 || abs(dy[i]) < 2 * abs(dx)) { return 0; }
    }
    s->finished = true;
    return direction;
}
