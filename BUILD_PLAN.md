# MeshtasticDS MVP Build & Test Plan

## Phase 0: Documentation Discovery ✅

**Sources Consulted:**
- `Dockerfile` (lines 1-36) – Multi-stage build pipeline
- `src/3ds/Makefile` (lines 1-77) – Compilation rules and artifact paths
- `CLAUDE.md` & `PROGRESS.md` – Architectural context
- `src/3ds/` source files – Project structure

**Key Findings:**
1. **Build Pipeline:** Dockerfile uses two-stage build (builder on Ubuntu 24.04 + devkitPro, final on Alpine for artifact export)
2. **Compilation:** Makefile compiles `*.cpp` files → ELF → `.3dsx` + `.cia` via devkitARM toolchain
3. **Artifact Output:** `/app/src/3ds/build/` inside container → `/out/` (mounted volume) at runtime
4. **Configuration:** Bridge port (4444) is compiled into binary via `-DBRIDGE_PORT=4444`
5. **Dependencies:** Requires Podman/Docker, devkitPro (containerized), ARM toolchain

---

## Phase 1: Podman Environment Setup
**Duration:** ~10 minutes | **Dependencies:** None | **Can Parallelize:** N/A (prerequisite)

**Tasks:**
1. Install Podman package
2. Create `/etc/containers/nodocker` to suppress Docker-CLI warning
3. Configure `registries.conf` for short-name image resolution
4. Verify Podman installation with `podman --version`

**Success Criteria:**
- `podman --version` runs without "Docker-CLI not found" warnings
- `podman image pull hello-world && podman run hello-world` succeeds
- Short names resolve (e.g., `podman build -t test-image .` works without full registry URL)

**Anti-Patterns to Avoid:**
- Don't use `sudo podman` (use rootless mode if possible)
- Don't skip registries.conf; it prevents `image not found` errors

**Verification Checklist:**
```bash
podman --version
cat /etc/containers/nodocker  # Should exist
cat /etc/containers/registries.conf | grep -A 5 "registry"
podman pull alpine && podman run alpine echo "OK"
```

---

## Phase 2: Docker Image Build & Source Compilation
**Duration:** ~25–40 minutes | **Dependencies:** Phase 1 ✅ | **Can Parallelize:** N/A (single long task)

**Tasks:**
1. Build Docker image: `podman build -t meshtastic-3ds .` from project root
2. Wait for devkitPro installation, library compilation, and 3DS app build to complete
3. Monitor Makefile output for successful `.3dsx` and `.cia` generation

**Success Criteria:**
- Build completes with exit code 0
- Dockerfile final stage reports `Artifacts written to /out/`
- No compilation errors in Makefile (grep output for "error:" or "undefined reference")

**Anti-Patterns to Avoid:**
- Don't interrupt mid-build; wait for completion
- Don't modify Dockerfile environment variables unless explicitly needed

**Verification Checklist:**
```bash
podman build -t meshtastic-3ds . 2>&1 | tee build.log
grep -i "error\|undefined" build.log  # Should be empty
podman images | grep meshtastic-3ds   # Should list the image
```

---

## Phase 3: Binary Extraction & Validation
**Duration:** ~5 minutes | **Dependencies:** Phase 2 ✅ | **Can Parallelize:** N/A (depends on Phase 2)

**Tasks:**
1. Create output directory: `mkdir -p out`
2. Run container: `podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds`
3. Verify binaries extracted to `out/meshtastic3ds.{3dsx,cia}`
4. Check file sizes are reasonable (`.3dsx` typically 500KB–5MB, `.cia` slightly larger)

**Success Criteria:**
- `out/meshtastic3ds.3dsx` exists and is > 100KB
- `out/meshtastic3ds.cia` exists and is > 100KB
- File timestamps are recent (from this build run)

**Anti-Patterns to Avoid:**
- Don't use stale binaries from a previous run (check `ls -lt out/`)
- Don't assume files were copied; verify they exist

**Verification Checklist:**
```bash
ls -lh out/meshtastic3ds.*
file out/meshtastic3ds.3dsx
file out/meshtastic3ds.cia
md5sum out/meshtastic3ds.*  # Record for later comparison
```

---

## Phase 4: Integration Testing
**Duration:** ~30–60 minutes | **Dependencies:** Phase 3 ✅ | **Prerequisite Hardware:** Heltec ESP32 + Meshtastic firmware OR Citra emulator

