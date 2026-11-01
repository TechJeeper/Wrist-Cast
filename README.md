# Wrist Cast

Micro-fishing on your Pebble. Flick your wrist to cast, wait for a haptic bite, hook with SELECT, then alternate UP/DOWN to keep tension in the green and reel it in — built to play by feel while looking away.

## Play

- **Flick wrist** (or SELECT) — cast the line
- **Wait** — bobber sits quiet until a vibe
- **SELECT** — hook when it buzzes
- **UP / DOWN** — alternate quickly to stay in the green tension zone
- **BACK** — return to ready / title; from title exits

Catches, streak, and best rarity persist between launches.

## Build (Windows)

```powershell
npm install
./build.ps1
```

Optional: `./build.ps1 -Phone 192.168.x.x` to install, or `-Zip` for a CloudPebble import.

Targets: basalt, diorite, flint, emery (rectangular Pebble Time / Pebble 2 / Time 2).

## Settings (phone)

Vibrate on/off, quiet hours (10pm–7am), bite wait (Short / Normal / Long), cast sensitivity, fight difficulty.
