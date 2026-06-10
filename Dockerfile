# Use official devkitPro image with devkitARM pre-installed
FROM devkitpro/devkitarm:latest

# Install 3DS-specific libraries
RUN dkp-pacman -Syu --noconfirm 3ds-dev 3dstools

WORKDIR /app
COPY . /app

WORKDIR /app/src/3ds
RUN make clean && make && make cia

# Export artifacts to /out at runtime
FROM alpine:3.18 AS final
COPY --from=0 /app/src/3ds/build/meshtastic3ds.3dsx /artifacts/meshtastic3ds.3dsx
COPY --from=0 /app/src/3ds/build/meshtastic3ds.cia  /artifacts/meshtastic3ds.cia

ENTRYPOINT ["sh", "-c", "cp /artifacts/* /out/ && echo 'Artifacts written to /out/'"]
