#pragma once
#include "mesh/ProtobufModule.h"
#include "meshtastic/mesh.pb.h"

/**
 * BridgeModule – simple TCP ↔ Mesh bridge.
 *
 * * Listens on TCP port 4444 (Wi‑Fi AP mode).
 * * Accepts JSON lines like {"text":"hello"}\n from the 3DS client.
 * * Converts them to a MeshPacket on PRIVATE_APP port and forwards to the mesh.
 * * Receives MeshPackets on the same port and forwards them back as JSON.
 */
class BridgeModule : public ProtobufModule {
public:
    BridgeModule();
    void setup() override;
    void loop() override;
    void handleReceivedProtobuf(const meshtastic_MeshPacket &mp) override;

private:
    void startTcpServer();
    void handleTcpClient();

    WiFiServer tcpServer;   // listening server (port 4444)
    WiFiClient client;      // current connected 3DS client (single connection)
};
