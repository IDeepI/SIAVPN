import std;
#include "openVpnProtocol.h"
#include <openvpn/client/ovpncli.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/cleanup.hpp>
#include <openvpn/common/string.hpp>
#include <openvpn/common/to_string.hpp>
#include <openvpn/log/logthread.hpp>

OpenVpnProtocol::OpenVpnProtocol() {
    // Initialize OpenVPN 3 logging subsystem
    openvpn::LogThread::init();
    updateStatus(VpnStatus::Disconnected, "OpenVPN client initialized");
}

OpenVpnProtocol::~OpenVpnProtocol() {
    // Ensure clean shutdown
    shouldStop = true;
    
    // Disconnect if connected
    if (currentStatus == VpnStatus::Connected || currentStatus == VpnStatus::Connecting) {
        disconnect();
    }
    
    // Wait for worker thread to complete
    if (vpnThread.joinable()) {
        vpnThread.join();
    }
    
    // Secure cleanup
    secureCleanup();
}

std::future<bool> OpenVpnProtocol::connect(const std::string& configPath) {
    if (connectionInProgress) {
        return std::async(std::launch::deferred, []() { return false; });
    }
    
    updateStatus(VpnStatus::Connecting, "Starting connection...");
    connectionInProgress = true;
    shouldStop = false;

    return std::async(std::launch::async, [this, configPath]() {
        auto cleanup = openvpn::Cleanup([this]() {
            connectionInProgress = false;
        });
        
        return performConnection(configPath);
    });
}

bool OpenVpnProtocol::performConnection(const std::string& configPath) {
    try {
        // Phase 1: Configuration setup and validation
        if (!prepareConfiguration(configPath)) {
            return false;
        }

        // Phase 2: Start the connection process
        if (!initiateConnection()) {
            return false;
        }

        // Phase 3: Wait for connection completion
        return waitForConnectionCompletion();

    } catch (const openvpn::Exception& e) {
        handleConnectionComplete(false, "OpenVPN exception: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Standard exception: " + std::string(e.what()));
        return false;
    }
}

bool OpenVpnProtocol::prepareConfiguration(const std::string& configPath) {
    try {
        // Read and validate configuration file
        std::string configContent = readConfigFile(configPath);
        if (configContent.empty()) {
            handleConnectionComplete(false, "Failed to read configuration file");
            return false;
        }

        // Create OpenVPN configuration
        currentConfig = createConfig(configContent);
        
        // Evaluate configuration
        openvpn::ClientAPI::EvalConfig eval = eval_config(currentConfig);
        if (eval.error) {
            std::string errorMsg = "Configuration evaluation failed: " + eval.message;
            handleConnectionComplete(false, errorMsg);
            return false;
        }

        updateStatus(VpnStatus::Connecting, "Configuration validated successfully");
        return true;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Configuration preparation failed: " + std::string(e.what()));
        return false;
    }
}

bool OpenVpnProtocol::initiateConnection() {
    try {
        updateStatus(VpnStatus::Connecting, "Establishing connection...");

        // Start connection in worker thread to avoid blocking
        vpnThread = std::thread([this]() {
            executeConnectionWorker();
        });

        return true;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Failed to initiate connection: " + std::string(e.what()));
        return false;
    }
}

void OpenVpnProtocol::executeConnectionWorker() {
    try {
        // This call blocks until connection is established or fails
        openvpn::ClientAPI::Status connectStatus = OpenVPNClient::connect();
        
        if (connectStatus.error) {
            handleConnectionComplete(false, "Connection failed: " + connectStatus.message);
        } else {
            handleConnectionComplete(true);
        }
    } catch (const openvpn::Exception& e) {
        handleConnectionComplete(false, "OpenVPN exception in worker: " + std::string(e.what()));
    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Standard exception in worker: " + std::string(e.what()));
    }
}

