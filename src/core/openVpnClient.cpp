import std;
#include "openVpnClient.h"

OpenVpnClient::OpenVpnClient() {
    lastError.clear();
    currentConfig.clear();
}

OpenVpnClient::~OpenVpnClient() {
    stopConnection();
    if (connectionThread.joinable()) {
        connectionThread.join();
    }
}

bool OpenVpnClient::startConnection(const std::string& configContent) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    if (isRunning) {
        lastError = "Connection already in progress";
        return false;
    }
    
    if (configContent.empty()) {
        lastError = "Configuration content is empty";
        return false;
    }
    
    currentConfig = configContent;
    shouldStop = false;
    isRunning = true;
    
    // Start connection simulation in background thread
    connectionThread = std::thread([this]() {
        simulateConnectionProcess();
    });
    
    handleInternalLog(3, "OpenVPN client connection initiated");
    return true;
}

void OpenVpnClient::stopConnection() {
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (!isRunning) {
            return;
        }
        
        shouldStop = true;
        isRunning = false;
    }
    
    if (connectionThread.joinable()) {
        connectionThread.join();
    }
    
    handleInternalEvent("DISCONNECTED", "Connection stopped by user");
    handleInternalLog(3, "OpenVPN client disconnected");
}

void OpenVpnClient::pauseConnection() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (isRunning) {
        handleInternalEvent("PAUSED", "Connection paused");
        handleInternalLog(3, "OpenVPN client paused");
    }
}

void OpenVpnClient::resumeConnection() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (isRunning) {
        handleInternalEvent("RESUMED", "Connection resumed");
        handleInternalLog(3, "OpenVPN client resumed");
    }
}

void OpenVpnClient::reconnectConnection() {
    handleInternalEvent("RECONNECTING", "Attempting to reconnect");
    handleInternalLog(3, "OpenVPN client reconnecting");
    
    // Stop current connection
    stopConnection();
    
    // Restart with current config
    if (!currentConfig.empty()) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Brief delay
        startConnection(currentConfig);
    }
}

bool OpenVpnClient::isConnected() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return isRunning && !shouldStop;
}

std::string OpenVpnClient::getLastError() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastError;
}

void OpenVpnClient::setEventHandler(std::function<void(const std::string&, const std::string&)> handler) {
    eventHandler = std::move(handler);
}

void OpenVpnClient::setLogHandler(std::function<void(int, const std::string&)> handler) {
    logHandler = std::move(handler);
}

void OpenVpnClient::simulateConnectionProcess() {
    const std::vector<std::pair<std::string, std::string>> connectionSteps = {
        {"CONNECTING", "Resolving server address..."},
        {"CONNECTING", "Establishing TCP/UDP connection..."},
        {"CONNECTING", "Performing TLS handshake..."},
        {"CONNECTING", "Authenticating with server..."},
        {"CONNECTING", "Configuring tunnel interface..."}
    };
    
    for (const auto& [event, info] : connectionSteps) {
        if (shouldStop) {
            return;
        }
        
        handleInternalEvent(event, info);
        handleInternalLog(3, "Connection step: " + info);
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    if (!shouldStop) {
        handleInternalEvent("CONNECTED", "VPN tunnel established successfully");
        handleInternalLog(3, "OpenVPN connection established");
        
        // Keep connection alive
        while (!shouldStop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void OpenVpnClient::handleInternalEvent(const std::string& eventName, const std::string& info) {
    if (eventHandler) {
        eventHandler(eventName, info);
    }
}

void OpenVpnClient::handleInternalLog(int level, const std::string& message) {
    if (logHandler) {
        logHandler(level, message);
    }
}
