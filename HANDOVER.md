# Phase 5 Handover — Cyberpunk UI Implementation

## Project

MeshtasticDS: Nintendo 3DS homebrew that sends/receives Meshtastic mesh messages over Wi-Fi via a Heltec ESP32 bridge. The 3DS app connects to `192.168.4.1:4444` over TCP and exchanges `{"text":"..."}\n` JSON lines.

## Phase 5 Goal

Replace the console-only `printf()` UI with a full Citro2D GPU-rendered interface: black background, matrix-green messages, pip-boy amber chrome, CRT scanlines, and the branding string `.:eLoRa.:.3Ds:.` in the header.

## Subagent Roles

| Agent | Files Owned | Responsibility |
|-------|-------------|----------------|
| A (this doc) | `src/3ds/Makefile`, `src/3ds/ui_handler.h`, `HANDOVER.md` | Build infra + class interface |
| C | `src/3ds/ui_handler.cpp` | Full Citro2D rendering implementation |
| D | `src/3ds/main.cpp` | Replace printf with UIHandler calls |

**No agent touches another agent's files.**
`src/3ds/network_client.h` and `src/3ds/network_client.cpp` are **never modified**.

## Key Invariants (must not be violated)

1. **Link order**: `LIBS := -lcitro2d -lcitro3d -lctru -lstdc++ -lm` — reversing breaks the build.
2. **`C2D_TextBufClear` before `C3D_FrameBegin`** in `Render()` — calling it after causes a GPU fault.
3. **`.:eLoRa.:.3Ds:.`** must appear verbatim (character-exact) in `DrawTopScreen()` header text.
4. **Z-depth layers**: bg=0.0f (via TargetClear), chrome/borders=0.1f, text=0.5f, scanlines=0.9f.
5. **`gfxInitDefault()`** stays in `main.cpp` — Citro2D requires it. Only `consoleInit()` is removed.
6. **`ui.Render()`** replaces the three-line block `gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();` — do not keep both.

## Build

```bash
# Hardware build (targets 192.168.4.1)
podman build -t meshtastic-3ds .
mkdir -p out && podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds

# Emulator build (targets 127.0.0.1)
podman build --build-arg BRIDGE_IP=127.0.0.1 -t meshtastic-3ds-emu .
podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds-emu
```

## Citro2D Library Paths

Installed inside the container by the `3ds-dev` package:
- Headers: `$(DEVKITPRO)/portlibs/3ds/include/citro2d.h`
- Libs: `$(DEVKITPRO)/portlibs/3ds/lib/libcitro2d.a` and `libcitro3d.a`

If build fails with `cannot find -lcitro2d`, verify with:
```bash
podman run --rm devkitpro/devkitarm:latest find /opt/devkitpro/portlibs -name "libcitro2d.a"
```
The output path sets `PORTLIBS` in the Makefile.

## Fallback (if citro2d ABI fails)

Use direct VRAM framebuffer: `gfxGetFramebuffer(GFX_TOP, GFX_LEFT, ...)` returns BGR888
column-major buffer. Pixel `(x,y)` → byte offset `(x*240 + (239-y)) * 3`.
Implement an 8×8 bitmap font for text rendering. Same visual design, no GPU dependency.
