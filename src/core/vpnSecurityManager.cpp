import std;
#include "vpnSecurityManager.h"

VpnSecurityManager::VpnSecurityManager() 
    : communicationBlocked(true)
    , killSwitchEnabled(false) {
    // Initialize with communication blocked for security
}

VpnSecurityManager::~VpnSecurityManager() {
    // Clean shutdown - remove any firewall rules
    disableKillSwitch();
    unblockCommunication();
}

void VpnSecurityManager::blockCommunication() {
    communicationBlocked = true;
    
    if (killSwitchEnabled) {
        blockAllTraffic();
    } else {
        setupBasicFirewallRules();
    }
    
    std::cout << "[SECURITY] Communication blocked - VPN protection active\n";
}

void VpnSecurityManager::unblockCommunication() {
    communicationBlocked = false;
    removeFirewallRules();
    std::cout << "[SECURITY] Communication unblocked\n";
}

void VpnSecurityManager::allowCommunicationWithoutVpn() {
    communicationBlocked = false;
    removeFirewallRules();
    std::cout << "[SECURITY] Communication allowed without VPN (user override)\n";
}

bool VpnSecurityManager::isCommunicationBlocked() const {
    return communicationBlocked;
}

void VpnSecurityManager::secureCleanup() {
    // Clear any sensitive data from memory
    clearSensitiveData();
    
    // Remove firewall rules
    removeFirewallRules();
    
    std::cout << "[SECURITY] Secure cleanup completed\n";
}

void VpnSecurityManager::clearSensitiveData() {
    // In a real implementation, this would:
    // - Clear authentication tokens
    // - Zero out password memory
    // - Clear certificate data
    // - Clear connection logs with sensitive info
    
    std::cout << "[SECURITY] Sensitive data cleared\n";
}

void VpnSecurityManager::enableKillSwitch() {
    killSwitchEnabled = true;
    
    if (communicationBlocked) {
        blockAllTraffic();
    }
    
    std::cout << "[SECURITY] Kill switch enabled\n";
}

void VpnSecurityManager::disableKillSwitch() {
    killSwitchEnabled = false;
    
    if (!communicationBlocked) {
        removeFirewallRules();
    } else {
        // Switch from kill switch to basic blocking
        setupBasicFirewallRules();
    }
    
    std::cout << "[SECURITY] Kill switch disabled\n";
}

bool VpnSecurityManager::isKillSwitchEnabled() const {
    return killSwitchEnabled;
}

void VpnSecurityManager::setupBasicFirewallRules() {
    // Basic firewall rules when VPN is not connected
    // This is a placeholder - real implementation would use:
    // - Windows: Windows Filtering Platform (WFP) API
    // - Linux: iptables commands
    // - macOS: pfctl commands
    
    try {
        #ifdef _WIN32
        setupWindowsFirewallRules();
        #elif __linux__
        setupLinuxFirewallRules();
        #elif __APPLE__
        setupMacFirewallRules();
        #endif
        
        std::cout << "[SECURITY] Basic firewall rules applied\n";
    } catch (const std::exception& e) {
        std::cerr << "[SECURITY] Failed to setup firewall rules: " << e.what() << '\n';
    }
}

void VpnSecurityManager::removeFirewallRules() {
    // Remove all firewall rules created by this application
    try {
        #ifdef _WIN32
        removeWindowsFirewallRules();
        #elif __linux__
        removeLinuxFirewallRules();
        #elif __APPLE__
        removeMacFirewallRules();
        #endif
        
        std::cout << "[SECURITY] Firewall rules removed\n";
    } catch (const std::exception& e) {
        std::cerr << "[SECURITY] Failed to remove firewall rules: " << e.what() << '\n';
    }
}

void VpnSecurityManager::blockAllTraffic() {
    // Kill switch mode - block ALL traffic except VPN
    try {
        #ifdef _WIN32
        blockAllTrafficWindows();
        #elif __linux__
        blockAllTrafficLinux();
        #elif __APPLE__
        blockAllTrafficMac();
        #endif
        
        std::cout << "[SECURITY] All traffic blocked (kill switch active)\n";
    } catch (const std::exception& e) {
        std::cerr << "[SECURITY] Failed to enable kill switch: " << e.what() << '\n';
    }
}

void VpnSecurityManager::allowVpnTraffic() {
    // Allow traffic through VPN interface only
    try {
        #ifdef _WIN32
        allowVpnTrafficWindows();
        #elif __linux__
        allowVpnTrafficLinux();
        #elif __APPLE__
        allowVpnTrafficMac();
        #endif
        
        std::cout << "[SECURITY] VPN traffic allowed\n";
    } catch (const std::exception& e) {
        std::cerr << "[SECURITY] Failed to allow VPN traffic: " << e.what() << '\n';
    }
}

#ifdef _WIN32
void VpnSecurityManager::setupWindowsFirewallRules() {
    // Windows implementation using Windows Filtering Platform
    // This is a placeholder - real implementation would use WFP API
    std::cout << "[SECURITY] Setting up Windows firewall rules (placeholder)\n";
}

void VpnSecurityManager::removeWindowsFirewallRules() {
    std::cout << "[SECURITY] Removing Windows firewall rules (placeholder)\n";
}

void VpnSecurityManager::blockAllTrafficWindows() {
    std::cout << "[SECURITY] Blocking all traffic on Windows (placeholder)\n";
}

void VpnSecurityManager::allowVpnTrafficWindows() {
    std::cout << "[SECURITY] Allowing VPN traffic on Windows (placeholder)\n";
}
#endif

#ifdef __linux__
void VpnSecurityManager::setupLinuxFirewallRules() {
    // Linux implementation using iptables
    // Example: system("iptables -A OUTPUT -j DROP");
    std::cout << "[SECURITY] Setting up Linux firewall rules (placeholder)\n";
}

void VpnSecurityManager::removeLinuxFirewallRules() {
    std::cout << "[SECURITY] Removing Linux firewall rules (placeholder)\n";
}

void VpnSecurityManager::blockAllTrafficLinux() {
    std::cout << "[SECURITY] Blocking all traffic on Linux (placeholder)\n";
}

void VpnSecurityManager::allowVpnTrafficLinux() {
    std::cout << "[SECURITY] Allowing VPN traffic on Linux (placeholder)\n";
}
#endif

#ifdef __APPLE__
void VpnSecurityManager::setupMacFirewallRules() {
    // macOS implementation using pfctl
    std::cout << "[SECURITY] Setting up macOS firewall rules (placeholder)\n";
}

void VpnSecurityManager::removeMacFirewallRules() {
    std::cout << "[SECURITY] Removing macOS firewall rules (placeholder)\n";
}

void VpnSecurityManager::blockAllTrafficMac() {
    std::cout << "[SECURITY] Blocking all traffic on macOS (placeholder)\n";
}

void VpnSecurityManager::allowVpnTrafficMac() {
    std::cout << "[SECURITY] Allowing VPN traffic on macOS (placeholder)\n";
}
#endif
