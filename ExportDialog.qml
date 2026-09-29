import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Dialog {
    id: root
    modal: true
    anchors.centerIn: Overlay.overlay

    property bool succeeded: false
    property string filePath: ""

    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.color: "#DCDDE1" // 建议微调为浅灰色，更符合扁平化风格
        border.width: 1

    }

    contentItem: ColumnLayout {
        spacing: 30
        implicitWidth: 400
        implicitHeight: 160

        Text {
            text: root.succeeded ? "报表导出成功" : "导出失败"
            font.pixelSize: 20
            font.bold: true
            color: "#2C3E50"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
        }

        Text {
            visible: root.succeeded
            text: {
                var parts = root.filePath.split("/")
                return parts[parts.length - 1]
            }
            font.pixelSize: 14
            color: "#7F8C8D"
            Layout.alignment: Qt.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }

    footer: DialogButtonBox {
        background: Rectangle { color: "transparent" }
        alignment: Qt.AlignHCenter
        spacing: 20
        bottomPadding: 25

        // --- 1. “打开文件夹” 放在左边 ---
        Button {
            id: openFolderBtn
            visible: root.succeeded
            text: "打开文件夹"
            // 改为 ActionRole，避免触发默认的 Accept 行为（如果需要特殊处理）
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole

            contentItem: Text {
                text: openFolderBtn.text
                font.pixelSize: 15
                font.bold: true
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 120
                implicitHeight: 38
                color: openFolderBtn.pressed ? "#1C5980" :
                       (openFolderBtn.hovered ? "#3498DB" : "#2980B9")
                radius: height / 2

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            onClicked: {
                var dir = root.filePath.substring(0, root.filePath.lastIndexOf("/"))
                Qt.openUrlExternally("file:///" + dir)
                root.close()
            }
        }

        // --- 2. “关闭” 放在右边 ---
        Button {
            id: closeBtn
            text: "关闭"
            // AcceptRole 或 RejectRole 放在后面通常会排列在右侧
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

            contentItem: Text {
                text: closeBtn.text
                font.pixelSize: 15
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 120
                implicitHeight: 38
                color: closeBtn.pressed ? "#992D22" :
                       (closeBtn.hovered ? "#E74C3C" : "#C0392B")
                radius: height / 2
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            onClicked: root.close()
        }
    }
}
