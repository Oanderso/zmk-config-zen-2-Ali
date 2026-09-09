# Azoteq TPS43 / PXM0016 touchpad

This branch adds an Azoteq ProxSense TPS43 (IQS5xx family) touchpad to the
**right/central** half of the Corne-ish Zen v2. The right half is now the
host-facing ZMK split central, so touchpad input and cursor inertia are handled
locally instead of forwarding pointer events over the split link.

The right e-ink display is intentionally disabled and its former signal pins
are reused for the touchpad. The left e-ink display remains enabled on the
left/peripheral half.

## Wiring

| TPS43 pin | Corne-ish Zen v2 right | Former e-ink use | Notes |
|---|---|---|---|
| `VDDHI` | `3.3 V` | VCC | Do not connect to 5 V. |
| `GND` | `GND` | GND | Common ground. |
| `SDA` | `P0.17` | SDA / MOSI | I2C data. |
| `SCL` | `P0.20` | SCL / SCK | I2C clock. |
| `nRST` | `P1.04` | RST | Active low. |
| `RDY` | `P1.06` | BUSY | Active high. |

On the exposed 8-pin screen socket this corresponds to:

- `GND` -> TPS43 `GND`
- `SDA` -> TPS43 `SDA`
- `SCL` -> TPS43 `SCL`
- `CS` -> not connected
- `DC` -> not connected
- `RST` -> TPS43 `nRST`
- `BUSY` -> TPS43 `RDY`
- `VCC` -> TPS43 `VDDHI` / 3.3 V

The former display `CS` (`P0.10`) and `DC` (`P0.09`) pins are left unused.

The firmware configures I2C at 400 kHz and expects the IQS5xx device at
7-bit address `0x74`.

## Split architecture

The right half is configured as `CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y` and is the
USB/BLE host-facing half. It contains the TPS43, its direct ZMK input listener,
and the inertia processor.

The left half is a BLE split peripheral. Its e-ink display remains active, but
central-only output/layer widgets are disabled and replaced by the dedicated
split-peripheral connection-status widget so the display can link correctly in
peripheral firmware.

Because the central role changed from the original firmware, flash **both**
halves after installing this branch. Existing Bluetooth bonds may also need to
be cleared and the keyboard re-paired, because the right half becomes the
host-facing identity.

## Pointer behavior

The IQS5xx driver provides:

- one-finger tap -> left click
- two-finger tap -> right click
- press-and-hold -> left click-and-drag
- two-finger horizontal and vertical scrolling
- relative X/Y pointer movement

The TPS43 feeds a local `zmk,input-listener` on the right central half, and
`zmk-input-inertia` processes the relative pointer events there. A quick finger
flick therefore continues moving the cursor and decelerates after lift, similar
to a trackball.

### Momentum tuning

The defaults are in `config/corneish_zen_v2_right.overlay`:

```dts
&zip_inertia {
    trigger-ms = <30>;
    move-decay-factor-int = <93>;
    move-report-interval-ms = <20>;
    move-threshold-start = <8>;
    move-threshold-stop = <1>;

    scroll-decay-factor-int = <85>;
    scroll-report-interval-ms = <50>;
    scroll-threshold-start = <2>;
    scroll-threshold-stop = <0>;
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
  cost of slightly more CPU activity.

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

## Dependencies and build compatibility

`config/west.yml` pins:

- ZMK firmware `v0.3.0`.
- `AYM1607/zmk-driver-azoteq-iqs5xx` for the TPS43/IQS5xx hardware driver.
- `amgskobo/zmk-input-inertia` at revision
  `57cea03b24ab87c031516abf8d0ab78a8339097b`.

The inertia revision is intentionally retained because it uses the plural
`zmk_endpoints_*` endpoint API provided by ZMK v0.3.0. That revision predates
an upstream CMake include-path fix, so `build.yaml` supplies ZMK's
`app/include` directory to the right-half build explicitly.

Per-half build overrides live in:

- `config/corneish_zen_v2_right.conf` for central, host, pointing, and
  no-display settings.
- `config/corneish_zen_v2_left.conf` for peripheral and display-safe settings.