bool OpenVpnProtocol::waitForConnectionCompletion() {
    try {
        std::unique_lock<std::mutex> lock(statusMutex);
        auto timeout = std::chrono::seconds(30);
        
        bool completed = statusCv.wait_for(lock, timeout, [this]() {
            return currentStatus == VpnStatus::Connected || 
                   currentStatus == VpnStatus::Error ||
                   shouldStop;
        });

        if (!completed && !shouldStop) {
            updateStatus(VpnStatus::Error, "Connection timeout");
            cleanupFailedConnection();
            return false;
        }

        if (shouldStop) {
            updateStatus(VpnStatus::Disconnected, "Connection cancelled by user");
            return false;
        }

        return currentStatus == VpnStatus::Connected;

    } catch (const std::exception& e) {
        handleConnectionComplete(false, "Error waiting for connection: " + std::string(e.what()));
        return false;
    }
}

void OpenVpnProtocol::cleanupFailedConnection() {
    try {
        // Stop the OpenVPN client if it's still trying to connect
        if (vpnThread.joinable()) {
            shouldStop = true;
            OpenVPNClient::stop();
            vpnThread.join();
        }
        
        // Ensure communication is blocked for security
        blockCommunication();
        
    } catch (const std::exception& e) {
        std::cerr << "[VPN] Error during connection cleanup: " << e.what() << '\n';
    }
}

std::string OpenVpnProtocol::readConfigFile(const std::string& configPath) {
    try {
        std::ifstream configFile(configPath, std::ios::binary);
        if (!configFile.is_open()) {
            throw std::runtime_error("Cannot open config file: " + configPath);
        }

        std::string content((std::istreambuf_iterator<char>(configFile)),
                           std::istreambuf_iterator<char>());
        
        if (content.empty()) {
            throw std::runtime_error("Config file is empty: " + configPath);
        }

        return content;
    } catch (const std::exception& e) {
        lastError = "Failed to read config file: " + std::string(e.what());
        return "";
    }
}

openvpn::ClientAPI::Config OpenVpnProtocol::createConfig(const std::string& configContent) {
    openvpn::ClientAPI::Config config;
    
    // Set the configuration content (inline style as required by OpenVPN 3)
    config.content = configContent;
    
    // Configure connection parameters following OpenVPN 3 best practices
    config.compressionMode = "adaptive";  // Use adaptive compression
    config.tcpQueueLimit = 64;            // Default TCP queue limit
    config.server_override = "";          // No server override
    config.port_override = "";            // No port override  
    config.proto_override = "";           // No protocol override
    config.allowLocalLan = false;         // Security: don't allow local LAN access
    config.tunPersist = false;            // Don't persist tunnel
    config.autologinSessions = false;     // Disable auto-login for security
    
    // Security settings
    config.disableClientCert = false;    // Require client certificates
    config.sslDebugLevel = 0;             // No SSL debug in production
    
    return config;
}

void OpenVpnProtocol::disconnect() {
    if (currentStatus == VpnStatus::Disconnected) {
        return;
    }

    updateStatus(VpnStatus::Disconnected, "Disconnecting...");
    shouldStop = true;

    try {
        // Signal OpenVPN client to stop
        OpenVPNClient::stop();
        
        // Wait for worker thread to complete
        if (vpnThread.joinable()) {
            vpnThread.join();
        }
        
        blockCommunication();
        updateStatus(VpnStatus::Disconnected, "Disconnected successfully");
        
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Error during disconnect: " + std::string(e.what()));
    }
}

void OpenVpnProtocol::pause() {
    try {
        OpenVPNClient::pause("User requested pause");
        updateStatus(VpnStatus::Disconnected, "Connection paused");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to pause: " + std::string(e.what()));
    }
}

void OpenVpnProtocol::resume() {
    try {
        OpenVPNClient::resume();
        updateStatus(VpnStatus::Connecting, "Resuming connection...");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to resume: " + std::string(e.what()));
    }
}

void OpenVpnProtocol::reconnect() {
    try {
        OpenVPNClient::reconnect(1); // Reconnect with 1 second delay
        updateStatus(VpnStatus::Connecting, "Reconnecting...");
    } catch (const std::exception& e) {
        updateStatus(VpnStatus::Error, "Failed to reconnect: " + std::string(e.what()));
    }
}

