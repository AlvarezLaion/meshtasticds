#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include <DNSServer.h>
#include <WiFi.h>

/**
 * BridgeModule – TCP ↔ Mesh bridge for the Nintendo 3DS client.
 *
 * Ported into the meshtastic/firmware tree (tested against v2.7.15). The module:
 *
 *  1. Brings up its OWN Wi-Fi access point ("meshtastic-bridge" @ 192.168.4.1)
 *     because current firmware is STA-only and has no device-as-AP mode. For
 *     this to work WITHOUT a mode conflict, leave Meshtastic's own Wi-Fi
 *     DISABLED in config (network.wifi_enabled = false); the module then fully
 *     owns the radio. ensureAccessPoint() re-asserts the AP defensively in case
 *     the firmware ever flips the Wi-Fi mode.
 *
 *     IMPORTANT (classic ESP32 / Heltec V1): build with BLE compiled out
 *     (-D MESHTASTIC_EXCLUDE_BLUETOOTH=1, already set in the heltec_v1
 *     platformio.ini). Meshtastic starts NimBLE whenever its own wifi_enabled is
 *     false, so it would run BLE alongside our soft-AP; WiFi+BLE coexistence on
 *     this ~80KB-heap chip causes a ~100s loopTask watchdog reset. The 3DS uses
 *     Wi-Fi only, so BLE is not needed here.
 *
 *  2. Runs a plain-TCP JSON-lines server on port 4444. Lines look like
 *     {"text":"hello"}\n. Inbound lines become broadcast TEXT_MESSAGE_APP mesh
 *     packets; received TEXT_MESSAGE_APP packets are forwarded back as JSON.
 *     (Extends SinglePortModule on TEXT_MESSAGE_APP for RX; the stock text
 *     module still runs because handleReceived() returns CONTINUE.)
 *
 *  3. Answers the Nintendo connection test so the 3DS will agree to SAVE the
 *     Wi-Fi connection on an AP with no internet: a wildcard DNS responder
 *     (port 53 -> 192.168.4.1) plus an HTTP responder (port 80) that returns
 *     200 with the "X-Organization: Nintendo" header.
 *
 * Periodic work (accepting clients, draining sockets, DNS) happens in runOnce()
 * via the OSThread base — there is no blocking loop.
 */
class BridgeModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    BridgeModule();

  protected:
    /// Forward received mesh text messages to the connected 3DS as JSON.
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

    /// Pump the AP, DNS, HTTP conntest, and TCP bridge.
    virtual int32_t runOnce() override;

  private:
    void ensureAccessPoint();   // bring up / re-assert the soft-AP + servers
    void serviceConntestHttp(); // answer the Nintendo connection test (port 80)
    void serviceTcpClient();    // accept + drain the 3DS JSON connection (port 4444)
    void sendJsonToClient(const char *text, size_t len);

    bool apStarted = false;
    WiFiServer tcpServer;  // port 4444 – 3DS JSON bridge
    WiFiClient tcpClient;  // single connected 3DS client
    WiFiServer httpServer; // port 80   – Nintendo conntest responder
    DNSServer dnsServer;   // port 53   – resolves everything to the AP IP
    String rxLine;         // accumulates a partial inbound JSON line
};
