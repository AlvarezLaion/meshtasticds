# Handoff — Flash & Verify the Heltec V1

## ⏱️ CONNECTION RE-VERIFIED (2026-06-11, later) — bridge network path is healthy; brownout persists
Re-tested the full network path the 3DS uses, all green:
- `wlan0` on `meshtastic-bridge` AP → `192.168.4.2/24`; default route still on `eth0` (PC internet intact).
- `ping 192.168.4.1` → 3/3 replies (~36 ms avg). **TCP 4444 OPEN.**
- A PC-side TCP client (mimicking the 3DS) connected; board serial confirmed `[BridgeModule] 3DS client connected`.
- So TCP/bridge layer is fine. Serial log still shows `Brownout detector was triggered` / `rst:0xc (SW_CPU_RESET)` — the known LoRa-TX power sag, NOT a code/connection issue.
- **Plan:** user will try a stronger charger first. Lowering `lora.tx_power` (14–17 dBm) is a fallback — but note this is flaky hardware, so don't over-trust either fix; re-test sends after powering from the charger. Adding a 2nd mesh node still pending.

## ⏱️ CURRENT STATUS (2026-06-11 PM) — END-TO-END WORKS; remaining issue is a LoRa-TX power BROWNOUT on the bridge

**THE BIG WINS THIS SESSION:**
1. **App is on the 3DS and connects to the bridge.** Netload succeeded; the 3DS app shows **"Connected to bridge!"** and the board logs `BridgeModule: 3DS client connected`. Sending a message logs `forwarded to mesh: <text>` (LoRa TX confirmed).
2. **Solved the netload blocker** (was: 3DS never associated with the AP). TWO root causes were fixed:
   - **3DS was joining the home WiFi, not the bridge.** The Homebrew Launcher auto-connects to whatever saved network it finds (home SSID was winning). Fix: on the 3DS, **delete/forget the home WiFi slot temporarily** so `meshtastic-bridge` is the only option, THEN open hbmenu + press **Y** (NetLoader). Confirm the netloader IP is **192.168.4.x** (NOT 192.168.1.x) — that's how you know it joined the bridge. Re-add home WiFi afterward.
   - **hbmenu/Rosalina 2.4.3 NetLoader is on-demand:** WiFi only comes up when you press **Y** on the NetLoader screen. It does NOT auto-connect just by opening hbmenu (that's why earlier 60s/180s watchers timed out).
3. **PC keeps internet while on the bridge AP — use the ETHERNET CABLE.** This box has `eth0` (192.168.1.28, wired, online) AND `wlan0`. Put `wlan0` on the bridge AP and internet still flows over `eth0`, so the assistant stays online and can netload + watch serial live. Config that made it clean:
   `nmcli con mod meshtastic-bridge ipv4.never-default yes ipv4.ignore-auto-dns yes ipv4.route-metric 200` then `nmcli con up meshtastic-bridge`. Verify: `ip route | grep default` should still show `dev eth0`.

**Exact working netload sequence (repeat this to get the app on the 3DS again):**
1. PC: `nmcli con up meshtastic-bridge` (wlan0 → 192.168.4.2; internet stays on eth0). `ping 192.168.4.1` to confirm AP.
2. 3DS: ensure home WiFi slot is deleted/forgotten so it joins `meshtastic-bridge`. Open Homebrew Launcher → press **Y** → note the **192.168.4.x** IP it shows.
3. PC: `~/.local/bin/3dslink -a <that-192.168.4.x-ip> ~/meshtasticds/out/meshtastic3ds.3dsx`. App launches already on the bridge AP → connects.

**⚠️ THE REMAINING ISSUE — bridge BROWNOUT-RESETS on LoRa transmit (this is HARDWARE/POWER, not code):**
- Symptom the user saw: send a message → "connection lost, press Y" → reconnects → drops again, intermittently ("sometimes works").
- **Root cause, proven from serial:** every drop is preceded by `[RadioIf] 0 packets remain in the TX queue` then **`Brownout detector was triggered`** then `rst:0xc (SW_CPU_RESET)`. The LoRa PA keys up at **20 dBm (100 mW, max)**; that current spike (on top of WiFi-AP draw) sags the 3.3V rail past the ESP32 brownout threshold → hard reset. The message DOES get transmitted (forwarded-to-mesh + TX queue drains) before the reset, so a 2nd node should still receive it.
- This is NOT TCP flakiness and NOT the BridgeModule code. It is power delivery.
- **Fixes, in priority order:**
  1. **Power from a strong source** — wall USB charger or powered hub (NOT a weak PC/front-panel/hub port), a short thick cable, or a **LiPo on the Heltec battery connector**. (User swapped to a newer micro-USB cable mid-session — re-test to see if that alone fixed it; brownouts were still logging on the old cable right up to the swap.)
  2. **Lower LoRa TX power** to ~14–17 dBm via Meshtastic config (`lora.tx_power`, app/CLI, no reflash). Halves the spike; costs range.
  3. **Confirm the LoRa antenna is attached** — TXing 100 mW into a missing/bad antenna worsens PA draw (and risks the radio).
  - Do NOT just disable the brownout detector in firmware — it's protecting against rail sag that can corrupt flash.
- **NEXT STEP when resuming:** re-test a single message after the cable swap / better power. Watch `grep -a -iE 'brownout|forwarded|rst:0x' /tmp/serial_live.log | tail`. If brownouts persist, drop tx_power or use battery/wall power. Then the user is adding a **2nd mesh node** to confirm the message is actually received over LoRa (needs LoRa region set — region is ANZ/US already per earlier logs; confirm both nodes share region + channel).

**Environment / tooling state (still in place):**
- App binary: `~/meshtasticds/out/meshtastic3ds.3dsx` (hard-coded bridge IP 192.168.4.1). `3dslink` at `~/.local/bin/3dslink`.
- `/tmp/serial_logger.py` — decoded serial → `/tmp/serial_live.log` (ANSI-stripped; grep it with `-a` since it has binary boot bytes). Running in background; only ONE process can hold `/dev/ttyUSB0`, so stop it before any `pio device monitor`.
- `/tmp/netload_orchestrator.sh` — hands-free: waits for AP, joins it, starts serial logger, loops 3dslink until the 3DS NetLoader answers. Logs to `/tmp/netload_orchestrator.log`. (Was used earlier; the manual `3dslink -a <ip>` is faster once you know the IP.)
- Board on `/dev/ttyUSB0`; firmware = Meshtastic v2.7.15 + BridgeModule, BLE excluded (see resolved watchdog note below).

---

## Original flash/verify guide (below) — flashing is already done

## State
- Firmware built: `~/meshtastic-firmware/.pio/build/heltec-v1/firmware.bin` (Meshtastic v2.7.15 + BridgeModule, **BLE excluded**, Flash 86.4%).
- Board: Heltec V1 on `/dev/ttyUSB0` (CP2102, auto-reset — **no bootloader button needed**).
- Tool: `~/.local/bin/pio` (PlatformIO 6.1.19).

## Step 1 — Flash (~60s)
```bash
cd ~/meshtastic-firmware && ~/.local/bin/pio run -e heltec-v1 -t upload --upload-port /dev/ttyUSB0
```
Expect `Hash of data verified.` / `[SUCCESS]`. If it can't connect, hold the board's **PRG/BOOT** button during "Connecting...".

## Step 2 — Watch serial
```bash
~/.local/bin/pio device monitor -p /dev/ttyUSB0 -b 115200
```
Look for (from BridgeModule):
- `BridgeModule: AP 'meshtastic-bridge' up at 192.168.4.1 (TCP 4444)`
- on 3DS connect: `3DS client connected`
- on send: `BridgeModule: forwarded to mesh: <text>`

## Step 3 — Config (one-time, via Meshtastic app/CLI)
- **Leave WiFi disabled** in Meshtastic config (module owns the AP; default is off — only act if it's already on).
- **Set LoRa region** (e.g. `US`/`EU_868`) or the radio won't TX to other nodes. (AP + 3DS round-trip works without it; mesh forwarding needs it + a 2nd node.)

## Step 4 — End-to-end test
1. 3DS joins WiFi `meshtastic-bridge` (pass `changeme123`). The Nintendo conntest spoof (DNS+HTTP on the board) lets it save the connection.
2. Launch `out/meshtastic3ds.3dsx` (already targets 192.168.4.1). Expect `Connected to bridge!`.
3. A → type → X to send. Serial shows `forwarded to mesh`. A 2nd Meshtastic node should receive it; its replies appear on the 3DS.

## If something's wrong
- Module source (canonical): `~/meshtasticds/src/modules/BridgeModule.{h,cpp}` (also copied into `~/meshtastic-firmware/src/modules/`, registered in `~/meshtastic-firmware/src/modules/Modules.cpp`).
- Rebuild: `cd ~/meshtastic-firmware && ~/.local/bin/pio run -e heltec-v1`.
- Master is broken for V1 (ADC error in Power.cpp) — stay on tag `v2.7.15.567b8ea`.

---

## ✅ RESOLVED (2026-06-10) — the diagnosis below was WRONG; real fix found

**What was actually happening (verified on live serial):**
- The radio inits fine every boot (`RF95 init success`) and the AP *does* come up
  (`BridgeModule: AP 'meshtastic-bridge' up at 192.168.4.1`). The crash was NOT at
  `lora->begin()`, and there was NO `config.proto`/`wifi_enabled` change (the only
  firmware diff is the 6-line `Modules.cpp` registration).
- The real bug: a **~102s loopTask watchdog reset caused by WiFi+BLE coexistence.**
  Meshtastic only starts BLE when its own `wifi_enabled == false`
  (`src/platform/esp32/main-esp32.cpp:32`) — to avoid coexistence. Our module brings
  up a soft-AP behind its back, so BLE + WiFi run together on this ~80KB-heap classic
  ESP32 and crash at ~100s. Proven by isolation: stock firmware (BridgeModule removed)
  is rock-stable; the bridge build crashes.

**Fix (applied):** compile BLE out of the bridge build — the 3DS uses WiFi only.
Added `-D MESHTASTIC_EXCLUDE_BLUETOOTH=1` to `variants/esp32/heltec_v1/platformio.ini`.
Result: **stable past 120s, no watchdog, AP up, radio up** (region ANZ already set).
Flash usage also dropped 90.7% → 86.4%.

**Current board state:** flashed with the working build; AP `meshtastic-bridge`
@ 192.168.4.1:4444 is live. **Ready for the Step 4 end-to-end 3DS test** (physical 3DS
needed — connect to the AP, launch `out/meshtastic3ds.3dsx`).

---

<details><summary>Original (incorrect) DEBUG STATE — kept for history</summary>

**Claimed Issue:** Board boots but crashes during LoRa radio init; BridgeModule WiFi AP never comes online.

### Symptoms
1. **Task watchdog timeout** on `loopTask` during radio initialization
   - Error: `E (102778) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time: - loopTask (CPU 1)`
   - Triggers at ~102s into boot (inside `RF95Interface::lora->begin()`)
   - Board reboots and loops the same crash

2. **OLED frozen** on first Meshtastic welcome frame
   - Display initializes but UI loop never progresses
   - Suggests a blocking call in the boot sequence

3. **BridgeModule startup message never appears**
   - Expected: `BridgeModule: AP 'meshtastic-bridge' up at 192.168.4.1 (TCP 4444)`
   - Never logged (module code never reaches `runOnce()`)

### Root Cause (Hypothesis)
**Upstream model (claude-opus) diagnosis:** The crash is in LoRa radio SPI init, not BridgeModule code itself. BridgeModule constructor only registers an OSThread; `runOnce()` doesn't execute until after radio init, which never completes. Two likely culprits:

1. **SPI bus contention** — WiFi peripheral and LoRa SX1276 both trying to use SPI during init, or radio reset/IRQ pins held/conflicted
2. **WiFi disable config breaking boot** — User disabled `config.network.wifi_enabled = true` by default to let BridgeModule own the WiFi AP (avoid router dependency). This may have changed the GPIO/clock init order in a way that leaves the SX1276 unreachable on SPI

### Evidence
- Flash successful: All 4 firmware segments verified (hash checked), hard reset via RTS pin worked
- Boot proceeds through: filesystem, I2C, protobuf load, module registration, SSD1306 display detected
- Stops at: `Start meshradio init` → `RF95Interface::lora->begin()` (SPI timeout on SX1276)
- Never reaches: `RF95 init result`, `initWifi()`, BridgeModule's `ensureAccessPoint()`

### Files Changed (Likely Breaking Change)
- `~/meshtastic-firmware/src/config/config.proto` or similar: `network.wifi_enabled` set to false by default
- This was done to prevent Meshtastic's stock WiFi init from competing with BridgeModule's AP

### Debugging Steps for Next Model
**Fix 1 (isolate cause — do first):**
- Comment out `new BridgeModule();` at `~/meshtastic-firmware/src/modules/Modules.cpp:290`
- Rebuild: `cd ~/meshtastic-firmware && ~/.local/bin/pio run -e heltec-v1`
- Flash and check serial output
- **If board boots past "Start meshradio init"** → WiFi/bridge config is the issue (proceed to Fix 2)
- **If board still hangs at "Start meshradio init"** → LoRa hardware/wiring is unreachable; not a bridge problem

**Fix 2 (if Fix 1 blames the bridge):**
- Ensure `config.network.wifi_enabled = false` is set in saved prefs or defaults
- Defer WiFi AP startup in `BridgeModule::runOnce()` until after boot settles (e.g., `if (millis() < 8000) return 500;`)
- This prevents soft-AP bringup from overlapping with LoRa radio SPI init

### Serial Log (Last 50 Lines Before Crash)
```
[32mINFO  [0m| ??:??:?? 1 [32mTurn on screen
[0m[34mDEBUG [0m| ??:??:?? 1 [34mwaypoint wants a UI Frame
[0m[34mDEBUG [0m| ??:??:?? 1 [34mtraceroute wants a UI Frame
[0m[34mDEBUG [0m| ??:??:?? 1 [34mcanned wants a UI Frame
[0m[34mDEBUG [0m| ??:??:?? 1 [34mRF95Interface(cs=18, irq=26, rst=14, busy=-1)
[0m[32mINFO  [0m| ??:??:?? 1 [32mStart meshradio init
[0m[→ HANG HERE for ~101 seconds, then watchdog fires]
E (102778) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (102778) task_wdt:  - loopTask (CPU 1)
E (102778) task_wdt: Tasks currently running:
E (102778) task_wdt: CPU 0: IDLE0
E (102778) task_wdt: CPU 1: loopTask
E (102778) task_wdt: Aborting.
abort() was called at PC 0x4016a3fc on core 0
```

### Key Files for Investigation
- `~/meshtasticds/src/modules/BridgeModule.cpp:213-220` — `runOnce()` (never executes pre-crash; WiFi/AP code is here)
- `~/meshtasticds/src/modules/BridgeModule.cpp:102-106` — constructor (safe; only registers thread)
- `~/meshtastic-firmware/src/modules/Modules.cpp:290` — BridgeModule instantiation
- `~/meshtastic-firmware/src/main.cpp:954` (setupModules), `:1211/1230` (rIf->init), `:1435` (initWifi) — boot sequence
- `~/meshtastic-firmware/src/mesh/RF95Interface.cpp:178` — `lora->begin()` (actual hang point)

### Questions for Next Model
1. Where was `wifi_enabled` disabled? (which file, which line)
2. Was it a default-config change or compiled-in?
3. Any changes to GPIO/SPI pin mappings for the SX1276 or WiFi?

</details>
