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

The TPS43 feeds a local input listener on the right central half. The vendored
driver reports actual finger contact to a release-only cursor processor.
Manual movement keeps the existing scaling and gets no added inertia. A recent
one-finger flick glides only after lift; a new touch immediately brakes it.
Clicks, dragging, palms and multi-finger/scroll gestures suppress cursor glide.
Holding still before lifting also prevents a glide.

The existing upstream inertia processor remains responsible for scrolling,
with its previous settings unchanged and its timer-based cursor inertia disabled.

### Momentum tuning

The `zip_cursor_release` node in `config/corneish_zen_v2_right.overlay` sets:

- `retention-percent = <90>`: increase for longer glide; keep below 100.
- `report-interval-ms = <20>`: glide update cadence.
- `start-threshold = <3>`: minimum velocity in counts per glide report.
- `stop-threshold = <1>`: stop once the remaining velocity is small.
- `release-window-ms = <80>`: final movement must be this recent at lift.

Two movement frames are required to estimate velocity. See
`local-modules/tps43-release-inertia/README.md` for driver provenance and tests.


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
- The TPS43/IQS5xx driver is vendored in `local-modules/tps43-release-inertia`
  from `AYM1607/zmk-driver-azoteq-iqs5xx` revision
  `27321f0232b50f0af31eb27ff97d539933467ea4`, with contact reporting added.
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
