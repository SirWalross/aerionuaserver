#pragma once

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cassert>
#include <cstring>
#include <mutex>
#include <optional>
#include <utility>

#pragma comment(lib, "Ws2_32.lib")

class SocketException : ::std::exception {};

class Socket {
   public:
    Socket(Socket const&) = delete;
    Socket& operator=(Socket const&) = delete;

    Socket(const char* addr, int port) : addr{addr}, port{port}, socket{} {
#ifdef WIN32
        ZeroMemory(&servinfo, sizeof(servinfo));
        servinfo.ai_family = AF_UNSPEC;
        servinfo.ai_socktype = SOCK_STREAM;
        servinfo.ai_protocol = IPPROTO_TCP;
#else
        addr_len = sizeof(servinfo);
        memset(&localinfo, 0, sizeof localinfo);
        memset(&servinfo, 0, sizeof servinfo);
        localinfo.sin_family = AF_INET;
        servinfo.sin_family = AF_INET;
        servinfo.sin_port = ::htons(port);
        servinfo.sin_addr.s_addr = ::inet_addr(addr);
#endif
    }

    std::optional<int> connect(int timeout_ms = 0) {
#ifdef WIN32
        ::WSADATA wsadata;
        auto iresult = WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (iresult != 0) {
            return {};
        }

        iresult = ::getaddrinfo(addr, std::to_string(port).data(), &servinfo, &localinfo);

        if (iresult != 0) {
            return {};
        }

        socket = ::socket(localinfo->ai_family, localinfo->ai_socktype, localinfo->ai_protocol);

        if (socket == INVALID_SOCKET) {
            return {};
        }

        iresult = ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms),
                               sizeof(timeout_ms));

        if (iresult == SOCKET_ERROR) {
            return {};
        }

        iresult = ::connect(socket, localinfo->ai_addr, static_cast<int>(localinfo->ai_addrlen));

        if (iresult == SOCKET_ERROR) {
            return {};
        }
        return iresult;
#else
        this->socket = ::socket(AF_INET, SOCK_STREAM, 0);

        struct timeval timeval {};
        timeval.tv_sec = 0;
        timeval.tv_usec = timeout_ms * 1000;
        int retVal = ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval));
        if (retVal < 0) {
            return {};
        }
        retVal = ::connect(socket, reinterpret_cast<struct sockaddr*>(&servinfo), addr_len);
        if (retVal == -1) {
            return {};
        }
        return retVal;
#endif
    }

    ~Socket() {
#ifdef WIN32
        ::closesocket(socket);
        WSACleanup();
#else
        ::close(socket);
#endif
    }

    std::optional<int> send(const void* sendData, const ::size_t& size) {
        const ssize_t numbytes = ::send(socket, static_cast<const char*>(sendData), size, 0);
#ifdef WIN32
        if (numbytes == SOCKET_ERROR) {
            return {};
        }
#else
        if (numbytes == -1) {
            return {};
        }
#endif
        return numbytes;
    }

    std::optional<int> recv(void* recvData, const ::size_t& size) {
        const ssize_t numbytes = ::recv(socket, static_cast<char*>(recvData), size, 0);
#ifdef WIN32
        if (numbytes == SOCKET_ERROR) {
            return {};
        }
#else
        if (numbytes <= 0) {
            return {};
        }
#endif
        static_cast<char*>(recvData)[numbytes] = 0;
        return numbytes;
    }

    const char* addr;
    int port;

   private:
#ifdef WIN32
    SOCKET socket;
    addrinfo servinfo{};
    addrinfo* localinfo{};
#else
    int socket;
    struct sockaddr_in servinfo {};
    struct sockaddr_in localinfo {};
    socklen_t addr_len{};
#endif
};