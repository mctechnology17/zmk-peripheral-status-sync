# zmk-peripheral-status-sync

<!-- mc-follow-top:start -->
<div align="center">

  <a href="https://youtube.com/@mctechnology17"><img align="center" alt="MC Technology | YouTube" width="22px" src="https://github.com/mctechnology17/mctechnology17/blob/main/src/youtube.png" /></a>
  <a href="https://www.instagram.com/mctechnology17/"><img align="center" alt="MC Technology | Instagram" width="22px" src="https://github.com/mctechnology17/mctechnology17/blob/main/src/instagram.png" /></a>
  <a href="https://www.facebook.com/mctechnology17/"><img align="center" alt="MC Technology | Facebook" width="22px" src="https://github.com/mctechnology17/mctechnology17/blob/main/src/facebook.png" /></a>
  <a href="https://www.tiktok.com/@mctechnology17"><img align="center" alt="MC Technology | TikTok" width="22px" src="https://github.com/mctechnology17/mctechnology17/blob/main/src/tiktok.png" /></a>

</div>
<!-- mc-follow-top:end -->

ZMK module that shows the state of the **central** half on the **peripheral**
half of a split keyboard: WPM, modifiers, BLE profile, active layer, central
battery and HID indicators (Caps Lock, Num Lock, Scroll Lock).

The central sends everything in **one** split message. The peripheral keeps the
state and raises one event per field, so any display widget can read it.

- No devicetree node is needed.
- Works over the normal ZMK split link (it uses the split behavior channel).
- Only the fields you enable are sent.

## Versions, branches and tags

All my ZMK projects follow the same rules. The idea is that every project can be paired with ZMK in its different versions (v0.3, v0.4, ...), so you always know which branch or tag fits the ZMK version you use.

What to write as `revision:` in your `west.yml`:

| You want | `revision:` |
|---|---|
| The latest stable version | `main` |
| Stay on ZMK v0.3 | `zmk-v0.3` |
| The next ZMK (v0.4, Zephyr 4.1), in preparation | `zmk-v0.4` (this branch) |
| A fixed version that never changes | a tag, for example `v0.0.3.1` |

Branches:

- `main`: the stable version. It follows the newest ZMK version that the project supports. Today that is ZMK v0.3.
- `zmk-v0.3`: the line for ZMK v0.3. While `main` is on ZMK v0.3 both are the same. When `main` moves to a newer ZMK, this branch stays on ZMK v0.3 and keeps receiving fixes. The next lines are created the same way (`zmk-v0.4`, ...).
- `dev`: development. It can be broken at any time, do not use it for your keyboard.

Tags:

The third number of a tag is the ZMK version it is made for:

- `v0.0.3.1`, `v0.0.3.2`, ...: versions for ZMK v0.3, new features and fixes.
- `v0.0.4`, `v0.0.4.1`, ...: the versions for the next ZMK version (v0.4).
- A version that is still being tested carries a suffix, for example `v0.0.3.1-fix`.

Every release says which ZMK version it was tested with. The full list of changes is in [CHANGELOG.txt](CHANGELOG.txt).

## Add the module

`config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: mctechnology17
      url-base: https://github.com/mctechnology17
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-peripheral-status-sync
      remote: mctechnology17
      revision: zmk-v0.4
  self:
    path: config
```

## Central half

Choose what to send, in the `.conf` of the central half:

```ini
CONFIG_ZMK_WPM=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_WPM=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS=y
```

Any of them turns the sender on.

| Option | Sends | Needs |
|---|---|---|
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_WPM` | words per minute | `CONFIG_ZMK_WPM=y` |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS` | explicit modifiers (Shift, Ctrl, Alt, GUI) | |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE` | BLE profile index, connected, bonded | Bluetooth |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER` | highest active layer | |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY` | central battery level and charging | a battery sensor on the central |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS` | Caps / Num / Scroll Lock | |

## Peripheral half

Choose what to keep, in the `.conf` of the peripheral half:

