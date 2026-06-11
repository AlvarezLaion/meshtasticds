# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MeshtasticDS** is a Nintendo 3DS homebrew application that enables sending and receiving messages over Meshtastic mesh networks via a Wi-Fi bridge (Heltec ESP32). The project consists of two main components:

1. **3DS Client** (`src/3ds/`) – C++ application for the 3DS, built with devkitPro
2. **Bridge Module** (`src/modules/BridgeModule.{h,cpp}`) – ESP32 module that bridges TCP connections to the Meshtastic mesh network

## Build & Development Commands

### Building the 3DS Application with Docker/Podman

The project is built inside a containerized environment to ensure consistency. The Dockerfile sets up devkitPro and all required 3DS libraries.

**Build the Docker image:**
```bash
podman build -t meshtastic-3ds .
```

**Run the container and extract binaries:**
```bash
mkdir -p out
podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds
```

This produces:
- `out/meshtastic3ds.3dsx` – runnable in Citra/Lime3DS and on hardware via the Homebrew Launcher (copy to SD card at `/3ds/meshtastic3ds.3dsx`)

No `.cia` is produced: real CIA packaging requires `makerom`, which is not in this toolchain.

The bridge IP is baked in at build time and defaults to the hardware AP (`192.168.4.1`). For emulator testing against the mock bridge, build with:
```bash
podman build --build-arg BRIDGE_IP=127.0.0.1 -t meshtastic-3ds .
```

### Direct Build (inside container or with devkitPro installed locally)

If devkitPro is installed locally:
```bash
cd src/3ds
make clean
make
```

Build targets:
- `make` – builds the `.3dsx` file (default); accepts `BRIDGE_IP=` / `BRIDGE_PORT=` overrides (run `make clean` first when changing them)
- `make clean` – removes build artifacts

## Architecture & Design

### Communication Protocol

The 3DS client and Bridge Module communicate via **JSON lines over TCP**:
- Port: `4444`
- Format: `{"text": "message_content"}\n`
- The bridge module listens on `192.168.4.1` (ESP32 Wi-Fi AP mode)

### Code Structure

**3DS Client Components:**
- `main.cpp` – Hardware initialization (`gfxInitDefault`/`consoleInit` + `socInit` for sockets), console UI, main loop (input → poll network → print)
- `network_client.{h,cpp}` – TCP socket client; handles `SendText()` and `PollMessage()` for JSON communication
- `ui_handler.{h,cpp}` – Phase 5 Citro2D graphical UI (fully implemented)

### UI Design (Cyberpunk / Pip-Boy / Matrix)

**Color Scheme (`COL_*` macros in `ui_handler.h`):**
- **Background:** Black — `C2D_Color32(0x00, 0x00, 0x00, 0xFF)`
- **Message text (received):** Matrix green — `C2D_Color32(0x00, 0xFF, 0x00, 0xFF)`
- **Message text (self):** Bright green — `C2D_Color32(0x66, 0xFF, 0x66, 0xFF)`
- **Chrome (borders, header, status, key hints):** Green — `C2D_Color32(0x00, 0xCC, 0x00, 0xFF)`
- **Warnings:** Orange — `C2D_Color32(0xFF, 0x80, 0x00, 0xFF)`
- **Errors:** White — `C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)`
- **Scanlines:** Semi-transparent black — `C2D_Color32(0x00, 0x00, 0x00, 0x60)`

**Layout (Top Screen, 400×240):**
1. Header bar: `.:eLoRa.:.3Ds:.` left + `[MESH CHAT]` right (black text on green fill)
2. Message log: up to 12 lines — green (received), bright green (self), orange (warnings), white (errors)
3. Status bar: `[CONNECTED :: ip:port]` / `[DISCONNECTED]` etc. in green
4. CRT scanline overlay: semi-transparent strips every 4px across both screens

**Bottom Screen (320×240):**
1. `INPUT:` label in green
2. `> text_` input preview with blinking cursor in green
3. Key hints: `[A]Type [X]Send [B]Del [Y]Reconn]` in green

**Philosophy:** Full-green matrix terminal aesthetic. Black background only. CRT scanlines for depth. Color hierarchy: bright green (self) > matrix green (received) > dim green (chrome) > orange (warn) > white (error).

**Bridge Module:**
- `BridgeModule.{h,cpp}` – Extends `ProtobufModule` (Meshtastic standard)
  - Listens on TCP port 4444 for 3DS connections
  - Converts incoming JSON `{"text":"..."}\n` to `MeshPacket` protobuf messages
  - Forwards received mesh packets back as JSON to the TCP client

