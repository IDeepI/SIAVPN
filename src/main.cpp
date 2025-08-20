#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ui/vpnController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    VpnController vpnController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("vpnController", &vpnController);
    engine.load(QUrl(QStringLiteral("qrc:/ui/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
