# Project Agents - PlaySoundMobile

## Project Overview
Pebble watchapp that triggers sounds to play on a paired iPhone. The C side sends sound type messages to PebbleKit JS, which calls `Pebble.playSound()` on iOS.

## Tech Stack
- **Platform:** Pebble SDK 4 / PebbleOS
- **Languages:** C (native), JavaScript (PebbleKit JS)
- **Build System:** Waf (pebble-sdk)
- **Target Platforms:** aplite, basalt, chalk, diorite, emery, flint, gabbro

## Project Structure
```
PlaySoundMobile/
├── src/
│   ├── c/
│   │   └── PlaySoundMobile.c          # Native C code (PebbleKit)
│   └── pkjs/
│       └── index.js                # PebbleKit JS code
├── build/                          # Build output (auto-generated)
├── package.json                    # Pebble project config
├── wscript                          # Build script (Waf)
├── AGENTS.md                       # This file
└── .lock-waf_darwin_build          # Build lock file
```

## Build Commands
```bash
# Build the app
pebble build

# Clean build
pebble build --clean
```

> **Note:** Built with a local SDK (`./waf configure --board asterix --internal_sdk_build`), target: `flint` only. The official Pebble SDK lacks newer APIs (`music_get_volume_percent`, `Pebble.playSound()`, etc.).  
> Multi-platform builds require the official prebuilt SDK.

## Features

### Sound Section
- **Voice** — plays a single voice alert
- **Voice Loop** — loops voice alert
- **Alarm** — plays a single alarm sound
- **Alarm Loop** — loops alarm sound
- **SMS** — plays a single SMS sound
- **SMS Loop** — loops SMS sound
- **Stop All** — sends STOP_SOUND to JS
- **Alarm + Vol Max** — raises volume to max (with exact step calculation), then plays alarm
- **Alt Loop Max** (subtitle: Vc+Alarm) — raises volume to max, then alternates voice/alarm every 1.5s; select again to stop
- **Default Ringtone** — plays the phone's default ringtone (via `Pebble.playDefaultRingtone()`)
- **Sound By ID 1004** — plays iOS system sound ID 1004 (mail sent) via `Pebble.playSoundById()`

### Airplane Mode Detection
- Uses `connection_service_peek_pebble_app_connection()` to check phone connectivity before playing sounds
- **In quick launch mode**: if phone is not connected, shows **"Phone not connected"** instead of auto-playing; Back exits
- **In menu mode**: selecting any sound action while phone is not connected pushes a warning window with "Phone not connected"; Back returns to menu
- Stop actions (Stop All, re-selecting a loop) are still allowed

### One-Click Action (Quick Launch)
- When launched via **Quick Launch** (`APP_LAUNCH_QUICK_LAUNCH`), the app bypasses the menu and enters one-click mode:
  - If phone is connected: **auto-starts** `alarm_loop` and shows **"Playing..."**; **Select** toggles start/stop; **Back** stops the loop and exits
  - If phone is not connected (airplane mode): shows **"Disable first airplane mode"**; Back exits
- When launched normally, the full menu is shown (unchanged)

### Volume Section
- **Vol Max** — sends 16 `music_volume_up()` calls via 100ms timers, calculating exact steps needed from current volume (`ceil((100 - current) * 4 / 25)`)
- **Vol Down** — single `music_volume_down()`
- **Vol Value** — displays current `music_get_volume_percent()` as subtitle

## Message Keys (AppMessage)
- `PLAY_SOUND` — sent from C to JS with sound type string ("voice", "alarm", "sms", etc.)
- `STOP_SOUND` — sent from C to JS to stop playback
- `PLAY_ERROR` — sent from JS to C on failure
- `PLAY_SUCCESS` — sent from JS to C on success
- `PLAY_DEFAULT_RINGTONE` — sent from C to JS to play the phone's default ringtone
- `PLAY_SOUND_BY_ID` — sent from C to JS with iOS system sound ID string

Note: `VOLUME_SET_MAX` is **not used**. Volume max is done entirely on the C side via direct firmware calls (`music_volume_up()`, `music_get_volume_percent()`).

## Reference
- [Official Pebble SDK docs (LLMs.txt)](https://developer.repebble.com/llms.txt)

## Important Notes
- This is a **watchface=false** app (interactive app)
- Uses `enableMultiJS: true` for JavaScript support
- Communication between JS and C via AppMessage protocol
- `simple_menu_layer_reload_data()` does **not exist** in PebbleOS SDK — use `menu_layer_reload_data(simple_menu_layer_get_menu_layer(...))` instead
- Volume ramp uses async 100ms timers; completion callbacks via `s_volume_done_cb` function pointer
- Alternating loop (Voice+Alarm) uses a separate 1500ms timer, cancellable via `stop_alt_loop()`
