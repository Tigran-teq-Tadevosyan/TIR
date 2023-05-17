import QtQuick 2.15
import QtQuick.Controls 2.15

import TIR 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("TIR")

    InterfaceModel { id: interfaceModel; }

    Text {
        text: "Interface: ";
        font.pixelSize: 19;
        anchors.top: parent.top;
        anchors.topMargin: 25;
        anchors.rightMargin: 10;
        anchors.right: interfaceComboBox.left;
    }

    TIRComboBox {
        id: interfaceComboBox;
        textRole: "interfaceName";
        valueRole: "interfacePath";
        model: interfaceModel;
        height: 25; width: 195;
        font.pixelSize: 19;

        anchors.top: parent.top;
        anchors.topMargin: 25;
        anchors.horizontalCenter: parent.horizontalCenter;

        onCurrentValueChanged: {
            if(currentValue) {
                TIRBackend.interface = currentValue;
            } else {
                TIRBackend.interface = "";
            }
        }
    }

    Button {
        id: connectButton;
        text: TIRBackend.connected ? "Disconnect" : "Connect";
        anchors.left: interfaceComboBox.right;
        anchors.top: parent.top;
        anchors.topMargin: 25;
        anchors.leftMargin: 10;

        onClicked: {
            TIRBackend.connectDisconnect();
        }
    }

    Button {
        text: "Refresh"
        anchors.left: connectButton.right;
        anchors.top: parent.top;
        anchors.topMargin: 25;
        anchors.leftMargin: 10;

        onClicked: {
            interfaceModel.reset();
        }
    }

    Text {
        visible: TIRBackend.connected;
        text: qsTr("Your MAC address: %1").arg(TIRBackend.hostMAC);
        font.pixelSize: 19;

        anchors.topMargin: 10;
        anchors.top: interfaceComboBox.bottom;
        anchors.horizontalCenter: parent.horizontalCenter;
    }

    property variant stringList: []

    Rectangle {
        border.color: "black";
        border.width: 1;

        width: parent.width - 30;
        height: parent.height - 100;

        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 10;

        anchors.horizontalCenter: parent.horizontalCenter;

        ScrollView {
            id: textView;
            anchors.fill: parent;

            ScrollBar.vertical.policy: ScrollBar.AlwaysOn;

            TextArea {
                id: textBox;
                font.pixelSize: 15;
                text: "";
            }
        }
    }

    Connections {
        target: TIRBackend;
        function onAppendLine(str) {
            textBox.text += str;
            textBox.text += "\n";
            textView.ScrollBar.vertical.position = 1 - textView.ScrollBar.vertical.size;
        }
    }
}
