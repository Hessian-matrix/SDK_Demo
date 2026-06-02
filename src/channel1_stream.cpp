#include "viobot_demo/channel1_stream.h"

#include "viobot_demo/console.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace viobot_demo {
namespace {

constexpr std::uint32_t kBatonFrameMagic = 0x33cccc33;
constexpr int kBatonFrameHeaderSize = 32;
constexpr unsigned int kMaxStreamPayloadBytes = 4 * 1024 * 1024;

template <typename T>
T read_le_value(const char* data) {
    T value;
    std::memcpy(&value, data, sizeof(T));
    return value;
}

}  // namespace

Channel1Stream::Channel1Stream() : fd_(-1) {}

Channel1Stream::~Channel1Stream() {
    shutdown_and_close();
}

bool Channel1Stream::connect(const std::string& device_ip) {
    shutdown_and_close();

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        std::cout << kColorYellow << "create Channel=1 socket failed"
                  << kColorReset << std::endl;
        return false;
    }

    timeval recv_timeout;
    recv_timeout.tv_sec = 1;
    recv_timeout.tv_usec = 0;
    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kDevicePort);
    if (inet_pton(AF_INET, device_ip.c_str(), &addr.sin_addr) != 1) {
        std::cout << kColorYellow << "invalid device ip: " << device_ip
                  << kColorReset << std::endl;
        shutdown_and_close();
        return false;
    }

    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cout << kColorYellow << "connect /Stream?Channel=1 failed"
                  << kColorReset << std::endl;
        shutdown_and_close();
        return false;
    }

    char request[256];
    std::snprintf(request,
                  sizeof(request),
                  "GET /Stream?Channel=1 HTTP/1.1\r\n"
                  "Host: %s:%d\r\n"
                  "Accept: */*\r\n"
                  "\r\n",
                  device_ip.c_str(),
                  kDevicePort);
    const std::size_t request_length = std::strlen(request);
    if (send(fd_, request, request_length, 0) !=
        static_cast<ssize_t>(request_length)) {
        std::cout << kColorYellow << "send /Stream?Channel=1 request failed"
                  << kColorReset << std::endl;
        shutdown_and_close();
        return false;
    }

    return true;
}

void Channel1Stream::shutdown_and_close() {
    if (fd_ >= 0) {
        shutdown(fd_, SHUT_RDWR);
        close(fd_);
        fd_ = -1;
    }
}

bool Channel1Stream::read_exact(char* data,
                                std::size_t length,
                                ContinueCallback continue_callback,
                                void* user_data) {
    std::size_t offset = 0;
    while (offset < length && continue_callback(user_data)) {
        const ssize_t received = recv(fd_, data + offset, length - offset, 0);
        if (received > 0) {
            offset += static_cast<std::size_t>(received);
            continue;
        }
        if (received == 0) {
            return false;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }
        return false;
    }
    return offset == length;
}

void Channel1Stream::read_loop(FrameCallback frame_callback,
                               ContinueCallback continue_callback,
                               void* user_data) {
    char header[kBatonFrameHeaderSize];
    while (continue_callback(user_data)) {
        if (!read_exact(header, sizeof(header), continue_callback, user_data)) {
            break;
        }

        const std::uint32_t magic = read_le_value<std::uint32_t>(header);
        const std::uint32_t frame_type =
            read_le_value<std::uint32_t>(header + 4);
        const std::uint32_t frame_length =
            read_le_value<std::uint32_t>(header + 28);
        if (magic != kBatonFrameMagic ||
            frame_length > kMaxStreamPayloadBytes) {
            std::cout << kColorYellow
                      << "invalid Channel=1 frame: magic=0x" << std::hex
                      << magic << std::dec << ", type=" << frame_type
                      << ", length=" << frame_length << kColorReset
                      << std::endl;
            break;
        }

        std::vector<char> payload(frame_length);
        if (frame_length > 0 &&
            !read_exact(payload.data(), payload.size(), continue_callback, user_data)) {
            break;
        }
        frame_callback(frame_type, frame_length, payload.data(), user_data);
    }
}

}  // namespace viobot_demo
