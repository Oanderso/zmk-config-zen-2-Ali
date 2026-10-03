/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* One trackpad owns one HID press per button, even when tap and drag overlap.
 * Keyboard mouse keys remain separate owners in ZMK's button reference count. */
struct tps43_buttons {
    uint8_t taps;
    uint8_t reported;
    bool hold;
};
typedef void (*tps43_button_emit)(void *context, unsigned button, bool down);

static inline void tps43_buttons_sync(struct tps43_buttons *s,
                                     tps43_button_emit emit, void *context) {
    uint8_t wanted = s->taps | (s->hold ? 1u : 0u);
    uint8_t released = s->reported & ~wanted;
    uint8_t pressed = wanted & ~s->reported;
    for (unsigned i = 0; i < 3; i++) {
        if (released & (1u << i)) {
            emit(context, i, false);
        }
    }
    for (unsigned i = 0; i < 3; i++) {
        if (pressed & (1u << i)) {
            emit(context, i, true);
        }
    }
    s->reported = wanted;
}

static inline void tps43_buttons_hold(struct tps43_buttons *s, bool down,
                                     tps43_button_emit emit, void *context) {
    s->hold = down;
    tps43_buttons_sync(s, emit, context);
}

static inline void tps43_buttons_release_taps(struct tps43_buttons *s,
                                             tps43_button_emit emit, void *context) {
    s->taps = 0;
    tps43_buttons_sync(s, emit, context);
}

static inline void tps43_buttons_tap(struct tps43_buttons *s, unsigned button,
                                    tps43_button_emit emit, void *context) {
    /* Finish the previous pulse before starting another. Never increment ZMK's
     * press count twice and leave only one pending release. Preserve any drag. */
    tps43_buttons_release_taps(s, emit, context);
    s->taps = 1u << button;
    tps43_buttons_sync(s, emit, context);
}

static inline void tps43_buttons_reset(struct tps43_buttons *s,
                                      tps43_button_emit emit, void *context) {
    s->hold = false;
    tps43_buttons_release_taps(s, emit, context);
}
