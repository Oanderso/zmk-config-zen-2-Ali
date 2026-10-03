#include <assert.h>
#include <stdio.h>
#include "../local-modules/tps43-release-inertia/drivers/input/tps43_buttons.h"

struct host {
    int count[3];
    unsigned downs[3], ups[3];
};

static void emit(void *context, unsigned button, bool down) {
    struct host *h = context;
    if (down) {
        h->count[button]++;
        h->downs[button]++;
    } else {
        assert(h->count[button] > 0);
        h->count[button]--;
        h->ups[button]++;
    }
}

int main(void) {
    struct tps43_buttons s = {0};
    struct host h = {0};
    /* Repeated taps before the 100 ms timeout must never leak a press count. */
    for (unsigned i = 0; i < 20; i++) {
        tps43_buttons_tap(&s, 0, emit, &h);
        assert(h.count[0] == 1);
    }
    tps43_buttons_release_taps(&s, emit, &h);
    assert(h.count[0] == 0 && h.downs[0] == 20 && h.ups[0] == 20);
    tps43_buttons_release_taps(&s, emit, &h); /* Harmless redundant timer. */

    /* Alternating left/right taps balance both buttons. */
    tps43_buttons_tap(&s, 0, emit, &h);
    tps43_buttons_tap(&s, 1, emit, &h);
    assert(h.count[0] == 0 && h.count[1] == 1);
    tps43_buttons_release_taps(&s, emit, &h);
    assert(h.count[1] == 0);

    /* A tap timeout cannot release a drag, or vice versa. */
    tps43_buttons_tap(&s, 0, emit, &h);
    tps43_buttons_hold(&s, true, emit, &h);
    tps43_buttons_hold(&s, true, emit, &h);
    assert(h.count[0] == 1);
    tps43_buttons_release_taps(&s, emit, &h);
    assert(h.count[0] == 1);
    tps43_buttons_hold(&s, false, emit, &h);
    assert(h.count[0] == 0);
    tps43_buttons_hold(&s, true, emit, &h);
    tps43_buttons_tap(&s, 0, emit, &h);
    tps43_buttons_hold(&s, false, emit, &h);
    assert(h.count[0] == 1);
    tps43_buttons_release_taps(&s, emit, &h);
    assert(h.count[0] == 0);

    /* Hold-release and tap in one frame still produce a complete click. */
    tps43_buttons_hold(&s, true, emit, &h);
    tps43_buttons_hold(&s, false, emit, &h);
    unsigned before = h.downs[0];
    tps43_buttons_tap(&s, 0, emit, &h);
    tps43_buttons_release_taps(&s, emit, &h);
    assert(h.downs[0] == before + 1 && h.count[0] == 0);

    /* Sensor recovery clears only trackpad ownership, not a held keyboard key. */
    emit(&h, 0, true); /* Keyboard MB1 owns one press. */
    tps43_buttons_hold(&s, true, emit, &h);
    tps43_buttons_tap(&s, 1, emit, &h);
    tps43_buttons_reset(&s, emit, &h);
    assert(h.count[0] == 1 && h.count[1] == 0);
    tps43_buttons_reset(&s, emit, &h);
    emit(&h, 0, false);
    assert(h.count[0] == 0);
    for (unsigned i = 0; i < 3; i++) {
        assert(h.downs[i] == h.ups[i]);
    }
    puts("TPS43 button ownership tests passed");
    return 0;
}
