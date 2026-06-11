# MeshtasticDS

A Nintendo 3DS homebrew app for sending and receiving messages over a [Meshtastic](https://meshtastic.org/) mesh network via a Wi-Fi bridge running on a Heltec ESP32.

```
┌─────────────┐   Wi-Fi / TCP   ┌──────────────┐   LoRa   ┌──────────────┐
│  Nintendo   │ ←─────────────→ │  Heltec ESP32│ ←──────→ │  Meshtastic  │
│  3DS (app)  │   192.168.4.1   │ BridgeModule │          │  mesh nodes  │
└─────────────┘     port 4444   └──────────────┘          └──────────────┘
```

## Hardware Required

| Component | Notes |
|-----------|-------|
| Nintendo 3DS / 3DS XL / 2DS | Any model; Wi-Fi required |
| Heltec LoRa 32 V1 (ESP32) | Other ESP32 boards need minor pin changes |
| Meshtastic firmware v2.7.15 | See [Firmware](#firmware--bridgemodule) |
| Micro-USB cable | For flashing the ESP32 |

## How It Works

The 3DS has no Bluetooth and no TLS stack in the homebrew toolchain, so it cannot talk directly to stock Meshtastic firmware. A custom `BridgeModule` running on the Heltec ESP32 solves this:

- Creates its own Wi-Fi access point: **`meshtastic-bridge`** at `192.168.4.1`
- Runs a plain-TCP JSON-lines server on **port 4444**
- Bridges messages between the 3DS and the LoRa mesh network
- Passes the Nintendo connection test so the 3DS will save the AP (no internet required)

Message format over TCP:
```
{"text":"hello mesh"}\n
```

## Building the 3DS App

Requires Podman or Docker.

**For hardware (default — targets `192.168.4.1`):**
```bash
podman build -t meshtastic-3ds .
mkdir -p out
podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds
```

**For emulator testing (targets `127.0.0.1`):**
```bash
podman build --build-arg BRIDGE_IP=127.0.0.1 -t meshtastic-3ds .
podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds
```

Output: `out/meshtastic3ds.3dsx`

### Installing on Hardware

Copy `out/meshtastic3ds.3dsx` to your SD card at `/3ds/meshtastic3ds.3dsx` and launch it from the Homebrew Launcher.

> No `.cia` is produced — CIA packaging requires `makerom`, which is not part of devkitPro.

### Testing with an Emulator

See [docs/emulator-testing.md](docs/emulator-testing.md) for step-by-step instructions using Azahar (Citra successor) and the included mock bridge server (`test-bridge-mock.sh`).

## Firmware / BridgeModule

The `src/modules/` directory contains the custom `BridgeModule` that must be compiled into Meshtastic firmware and flashed to the Heltec.

**Tested against:** Meshtastic `v2.7.15` (the current stable release for Heltec V1). The `master` branch has a known regression for Heltec V1 as of 2026-06.

**Setup summary:**

1. Clone Meshtastic firmware at the `v2.7.15` tag:
   ```bash
   git clone --branch v2.7.15.567b8ea https://github.com/meshtastic/firmware ~/meshtastic-firmware
   ```

2. Copy the bridge module into the firmware tree:
   ```bash
   cp src/modules/BridgeModule.{h,cpp} ~/meshtastic-firmware/src/modules/
   ```

3. Register it in `src/mesh/generated/Modules.cpp` (add one `new BridgeModule();` line, same pattern as `ExternalNotificationModule`).

4. Add the BLE-exclude flag to `variants/esp32/heltec_v1/platformio.ini`:
   ```ini
   build_flags = ... -D MESHTASTIC_EXCLUDE_BLUETOOTH=1
   ```
   > This is required. The classic ESP32 crashes under simultaneous BLE + WiFi AP due to heap constraints. The 3DS uses Wi-Fi only, so BLE is unnecessary.

5. Flash:
   ```bash
   cd ~/meshtastic-firmware
   pio run -e heltec-v1 -t upload --upload-port /dev/ttyUSB0
   ```
   No manual bootloader mode needed — the CP2102 auto-resets via DTR/RTS.

6. **After flashing:** leave Meshtastic's own Wi-Fi **disabled** in config. The BridgeModule owns the Wi-Fi radio and brings up its own AP.

See [docs/flashing.md](docs/flashing.md) for the full guide including post-flash configuration and verification steps.

## Controls (3DS App)

| Button | Action |
|--------|--------|
| A | Open on-screen keyboard and type a message |
| X | Send the typed message |
| B | Backspace |
| Y | Reconnect to bridge |
| START | Exit |

## Project Structure

```
src/
  3ds/              3DS homebrew app (C++, devkitPro)
    main.cpp        Main loop, console UI
    network_client  TCP socket client, JSON protocol
    ui_handler      UI stubs (Phase 5 graphics target)
  modules/          ESP32 firmware module
    BridgeModule.h
    BridgeModule.cpp
Dockerfile          Containerised 3DS build environment
test-bridge-mock.sh Python mock TCP bridge for local testing
docs/
  emulator-testing.md  Full emulator + mock bridge walkthrough
  flashing.md          ESP32 flashing + configuration guide
  PROGRESS.md          Detailed build history and decisions
```

## Status

The system is fully functional and hardware-verified:

- 3DS app: Citro2D graphical UI complete — matrix-green cyberpunk aesthetic, CRT scanlines, `.:eLoRa.:.3Ds:.` branding
- ESP32 firmware: compiled, flashed to Heltec V1, AP confirmed up, LoRa TX confirmed (`txGood=1`)
- End-to-end PC simulation (standing in for the 3DS) verified message transmission over LoRa

**Pending:**
- Message persistence to SD card
- Message scroll (view beyond 12 messages), timestamps, sender info
- Wi-Fi reconnection countdown feedback

## License

MIT
