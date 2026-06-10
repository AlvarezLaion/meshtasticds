#include "BridgeModule.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <cstring>

static const char *AP_SSID = "meshtastic-bridge";
// Fix #13: override BRIDGE_AP_PASS at compile time (-DBRIDGE_AP_PASS=...) to avoid
// committing credentials. Never hardcode a real password here.
#ifndef BRIDGE_AP_PASS
#define BRIDGE_AP_PASS "changeme123"
#endif
static const char *AP_PASS = BRIDGE_AP_PASS;

static const uint16_t TCP_PORT = 4444;

// Fix #15: 200 bytes only fits ~180-char messages; 512 is safer for real payloads.
static constexpr size_t JSON_DOC_SIZE = 512;

// Fix #14/#16: maximum raw payload we will copy before sending to the mesh.
static constexpr size_t MAX_PAYLOAD = 240;

BridgeModule::BridgeModule()
    : ProtobufModule(meshtastic_PortNum_PRIVATE_APP),
      tcpServer(TCP_PORT) {}

void BridgeModule::setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.println("Bridge AP started");
    startTcpServer();
}

void BridgeModule::startTcpServer() {
    tcpServer.begin();
    Serial.printf("TCP server listening on port %d\n", TCP_PORT);
}

void BridgeModule::loop() {
    if (!client || !client.connected()) {
        client = tcpServer.available();
        if (client) {
            Serial.println("3DS client connected");
        }
        return;
    }

    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        DeserializationError err = deserializeJson(doc, line);
        if (err) {
            Serial.printf("JSON parse error: %s\n", err.c_str());
            continue;
        }
        const char *txt = doc["text"];
        if (!txt) continue;

        // Fix #14: copy into a local buffer so we are not holding a pointer into
        // the ArduinoJson document (which may be freed before sendToMesh finishes).
        size_t payloadLen = strnlen(txt, MAX_PAYLOAD);
        uint8_t payloadBytes[MAX_PAYLOAD];
        memcpy(payloadBytes, txt, payloadLen);

        meshtastic_MeshPacket mp = meshtastic_MeshPacket_init_default;
        // Fix #17: use the decoded sub-struct for outbound packets.
        mp.decoded.portnum = meshtastic_PortNum_PRIVATE_APP;
        mp.decoded.payload.size = payloadLen;
        memcpy(mp.decoded.payload.bytes, payloadBytes,
               payloadLen < sizeof(mp.decoded.payload.bytes)
                   ? payloadLen
                   : sizeof(mp.decoded.payload.bytes));

        service.sendToMesh(&mp);
        Serial.printf("Forwarded to mesh: %.*s\n", (int)payloadLen, payloadBytes);
    }
}

void BridgeModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp) {
    if (mp.decoded.portnum != meshtastic_PortNum_PRIVATE_APP) return;  // Fix #17

    // Fix #17: read from mp.decoded.payload for received packets.
    String payload = String((const char *)mp.decoded.payload.bytes, mp.decoded.payload.size);

    StaticJsonDocument<JSON_DOC_SIZE> out;
    out["text"] = payload;

    // Fix #16: buf is 257 bytes so writing the '\n' at index len can never overflow.
    char buf[257];
    size_t len = serializeJson(out, buf, sizeof(buf) - 1);
    buf[len] = '\n';

    if (client && client.connected()) {
        client.write((const uint8_t *)buf, len + 1);
        Serial.printf("Sent to 3DS: %s", buf);
    }
}
