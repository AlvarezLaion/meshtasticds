# Project Progress – Nintendo 3DS Meshtastic Messenger

## ✅ Completed steps (updated 2026-06-11)

- **FULL SYSTEM VALIDATED ON REAL 3DS HARDWARE (2026-06-11)** — End-to-end test with the actual Nintendo 3DS for the first time:
  - Netloaded the Phase 5 `.3dsx` to the 3DS over Wi-Fi via `3dslink` (PC `wlan0` joined the `meshtastic-bridge` AP at `192.168.4.4`; 3DS at `192.168.4.6`). 221 KB transferred, app launched.
  - Green cyberpunk UI confirmed rendering correctly on real hardware (header `.:eLoRa.:.3Ds:.`, message log, status bar, scanlines).
  - Sent the message "kk" from the 3DS; captured the complete forwarding chain on the Heltec serial console: `[BridgeModule] decoded message (... Portnum=1 ...)` → `forwarded to mesh: kk` → `[RadioIf] Started Tx (encrypted len=24)` → `Packet TX: 1975ms` → `Completed sending`. **Message genuinely transmitted over LoRa.**
  - Note: attaching to the Heltec serial (`/dev/ttyUSB0`) triggers the CP2102 auto-reset, rebooting the board (~50 s back to AP-ready); the 3DS must press **Y** to reconnect afterward. Serial monitoring used a no-DTR/RTS pyserial script (`/tmp/watch_serial.py`) but the open-time reset is unavoidable on this board.
  - **Result: the entire pipeline — 3DS app → Wi-Fi/TCP bridge → LoRa mesh — is proven working on real hardware.**

- **Phase 5: Citro2D graphical UI — COMPLETE (2026-06-11)** — Replaced console-only `printf()` rendering with a full GPU-accelerated cyberpunk/matrix UI using Citro2D:
  - `ui_handler.{h,cpp}` fully implemented; `main.cpp` refactored to use `UIHandler`
  - Full-green matrix aesthetic: black background, `#00FF00` message text, `#00CC00` chrome (borders, header, status bar), CRT scanline overlay every 4px
  - Top screen: amber→green header bar with `.:eLoRa.:.3Ds:.` branding + `[MESH CHAT]`, 12-line scrolling message log, connection status bar
  - Bottom screen: `INPUT:` label, blinking `> _` cursor, key hint bar
  - Color-coded messages: bright green (self), matrix green (received), orange (warnings), white (errors)
  - Citro2D linked via `portlibs/3ds` — VFP ABI compatible; build: zero warnings, zero errors
  - Verified live in Azahar emulator; screenshot saved

## ✅ Completed steps (updated 2026-06-10)
- **Repository scaffolding** – `README.md`, `.gitignore` created.
- **Heltec bridge module** – `src/modules/BridgeModule.{h,cpp}` added and registered in `src/mesh/Modules.cpp`.
- **Dockerfile** – written to build the 3DS home‑brew app with devkitPro inside a container.
- **Task tracking** – created task #1 (`Create PROGRESS.md file`).
- **API capability research** – Confirmed stock Meshtastic exposes REST API (HTTPS 4403) and JSON-RPC TCP (4369), but custom BridgeModule is justified for 3DS because devkitPro lacks TLS/SSL support for REST API and JSON-RPC is overly complex for resource-constrained platform.
- **Phase 2: Docker build** – Successfully built `out/meshtastic3ds.3dsx` (180KB) using official `devkitpro/devkitarm` Docker image. Key toolchain fixes required:
  - `-march=armv6k` (not `arm1176jzf-s`) to select correct multilib path with `3dsx_crt0.o`
  - `-B$(DEVKITARM)/arm-none-eabi/lib/armv6k/fpu` so specs file resolves `3dsx_crt0.o`
  - `g++` as linker driver + `-lstdc++ -lm` for C++ stdlib
  - Console-only build (no Citro2D/3D graphics) due to VFP ABI incompatibility with Ubuntu-based toolchain
  - CIA packaging skipped (makerom not available); `.3dsx` is primary output
