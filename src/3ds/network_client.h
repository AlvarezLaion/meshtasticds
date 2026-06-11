#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include <cstdint>
#include <vector>

// Bridge endpoint. Defaults target the Heltec bridge AP (real hardware).
// Override at build time for emulator testing against the mock bridge:
//   make BRIDGE_IP=127.0.0.1
#ifndef BRIDGE_IP
#define BRIDGE_IP "192.168.4.1"
#endif
#ifndef BRIDGE_PORT
#define BRIDGE_PORT 4444
#endif

/**
 * Protocol: JSON lines ending with '\n'.
 * Message format: {"text": "message_content"}
 */
class NetworkClient {
public:
    static constexpr const char* TARGET_IP = BRIDGE_IP;
    static constexpr uint16_t TARGET_PORT = BRIDGE_PORT;

    NetworkClient();
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    bool Connect();
    void Disconnect();
    bool SendText(const std::string& text);
    bool PollMessage(std::string& outText);
    bool IsConnected() const { return m_connected; }

private:
    int m_socket;
    bool m_connected;
    std::vector<char> m_read_buffer;

    static constexpr int CONNECT_TIMEOUT_MS = 5000;
    static constexpr int SEND_TIMEOUT_MS = 3000;
};

#endif // NETWORK_CLIENT_H
