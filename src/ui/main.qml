import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "VPN Client"

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: vpnController.status
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        Button {
            text: "Connect"
            onClicked: vpnController.connectVpn("config.ovpn")
        }

        Button {
            text: "Disconnect"
            onClicked: vpnController.disconnectVpn()
        }
    }
}
