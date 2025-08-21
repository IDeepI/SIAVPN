#pragma once
import std;

class VpnConfigManager {
public:
    struct ConfigValidation {
        bool isValid;
        std::string errorMessage;
        std::vector<std::string> warnings;
    };

    // Configuration structure without OpenVPN dependencies
    struct ClientConfig {
        std::string content;
        std::string compressionMode = "adaptive";
        int tcpQueueLimit = 64;
        std::string server_override;
        std::string port_override;
        std::string proto_override;
        bool allowLocalLan = false;
        bool tunPersist = false;
        bool autologinSessions = false;
        bool disableClientCert = false;
        int sslDebugLevel = 0;
    };

    // Configuration operations
    std::string loadConfigFromFile(const std::string& configPath);
    ClientConfig createConfig(const std::string& configContent);
    ConfigValidation validateConfig(const ClientConfig& config);
    
    // Profile management
    void saveProfile(const std::string& name, const std::string& configContent);
    std::string loadProfile(const std::string& name);
    std::vector<std::string> listProfiles();
    void deleteProfile(const std::string& name);

private:
    std::string profilesDirectory;
    void ensureProfilesDirectory();
    std::string sanitizeProfileName(const std::string& name);
};