#pragma once
import std;
#include "vpnProtocol.h"
#include "vpnConnectionManager.h"
#include "vpnSecurityManager.h"

class OpenVpnProtocol : public VpnProtocol {
public:
    OpenVpnProtocol();
    ~OpenVpnProtocol() override;

    // VpnProtocol interface
    std::future<bool> connect(const std::string& configPath) override;
    void disconnect() override;
    VpnStatus status() const override;
    
    // Additional control methods
    void pause();
    void resume();
    void reconnect();
    void allowCommunicationWithoutVpn();

private:
    std::unique_ptr<VpnConnectionManager> connectionManager;
    std::unique_ptr<VpnSecurityManager> securityManager;
    
    void onStatusChanged(VpnStatus status, const std::string& message);
};
