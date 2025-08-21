#pragma once
import std;
#include "vpnProtocol.h"
#include <openvpn/client/cliconnect.hpp>

// Forward declarations
namespace openvpn {
    class ClientConnect;
}

class OpenVpnProtocol : public VpnProtocol {
public:
    OpenVpnProtocol();
    ~OpenVpnProtocol() override;

    std::future<bool> connect(const std::string& configPath) override;
    void disconnect() override;
    VpnStatus status() const override;
    
    // Call this when user chooses "Don't use VPN"
    void allowCommunicationWithoutVpn();

    // Callback from OpenVPN3 client
    void onConnectionStateChange(bool connected, const std::string& error = "");

private:
    std::atomic<VpnStatus> currentStatus{VpnStatus::Disconnected};
    std::thread vpnThread;
    std::unique_ptr<openvpn::ClientConnect> ovpnClient;
    std::mutex clientMutex;
    
    // Private helper methods for connection management
    bool performConnection(const std::string& configPath);
    std::optional<std::string> readConfigFile(const std::string& configPath);
    bool initializeClient(const std::string& configContent);
    openvpn::ClientConnect::Config createClientConfig(const std::string& configContent);
    void setupEventCallbacks();
    void handleClientEvent(const openvpn::ClientConnect::Event& event);
    bool waitForConnection();
    void startMonitoringThread();
    void monitorConnection();
    bool handleConnectionError(const std::string& errorMessage, VpnStatus status);
};
