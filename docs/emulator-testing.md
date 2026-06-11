# Phase 4 — Emulator Testing Guide

> ✅ **COMPLETED 2026-06-10:** Emulator testing passed end-to-end using
> **Azahar** (the active successor to Citra/Lime3DS, which are defunct),
> installed at `tools/azahar.AppImage`. Launch with:
> `./tools/azahar.AppImage out/meshtastic3ds-emu.3dsx`
> Default key mapping: A→`a`, B→`s`, X→`z`, Y→`x`, START→`m`.
> See `PROGRESS.md` → Phase 4.6 for results.

## Summary

The 3DS Meshtastic app binary has been rebuilt with the correct **3DSX magic header** (`3344 5358`). 
The 3DS client now connects to `127.0.0.1:4444` (emulator localhost) instead of `192.168.4.1` (hardware).

## Pre-requisites

1. **Corrected 3DSX binary** — `out/meshtastic3ds.3dsx` with proper 3DSX header ✅
2. **Mock bridge server** — `test-bridge-mock.sh` ready in project root ✅
3. **Lime3DS emulator** — Install from https://github.com/lime3ds/lime3ds (or use Citra if still available)
   - Emulator requires a dumped 3DS system ROM (`boot9.bin`, `boot11.bin`, `secret_sector.bin`)
   - Alternative: Use a mock bridge server to test network code without full emulation

## Testing Approach

### Option A: Full Emulator Test (Lime3DS + Mock Bridge)

**Requirements:**
- Lime3DS installed
- 3DS system ROM dumped
- Network loopback working

**Steps:**

1. **Terminal A:** Start the mock bridge server
   ```bash
   cd /home/indreaven/meshtasticds
   ./test-bridge-mock.sh
   ```
   Expected output:
   ```
   Mock bridge listening on port 4444 ...
   ```

2. **Launch Lime3DS**
   - **File → Load File** → Select `/home/indreaven/meshtasticds/out/meshtastic3ds.3dsx`
   - 3DS console should print:
     ```
     === Meshtastic 3DS Console ===
     Initializing...
     Connected to bridge!
     ```

3. **Send a test message**
   - Press **A** to open keyboard
   - Type: `Hello mesh`
   - Press **Enter** to confirm
   - Console should show: `[You] Hello mesh`

4. **Send to bridge (press X)**
   - Press **X** to send
   - Console should show: `[Sent] Hello mesh`
   - Terminal A should show:
     ```
     Bridge received: {"text":"Hello mesh"}
     ```

5. **Receive echo response**
   - Console should show: `[Received] Echo: Hello mesh`

### Option B: Network-Only Test (Mock Bridge + Manual JSON)

If you don't have Lime3DS or system ROMs, you can still test the bridge:

```bash
# Terminal A: Start mock bridge
./test-bridge-mock.sh

# Terminal B: Send JSON manually
echo '{"text":"Test message"}' | nc 127.0.0.1 4444

# Terminal A should show:
# Bridge received: {"text":"Test message"}
```

## Verification Checklist

- [ ] Binary has correct 3DSX magic (`xxd out/meshtastic3ds.3dsx | head -1` shows `3344 5358`)
- [ ] Mock bridge script is executable (`ls -l test-bridge-mock.sh` shows `x` permissions)
- [ ] Emulator build was made with `--build-arg BRIDGE_IP=127.0.0.1` (hardware default is `192.168.4.1`)
- [ ] Mock bridge echoes messages correctly (manual `nc` test in Option B)

## Hardware Testing (After Emulator Verification)

**When ready to test on real hardware:**

1. **Rebuild for hardware** — the default build already targets `192.168.4.1` (no source edits needed; emulator builds use `--build-arg BRIDGE_IP=127.0.0.1`):
   ```bash
   podman build -t meshtastic-3ds .
   mkdir -p out
   podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds
   ```

3. **Install on 3DS:**
   - Copy `out/meshtastic3ds.3dsx` to 3DS SD card at `/3ds/meshtastic3ds.3dsx`
   - Launch via Homebrew Launcher

4. **Flash Heltec ESP32:**
   - Clone Meshtastic firmware: https://github.com/meshtastic/firmware
   - Build with BridgeModule included
   - Flash to Heltec via USB-UART

5. **Connect and test:**
   - 3DS connects to `meshtastic-bridge` Wi-Fi AP
   - App connects to bridge at `192.168.4.1:4444`
   - Send messages; ESP32 serial output should show `Forwarded to mesh: ...`

## Known Issues & Limitations

- **No GUI yet** — Console-only interface (terminal aesthetic, no graphics)
- **Single TCP client** — Bridge handles only one 3DS at a time
- **Manual IP switching** — Need to rebuild for emulator vs. hardware (see Step 2 in hardware section)
- **Mock bridge limitations** — Only echoes messages; real mesh routing requires Heltec + Meshtastic firmware

## Next Steps

1. Test network code with mock bridge ✅ (ready now)
2. Install and configure Lime3DS (requires system ROM)
3. Flash Heltec ESP32 and test full mesh integration (hardware dependent)
4. Phase 5: Optional UI enhancements (graphics, message persistence, scrolling)
