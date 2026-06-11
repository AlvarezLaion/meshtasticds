#include "network_client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cctype>
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

    // Non-blocking from the start so a dead bridge cannot freeze the app
    // for the full TCP timeout (~75s); we bound the wait with poll() below.
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(m_socket);
        m_socket = -1;
        return false;
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TARGET_PORT);
    if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
        close(m_socket);
        m_socket = -1;
        return false;
    }

    int rc = connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (rc < 0) {
        if (errno != EINPROGRESS) {
            close(m_socket);
            m_socket = -1;
            return false;
        }

        struct pollfd pfd;
        pfd.fd = m_socket;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, CONNECT_TIMEOUT_MS);
        if (pr <= 0 || (pfd.revents & (POLLERR | POLLHUP))) {
            close(m_socket);
            m_socket = -1;
            return false;
        }

        // Portable completion check: a second connect() reports EISCONN on success.
        rc = connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (rc < 0 && errno != EISCONN) {
            close(m_socket);
            m_socket = -1;
            return false;
        }
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

    // Escape special JSON characters so the payload is always valid JSON.
    std::string escaped;
    escaped.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n";  break;
            case '\r': escaped += "\\r";  break;
            case '\t': escaped += "\\t";  break;
            default:   escaped += c;      break;
        }
    }

    std::string payload = "{\"text\":\"" + escaped + "\"}\n";

    ssize_t total_sent = 0;
    const ssize_t len = static_cast<ssize_t>(payload.length());

    while (total_sent < len) {
        ssize_t bytes_sent = send(m_socket, payload.c_str() + total_sent, len - total_sent, 0);
        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Wait until the socket is writable again rather than busy-spinning;
                // give up if the bridge stays stalled past the timeout.
                struct pollfd pfd;
                pfd.fd = m_socket;
                pfd.events = POLLOUT;
                pfd.revents = 0;
                if (poll(&pfd, 1, SEND_TIMEOUT_MS) <= 0) {
                    m_connected = false;
                    return false;
                }
                continue;
            }
            m_connected = false;
            return false;
        }
        total_sent += bytes_sent;
    }

    return true;
}

// Extracts the "text" value from a JSON line, tolerating whitespace after the
// colon and handling backslash escapes (the bridge and mock server both emit
// escaped quotes inside values).
static bool ExtractTextField(const std::string& line, std::string& out) {
    size_t key = line.find("\"text\"");
    if (key == std::string::npos) return false;

    size_t i = key + 6;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) i++;
    if (i >= line.size() || line[i] != ':') return false;
    i++;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) i++;
    if (i >= line.size() || line[i] != '"') return false;
    i++;

    out.clear();
    while (i < line.size()) {
        char c = line[i];
        if (c == '"') return true;
        if (c == '\\' && i + 1 < line.size()) {
            char e = line[++i];
            switch (e) {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default:  out += e;    break;
            }
        } else {
            out += c;
        }
        i++;
    }
    return false;  // no closing quote
}

bool NetworkClient::PollMessage(std::string& outText) {
    if (m_connected) {
        // Drain everything currently available on the socket into the buffer.
        char tmp_buf[1024];
        ssize_t bytes_read;
        while ((bytes_read = recv(m_socket, tmp_buf, sizeof(tmp_buf), 0)) > 0) {
            m_read_buffer.insert(m_read_buffer.end(), tmp_buf, tmp_buf + bytes_read);
        }
        if (bytes_read == 0) {
            m_connected = false;  // peer closed; still deliver buffered lines below
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_connected = false;
        }
    }

    // Deliver the next parseable line, skipping malformed ones.
    while (true) {
        auto it = std::find(m_read_buffer.begin(), m_read_buffer.end(), '\n');
        if (it == m_read_buffer.end()) return false;

        std::string line(m_read_buffer.begin(), it);
        m_read_buffer.erase(m_read_buffer.begin(), it + 1);

        if (ExtractTextField(line, outText)) return true;
    }
}
