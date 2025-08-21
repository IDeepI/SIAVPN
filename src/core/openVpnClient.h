#pragma once
import std;

class OpenVpnClient {
public:
    OpenVpnClient();
    ~OpenVpnClient();

    // Core OpenVPN operations
    bool startConnection(const std::string& configContent);
    void stopConnection();
    void pauseConnection();
    void resumeConnection();
    void reconnectConnection();
    
    // Status
    bool isConnected() const;
    std::string getLastError() const;

    // Event subscription
    void setEventHandler(std::function<void(const std::string&, const std::string&)> handler);
    void setLogHandler(std::function<void(int, const std::string&)> handler);

private:
    // Internal OpenVPN client implementation
    void simulateConnectionProcess();
    void handleInternalEvent(const std::string& eventName, const std::string& info);
    void handleInternalLog(int level, const std::string& message);

    std::function<void(const std::string&, const std::string&)> eventHandler;
    std::function<void(int, const std::string&)> logHandler;
    
    std::atomic<bool> isRunning{false};
    std::atomic<bool> shouldStop{false};
    std::thread connectionThread;
    std::string lastError;
    std::string currentConfig;
    
    mutable std::mutex stateMutex;
};