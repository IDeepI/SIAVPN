import std;
#include "vpnController.h"

VpnController::VpnController(QObject* parent) : QObject(parent) {}

void VpnController::connectVpn(const QString& configPath) {
    auto result = vpn.connect(configPath.toStdString());
    // not blocking, just fire async
    result.wait(); // you can refine with signals instead of blocking here
    emit statusChanged();
}

void VpnController::disconnectVpn() {
    vpn.disconnect();
    emit statusChanged();
}

void VpnController::allowCommunicationWithoutVpn() {
    vpn.allowCommunicationWithoutVpn();
    emit statusChanged();
}

QString VpnController::status() const {
    switch (vpn.status()) {
        case VpnStatus::Disconnected: return "Disconnected";
        case VpnStatus::Connecting:   return "Connecting...";
        case VpnStatus::Connected:    return "Connected";
        case VpnStatus::Error:        return "Error";
    }
    return "Unknown";
}
