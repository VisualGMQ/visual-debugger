/* A thin layer for Win32 Socket
    usage:

    ```cpp
    #define NET_IMPLEMENTATION
    #include "net.hpp"
    ```
*/ 
#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <array>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

namespace net
{

#define MAXCONN SOMAXCONN

static std::unordered_map<int, const char*> ErrorStrMap = {
    {WSA_INVALID_HANDLE, "WSA_INVALID_HANDLE"},
    {WSA_NOT_ENOUGH_MEMORY, "WSA_NOT_ENOUGH_MEMORY"},
    {WSA_INVALID_PARAMETER, "WSA_INVALID_PARAMETER"},
    {WSA_OPERATION_ABORTED, "WSA_OPERATION_ABORTED"},
    {WSA_IO_INCOMPLETE, "WSA_IO_INCOMPLETE"},
    {WSA_IO_PENDING, "WSA_IO_PENDING"},
    {WSAEINTR, "WSAEINTR"},
    {WSAEBADF, "WSAEBADF"},
    {WSAEACCES, "WSAEACCES"},
    {WSAEFAULT, "WSAEFAULT"},
    {WSAEINVAL, "WSAEINVAL"},
    {WSAEMFILE, "WSAEMFILE"},
    {WSAEWOULDBLOCK, "WSAEWOULDBLOCK"},
    {WSAEINPROGRESS, "WSAEINPROGRESS"},
    {WSAEALREADY, "WSAEALREADY"},
    {WSAENOTSOCK, "WSAENOTSOCK"},
    {WSAEDESTADDRREQ, "WSAEDESTADDRREQ"},
    {WSAEMSGSIZE, "WSAEMSGSIZE"},
    {WSAEPROTOTYPE, "WSAEPROTOTYPE"},
    {WSAENOPROTOOPT, "WSAENOPROTOOPT"},
    {WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT"},
    {WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT"},
    {WSAEOPNOTSUPP, "WSAEOPNOTSUPP"},
    {WSAEPFNOSUPPORT, "WSAEPFNOSUPPORT"},
    {WSAEAFNOSUPPORT, "WSAEAFNOSUPPORT"},
    {WSAEADDRINUSE, "WSAEADDRINUSE"},
    {WSAEADDRNOTAVAIL, "WSAEADDRNOTAVAIL"},
    {WSAENETDOWN, "WSAENETDOWN"},
    {WSAENETUNREACH, "WSAENETUNREACH"},
    {WSAENETRESET, "WSAENETRESET"},
    {WSAECONNABORTED, "WSAECONNABORTED"},
    {WSAECONNRESET, "WSAECONNRESET"},
    {WSAENOBUFS, "WSAENOBUFS"},
    {WSAEISCONN, "WSAEISCONN"},
    {WSAENOTCONN, "WSAENOTCONN"},
    {WSAESHUTDOWN, "WSAESHUTDOWN"},
    {WSAETOOMANYREFS, "WSAETOOMANYREFS"},
    {WSAETIMEDOUT, "WSAETIMEDOUT"},
    {WSAECONNREFUSED, "WSAECONNREFUSED"},
    {WSAELOOP, "WSAELOOP"},
    {WSAENAMETOOLONG, "WSAENAMETOOLONG"},
    {WSAEHOSTDOWN, "WSAEHOSTDOWN"},
    {WSAEHOSTUNREACH, "WSAEHOSTUNREACH"},
    {WSAENOTEMPTY, "WSAENOTEMPTY"},
    {WSAEPROCLIM, "WSAEPROCLIM"},
    {WSAEUSERS, "WSAEUSERS"},
    {WSAEDQUOT, "WSAEDQUOT"},
    {WSAESTALE, "WSAESTALE"},
    {WSAEREMOTE, "WSAEREMOTE"},
    {WSASYSNOTREADY, "WSASYSNOTREADY"},
    {WSAVERNOTSUPPORTED, "WSAVERNOTSUPPORTED"},
    {WSANOTINITIALISED, "WSANOTINITIALISED"},
    {WSAEDISCON, "WSAEDISCON"},
    {WSAENOMORE, "WSAENOMORE"},
    {WSAECANCELLED, "WSAECANCELLED"},
    {WSAEINVALIDPROCTABLE, "WSAEINVALIDPROCTABLE"},
    {WSAEINVALIDPROVIDER, "WSAEINVALIDPROVIDER"},
    {WSAEPROVIDERFAILEDINIT, "WSAEPROVIDERFAILEDINIT"},
    {WSASYSCALLFAILURE, "WSASYSCALLFAILURE"},
    {WSASERVICE_NOT_FOUND, "WSASERVICE_NOT_FOUND"},
    {WSATYPE_NOT_FOUND, "WSATYPE_NOT_FOUND"},
    {WSA_E_NO_MORE, "WSA_E_NO_MORE"},
    {WSA_E_CANCELLED, "WSA_E_CANCELLED"},
    {WSAEREFUSED, "WSAEREFUSED"},
    {WSAHOST_NOT_FOUND, "WSAHOST_NOT_FOUND"},
    {WSATRY_AGAIN, "WSATRY_AGAIN"},
    {WSANO_RECOVERY, "WSANO_RECOVERY"},
    {WSANO_DATA, "WSANO_DATA"},
    {WSA_QOS_RECEIVERS, "WSA_QOS_RECEIVERS"},
    {WSA_QOS_SENDERS, "WSA_QOS_SENDERS"},
    {WSA_QOS_NO_SENDERS, "WSA_QOS_NO_SENDERS"},
    {WSA_QOS_NO_RECEIVERS, "WSA_QOS_NO_RECEIVERS"},
    {WSA_QOS_REQUEST_CONFIRMED, "WSA_QOS_REQUEST_CONFIRMED"},
    {WSA_QOS_ADMISSION_FAILURE, "WSA_QOS_ADMISSION_FAILURE"},
    {WSA_QOS_POLICY_FAILURE, "WSA_QOS_POLICY_FAILURE"},
    {WSA_QOS_BAD_STYLE, "WSA_QOS_BAD_STYLE"},
    {WSA_QOS_BAD_OBJECT, "WSA_QOS_BAD_OBJECT"},
    {WSA_QOS_TRAFFIC_CTRL_ERROR, "WSA_QOS_TRAFFIC_CTRL_ERROR"},
    {WSA_QOS_GENERIC_ERROR, "WSA_QOS_GENERIC_ERROR"},
    {WSA_QOS_ESERVICETYPE, "WSA_QOS_ESERVICETYPE"},
    {WSA_QOS_EFLOWSPEC, "WSA_QOS_EFLOWSPEC"},
    {WSA_QOS_EPROVSPECBUF, "WSA_QOS_EPROVSPECBUF"},
    {WSA_QOS_EFILTERSTYLE, "WSA_QOS_EFILTERSTYLE"},
    {WSA_QOS_EFILTERTYPE, "WSA_QOS_EFILTERTYPE"},
    {WSA_QOS_EFILTERCOUNT, "WSA_QOS_EFILTERCOUNT"},
    {WSA_QOS_EOBJLENGTH, "WSA_QOS_EOBJLENGTH"},
    {WSA_QOS_EFLOWCOUNT, "WSA_QOS_EFLOWCOUNT"},
    {WSA_QOS_EUNKOWNPSOBJ, "WSA_QOS_EUNKOWNPSOBJ"},
    {WSA_QOS_EPOLICYOBJ, "WSA_QOS_EPOLICYOBJ"},
    {WSA_QOS_EFLOWDESC, "WSA_QOS_EFLOWDESC"},
    {WSA_QOS_EPSFLOWSPEC, "WSA_QOS_EPSFLOWSPEC"},
    {WSA_QOS_EPSFILTERSPEC, "WSA_QOS_EPSFILTERSPEC"},
    {WSA_QOS_ESDMODEOBJ, "WSA_QOS_ESDMODEOBJ"},
    {WSA_QOS_ESHAPERATEOBJ, "WSA_QOS_ESHAPERATEOBJ"},
    {WSA_QOS_RESERVED_PETYPE, "WSA_QOS_RESERVED_PETYPE"},
};

enum class SockType {
    STREAM = 0,
    DGRAM,
};

enum class Protocol {
    TCP = 0,
    UDP,
};

enum class Family {
    IPv4 = 0,
    IPv6,
};

enum class Flags {
    None = 0,
    Passiv,
};

template <typename ErrorCodeType, typename T>
struct Result {
    ErrorCodeType result;
    T value;

    Result(ErrorCodeType result, const T& value): result(result), value(value) {}
    Result(ErrorCodeType result, T&& val): result(result), value(std::forward<T>(val)) {}
};

// structure implement

struct AddrInfo {
    addrinfo info;

    AddrInfo(addrinfo* info) {
        memcpy(&this->info, info, sizeof(addrinfo));
        freeaddrinfo(info);
    }

    AddrInfo() {
        memset(&info, 0, sizeof(addrinfo));
    }
};


struct AddrInfo;
struct AddrInfoBuilder {
    static AddrInfoBuilder CreateTCP(const std::string& address, uint16_t port) {
        AddrInfoBuilder builder;
        builder.SetAddress(address)
               .SetPort(port)
               .SetFamily(Family::IPv4)
               .SetProtocol(Protocol::TCP)
               .SetSockType(SockType::STREAM)
               .SetFlags(Flags::Passiv);
        return builder;
    }

    static AddrInfoBuilder CreateUDP(const std::string& address, uint16_t port) {
        AddrInfoBuilder builder;
        builder.SetAddress(address)
               .SetPort(port)
               .SetFamily(Family::IPv4)
               .SetProtocol(Protocol::UDP)
               .SetSockType(SockType::DGRAM)
               .SetFlags(Flags::Passiv);
        return builder;
    }

    AddrInfoBuilder& SetAddress(const std::string& address) {
        address_ = address;
        return *this;
    }
    AddrInfoBuilder& SetPort(uint16_t port) {
        port_ = port;
        return *this;
    }
    AddrInfoBuilder& SetProtocol(Protocol protocol) {
        protocol_ = protocol;
        return *this;
    }
    AddrInfoBuilder& SetFlags(Flags flags) {
        flags_ = flags;
        return *this;
    }
    AddrInfoBuilder& SetFamily(Family family) {
        family_ = family;
        return *this;
    }
    AddrInfoBuilder& SetSockType(SockType socktype) {
        socktype_ = socktype;
        return *this;
    }
    
    Result<int, AddrInfo> Build();

private:
    Family family_;
    SockType socktype_;
    Protocol protocol_;
    uint16_t port_;
    Flags flags_;
    std::string address_;
};

class Socket final {
public:
    Socket() = default;
    explicit Socket(SOCKET s);
    explicit Socket(const AddrInfo& addr);
    Socket(Socket&&);
    Socket(const Socket&) = delete;
    ~Socket();

    Socket& operator=(Socket&&);
    Socket& operator=(const Socket&) = delete;

    int EnableNoBlock();

    int Bind();
    int Listen(int backlog = MAXCONN);
    int Close(int = SD_BOTH);
    Result<int, std::unique_ptr<Socket>> Accept(sockaddr&);
    bool Valid() const;
    int Connect();

    int Recv(char* buf, size_t size);
    int Send(char* buf, size_t size);
    
    template <typename T>
    int Recv(const T& buf);
    template <typename T>
    int Send(const T& buf);

private:
    SOCKET s_ = INVALID_SOCKET;
    const AddrInfo* addr_ = nullptr;

    friend void swap(Socket& lhs, Socket& rhs) {
        std::swap(lhs.s_, rhs.s_);
        std::swap(lhs.addr_, rhs.addr_);
    }
};

class Net final {
public:
    Net();
    ~Net();

    Socket* CreateSocket(const AddrInfo& addr);

private:
    WSADATA wsaData_;
    std::vector<std::unique_ptr<Socket>> sockets_;
};

// some platform specific mapper

constexpr std::array<uint8_t, 2> SockTypeMapper {
    SOCK_STREAM,
    SOCK_DGRAM,
};

constexpr std::array<uint8_t, 2> ProtocolMapper {
    IPPROTO_TCP,
    IPPROTO_UDP,
};

constexpr std::array<uint8_t, 2> FamilyMapper {
    AF_INET,
    AF_INET6,
};

constexpr std::array<uint8_t, 2> FlagsMapper {
    0,
    AI_PASSIVE,
};

// function declarations

extern int GetLastErrorCode();
extern const char* GetLastError();
extern const char* Error2Str(int);

extern [[nodiscard]] std::unique_ptr<Net> Init();


#ifdef NET_IMPLEMENTATION


// function implementations

Result<int, AddrInfo> AddrInfoBuilder::Build() {
    addrinfo hint;
    ZeroMemory(&hint, sizeof(hint));
    hint.ai_family = FamilyMapper[static_cast<uint8_t>(family_)];
    hint.ai_flags = FlagsMapper[static_cast<uint8_t>(flags_)];
    hint.ai_socktype = SockTypeMapper[static_cast<uint8_t>(socktype_)];
    hint.ai_protocol = ProtocolMapper[static_cast<uint8_t>(protocol_)];

    addrinfo* result;
    auto port = std::to_string(port_);
    int resultCode = getaddrinfo(address_.c_str() == "" ? nullptr : address_.c_str(), port.c_str(), &hint, &result);
    return Result<int, AddrInfo>(resultCode, AddrInfo(result));
}

[[nodiscard]] std::unique_ptr<Net> Init()
{

    return std::make_unique<Net>();
}

int GetLastErrorCode() {
    return WSAGetLastError();
}

const char* GetLastError() {
    return Error2Str(WSAGetLastError());
}

const char* Error2Str(int error) {
    auto it = ErrorStrMap.find(error);
    if (it != ErrorStrMap.end()) {
        return it->second;
    }
    return "Unknown Error";
}

Socket::Socket(SOCKET s) : s_(s) {}

Socket::Socket(const AddrInfo &addr) : addr_(&addr) {
    s_ = socket(addr.info.ai_family, addr.info.ai_socktype, addr.info.ai_protocol);
    if (s_ == INVALID_SOCKET) {
        std::cerr << "socket create failed" << GetLastError() << std::endl;
    }
}

Socket::Socket(Socket&& o) {
    swap(*this, o);
}

Socket& Socket::operator=(Socket&& o) {
    if (&o != this) {
        swap(*this, o);
    }
    return *this;
}

Socket::~Socket() {
    closesocket(s_);
}


bool Socket::Valid() const {
    return s_ != INVALID_SOCKET;
}

int Socket::Close(int flag) {
    if (Valid()) {
        shutdown(s_, flag);
        int result = closesocket(s_);
        if (result == 0) {
            s_ = INVALID_SOCKET;
        }
        return result;
    }
    return 0;
}

int Socket::Bind() {
    return bind(s_, addr_->info.ai_addr, (int)addr_->info.ai_addrlen);
}

int Socket::Listen(int backlog) {
    return listen(s_, backlog);
}

int Socket::Connect() {
    return connect(s_, addr_->info.ai_addr, addr_->info.ai_addrlen);
}

int Socket::EnableNoBlock() {
    unsigned long mode = 1;
    int result = ioctlsocket(s_, FIONBIO, &mode);
    if (result != NO_ERROR) {
        std::cerr << "enable no blocking mode failed" << GetLastError() << std::endl;
    }
    return result;
}

Result<int, std::unique_ptr<Socket>> Socket::Accept(sockaddr& addr) {
    int len = sizeof(sockaddr);
    SOCKET clientSocket = accept(s_, &addr, &len);
    int result = 0;
    if (clientSocket == INVALID_SOCKET) {
        result = WSAGetLastError();
    }

    return Result<int, std::unique_ptr<Socket>>(result, std::make_unique<Socket>(clientSocket));
}


int Socket::Recv(char* buf, size_t size) {
    return recv(s_, buf, size, 0);
}

int Socket::Send(char* buf, size_t size) {
    return send(s_, buf, size, 0);
}

template <typename T>
int Socket::Recv(const T& buf) {
    return recv(s_, buf.data(), buf.size(), 0);
}

template <typename T>
int Socket::Send(const T& buf) {
    return send(s_, buf.data(), buf.size(), 0);
}

Net::Net() {
    WSAStartup(MAKEWORD(2, 2), &wsaData_);
}

Net::~Net() {
    sockets_.clear();
    WSACleanup();
}

Socket* Net::CreateSocket(const AddrInfo& addr) {
    sockets_.push_back(std::make_unique<Socket>(addr));
    return sockets_.back().get();
}

#endif

}
