#pragma once
import std;
#include "vpnProtocol.h"
#include <openvpn/client/ovpncli.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/cleanup.hpp>

// Forward declarations
namespace openvpn {
    namespace ClientAPI {
        class OpenVPNClient;
    }
}

class OpenVpnProtocol : public VpnProtocol, public openvpn::ClientAPI::OpenVPNClient {
public:
    OpenVpnProtocol();
    ~OpenVpnProtocol() override;

    // VpnProtocol interface
    std::future<bool> connect(const std::string& configPath) override;
    void disconnect() override;
    VpnStatus status() const override;
    
    // OpenVPNClient callbacks - must be implemented
    void event(const openvpn::ClientAPI::Event& ev) override;
    void log(const openvpn::ClientAPI::LogInfo& log) override;
    
    // Additional control methods
    void pause();
    void resume();
    void reconnect();
    void allowCommunicationWithoutVpn();

private:
    // Connection management methods
    bool performConnection(const std::string& configPath);
    bool prepareConfiguration(const std::string& configPath);
    bool initiateConnection();
    void executeConnectionWorker();
    bool waitForConnectionCompletion();
    void cleanupFailedConnection();

    // Configuration and helper methods
    std::string readConfigFile(const std::string& configPath);
    openvpn::ClientAPI::Config createConfig(const std::string& configContent);
    
    // Status and communication management
    void updateStatus(VpnStatus newStatus, const std::string& message = "");
    void blockCommunication();
    void unblockCommunication();
    void handleConnectionComplete(bool success, const std::string& error = "");
    void secureCleanup();

    // Member variables
    VpnStatus currentStatus = VpnStatus::Disconnected;
    std::string lastError;
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> connectionInProgress{false};
    std::atomic<bool> communicationBlocked{true};
    
    mutable std::mutex statusMutex;
    std::condition_variable statusCv;
    std::thread vpnThread;
    
    // Store current configuration for reuse
    openvpn::ClientAPI::Config currentConfig;
};