**Supporting Files:**
- `src/mesh/Modules.cpp` – Registers BridgeModule with the Meshtastic mesh system
- `src/3ds/Makefile` – Compilation rules; includes devkitPro toolchain flags and library paths

### Key Design Decisions

1. **Custom BridgeModule over stock APIs** – Stock Meshtastic firmware exposes REST API (HTTPS port 4403) and JSON-RPC TCP interface (port 4369), but:
   - **REST API requires TLS/SSL** – devkitPro lacks OpenSSL/mbedTLS support; certificate handling infeasible on 3DS
   - **JSON-RPC is complex** – Requires full JSON-RPC protocol implementation
   - **BridgeModule solves both** – Plain TCP + simple JSON lines (`{"text":"message"}\n`) with minimal dependencies
   - **Trade-off:** Custom firmware required on Heltec, but 3DS implementation is trivial and maintainable

2. **Non-blocking network I/O** – The `NetworkClient::PollMessage()` is non-blocking so the 3DS main loop never stalls

3. **Single TCP connection** – Bridge module handles only one 3DS client at a time (simplifies state management)

4. **Simple JSON protocol** – No binary serialization; human-readable for debugging

5. **Containerized build** – Dockerfile ensures reproducible builds across different development machines

## Testing & Debugging

### Prerequisites for Testing

1. **Heltec ESP32 with BridgeModule firmware** – Flashes Meshtastic with the custom BridgeModule
   - Creates Wi-Fi AP: `meshtastic-bridge` at `192.168.4.1`
   - Listens on TCP port 4444

2. **3DS or Citra emulator** – Connected to the bridge Wi-Fi AP

### Integration Test Flow

1. Connect 3DS/Citra to `meshtastic-bridge` Wi-Fi AP
2. Launch the built 3DS application
3. Press **A** to open on-screen keyboard, type a message, confirm with **OK**
4. Press **X** to send
5. **Expected behavior:**
   - ESP32 serial console should print `Forwarded to mesh: <your text>`
   - Incoming mesh messages should appear in the 3DS console

### Debugging Hints

- **Connection failures** – Check `NetworkClient::Connect()` return value; verify bridge is running and reachable at `192.168.4.1:4444`. On hardware, sockets only work after `socInit()` succeeds (see `main.cpp`). Press **Y** in-app to retry the connection.
- **JSON parsing errors** – Ensure messages are null-terminated and end with `\n`
- **Display issues** – Verify `gfxInitDefault()` + `consoleInit()` in `main.cpp`; check libctru headers in Dockerfile

## Important Files & Line References

- **Main Loop:** `src/3ds/main.cpp:53–125` – Input → poll network → print cycle
- **Protocol Definition:** `src/3ds/network_client.h:18–21` – JSON message format
- **Bridge Endpoint Config:** `src/3ds/Makefile` – `BRIDGE_IP`/`BRIDGE_PORT` variables (consumed in `src/3ds/network_client.h:11–16`)
- **Bridge Startup:** `src/modules/BridgeModule.h` – TCP server setup, forwarding logic, and firmware integration notes

## UI Enhancement Guidelines (Phase 5)

When implementing optional features, **maintain the minimal terminal aesthetic:**
- **Black background only** – No images, gradients, or decorations
- **Main text: Green (#00FF00)** – All regular message content
- **Accent colors:** Yellow (titles, help, neutral status), Orange (warnings), White (errors)
- **Examples of proper use:**
  - Yellow: Status updates ("Connecting..."), help text, section headers
  - Orange: Non-critical alerts ("Retrying connection in 5s")
  - White: Critical errors ("Connection failed")
  - Green: Regular messages, timestamps (inline), user input

Do not add visual flourishes outside this palette. The goal is functional clarity, not aesthetics.

## Known Limitations & Future Enhancements

See `PROGRESS.md` for full project status. Notable pending items:
- Message log persistence to SD card
- Message scrolling (view >15 messages), timestamps, sender info
- Wi-Fi reconnection logic with status feedback

## Build Environment Details

- **Container OS:** Ubuntu 24.04
- **Toolchain:** devkitPro + devkitARM
- **Graphics Libraries:** Citro2D, Citro3D, libctru
- **Target:** ARM1176JZF-S (3DS CPU)
- **Compiler Flags:** `-O2`, hardware float ABI, garbage collection enabled
