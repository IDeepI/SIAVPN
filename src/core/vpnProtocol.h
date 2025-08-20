#pragma once
#include <string>
#include <future>

enum class VpnStatus { Disconnected, Connecting, Connected, Error };

class VpnProtocol {
public:
    virtual ~VpnProtocol() = default;

    // async connect so UI thread never blocks
    virtual std::future<bool> connect(const std::string& configPath) = 0;
    virtual void disconnect() = 0;
    virtual VpnStatus status() const = 0;
};
