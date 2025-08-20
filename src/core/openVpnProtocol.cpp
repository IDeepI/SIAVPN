#include "openVpnProtocol.h"
#include <chrono>
#include <thread>
#include <iostream>

OpenVpnProtocol::OpenVpnProtocol() = default;

OpenVpnProtocol::~OpenVpnProtocol() {
    disconnect();
    if (vpnThread.joinable())
        vpnThread.join();
}

std::future<bool> OpenVpnProtocol::connect(const std::string& configPath) {
    currentStatus = VpnStatus::Connecting;

    return std::async(std::launch::async, [this, configPath]() {
        std::cout << "Simulating OpenVPN connection using config: "
                  << configPath << '\n';

        std::this_thread::sleep_for(std::chrono::seconds(2)); // simulate work
        currentStatus = VpnStatus::Connected;
        return true;
    });
}

void OpenVpnProtocol::disconnect() {
    if (currentStatus == VpnStatus::Connected ||
        currentStatus == VpnStatus::Connecting) {
        std::cout << "Disconnecting OpenVPN...\n";
        currentStatus = VpnStatus::Disconnected;
    }
}

VpnStatus OpenVpnProtocol::status() const {
    return currentStatus;
}
