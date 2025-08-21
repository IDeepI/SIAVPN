import std;
#include "vpnConnectionManager.h"

VpnConnectionManager::VpnConnectionManager() 
    : vpnClient(std::make_unique<OpenVpnClient>())
    , configManager(std::make_unique<VpnConfigManager>())
    , currentStatus(VpnStatus::Disconnected)
    , shouldStop(false)
    , connectionInProgress(false) {
    
    // Set up event handlers
    vpnClient->setEventHandler([this](const std::string& eventName, const std::string& info) {
        handleConnectionEvent(eventName, info);
    });
    
    vpnClient->setLogHandler([this](int level, const std::string& message) {
        handleLogMessage(level, message);
    });
}

VpnConnectionManager::~VpnConnectionManager() {
    disconnect();
    if (connectionThread.joinable()) {
        connectionThread.join();
    }
}

std::future<bool> VpnConnectionManager::connect(const std::string& configPath) {
    if (connectionInProgress) {
        return std::async(std::launch::deferred, []() { return false; });
    }
    
    updateStatus(VpnStatus::Connecting, "Starting connection...");
    connectionInProgress = true;
    shouldStop = false;

    return std::async(std::launch::async, [this, configPath]() {
        bool result = performConnection(configPath);
        connectionInProgress = false;
        return result;
    });
}

void VpnConnectionManager::disconnect() {
    if (currentStatus == VpnStatus::Disconnected) {
        return;
    }

    updateStatus(VpnStatus::Disconnected, "Disconnecting...");
    shouldStop = true;

    try {
        // Signal VPN client to stop
        vpnClient->stopConnection();
        
        // Wait for worker thread to complete
        if (connectionThread.joinable()) {
            connectionThread.join();
        }
        
        updateStatus(VpnStatus::Disconnected, "Disconnected successfully");
        
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Error during disconnect: " + std::string(e.what()));
    }
}

void VpnConnectionManager::pause() {
    try {
        vpnClient->pauseConnection();
        updateStatus(VpnStatus::Disconnected, "Connection paused");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to pause: " + std::string(e.what()));
    }
}

void VpnConnectionManager::resume() {
    try {
        vpnClient->resumeConnection();
        updateStatus(VpnStatus::Connecting, "Resuming connection...");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to resume: " + std::string(e.what()));
    }
}

void VpnConnectionManager::reconnect() {
    try {
        vpnClient->reconnectConnection();
        updateStatus(VpnStatus::Connecting, "Reconnecting...");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to reconnect: " + std::string(e.what()));
    }
}

VpnStatus VpnConnectionManager::getCurrentStatus() const {
    return currentStatus;
}

std::string VpnConnectionManager::getLastError() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return lastError;
}

void VpnConnectionManager::setStatusCallback(std::function<void(VpnStatus, const std::string&)> callback) {
    statusCallback = std::move(callback);
}

bool VpnConnectionManager::performConnection(const std::string& configPath) {
    try {
        // Phase 1: Configuration preparation
        if (!prepareConfiguration(configPath)) {
            return false;
        }

        // Phase 2: Start the connection process
        if (!initiateConnection()) {
            return false;
        }

        // Phase 3: Wait for connection completion
        return waitForConnectionCompletion();

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Connection error: " + std::string(e.what()));
        return false;
    }
}

bool VpnConnectionManager::prepareConfiguration(const std::string& configPath) {
    try {
        updateStatus(VpnStatus::Connecting, "Loading configuration...");
        
        // Load and validate configuration file
        std::string configContent = configManager->loadConfigFromFile(configPath);
        if (configContent.empty()) {
            handleConnectionComplete(false, "Failed to read configuration file");
            return false;
        }

        // Create configuration object
        currentConfig = configManager->createConfig(configContent);
        
        // Validate configuration
        auto validation = configManager->validateConfig(currentConfig);
        if (!validation.isValid) {
            handleConnectionComplete(false, "Configuration validation failed: " + validation.errorMessage);
            return false;
        }

        // Log warnings if any
        for (const auto& warning : validation.warnings) {
            handleLogMessage(2, warning); // Warning level
        }

        updateStatus(VpnStatus::Connecting, "Configuration validated successfully");
        return true;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Configuration preparation failed: " + std::string(e.what()));
        return false;
    }
}

