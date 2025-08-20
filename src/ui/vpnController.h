#pragma once
#include <QObject>
#include "openVpnProtocol.h"

class VpnController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit VpnController(QObject* parent = nullptr);

    Q_INVOKABLE void connectVpn(const QString& configPath);
    Q_INVOKABLE void disconnectVpn();
    QString status() const;

signals:
    void statusChanged();

private:
    OpenVpnProtocol vpn;
};
