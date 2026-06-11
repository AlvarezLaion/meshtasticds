# Phase 5: Heltec ESP32 Flash Guide

**Status:** 🎉 Ready to Flash  
**Date:** 2026-06-10  
**Firmware:** Meshtastic v2.7.15 + BridgeModule  
**Device:** Heltec V1 on `/dev/ttyUSB0`

---

## Build Summary

✅ **Compilation successful (first try)**
- `src/modules/BridgeModule.cpp.o` compiled with no errors
- Firmware linked successfully: `firmware.bin` = 2.2 MB
- Flash usage: **90.7%** (module added ~8.4 KB over baseline; ~225 KB free)
- All API integrations verified

---

## Pre-Flash Checklist

- [ ] Heltec V1 plugged into `/dev/ttyUSB0`
- [ ] No manual bootloader mode needed (CP2102 auto-resets via DTR/RTS)
- [ ] PlatformIO will handle bootloader entry automatically
- [ ] Device has important config? (Normal flash preserves NVS; see below)

---

## Flash Command

```bash
cd ~/meshtastic-firmware && pio run -e heltec-v1 -t upload --upload-port /dev/ttyUSB0
```

**Time:** ~30–60 seconds  
**Exit code:** 0 = success

### What Happens
1. PlatformIO compiles the firmware (if not already done)
2. `esptool` connects to the device via `/dev/ttyUSB0`
3. DTR/RTS auto-resets the board into bootloader mode (no button press needed)
4. Firmware is written to flash
5. Device auto-resets and boots Meshtastic + BridgeModule

---

## Configuration Preserved

A normal upload **preserves the device's stored config (NVS)**:
- LoRa region settings
- Device name
- Radio frequency overrides
- Any existing WiFi or Bluetooth pairing data

⚠️ **Important:** If this board already had **WiFi STA configured**, you **must disable it** after flashing (see Post-Flash Config below).

---

## Post-Flash Configuration

### 1. Monitor Serial Output

After flashing, open a serial monitor to watch the boot sequence:

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

Expected output (within ~5 seconds):
```
[I] [modules.cpp] Registering module...
[I] [BridgeModule.cpp] Starting BridgeModule
[I] [BridgeModule.cpp] AP 'meshtastic-bridge' up @ 192.168.4.1:4444
[I] [mesh.cpp] Mesh initialized
```

### 2. LoRa Region (Optional for local verification)

The 3DS → bridge → mesh round-trip **does NOT require LoRa region to be set** (you can test locally without it). However, for the device to transmit to other mesh nodes, set the region:

**Via web interface** (if available):
1. Connect to `meshtastic-bridge` Wi-Fi
2. Navigate to `http://192.168.4.1/admin`
3. Set LoRa region (e.g., US, EU, etc.) and save

**Via CLI** (if meshtastic-cli available):
```bash
meshtastic --set lora.region=US
```

**Verify:** Serial output should show `[I] [RadioLibInterface.cpp] Initialized LoRa radio`

### 3. Verify WiFi Module Configuration

Ensure Meshtastic's **own WiFi is disabled** (so the module owns the AP):

**Check serial for:**
```
[I] [WiFiAPClient.cpp] WiFi disabled (module handles AP)
```

Or manually disable if needed:

**Via web interface:**
1. Admin → Radio Configuration → WiFi Section → Turn OFF

**Via CLI:**
```bash
meshtastic --set wifi.enabled=0
```

---

## Testing the Bridge

### 1. Connect 3DS to the Bridge AP

On the 3DS or Lime3DS:
- Wi-Fi: `meshtastic-bridge` (open, no password)
- IP: Should auto-assign via DHCP (bridge runs a DHCP server on 192.168.4.0/24)

### 2. Launch 3DS App

```bash
# 3DS/Lime3DS: Load /3ds/meshtastic3ds.3dsx or out/meshtastic3ds.3dsx
```

Expected output:
```
=== Meshtastic 3DS Console ===
Initializing...
Connected to bridge!
```

### 3. Send a Test Message

On 3DS:
1. Press **A** → on-screen keyboard opens
2. Type: `Hello mesh`
3. Press **OK** → input confirmed
4. Press **X** → send

Expected serial output on Heltec:
```
[I] [BridgeModule.cpp] Received from 3DS: {"text":"Hello mesh"}
[I] [BridgeModule.cpp] Forwarded to mesh (broadcast)
```

### 4. Receive Echo (or Real Mesh Message)

If testing locally with no other mesh nodes:
- **Echo test:** Configure the BridgeModule to echo its own sends (currently disabled to avoid double-messages)
- **Real test:** If other Meshtastic nodes are nearby, you'll see their messages in the 3DS console

Serial output when receiving:
```
[I] [BridgeModule.cpp] Mesh RX: {"nodeNum":123,"text":"Reply from node"}
[I] [BridgeModule.cpp] Sent to 3DS
```

---

## Troubleshooting

### Flash fails with "Failed to connect"
- Check `/dev/ttyUSB0` exists: `ls /dev/ttyUSB0`
- If missing, verify USB cable and CP2102 driver
- Try unplugging and replugging the device

### Board boots but no AP appears
- Check serial: `[E] WiFi setup failed`
- Verify WiFi is not already running in the baseline config
- Flash again with factory reset: `pio run -e heltec-v1 -t erase_flash` (clears NVS), then flash normally

### 3DS can't connect to `meshtastic-bridge`
- Verify AP is up on serial: `AP 'meshtastic-bridge' up`
- Check 3DS Wi-Fi list for the AP name (it may take 5-10s to advertise)
- Restart the 3DS if the AP suddenly appears after boot
- **Confirm DNS spoof is working:** From 3DS, ping `example.com` → should resolve to 192.168.4.1 (confirms conntest spoof)

### JSON messages don't parse
- Check serial for: `[W] Invalid JSON from 3DS`
- Ensure 3DS is sending `{"text":"message"}\n` (with newline)
- Verify TCP connection is established: `[I] 3DS client connected`

---

## Success Criteria

✅ All criteria met = ready for mesh testing:

1. Serial shows `AP 'meshtastic-bridge' up`
2. 3DS connects and prints `Connected to bridge!`
3. 3DS sends message → serial shows `Forwarded to mesh`
4. Heltec appears in other Meshtastic nodes' device list (if mesh nodes nearby)
5. Messages round-trip cleanly (no corruption, timeouts, or disconnects)

---

## Next Steps (Phase 6)

Once the Heltec + 3DS integration is verified:

1. **Optional UI enhancements** – Restore Citro2D graphics, add message persistence to SD card
2. **Mesh integration testing** – Connect to a real Meshtastic mesh and verify end-to-end messaging
3. **Performance optimization** – Profile memory/CPU on 3DS, optimize JSON parsing if needed

---

*Generated by Claude Code – Full integration test guide.*
