#pragma once
import std;
#include "openVpnClient.h"
#include "vpnConfigManager.h"
#include "vpnProtocol.h"

class VpnConnectionManager {
public:
    VpnConnectionManager();
    ~VpnConnectionManager();

    // Connection management
    std::future<bool> connect(const std::string& configPath);
    void disconnect();
    void pause();
    void resume();
    void reconnect();

    // Status monitoring
    VpnStatus getCurrentStatus() const;
    std::string getLastError() const;
    
    // Event subscription
    void setStatusCallback(std::function<void(VpnStatus, const std::string&)> callback);

private:
    // Connection phases
    bool performConnection(const std::string& configPath);
    bool prepareConfiguration(const std::string& configPath);
    bool initiateConnection();
    bool waitForConnectionCompletion();
    void handleConnectionEvent(const std::string& eventName, const std::string& info);
    void handleLogMessage(int level, const std::string& message);
    void updateStatus(VpnStatus newStatus, const std::string& message = "");
    void handleConnectionComplete(bool success, const std::string& error = "");

    std::unique_ptr<OpenVpnClient> vpnClient;
    std::unique_ptr<VpnConfigManager> configManager;
    
    VpnStatus currentStatus = VpnStatus::Disconnected;
    std::string lastError;
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> connectionInProgress{false};
    
    mutable std::mutex statusMutex;
    std::condition_variable statusCv;
    std::thread connectionThread;
    
    // Configuration cache
    VpnConfigManager::ClientConfig currentConfig;
    
    std::function<void(VpnStatus, const std::string&)> statusCallback;
};