**Option A: Emulator Testing (Citra)**
1. Install Citra emulator (if not already installed)
2. Load `out/meshtastic3ds.3dsx` in Citra
3. Create virtual Wi-Fi connection to simulated `meshtastic-bridge` AP
4. Launch app, open keyboard (press A), type message, press Enter
5. Verify no crash; check console output

**Option B: Hardware Testing (Recommended)**
1. Flash Heltec ESP32 with custom Meshtastic + BridgeModule
2. Create Wi-Fi AP: `meshtastic-bridge` at `192.168.4.1`
3. Connect 3DS to AP
4. Launch `.cia` file via GodMode9 or Homebrew Launcher
5. Test send/receive flow (see PROGRESS.md, step 4)

**Success Criteria:**
- 3DS app launches without errors
- Keyboard input is responsive
- Network connects to bridge (no "connection refused" errors)
- Sending a message does NOT crash the app
- *(Hardware only)* ESP32 console prints `Forwarded to mesh: <your message>`
- *(Hardware only)* Incoming mesh messages appear in 3DS console

**Anti-Patterns to Avoid:**
- Don't skip the network connection check; verify `NetworkClient::Connect()` succeeds
- Don't test with wrong bridge IP/port (should be `192.168.4.1:4444`)

**Verification Checklist:**
```
Emulator:
- App launches in Citra
- No segfault on input
- Console logs no network errors

Hardware:
- 3DS connects to meshtastic-bridge AP
- Message sends without crashing
- ESP32 serial output: "Forwarded to mesh: <message>"
- Test receiving from another Meshtastic node
```

---

## Phase 5 (Optional): UI/UX Enhancements & Persistence
**Duration:** ~60+ minutes | **Dependencies:** Phase 4 ✅ | **Can Parallelize:** Yes (3 independent features)

**UI Design Constraints (Minimal Terminal Aesthetic):**
- **Black background only** – No images, gradients, or decorations
- **Main text: Green (#00FF00)** – Standard message display
- **Accents (Yellow, Orange, White)** – Message status, warnings, error indicators only
- **Examples:**
  - Yellow: Titles, help text, neutral status
  - Orange: Warnings (e.g., "Retrying connection...")
  - White: Error messages, critical alerts
  - Green: All regular message content

**Subphase 5a: Message Log Persistence (SD Card)**
- Implement file write to SD card via libctru
- Store messages in CSV or JSON format
- Add UI toggle to view message history
- *Color guidance:* Yellow status indicator when saving

**Subphase 5b: Message Scrolling & Metadata**
- Replace fixed 15-message buffer with scrollable list
- Add timestamps (Green, inline with messages)
- Add sender info (Green text: "Alice: Hello")
- Use D-Pad to scroll through message history

**Subphase 5c: Wi-Fi Reconnection Logic**
- Detect AP disconnection
- Implement exponential backoff retry (max 3 attempts, 5–30 second delays)
- Display "Reconnecting..." status on screen (Orange)
- Auto-resume messaging after reconnect

**Success Criteria:**
- All three features work independently without breaking core messaging
- No memory leaks (test on 3DS for extended periods)
- Graceful degradation if SD card unavailable
- Color scheme adheres to UI design guidelines (no unapproved colors added)

**Verification Checklist:**
- Each feature has a dedicated code review pass
- Test core messaging still works with all enhancements enabled
- Verify SD card files are created (if using real 3DS)
- Visual inspection: all text colors match approved palette

---

## Execution Model

**Recommended Workflow:**
1. ✅ **Phase 1** (manual) – Run Podman setup commands yourself
2. ▶️ **Phase 2** (attend) – Run build, monitor for errors in real-time
3. ▶️ **Phase 3** (quick) – Extract and validate binaries
4. ▶️ **Phase 4** (hands-on) – Test with emulator or hardware
5. 🔄 **Phase 5** (optional, parallelizable) – Spawn 3 subagents for independent features

**When to Use Subagents:**
- Phase 2: None (single long build task)
- Phase 3: None (simple extraction)
- Phase 4: One agent to audit code for network bugs while you set up emulator
- Phase 5: Three agents in parallel (persistence + scrolling + reconnect logic)

---

**Status:** Ready to execute Phase 1. Say "execute" to start.