- **Phase 3: Binary extraction** – `out/meshtastic3ds.3dsx` and `out/meshtastic3ds.cia` extracted successfully. `.3dsx` is ready for Citra emulator or 3DS Homebrew Launcher (`/3ds/` on SD card).
- **Phase 4: 3DSX binary format fix & emulator testing setup** – Fixed critical binary format issue:
  - **Problem discovered:** Original binary was flat ARM code (produced by `objcopy -O binary`), lacking the required 3DSX header. Neither Citra nor Homebrew Launcher would accept it.
  - **Solution:** Rebuilt with `3dsxtool` (from devkitPro toolchain) to generate proper 3DSX header with relocation table and segments.
  - **Result:** Binary now has correct magic header (`3344 5358` = "3DSX") and is loadable by emulators and 3DS hardware.
  - **Network emulator config:** Changed `TARGET_IP` from `192.168.4.1` (Heltec hardware) to `127.0.0.1` (localhost) for emulator testing.
  - **Mock bridge server:** Created `test-bridge-mock.sh` — Python-based TCP server that listens on port 4444, parses incoming JSON, and echoes messages back. Allows testing network code without real mesh hardware.
  - **Testing guide:** Created `PHASE4-EMULATOR-TESTING.md` with step-by-step instructions for both emulator (with Lime3DS) and network-only tests.

- **Phase 4.5: Hardware-readiness review & fixes (2026-06-10)** – Full-code review found and fixed bugs that would have made hardware testing fail outright:
  - **`main.cpp` was missing all 3DS service initialization** – added `gfxInitDefault()` + `consoleInit()` (screens/printf) and `socInit()` with the required 0x1000-aligned SOC buffer (without it every socket call fails on hardware), plus matching cleanup and per-frame `gspWaitForVBlank()`.
  - **Bridge IP is now a build-time variable** – `BRIDGE_IP` make/Docker arg, default `192.168.4.1` (hardware); use `--build-arg BRIDGE_IP=127.0.0.1` for emulator builds. No more manual header edits.
  - **`Connect()` bounded by a 5s timeout** (non-blocking connect + `poll()`); **Y button retries the connection** in-app; connection loss is reported.
  - **`PollMessage()` rewritten** – drains the socket fully, delivers buffered lines even when `recv` returns `EAGAIN`, tolerates `"text": "` with whitespace (the mock bridge emits this — previously its echoes never parsed!), and unescapes `\" \\ \n \r \t`.
  - **`SendText()` no longer busy-spins** on `EAGAIN` (waits via `poll()` with a 3s cap).
  - **BridgeModule fixes** – `<WiFi.h>` now included in the header (was a compile error); outbound packets set `which_payload_variant = decoded_tag` and `to = NODENUM_BROADCAST`; payload size clamped to the actual protobuf capacity; switched `PRIVATE_APP` → `TEXT_MESSAGE_APP` so standard Meshtastic nodes can exchange texts with the 3DS; output buffer enlarged so max-length payloads can't truncate into invalid JSON. Header now carries explicit firmware-integration notes.
  - **Fake `.cia` removed** – the old `make cia` just copied the `.3dsx` (confirmed byte-identical); installing it would fail. Build now produces `.3dsx` only; install via Homebrew Launcher.
  - **Makefile** – header dependency tracking (`-MMD -MP`); `BRIDGE_IP`/`BRIDGE_PORT` variables wired into the code.

