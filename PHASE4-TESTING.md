# Phase 4: Integration Testing Guide

This document details how to test the MeshtasticDS application after building binaries in Phase 3.

---

## Overview

You have two testing paths:
1. **Emulator (Citra)** – Fast, convenient, but no actual mesh
2. **Hardware (3DS + Heltec ESP32)** – Real end-to-end flow, requires devices

Start with **Emulator** if you just want to verify the app builds and runs. Use **Hardware** for full testing.

---

## Test Path A: Emulator (Citra)

### Prerequisites
- Citra emulator installed (3DS emulator, runs on PC)
- `out/meshtastic3ds.3dsx` binary from Phase 3

### Setup (One-Time)

1. **Install Citra:**
   ```bash
   # On Ubuntu/Debian:
   sudo apt-get install citra-qt
   # Or download from: https://github.com/citra-emu/citra/releases
   ```

2. **Create a fake Wi-Fi AP (for testing):**
   ```bash
   # Option A: Use hostapd to create a real AP (advanced)
   sudo apt-get install hostapd dnsmasq
   
   # Option B: Use netcat to mock the bridge (simple):
   # See "Mock Bridge Server" section below
   ```

### Running the Test

1. **Start a mock bridge server** (in a separate terminal):
   ```bash
   # This creates a fake Meshtastic bridge on port 4444
   ./test-bridge-mock.sh
   ```

2. **Open Citra and load the 3DSX:**
   - File → Open File
   - Select `out/meshtastic3ds.3dsx`
   - Click "Run"

3. **In Citra, connect to Wi-Fi:**
   - Emulation menu → Configure → System
   - Enable Wi-Fi
   - Connect to `meshtastic-bridge` at `192.168.4.1`

4. **Test the app:**
   - App should launch and show "--- Mesh Chat ---" title
   - Press **A** to open the keyboard
   - Type: `Hello from Citra`
   - Press **X** to send
   - Expected: Message appears in the log, no crash

5. **Verify the mock bridge received it:**
   - Check the mock bridge terminal for: `Forwarded to mesh: Hello from Citra`

### Success Criteria
- ✓ App launches without segfault
- ✓ Keyboard input is responsive (press A, type, press X)
- ✓ Message sends and appears in the log
- ✓ No network connection errors
- ✓ App stays responsive after sending multiple messages

---

## Test Path B: Hardware (3DS + Heltec ESP32)

### Prerequisites

1. **Heltec V1 with Meshtastic + BridgeModule**
   - Flash the custom firmware with BridgeModule enabled
   - Verify ESP32 boots and creates Wi-Fi AP: `meshtastic-bridge`

2. **Nintendo 3DS or 3DS XL**
   - Modded 3DS with Homebrew Launcher or GodMode9
   - `out/meshtastic3ds.cia` binary from Phase 3

3. **Second Meshtastic device** (optional, for testing receive)
   - Another Heltec, or any Meshtastic-compatible radio
   - Used to send messages to the 3DS

### Setup (One-Time)

1. **Flash Heltec with Meshtastic + BridgeModule:**
   ```bash
   # Use PlatformIO to build Meshtastic with BridgeModule
   cd ~/meshtastic-firmware  # or wherever you have it
   pio run -e esp32dev -t upload
   ```

2. **Verify the bridge is running:**
   ```bash
   # Open the ESP32 serial monitor
   pio device monitor
   # Should show:
   # "Bridge AP started"
   # "TCP server listening on port 4444"
   ```

3. **Install the CIA on the 3DS:**
   - Copy `meshtastic3ds.cia` to the 3DS SD card
   - Use GodMode9 or Homebrew Launcher to install
   - Verify it appears in the home menu

### Running the Test

1. **Connect 3DS to the Wi-Fi AP:**
   - 3DS Settings → Internet Settings → Wi-Fi Connection
   - Select `meshtastic-bridge` and connect
   - Note the 3DS's assigned IP (should be `192.168.4.2` or similar)

