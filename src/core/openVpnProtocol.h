#pragma once
#include "vpnProtocol.h"
#include <atomic>
#include <thread>

class OpenVpnProtocol : public VpnProtocol {
public:
    OpenVpnProtocol();
    ~OpenVpnProtocol() override;

    std::future<bool> connect(const std::string& configPath) override;
    void disconnect() override;
    VpnStatus status() const override;

private:
    std::atomic<VpnStatus> currentStatus{VpnStatus::Disconnected};
    std::thread vpnThread;
};
