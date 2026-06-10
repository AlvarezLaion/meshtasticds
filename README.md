# MeshtasticDS
C++ 3DS Meshtastic Messenger

## Description
A homebrew application for the Nintendo 3DS that allows sending and receiving messages over the Meshtastic network via a Wi-Fi bridge (e.g., Heltec ESP32).

## Features
- Send and receive messages from a Nintendo 3DS.
- Integration with a custom Heltec bridge module.
- Built using devkitPro and C++.

## Building the Project
The project is designed to be built within a Docker container to provide a consistent development environment with devkitPro and the necessary 3DS libraries.

### Prerequisites
- Podman or Docker installed.

### Build Steps
1. Build the image:
   ```bash
   podman build -t meshtastic-3ds .
   ```
2. Run the container to compile and extract the binaries:
   ```bash
   mkdir -p out
   podman run --rm -v "$(pwd)/out:/out" meshtastic-3ds
   ```

The resulting `.cia` and `.3dsx` files will be available in the `out/` directory.

## Project Structure
- `src/` – C++ source code for the 3DS application.
- `Dockerfile` – Container definition for the build environment.
- `PROGRESS.md` – Detailed project tracking and build instructions.
- `.gitignore` – Files and folders to be ignored by Git.
