#include "BridgeModule.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "configuration.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include <Arduino.h>

static const char *AP_SSID = "meshtastic-bridge";
// Override at compile time (-DBRIDGE_AP_PASS=...) to avoid committing real
// credentials. WPA2 requires >= 8 chars; "" would make an open network.
#ifndef BRIDGE_AP_PASS
#define BRIDGE_AP_PASS "changeme123"
#endif
static const char *AP_PASS = BRIDGE_AP_PASS;

static const uint16_t TCP_PORT = 4444;
static const uint16_t HTTP_PORT = 80;
static const uint16_t DNS_PORT = 53;
static const IPAddress AP_IP(192, 168, 4, 1);

// --- minimal JSON helpers (the firmware does not bundle ArduinoJson, and the
//     protocol is a single flat {"text":"..."} object, so hand-rolling is both
//     dependency-free and matches the 3DS client's parser exactly) -----------

// Append `in` to `out`, escaping the characters JSON requires inside a string.
static void jsonEscapeInto(String &out, const uint8_t *in, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char c = (char)in[i];
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
}

// Extract and unescape the "text" value from one JSON line into `out` (capped at
// `outCap`). Tolerates whitespace after the colon. Returns the byte count, or 0
// if the field is missing/malformed.
static size_t jsonExtractText(const String &line, uint8_t *out, size_t outCap)
{
    int key = line.indexOf("\"text\"");
    if (key < 0)
        return 0;
    int i = key + 6;
    int n = line.length();
    while (i < n && isspace((unsigned char)line[i]))
        i++;
    if (i >= n || line[i] != ':')
        return 0;
    i++;
    while (i < n && isspace((unsigned char)line[i]))
        i++;
    if (i >= n || line[i] != '"')
        return 0;
    i++;

    size_t w = 0;
    while (i < n && w < outCap) {
        char c = line[i];
        if (c == '"')
            break; // closing quote
        if (c == '\\' && i + 1 < n) {
            char e = line[++i];
            switch (e) {
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            default:
                c = e;
                break;
            }
        }
        out[w++] = (uint8_t)c;
        i++;
    }
    return w;
}

BridgeModule::BridgeModule()
    : SinglePortModule("BridgeModule", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("BridgeModule"),
      tcpServer(TCP_PORT), httpServer(HTTP_PORT)
{
}

void BridgeModule::ensureAccessPoint()
{
    wifi_mode_t mode = WiFi.getMode();
    if (apStarted && (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA))
        return;

    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASS);

    tcpServer.begin();
    tcpServer.setNoDelay(true);
    httpServer.begin();
    dnsServer.start(DNS_PORT, "*", AP_IP); // wildcard: every lookup -> AP_IP

    apStarted = true;
    LOG_INFO("BridgeModule: AP '%s' up at %s (TCP %u)", AP_SSID, WiFi.softAPIP().toString().c_str(), TCP_PORT);
}

// The 3DS performs a connection test against conntest.nintendowfc.net before it
// will save a Wi-Fi profile. Our wildcard DNS points that name here; we just
// have to answer any HTTP request with 200 + the X-Organization: Nintendo
// header that the test looks for.
void BridgeModule::serviceConntestHttp()
{
    WiFiClient http = httpServer.available();
    if (!http)
        return;

    // Drain the request line(s); we do not care about the contents.
    uint32_t start = millis();
    while (http.connected() && http.available() && millis() - start < 50)
        http.read();

    static const char *RESPONSE = "HTTP/1.1 200 OK\r\n"
                                  "Content-Length: 0\r\n"
                                  "X-Organization: Nintendo\r\n"
                                  "Connection: close\r\n"
                                  "\r\n";
    http.print(RESPONSE);
    http.flush();
    http.stop();
}

void BridgeModule::sendJsonToClient(const char *text, size_t len)
{
    if (!tcpClient || !tcpClient.connected())
        return;
    String out = "{\"text\":\"";
    jsonEscapeInto(out, (const uint8_t *)text, len);
    out += "\"}\n";
    tcpClient.print(out);
}

void BridgeModule::serviceTcpClient()
{
    // Accept a new client if we do not currently have a live one.
    if (!tcpClient || !tcpClient.connected()) {
        WiFiClient incoming = tcpServer.available();
        if (incoming) {
            tcpClient.stop();
            tcpClient = incoming;
            rxLine = "";
            LOG_INFO("BridgeModule: 3DS client connected");
        }
        return;
    }

    while (tcpClient.available()) {
        char c = (char)tcpClient.read();
        if (c == '\n') {
            // Parse into a stack buffer first; only touch the packet pool on success.
            meshtastic_Data scratch = meshtastic_Data_init_default;
            size_t plen = jsonExtractText(rxLine, scratch.payload.bytes, sizeof(scratch.payload.bytes));
            rxLine = "";
            if (plen == 0)
                continue;

            meshtastic_MeshPacket *p = allocDataPacket(); // portnum = TEXT_MESSAGE_APP
            p->to = NODENUM_BROADCAST;
            p->channel = 0; // primary channel
            p->want_ack = false;
            p->decoded.payload.size = plen;
            memcpy(p->decoded.payload.bytes, scratch.payload.bytes, plen);
            service->sendToMesh(p, RX_SRC_LOCAL, true);
            LOG_INFO("BridgeModule: forwarded to mesh: %.*s", (int)plen, scratch.payload.bytes);
        } else if (c != '\r') {
            if (rxLine.length() < 1024) // guard against unbounded growth
                rxLine += c;
            else
                rxLine = ""; // oversized line with no newline -> drop
        }
    }
}

ProcessMessage BridgeModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    // Skip our own outbound packets so we do not echo the 3DS's sends back to it.
    if (!isFromUs(&mp) && mp.which_payload_variant == meshtastic_MeshPacket_decoded_tag) {
        sendJsonToClient((const char *)mp.decoded.payload.bytes, mp.decoded.payload.size);
        LOG_INFO("BridgeModule: sent to 3DS: %.*s", (int)mp.decoded.payload.size, mp.decoded.payload.bytes);
    }
    return ProcessMessage::CONTINUE; // let the stock text-message module run too
}

int32_t BridgeModule::runOnce()
{
    ensureAccessPoint();
    dnsServer.processNextRequest();
    serviceConntestHttp();
    serviceTcpClient();
    return 20; // poll every 20 ms for responsive TCP
}