- **Phase 4.6: Emulator integration testing PASSED (2026-06-10)** – Full end-to-end test in the Azahar emulator (successor to Citra/Lime3DS; installed at `tools/azahar.AppImage`, gitignored):
  - Built emulator variant with `--build-arg BRIDGE_IP=127.0.0.1` → `out/meshtastic3ds-emu.3dsx` (hardware build at `out/meshtastic3ds.3dsx` untouched)
  - Mock bridge upgraded to persistent connections (was closing after one message) + sends a `Mock bridge online` greeting on connect
  - **All paths verified on-screen at 60 FPS:** boot/console init → `socInit` → TCP connect → `[Received] Mock bridge online` (greeting parse, incl. `"text": "` whitespace format) → swkbd input → `[Sent] hello mesh` → bridge log `Bridge received: {"text":"hello mesh"}` → `[Received] Echo: hello mesh` → bridge killed → `[Warn] Connection lost` → bridge restarted → **Y** → `Connected to bridge!`
  - Test was driven programmatically via XTEST key injection (Azahar default mapping: A→`a`, X→`z`, Y→`x`, START→`m`)

- **Phase 5: Heltec ESP32 firmware port — COMPLETE (2026-06-10)** – Porting `BridgeModule` into the real meshtastic/firmware tree and build verification.

