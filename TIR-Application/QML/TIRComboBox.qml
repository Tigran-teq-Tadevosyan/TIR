import QtQuick 2.15
import QtQuick.Controls 2.15

ComboBox {
    id: root;
    font.pixelSize: 13;

    //the background of the combobox
    background: Rectangle {
        color: "white";
        border.width: 1;
        border.color: "black";
    }

    // The delegate item that will display individual option of combobox
    delegate: ItemDelegate {
        id: itemDlgt
        width: root.width
        height: root.height;
        padding: 0;

        contentItem: Text {
            id: textItem
            text: root.textRole ? model[root.textRole] : modelData
            color: "black";
            font: root.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            leftPadding: 10

            ToolTip.delay: 500;
            ToolTip.timeout: 5000;
            ToolTip.visible: hovered;
            ToolTip.text: root.textRole ? model[root.textRole] : modelData;
        }

        background: Rectangle {
            id: rectDlgt
            color: itemDlgt.hovered ? "#cccccc" : "white";
            anchors.left: itemDlgt.left
            anchors.leftMargin: 0
            width: itemDlgt.width-2
        }
    }

    //the arrow on the right in the combobox
    indicator: Image {
        width: 10; height: width;
        anchors.right: parent.right;
        anchors.rightMargin: 5;
        anchors.verticalCenter: parent.verticalCenter;
        source: "qrc:/images/down-arrow-primary.png";
        rotation: comboPopup.visible ? 180 : 0;
    }

    // The component that contains the selected option
    contentItem: Text {
            text: root.displayText
            font: root.font
            color: "black";
            verticalAlignment: Text.AlignVCenter;
            horizontalAlignment: Text.AlignLeft;
            elide: Text.ElideRight
            leftPadding: 5
        }


    // The list of elements and their style when the combobox is open
    popup: Popup {
        id: comboPopup
        y: root.height - 1
        width: root.width
        height: contentItem.implicitHeigh
        padding: 1

        contentItem: ListView {
//            id:listView
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            border.width: 1
            border.color: "#007fd4";
        }
    }
}
