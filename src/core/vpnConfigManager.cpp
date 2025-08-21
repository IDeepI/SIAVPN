import std;
#include "vpnConfigManager.h"

VpnConfigManager::VpnConfigManager() {
    profilesDirectory = "vpn_profiles";
    ensureProfilesDirectory();
}

VpnConfigManager::~VpnConfigManager() = default;

std::string VpnConfigManager::loadConfigFromFile(const std::string& configPath) {
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
        throw std::runtime_error("Failed to load config: " + std::string(e.what()));
    }
}

VpnConfigManager::ClientConfig VpnConfigManager::createConfig(const std::string& configContent) {
    ClientConfig config;
    
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

VpnConfigManager::ConfigValidation VpnConfigManager::validateConfig(const ClientConfig& config) {
    ConfigValidation result;
    result.isValid = true;
    result.warnings.clear();
    
    // Basic validation checks for OpenVPN config
    if (config.content.find("remote ") == std::string::npos) {
        result.isValid = false;
        result.errorMessage = "Configuration missing remote server specification";
        return result;
    }
    
    if (config.content.find("client") == std::string::npos) {
        result.isValid = false;
        result.errorMessage = "Configuration not set for client mode";
        return result;
    }
    
    // Check for required certificates/keys
    bool hasAuth = (config.content.find("cert ") != std::string::npos ||
                   config.content.find("<cert>") != std::string::npos ||
                   config.content.find("auth-user-pass") != std::string::npos);
    
    if (!hasAuth) {
        result.isValid = false;
        result.errorMessage = "Configuration missing authentication credentials";
        return result;
    }
    
    // Check for security warnings
    if (config.content.find("cipher none") != std::string::npos) {
        result.warnings.push_back("Warning: No encryption cipher specified");
    }
    
    if (config.content.find("auth none") != std::string::npos) {
        result.warnings.push_back("Warning: No authentication algorithm specified");
    }
    
    if (config.content.find("verify-x509-name") == std::string::npos) {
        result.warnings.push_back("Warning: X.509 name verification not enabled");
    }
    
    return result;
}

void VpnConfigManager::saveProfile(const std::string& name, const std::string& configContent) {
    std::string sanitizedName = sanitizeProfileName(name);
    std::string profilePath = profilesDirectory + "/" + sanitizedName + ".ovpn";
    
    try {
        std::ofstream profileFile(profilePath, std::ios::binary);
        if (!profileFile.is_open()) {
            throw std::runtime_error("Cannot create profile file: " + profilePath);
        }
        
        profileFile << configContent;
        profileFile.close();
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save profile: " + std::string(e.what()));
    }
}

std::string VpnConfigManager::loadProfile(const std::string& name) {
    std::string sanitizedName = sanitizeProfileName(name);
    std::string profilePath = profilesDirectory + "/" + sanitizedName + ".ovpn";
    
    return loadConfigFromFile(profilePath);
}

std::vector<std::string> VpnConfigManager::listProfiles() {
    std::vector<std::string> profiles;
    
    try {
        std::filesystem::directory_iterator dirIter(profilesDirectory);
        
        for (const auto& entry : dirIter) {
            if (entry.is_regular_file() && entry.path().extension() == ".ovpn") {
                std::string filename = entry.path().stem().string();
                profiles.push_back(filename);
            }
        }
        
        std::sort(profiles.begin(), profiles.end());
        
    } catch (const std::exception& e) {
        // Directory might not exist or be accessible
        // Return empty list
    }
    
    return profiles;
}

void VpnConfigManager::deleteProfile(const std::string& name) {
    std::string sanitizedName = sanitizeProfileName(name);
    std::string profilePath = profilesDirectory + "/" + sanitizedName + ".ovpn";
    
    try {
        if (std::filesystem::exists(profilePath)) {
            std::filesystem::remove(profilePath);
        } else {
            throw std::runtime_error("Profile does not exist: " + name);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to delete profile: " + std::string(e.what()));
    }
}

void VpnConfigManager::ensureProfilesDirectory() {
    try {
        if (!std::filesystem::exists(profilesDirectory)) {
            std::filesystem::create_directories(profilesDirectory);
        }
    } catch (const std::exception& e) {
        // If we can't create the directory, profiles won't work
        // but the rest of the functionality should still work
    }
}

std::string VpnConfigManager::sanitizeProfileName(const std::string& name) {
    std::string sanitized = name;
    
    // Remove or replace invalid filename characters
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char& c : sanitized) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }
    
    // Limit length
    if (sanitized.length() > 50) {
        sanitized = sanitized.substr(0, 50);
    }
    
    // Ensure not empty
    if (sanitized.empty()) {
        sanitized = "unnamed_profile";
    }
    
    return sanitized;
}
