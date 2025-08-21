#pragma once
import std;

class VpnSecurityManager {
public:
    VpnSecurityManager();
    ~VpnSecurityManager();

    // Communication control
    void blockCommunication();
    void unblockCommunication();
    void allowCommunicationWithoutVpn();
    bool isCommunicationBlocked() const;

    // Security operations
    void secureCleanup();
    void clearSensitiveData();
    
    // Kill switch functionality
    void enableKillSwitch();
    void disableKillSwitch();
    bool isKillSwitchEnabled() const;

private:
    std::atomic<bool> communicationBlocked{true};
    std::atomic<bool> killSwitchEnabled{false};
    
    void setupBasicFirewallRules();
    void removeFirewallRules();
    void blockAllTraffic();
    void allowVpnTraffic();
    
    // Platform-specific implementations
    #ifdef _WIN32
    void setupWindowsFirewallRules();
    void removeWindowsFirewallRules();
    void blockAllTrafficWindows();
    void allowVpnTrafficWindows();
    #endif
    
    #ifdef __linux__
    void setupLinuxFirewallRules();
    void removeLinuxFirewallRules();
    void blockAllTrafficLinux();
    void allowVpnTrafficLinux();
    #endif
    
    #ifdef __APPLE__
    void setupMacFirewallRules();
    void removeMacFirewallRules();
    void blockAllTrafficMac();
    void allowVpnTrafficMac();
    #endif
};