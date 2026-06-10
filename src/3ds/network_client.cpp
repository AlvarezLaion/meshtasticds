#include "network_client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <errno.h>

NetworkClient::NetworkClient() : m_socket(-1), m_connected(false) {}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect() {
    if (m_connected) return true;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) return false;

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TARGET_PORT);
    if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
        close(m_socket);
        m_socket = -1;
        return false;
    }

    if (connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(m_socket);
        m_socket = -1;
        return false;
    }

    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        Disconnect();
        return false;
    }

    m_connected = true;
    return true;
}

void NetworkClient::Disconnect() {
    if (m_socket != -1) {
        close(m_socket);
        m_socket = -1;
    }
    m_connected = false;
    m_read_buffer.clear();
}

bool NetworkClient::SendText(const std::string& text) {
    if (!m_connected) return false;

    // Fix #8: escape special JSON characters so the payload is always valid JSON.
    std::string escaped;
    escaped.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n";  break;
            case '\r': escaped += "\\r";  break;
            default:   escaped += c;      break;
        }
    }

    std::string payload = "{\"text\":\"" + escaped + "\"}\n";

    ssize_t total_sent = 0;
    const ssize_t len = static_cast<ssize_t>(payload.length());

    while (total_sent < len) {
        ssize_t bytes_sent = send(m_socket, payload.c_str() + total_sent, len - total_sent, 0);
        if (bytes_sent < 0) {
            // Fix #9: non-blocking socket may return EAGAIN; retry instead of treating it as an error.
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            m_connected = false;
            return false;
        }
        total_sent += bytes_sent;
    }

    return true;
}

bool NetworkClient::PollMessage(std::string& outText) {
    if (!m_connected) return false;

    char tmp_buf[1024];
    ssize_t bytes_read = recv(m_socket, tmp_buf, sizeof(tmp_buf), 0);

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        m_connected = false;
        return false;
    } else if (bytes_read == 0) {
        m_connected = false;
        return false;
    }

    m_read_buffer.insert(m_read_buffer.end(), tmp_buf, tmp_buf + bytes_read);

    auto it = std::find(m_read_buffer.begin(), m_read_buffer.end(), '\n');
    if (it == m_read_buffer.end()) {
        return false;
    }

    std::string line(m_read_buffer.begin(), it);
    m_read_buffer.erase(m_read_buffer.begin(), it + 1);

    size_t text_start = line.find("\"text\":\"");
    if (text_start == std::string::npos) {
        return false;
    }
    text_start += 8;

    size_t text_end = line.find("\"", text_start);
    if (text_end == std::string::npos) {
        return false;
    }

    outText = line.substr(text_start, text_end - text_start);
    return true;
}
