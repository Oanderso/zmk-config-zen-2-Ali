# Azoteq TPS43 / PXM0016 touchpad

This branch adds an Azoteq ProxSense TPS43 (IQS5xx family) touchpad to the
**right/peripheral** half of the Corne-ish Zen v2 and forwards its pointer
events to the left/central half over ZMK split.

The right e-ink display is intentionally disabled and its former signal pins
are reused for the touchpad.

## Wiring

| TPS43 pin | Corne-ish Zen v2 right | Former e-ink use | Notes |
|---|---|---|---|
| `VDDHI` | `3.3 V` | — | Do not connect to 5 V. |
| `GND` | `GND` | — | Common ground. |
| `SDA` | `P0.17` | MOSI | I2C data. |
| `SCL` | `P0.20` | SCK | I2C clock. |
| `nRST` | `P1.04` | EPD reset | Active low. |
| `RDY` | `P1.06` | EPD busy | Active high. |

The former display pins `P0.05`, `P0.09`, and `P0.10` are left unused/free.

The firmware configures I2C at 400 kHz and expects the IQS5xx device at
7-bit address `0x74`.

## Pointer behavior

The IQS5xx driver provides:

- one-finger tap -> left click
- two-finger tap -> right click
- press-and-hold -> left click-and-drag
- two-finger horizontal and vertical scrolling
- relative X/Y pointer movement

The raw relative events are forwarded from the right half with
`zmk,input-split`. The left/central half applies `zmk-input-inertia` last in
the input pipeline, so a quick finger flick continues moving the cursor and
decelerates after lift, similar to a trackball.

### Momentum tuning

The defaults are in `config/corneish_zen_v2_left.overlay`:

```dts
&zip_inertia {
    trigger-ms = <30>;
    move-decay-factor-int = <93>;
    move-report-interval-ms = <20>;
    move-threshold-start = <8>;
    move-threshold-stop = <1>;
};
```

Useful adjustments:

- **Longer glide:** raise `move-decay-factor-int` toward 95-97.
- **Shorter glide:** lower it toward 88-91.
- **Make momentum easier to trigger:** lower `move-threshold-start`.
- **Reduce accidental momentum:** raise `move-threshold-start`.
- **If momentum starts before the finger actually lifts:** raise `trigger-ms`
  (for example 35-40 ms).
- **Smoother/faster inertia updates:** lower `move-report-interval-ms`, at the
  cost of slightly more radio/CPU activity.

Do not set the decay factor to 100: inertia would not naturally decay.

## Orientation

The firmware assumes the pad is mounted in its native orientation. If the
cursor axes are wrong after assembly, edit the `tps43: iqs5xx@74` node in
`config/corneish_zen_v2_right.overlay` and add the appropriate driver
properties:

```dts
switch-xy;
flip-x;
flip-y;
```

Use only the properties needed for the actual physical orientation.

## Dependencies

`config/west.yml` pins:

- `AYM1607/zmk-driver-azoteq-iqs5xx` for the TPS43/IQS5xx hardware driver.
- `amgskobo/zmk-input-inertia` at a revision compatible with this repository's
  pinned ZMK v0.2 endpoint API.

The main ZMK revision remains `v0.2`; this integration does not require a
wholesale ZMK upgrade.