bool VpnConnectionManager::initiateConnection() {
    try {
        updateStatus(VpnStatus::Connecting, "Establishing connection...");

        // Start connection using the VPN client
        bool started = vpnClient->startConnection(currentConfig.content);
        if (!started) {
            std::string error = vpnClient->getLastError();
            handleConnectionComplete(false, "Failed to start connection: " + error);
            return false;
        }

        updateStatus(VpnStatus::Connecting, "Connection initiated successfully");
        return true;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Failed to initiate connection: " + std::string(e.what()));
        return false;
    }
}

bool VpnConnectionManager::waitForConnectionCompletion() {
    try {
        std::unique_lock<std::mutex> lock(statusMutex);
        auto timeout = std::chrono::seconds(30);
        
        updateStatus(VpnStatus::Connecting, "Waiting for connection establishment...");
        
        bool completed = statusCv.wait_for(lock, timeout, [this]() {
            return currentStatus == VpnStatus::Connected || 
                   currentStatus == VpnStatus::Error ||
                   shouldStop;
        });

        if (shouldStop) {
            handleConnectionComplete(false, "Connection cancelled by user");
            return false;
        }
        
        if (!completed) {
            handleConnectionComplete(false, "Connection timeout - unable to establish VPN tunnel");
            return false;
        }

        if (currentStatus == VpnStatus::Error) {
            // Error details should already be set
            return false;
        }
        
        return currentStatus == VpnStatus::Connected;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Error waiting for connection: " + std::string(e.what()));
        return false;
    }
}

void VpnConnectionManager::handleConnectionEvent(const std::string& eventName, const std::string& info) {
    if (eventName == "CONNECTED") {
        updateStatus(VpnStatus::Connected, "VPN connection established");
        
    } else if (eventName == "DISCONNECTED") {
        updateStatus(VpnStatus::Disconnected, info.empty() ? "Disconnected" : info);
        
    } else if (eventName == "RECONNECTING") {
        updateStatus(VpnStatus::Connecting, "Reconnecting...");
        
    } else if (eventName == "CONNECTING") {
        updateStatus(VpnStatus::Connecting, info);
        
    } else if (eventName == "PAUSED") {
        updateStatus(VpnStatus::Disconnected, "Connection paused");
        
    } else if (eventName == "RESUMED") {
        updateStatus(VpnStatus::Connecting, "Connection resumed");
        
    } else {
        // Log other events for debugging
        handleLogMessage(3, "Event: " + eventName + " - " + info);
    }
}

void VpnConnectionManager::handleLogMessage(int level, const std::string& message) {
    // Filter and format log messages
    std::string prefix;
    
    switch (level) {
        case 0: // Fatal
            prefix = "[FATAL] ";
            std::cerr << prefix << message << '\n';
            break;
        case 1: // Error  
            prefix = "[ERROR] ";
            std::cerr << prefix << message << '\n';
            break;
        case 2: // Warning
            prefix = "[WARN] ";
            std::cout << prefix << message << '\n';
            break;
        case 3: // Info
            prefix = "[INFO] ";
            std::cout << prefix << message << '\n';
            break;
        default: // Debug and verbose
            #ifdef _DEBUG
            prefix = "[DEBUG] ";
            std::cout << prefix << message << '\n';
            #endif
            break;
    }
}

void VpnConnectionManager::updateStatus(VpnStatus newStatus, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        currentStatus = newStatus;
        if (!message.empty()) {
            lastError = message;
        }
    }
    statusCv.notify_all();
    
    // Notify callback if set
    if (statusCallback) {
        statusCallback(newStatus, message);
    }
    
    if (!message.empty()) {
        std::cout << "[VPN] " << message << '\n';
    }
}

void VpnConnectionManager::handleConnectionComplete(bool success, const std::string& error) {
    if (success) {
        updateStatus(VpnStatus::Connected, "VPN connection established successfully");
    } else {
        updateStatus(VpnStatus::Error, error.empty() ? "Connection failed" : error);
    }
}
