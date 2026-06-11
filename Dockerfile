# Use official devkitPro image with devkitARM pre-installed
FROM devkitpro/devkitarm:latest

# Install 3DS-specific libraries
RUN dkp-pacman -Syu --noconfirm 3ds-dev 3dstools

# Bridge endpoint baked into the binary.
# Default targets the Heltec hardware AP; for emulator testing build with:
#   podman build --build-arg BRIDGE_IP=127.0.0.1 -t meshtastic-3ds .
ARG BRIDGE_IP=192.168.4.1

WORKDIR /app
COPY . /app

WORKDIR /app/src/3ds
RUN make clean && make BRIDGE_IP=$BRIDGE_IP

# Export artifacts to /out at runtime
FROM alpine:3.18 AS final
COPY --from=0 /app/src/3ds/build/meshtastic3ds.3dsx /artifacts/meshtastic3ds.3dsx

ENTRYPOINT ["sh", "-c", "cp /artifacts/* /out/ && echo 'Artifacts written to /out/'"]
