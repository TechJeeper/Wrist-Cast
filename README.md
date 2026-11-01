# Wrist Cast

Micro-fishing on your Pebble. Flick your wrist to cast, wait for a haptic bite, hook with SELECT, then hold UP to reel and DOWN to ease so tension stays in the green — built to play by feel while looking away.

## Play

- **Flick wrist** (or SELECT) — cast the line
- **Wait** — bobber sits quiet until a vibe
- **SELECT** — hook when it buzzes
- **Hold UP** — reel in (fills the reel bar; tension rises)
- **Hold DOWN** — ease the line (tension drops)
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