2. **Launch the app:**
   - 3DS home menu → Meshtastic
   - App should launch and show "--- Mesh Chat ---"

3. **Send a message from 3DS:**
   - Press **A** to open keyboard
   - Type: `Test message from 3DS`
   - Press **X** to send
   - Expected: Message appears in app log, no crash

4. **Verify ESP32 received it:**
   - Check ESP32 serial monitor for: `Forwarded to mesh: Test message from 3DS`

5. **Test receiving (optional):**
   - Send a message from another Meshtastic device
   - Expected: Message appears in the 3DS app log
   - Example: `Alice: Hello 3DS`

6. **Stress test (optional):**
   - Send 10+ messages rapidly
   - Verify no memory leaks or crashes
   - Check that older messages scroll out of the 15-message buffer

### Success Criteria
- ✓ 3DS connects to the Wi-Fi AP
- ✓ App launches without crashing
- ✓ Keyboard input works
- ✓ Sent message appears in the log
- ✓ ESP32 console shows `Forwarded to mesh: <message>`
- ✓ Received messages appear in the 3DS log (if testing with another device)
- ✓ No connection drops after multiple sends/receives

### Troubleshooting

**Issue: 3DS cannot connect to `meshtastic-bridge`**
- Verify Heltec AP is broadcasting (should be visible in Wi-Fi scan)
- Check Heltec serial output: `Bridge AP started`
- Try restarting Heltec (power cycle)

**Issue: App launches but shows "Not connected to bridge"**
- Verify 3DS has an IP assigned (check Settings → Internet Status)
- Check 3DS can ping Heltec: ping 192.168.4.1 (if available)
- Verify Heltec has TCP server running: `TCP server listening on port 4444`

**Issue: Message sends but doesn't appear in log**
- Check UIHandler is polling messages (verify main loop calls `uiHandler.Update()`)
- Check ESP32 is actually receiving and forwarding (check serial console)

**Issue: Received message doesn't appear**
- Verify the other Meshtastic device is on the same mesh
- Check the message was actually sent (check sender's device)
- Verify BridgeModule is calling `handleReceivedProtobuf()` (add debug serial prints)

---

## Mock Bridge Server (for Emulator Testing)

If you don't want to set up a real Heltec, use this mock server to test the 3DS app in Citra:

**Create `test-bridge-mock.sh`:**
```bash
#!/bin/bash
# Mock Meshtastic bridge server for testing
# Listens on 192.168.4.1:4444 and echoes messages back

PORT=4444
echo "Starting mock bridge on port $PORT..."

# Use socat to create a simple echo server
# Or use netcat with a loop:
while true; do
    echo "Waiting for connection on port $PORT..."
    (
        while read -t 5 line; do
            echo "[Bridge] Received: $line"
            echo "$line" | sed 's/{"text":"\(.*\)"}/{"text":"Received: \1"}/'
        done
    ) | nc -l -p $PORT
done
```

Run it:
```bash
chmod +x test-bridge-mock.sh
./test-bridge-mock.sh
```

The mock server will echo back every message it receives with a "Received: " prefix.

---

## Post-Test Checklist

After testing, verify:
- [ ] App launches without segfault
- [ ] UI renders correctly (black background, green text, yellow titles)
- [ ] Keyboard input works (A to open, X to send, B to backspace)
- [ ] Messages are echoed back to the log
- [ ] No memory leaks (run for 5+ minutes)
- [ ] Connection survives Wi-Fi brief disconnections
- [ ] Message buffer correctly limits to 15 messages (older ones scroll out)

If all checks pass, **Phase 4 is complete** and the MVP is ready.

---

## Next Steps

After successful Phase 4 testing:
- Phase 5 (optional): Implement persistence, scrolling, and reconnection logic
- Or: Deploy to more Meshtastic devices for real-world testing

