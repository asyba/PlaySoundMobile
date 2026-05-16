# PlaySoundMobile

Pebble watchapp that plays sounds on a paired iPhone. Select a sound type from the menu, and the C side tells PebbleKit JS to call `Pebble.playSound()` on iOS.

## Features

- Voice / Alarm / SMS sounds and their loops
- Volume control: max (with exact step calc), down, show current %
- **Alarm + Vol Max**: raises volume to max, then plays alarm
- **Voice+Alarm+Max Lp**: raises volume to max, then alternates voice/alarm every 1.5s (select again to stop)

## Build

```sh
pebble build
pebble build --clean   # clean build
```

> **Note:** Built with a local SDK (`--internal_sdk_build`), target: `flint` only. The official Pebble SDK doesn't have the newer APIs (`music_get_volume_percent`, `Pebble.playSound()`, etc.).