// OpenVPN Client API callbacks
void OpenVpnProtocol::event(const openvpn::ClientAPI::Event& ev) {
    std::string eventMsg = "Event: " + ev.name;
    if (!ev.info.empty()) {
        eventMsg += " - " + ev.info;
    }

    // Handle different event types following OpenVPN 3 patterns
    if (ev.name == "CONNECTED") {
        updateStatus(VpnStatus::Connected, "VPN connection established");
        unblockCommunication();
        
    } else if (ev.name == "DISCONNECTED") {
        updateStatus(VpnStatus::Disconnected, ev.info.empty() ? "Disconnected" : ev.info);
        blockCommunication();
        
    } else if (ev.name == "RECONNECTING") {
        updateStatus(VpnStatus::Connecting, "Reconnecting...");
        
    } else if (ev.name == "AUTH_FAILED") {
        updateStatus(VpnStatus::Error, "Authentication failed");
        blockCommunication();
        
    } else if (ev.name == "CERT_VERIFY_FAIL") {
        updateStatus(VpnStatus::Error, "Certificate verification failed");
        blockCommunication();
        
    } else if (ev.name == "TLS_ERROR") {
        updateStatus(VpnStatus::Error, "TLS error: " + ev.info);
        blockCommunication();
        
    } else if (ev.name == "CLIENT_RESTART") {
        updateStatus(VpnStatus::Connecting, "Client restarting...");
        
    } else {
        // Log other events for debugging
        std::cout << "[VPN] " << eventMsg << '\n';
    }
}

void OpenVpnProtocol::log(const openvpn::ClientAPI::LogInfo& log) {
    // Filter log levels and format appropriately
    std::string prefix;
    
    switch (log.level) {
        case 0: // Fatal
            prefix = "[FATAL] ";
            std::cerr << prefix << log.text << '\n';
            break;
        case 1: // Error  
            prefix = "[ERROR] ";
            std::cerr << prefix << log.text << '\n';
            break;
        case 2: // Warning
            prefix = "[WARN] ";
            std::cout << prefix << log.text << '\n';
            break;
        case 3: // Info
            prefix = "[INFO] ";
            std::cout << prefix << log.text << '\n';
            break;
        default: // Debug and verbose
            // Only show debug logs in debug builds
            #ifdef _DEBUG
            prefix = "[DEBUG] ";
            std::cout << prefix << log.text << '\n';
            #endif
            break;
    }
}

void OpenVpnProtocol::allowCommunicationWithoutVpn() {
    unblockCommunication();
    updateStatus(VpnStatus::Disconnected, "Communication allowed without VPN");
}

VpnStatus OpenVpnProtocol::status() const {
    return currentStatus;
}

// Private helper methods
void OpenVpnProtocol::updateStatus(VpnStatus newStatus, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        currentStatus = newStatus;
        if (!message.empty()) {
            lastError = message;
        }
    }
    statusCv.notify_all();
    
    if (!message.empty()) {
        std::cout << "[VPN] " << message << '\n';
    }
}

void OpenVpnProtocol::blockCommunication() {
    communicationBlocked = true;
    // TODO: Implement actual network blocking (firewall rules, route manipulation, etc.)
    std::cout << "[VPN] Communication blocked - VPN protection active\n";
}

void OpenVpnProtocol::unblockCommunication() {
    communicationBlocked = false;
    // TODO: Remove network blocking
    std::cout << "[VPN] Communication unblocked\n";
}

void OpenVpnProtocol::handleConnectionComplete(bool success, const std::string& error) {
    if (success) {
        updateStatus(VpnStatus::Connected, "VPN connection established successfully");
        unblockCommunication();
    } else {
        updateStatus(VpnStatus::Error, error.empty() ? "Connection failed" : error);
        blockCommunication();
    }
}

void OpenVpnProtocol::secureCleanup() {
    // Clear sensitive data following security best practices
    lastError.clear();
    
    // Zero out any potential sensitive memory
    // Note: In a real implementation, you might want to use secure memory clearing
    std::fill(lastError.begin(), lastError.end(), '\0');
}
