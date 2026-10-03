# TPS43 release-only cursor inertia

The IQS5xx driver and bindings are vendored from
`AYM1607/zmk-driver-azoteq-iqs5xx` at
`27321f0232b50f0af31eb27ff97d539933467ea4` under its MIT license.
The hardware configuration, gesture handling and relative motion are retained.
The driver now reports finger count through ABS_MISC on every ready frame and
sets an immediate atomic contact gate before queuing those reports. Palm,
excess-finger and communication/reset conditions inhibit cursor glide.
The existing TP_EVENT mode already reports finger additions/removals; see
[Azoteq section 8.8.1](https://www.azoteq.com/images/stories/pdf/iqs5xx-b000_trackpad_datasheet.pdf).

The local ZMK processor consumes contact metadata and passes manual X/Y
unchanged. Only a recent one-finger lift starts a decaying cursor glide.
Touching again cancels it, even without movement. Button presses, drag and
scroll/multi-finger gestures block cursor glide until the next fresh touch.
The existing upstream processor continues to handle scrolling unchanged.

Defaults in the right overlay: 20 ms reports, 90% velocity retention per
report, start threshold 3 counts/report, stop threshold 1, and 80 ms maximum
age of the final movement sample. Two movement frames are needed to infer
velocity. Pause before lifting to avoid a flick. Higher retention gives a
longer glide; keep it below 100. Higher start threshold requires a faster flick.

`tests/test_cursor_glide.c` exercises the same portable state engine used by
firmware. CI runs these tests with address/undefined-behavior sanitizers before
building firmware. Hardware validation is still needed for feel and timing.
