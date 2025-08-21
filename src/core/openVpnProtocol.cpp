
import std;
#include "openVpnProtocol.h"
#include <openvpn/client/cliconnect.hpp>
#include <openvpn/client/clilife.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/log/logthread.hpp>

// Helper to block/unblock communication
static std::atomic<bool> communicationBlocked{false};
static std::mutex commMutex;
static std::condition_variable commCv;

void blockCommunication() {
    communicationBlocked = true;
    // TODO: Implement actual network block logic (e.g., firewall rules)
    std::cout << "[VPN] Communication blocked due to VPN disconnect.\n";
}

void unblockCommunication() {
    communicationBlocked = false;
    // TODO: Remove network block logic
    std::cout << "[VPN] Communication unblocked by user action.\n";
}

OpenVpnProtocol::OpenVpnProtocol() : ovpnClient(nullptr) {
    // Initialize OpenVPN3 logging
    openvpn::LogThread::init();
}

OpenVpnProtocol::~OpenVpnProtocol() {
    disconnect();
    if (vpnThread.joinable())
        vpnThread.join();
    
    std::lock_guard<std::mutex> lock(clientMutex);
    ovpnClient.reset();
}

std::future<bool> OpenVpnProtocol::connect(const std::string& configPath) {
    currentStatus = VpnStatus::Connecting;

    return std::async(std::launch::async, [this, configPath]() {
        return performConnection(configPath);
    });
}

bool OpenVpnProtocol::performConnection(const std::string& configPath) {
    try {
        std::cout << "[VPN] Starting OpenVPN3 using config: " << configPath << '\n';

        auto configContent = readConfigFile(configPath);
        if (!configContent.has_value()) {
            return handleConnectionError("Failed to read config file", VpnStatus::Error);
        }

        if (!initializeClient(*configContent)) {
            return handleConnectionError("Failed to initialize OpenVPN3 client", VpnStatus::Error);
        }

        if (!waitForConnection()) {
            return handleConnectionError("Connection timeout or failed", VpnStatus::Error);
        }

        startMonitoringThread();
        std::cout << "[VPN] OpenVPN3 connected successfully.\n";
        return true;

    } catch (const openvpn::Exception& e) {
        return handleConnectionError("OpenVPN3 exception: " + std::string(e.what()), VpnStatus::Error);
    } catch (const std::exception& e) {
        return handleConnectionError("Standard exception: " + std::string(e.what()), VpnStatus::Error);
    }
}

std::optional<std::string> OpenVpnProtocol::readConfigFile(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "[VPN] Failed to open config file: " << configPath << '\n';
        return std::nullopt;
    }

    std::string configContent((std::istreambuf_iterator<char>(configFile)),
                             std::istreambuf_iterator<char>());
    configFile.close();
    return configContent;
}

bool OpenVpnProtocol::initializeClient(const std::string& configContent) {
    std::lock_guard<std::mutex> lock(clientMutex);
    
    // Create OpenVPN3 client
    ovpnClient = std::make_unique<openvpn::ClientConnect>();
    
    // Set up connection parameters
    auto config = createClientConfig(configContent);
    setupEventCallbacks();
    
    // Start the connection
    ovpnClient->connect(config);
    return true;
}

openvpn::ClientConnect::Config OpenVpnProtocol::createClientConfig(const std::string& configContent) {
    openvpn::ClientConnect::Config config;
    config.content = configContent;
    config.server_override = "";
    config.port_override = "";
    config.proto_override = "";
    config.compress_mode = openvpn::CompressionMode::COMP_AUTO;
    config.tcp_queue_limit = 64;
    config.socket_protect = false;
    return config;
}

void OpenVpnProtocol::setupEventCallbacks() {
    ovpnClient->set_event_callback([this](const openvpn::ClientConnect::Event& event) {
        handleClientEvent(event);
    });
}

void OpenVpnProtocol::handleClientEvent(const openvpn::ClientConnect::Event& event) {
    switch (event.type) {
        case openvpn::ClientConnect::Event::CONNECTED:
            onConnectionStateChange(true);
            break;
        case openvpn::ClientConnect::Event::DISCONNECTED:
            onConnectionStateChange(false, event.info);
            break;
        case openvpn::ClientConnect::Event::RECONNECTING:
            currentStatus = VpnStatus::Connecting;
            break;
        case openvpn::ClientConnect::Event::AUTH_FAILED:
            onConnectionStateChange(false, "Authentication failed");
            break;
        default:
            break;
    }
}

bool OpenVpnProtocol::waitForConnection() {
    constexpr auto timeout = std::chrono::seconds(30);
    constexpr auto pollInterval = std::chrono::milliseconds(100);
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (currentStatus == VpnStatus::Connecting) {
        std::this_thread::sleep_for(pollInterval);
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        
        if (elapsed > timeout) {
            std::cerr << "[VPN] Connection timeout\n";
            return false;
        }
    }
    
    return currentStatus == VpnStatus::Connected;
}

void OpenVpnProtocol::startMonitoringThread() {
    vpnThread = std::thread([this]() {
        monitorConnection();
    });
}

void OpenVpnProtocol::monitorConnection() {
    constexpr auto monitorInterval = std::chrono::seconds(1);
    
    while (currentStatus == VpnStatus::Connected) {
        std::this_thread::sleep_for(monitorInterval);
        
        std::lock_guard<std::mutex> lock(clientMutex);
        if (ovpnClient && !ovpnClient->is_connected()) {
            std::cout << "[VPN] OpenVPN3 connection lost!\n";
            currentStatus = VpnStatus::Disconnected;
            blockCommunication();
            break;
        }
    }
}

bool OpenVpnProtocol::handleConnectionError(const std::string& errorMessage, VpnStatus status) {
    std::cerr << "[VPN] " << errorMessage << '\n';
    currentStatus = status;
    blockCommunication();
    return false;
}

void OpenVpnProtocol::disconnect() {
    if (currentStatus == VpnStatus::Connected ||
        currentStatus == VpnStatus::Connecting) {
        std::cout << "[VPN] Disconnecting OpenVPN3...\n";
        
        std::lock_guard<std::mutex> lock(clientMutex);
        if (ovpnClient) {
            ovpnClient->disconnect();
        }
        
        currentStatus = VpnStatus::Disconnected;
        blockCommunication();
    }
}

void OpenVpnProtocol::onConnectionStateChange(bool connected, const std::string& error) {
    if (connected) {
        currentStatus = VpnStatus::Connected;
        std::cout << "[VPN] OpenVPN3 connection established.\n";
    } else {
        currentStatus = VpnStatus::Disconnected;
        if (!error.empty()) {
            std::cerr << "[VPN] OpenVPN3 connection failed/lost: " << error << '\n';
        }
        blockCommunication();
    }
}

void OpenVpnProtocol::allowCommunicationWithoutVpn() {
    unblockCommunication();
}

VpnStatus OpenVpnProtocol::status() const {
    return currentStatus;
}
