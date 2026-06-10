#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include <cstdint>
#include <vector>   // Fix #7: header was missing this include

/**
 * Protocol: JSON lines ending with '\n'.
 * Message format: {"text": "message_content"}
 */
class NetworkClient {
public:
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

    static constexpr const char* TARGET_IP = "127.0.0.1";  // Emulator: 127.0.0.1, Hardware: 192.168.4.1
    static constexpr uint16_t TARGET_PORT = 4444;
};

#endif // NETWORK_CLIENT_H
