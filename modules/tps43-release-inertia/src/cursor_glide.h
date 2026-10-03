/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

struct cursor_glide_config {
    uint16_t interval_ms;
    uint16_t release_window_ms;
    uint16_t start_threshold;
    uint16_t stop_threshold;
    uint8_t retention_percent;
};

struct cursor_glide_state {
    uint8_t fingers;
    bool blocked;
    bool sampled;
    bool active;
    uint32_t last_sample_ms;
    int32_t vx_q8, vy_q8;
};

static inline int32_t glide_abs(int32_t v) { return v < 0 ? -v : v; }

static inline void cursor_glide_block(struct cursor_glide_state *s) {
    s->active = false;
    s->blocked = true;
    s->vx_q8 = s->vy_q8 = 0;
    s->sampled = false;
}

static inline void cursor_glide_contact(struct cursor_glide_state *s,
                                       const struct cursor_glide_config *c,
                                       uint8_t fingers, uint32_t now) {
    uint8_t previous = s->fingers;
    s->fingers = fingers;
    if (fingers) {
        s->active = false;
        if (previous == 0) {
            s->blocked = false;
            s->sampled = false;
            s->vx_q8 = s->vy_q8 = 0;
        }
        if (fingers != 1) {
            cursor_glide_block(s);
        }
    } else if (previous != 0) {
        /* Only an actual one-finger lift can launch a recent flick. A pause,
         * drag, tap, or two-finger gesture never launches cursor inertia. */
        s->active = previous == 1 && !s->blocked && s->sampled &&
                    (uint32_t)(now - s->last_sample_ms) <= c->release_window_ms &&
                    (glide_abs(s->vx_q8) >= (int32_t)c->start_threshold * 256 ||
                     glide_abs(s->vy_q8) >= (int32_t)c->start_threshold * 256);
    }
}

static inline void cursor_glide_sample(struct cursor_glide_state *s,
                                      const struct cursor_glide_config *c,
                                      int16_t dx, int16_t dy, uint32_t now) {
    if (s->fingers != 1 || s->blocked) {
        return;
    }
    uint32_t dt = now - s->last_sample_ms;
    bool usable = s->sampled && dt > 0 && dt <= c->release_window_ms;
    s->last_sample_ms = now;
    s->sampled = true;
    s->vx_q8 = s->vy_q8 = 0;
    if (usable) {
        /* Normalize to the glide cadence without changing the manual delta.
         * Bound each output report and use signed multiplication (no negative
         * left shifts). X/Y come from the same completed sensor frame. */
        int32_t vx = (int32_t)dx * c->interval_ms * 256 / (int32_t)dt;
        int32_t vy = (int32_t)dy * c->interval_ms * 256 / (int32_t)dt;
        const int32_t limit = 127 * 256;
        s->vx_q8 = vx > limit ? limit : (vx < -limit ? -limit : vx);
        s->vy_q8 = vy > limit ? limit : (vy < -limit ? -limit : vy);
    }
}

static inline bool cursor_glide_step(struct cursor_glide_state *s,
                                    const struct cursor_glide_config *c,
                                    int16_t *dx, int16_t *dy) {
    *dx = *dy = 0;
    if (!s->active || s->fingers != 0 || s->blocked) {
        return false;
    }
    s->vx_q8 = s->vx_q8 * c->retention_percent / 100;
    s->vy_q8 = s->vy_q8 * c->retention_percent / 100;
    if (glide_abs(s->vx_q8) <= (int32_t)c->stop_threshold * 256 &&
        glide_abs(s->vy_q8) <= (int32_t)c->stop_threshold * 256) {
        s->active = false;
        return false;
    }
    *dx = s->vx_q8 / 256;
    *dy = s->vy_q8 / 256;
    return true;
}