- **Phase 5.1: Firmware build success & flash-ready (2026-06-10)** – BridgeModule integrated into Meshtastic v2.7.15, compiled clean, and verified flash-ready.
  - **Hardware confirmed** – Heltec V1 on `/dev/ttyUSB0` (CP2102 USB-UART). `esptool` reads it via DTR/RTS **auto-reset**, so **no manual bootloader/BOOT-button step is needed** to flash. Chip: ESP32, 4 MB flash, 26 MHz crystal, MAC `b4:e6:2d:8c:18:39`.
  - **Toolchain** – PlatformIO 6.1.19 (`~/.local/bin/pio`) installed; ESP32 toolchain (~2.5 GB) pulled. Firmware cloned to `~/meshtastic-firmware` (outside this repo).
  - **Master is broken for Heltec V1** – latest `master` fails to compile in stock `src/Power.cpp:90` on the `ADC2_GPIO13_CHANNEL` macro (the V1 is `actively_supported = false`, so master bit-rots). Diagnosed as a **post-2.7.15 regression** by diffing `Power.cpp`/`variant.h` between master and the tag. **Pivoted to the latest published stable release `v2.7.15.567b8ea`** (what Meshtastic CI actually ships an ESP32 build for); baseline `heltec-v1` build compiles clean past the failure point.
  - **WiFi architecture decided (with user)** – the 3DS has **no Bluetooth** (no model does; libctru exposes none), so WiFi/TCP is the only transport. Current firmware is **STA-only** (no built-in device-as-AP mode — zero `WiFi.softAP()` calls in `src/`). Chosen approach: **Heltec is its own AP** (keeps the 3DS's hard-coded `192.168.4.1` and all docs valid). The module brings up the AP itself; Meshtastic's own WiFi must be left **disabled** in config so the two don't fight over the radio.
  - **BridgeModule fully re-ported to the real firmware API** (`src/modules/BridgeModule.{h,cpp}`), verified against the v2.7.15 source tree:
    - Extends `SinglePortModule(TEXT_MESSAGE_APP)` + `concurrency::OSThread` (same pattern as `ExternalNotificationModule`); all socket/AP/DNS work happens in non-blocking `runOnce()` (20 ms poll) — no blocking loop.
    - Brings up its own soft-AP `meshtastic-bridge` @ 192.168.4.1, re-asserted defensively if the WiFi mode ever flips.
    - JSON-lines TCP server on **4444**; inbound `{"text":"..."}` → broadcast `TEXT_MESSAGE_APP` packet via `allocDataPacket()` + `service->sendToMesh()`; received text → JSON back to the 3DS. Uses `isFromUs()` to avoid echoing the 3DS's own sends. Hand-rolled JSON parse/escape (firmware bundles no ArduinoJson) matching the 3DS client exactly.
    - **Nintendo conntest spoof** so the 3DS will agree to SAVE an internet-less AP: wildcard `DNSServer` (port 53 → 192.168.4.1) + HTTP responder (port 80) returning `200` with the `X-Organization: Nintendo` header.
  - **Build result (2026-06-10):** ✅ Firmware compiled successfully on first try
    - `src/modules/BridgeModule.cpp.o` compiled with zero errors (upfront API verification paid off)
    - Firmware size: 2.2 MB; flash usage 90.7% (module added ~8.4 KB; 225 KB free)
    - EXIT=0; ready to flash
    - **Flash command:** `cd ~/meshtastic-firmware && pio run -e heltec-v1 -t upload --upload-port /dev/ttyUSB0` (~30–60 s)
    - **No bootloader mode needed:** CP2102 auto-resets via DTR/RTS; PlatformIO handles entry automatically
    - **Post-flash:** NVS (config) preserved; LoRa region optional for local verification; leave Meshtastic WiFi disabled (module owns AP)
    - See `PHASE5-FLASH-GUIDE.md` for detailed flashing + configuration + testing steps

- **Phase 5.2: Flashed, crash diagnosed & fixed — board STABLE (2026-06-10)** – Flashed the board and resolved a real runtime crash.
  - **A prior handoff's diagnosis was wrong** (claimed a crash in `lora->begin()` and a `wifi_enabled` config change). Verified on live serial that neither was true: `RF95 init success` every boot, the AP comes up, and the only firmware diff is the 6-line `Modules.cpp` registration.
  - **Real bug:** ~102 s `loopTask` watchdog reset from **WiFi soft-AP + BLE coexistence**. Meshtastic only starts NimBLE when its own `wifi_enabled == false` (`src/platform/esp32/main-esp32.cpp:32`) precisely to avoid coexistence; BridgeModule brings up an AP behind its back, so BLE + WiFi run together on this ~80 KB-heap classic ESP32 and crash. **Proven by isolation:** stock firmware (BridgeModule commented out) is rock-stable past 137 s; the bridge build crashes at ~100 s.
  - **Fix:** compile BLE out of the bridge build (the 3DS uses WiFi only) — added `-D MESHTASTIC_EXCLUDE_BLUETOOTH=1` to `variants/esp32/heltec_v1/platformio.ini`. **Result: stable past 120 s, no watchdog**, `RF95 init success`, `AP 'meshtastic-bridge' up at 192.168.4.1 (TCP 4444)`, region ANZ already set. Flash also dropped 90.7% → 86.4%.
  - **Diagnostic tooling:** drove the board's serial directly via pyserial (`/tmp/cap_serial.py`) with DTR/RTS reset control to capture real boot logs rather than trust the transcript.
  - **Board state now:** flashed with the working build, AP live, radio up, BLE off. **Ready for the physical 3DS end-to-end test.**

- **Phase 5.3: Bridge firmware VALIDATED on hardware (2026-06-10)** – Verified the full firmware path on the real Heltec V1 using this Linux PC as a stand-in for the 3DS (same TCP+JSON protocol — the bridge cannot tell the difference).
  - PC's `wlan0` joined the board's `meshtastic-bridge` AP → DHCP gave `192.168.4.2`, ping to `192.168.4.1` ~4 ms. **Confirms BridgeModule's soft-AP + DHCP work on hardware.**
  - Opened TCP to `192.168.4.1:4444` and sent `{"text":"hello from PC test"}`. Serial showed the complete chain: `3DS client connected` → parsed `Portnum=1` (TEXT_MESSAGE_APP) → `enqueue for send ... encrypted len=40` → **`txGood=1` (actually transmitted over LoRa, region ANZ)** → `forwarded to mesh: hello from PC test`. `isFromUs()` correctly suppressed echoing our own packet.
  - Board stayed stable throughout (uptime ~1557 s, no watchdog) — re-confirms the BLE-exclude fix.
  - **Net result: the entire bridge firmware is proven on hardware.** Combined with the 3DS app already validated in Azahar (Phase 4.6) over the identical protocol, both halves of the system are independently verified.
  - Tooling: `3dslink 0.6.3` extracted from the build container to `~/.local/bin/` for netloading the `.3dsx` to the 3DS over WiFi (no SD card reader needed). Note: ESP32 soft-AP client link can be flaky (WiFi power-save); a clean `nmcli` reconnect restores it.

## ⏳ Pending steps (what still needs to be done)
1. ~~**Install and configure Podman**~~ ✅ Done (Podman 5.8.2)
2. ~~**Build the image**~~ ✅ Done – Now uses `3dsxtool` to generate proper 3DSX header
3. ~~**Run the container and retrieve the binaries**~~ ✅ Done – `out/meshtastic3ds.3dsx` (177KB) with correct 3DSX magic header
4. ~~**Phase 4: Binary format fix & emulator testing setup**~~ ✅ Done:
   - Makefile fixed: replaced `objcopy -O binary` with `3dsxtool` in `$(BIN)` rule
   - Network client configured for emulator: `TARGET_IP = 127.0.0.1:4444`
   - Mock bridge server created: `test-bridge-mock.sh` (Python-based, tested and working)
   - Testing guide written: `PHASE4-EMULATOR-TESTING.md`
4. **Test the end‑to‑end flow**:
   - Flash the Heltec with the `BridgeModule` firmware (use PlatformIO or `pio run -e esp32dev -t upload`).
   - The ESP32 will create a Wi‑Fi AP called `meshtastic-bridge` (IP `192.168.4.1`).
   - Connect the 3DS (or Citra) to that AP.
   - Launch the built app, press **A** to open the on‑screen keyboard, type a message, and press **Enter**.
   - Verify the Serial console of the ESP32 prints `Forwarded to mesh: <your text>` and that incoming mesh messages appear in the 3DS console.
5. **Optional enhancements** (not required for the MVP but nice to have):
   - Persist a message log on the SD card.
   - Add a small UI polish (background, cursor, scroll bar).
   - Add reconnection logic for Wi‑Fi loss.

## 📋 Next actions (what to do now)
- **[✅]** Podman installed and configured (5.8.2)
- **[✅]** Docker image built with correct 3DSX binary format
- **[✅]** Binaries extracted and tested: `out/meshtastic3ds.3dsx` has correct magic header
- **[✅]** Mock bridge server created and tested: JSON echo working
- **[✅]** Emulator testing complete – Azahar (`tools/azahar.AppImage`) + mock bridge: connect, send, receive, and Y-reconnect all verified on-screen (see Phase 4.6)
- **[✅]** Hardware/toolchain ready – Heltec V1 on `/dev/ttyUSB0` (auto-reset, no bootloader step); PlatformIO + ESP32 toolchain installed; firmware pinned to stable `v2.7.15` (master is broken for V1)
- **[✅]** BridgeModule re-ported to the real firmware API + Nintendo conntest spoof (see Phase 5)
- **[✅]** BridgeModule firmware build successful — ready to flash (2026-06-10)
  - ✅ `src/modules/BridgeModule.cpp.o` compiled clean, no errors
  - ✅ Firmware linked successfully: `firmware.bin` = 2.2 MB
  - ✅ Flash usage: 90.7% (module added ~8.4 KB over baseline; ~225 KB free)
  - ✅ All API integrations verified; first-try compile
  - **[⏳]** Flash Heltec ESP32 with Meshtastic + BridgeModule
    - Flash cmd: `cd ~/meshtastic-firmware && pio run -e heltec-v1 -t upload --upload-port /dev/ttyUSB0` (~30–60 s)
    - No bootloader mode needed: CP2102 auto-resets via DTR/RTS; PlatformIO handles it
    - Flash preserves existing device config (NVS); region + WiFi settings survive
    - **Post-flash config:** LoRa region must be set for mesh; leave Meshtastic WiFi **disabled** (module owns AP)
    - **Verification:** serial console should show `AP 'meshtastic-bridge' up` + successful 3DS connects
- **[ ]** (Optional) Phase 6: Restore graphics UI and add message persistence

---
*Generated by Claude Code – progress documented for easy hand‑off.*