```ini
CONFIG_ZMK_WPM_PERIPHERAL=y
CONFIG_ZMK_MODIFIERS_PERIPHERAL=y
CONFIG_ZMK_PROFILE_PERIPHERAL=y
CONFIG_ZMK_LAYER_PERIPHERAL=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY=y
CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS=y
```

The receiver turns itself on when any of them is enabled.

| Option | State | Read it with | Event |
|---|---|---|---|
| `CONFIG_ZMK_WPM_PERIPHERAL` | WPM | `zmk_wpm_peripheral_get_state()` | `zmk_wpm_peripheral_state_changed` |
| `CONFIG_ZMK_MODIFIERS_PERIPHERAL` | modifiers | `zmk_modifiers_peripheral_get_state()` | `zmk_modifiers_peripheral_state_changed` |
| `CONFIG_ZMK_PROFILE_PERIPHERAL` | BLE profile | `zmk_profile_peripheral_get_state()` | `zmk_profile_peripheral_state_changed` |
| `CONFIG_ZMK_LAYER_PERIPHERAL` | layer | `zmk_layer_peripheral_get_state()` | `zmk_layer_peripheral_state_changed` |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY` | central battery | `zmk_peripheral_status_sync_get_central_battery()` | `zmk_central_battery_peripheral_state_changed` |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS` | HID indicators | `zmk_peripheral_status_sync_get_hid_indicators()` | `zmk_central_hid_indicators_peripheral_state_changed` |

Extras on the peripheral:

- **WPM without the central:** `CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_LOCAL=y`
  calculates WPM from the keys of the peripheral half itself.
- **Layer names:** the central sends only the layer number. Names are set on
  the peripheral with `CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_0="Base"` ...
  `CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_15`.
- **Profile after a reboot:** with `CONFIG_SETTINGS=y` the peripheral remembers
  the last profile it received (`CONFIG_ZMK_PROFILE_PERIPHERAL_SETTINGS`).
- **Central battery:** it has its own event, so it never overwrites the battery
  of the peripheral half.

## Display widgets

[zmk-nice-oled](https://github.com/mctechnology17/zmk-nice-oled) draws all of
this on the peripheral screen (`CONFIG_NICE_OLED_WIDGET_WPM_PERIPHERAL`,
`..._MODIFIERS_PERIPHERAL_INDICATORS`, `..._PROFILE_PERIPHERAL`,
`..._LAYER_PERIPHERAL`, `..._CENTRAL_BATTERY_PERIPHERAL`). Those widgets stay
disabled without this module.

A plain ZMK label for WPM is included: `CONFIG_ZMK_WIDGET_WPM_PERIPHERAL_STATUS=y`.

## Tuning

| Option | Default | What it does |
|---|---|---|
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INTERVAL_MS` | 10 | Joins changes that happen close together into one message |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_DELAY_MS` | 1500 | Wait after boot before the first message |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_RETRY_MS` | 1000 | Time between the first messages while the split link settles |
| `CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_ATTEMPTS` | 60 | How many of those first messages are sent |

## How it works

The message is the split behavior `ussync` with one 64-bit snapshot:

```txt
param1 bits:
  0..3    version
  4..10   dirty_mask (which fields changed)
  11..18  wpm
  19..26  modifiers
  27..31  layer

param2 bits:
  0..2    profile_index
  3       profile_connected
  4       profile_bonded
  5..12   central_battery_level
  13      central_battery_charging
  14..16  hid_indicators
  17..28  reserved
  29..31  sequence
```

Pointer status is not part of this message: it lives in its own module.

<!-- mc-follow:start -->
## Support this project

This is a personal project that I build in my free time. If it is useful to
you, the best way to help is to follow and subscribe:

[YouTube](https://youtube.com/@mctechnology17) · [Instagram](https://www.instagram.com/mctechnology17/) · [Facebook](https://www.facebook.com/mctechnology17/) · [TikTok](https://www.tiktok.com/@mctechnology17)

I make videos about split keyboards, ZMK and open source (in Spanish, long
videos with English subtitles). Every follow and every subscription helps me a
lot and keeps me motivated to bring more to the open source community.

Thank you for every report and every issue: they make these projects better.
<!-- mc-follow:end -->

## License

MIT
