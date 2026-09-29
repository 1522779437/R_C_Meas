import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.folderlistmodel

Dialog {
    id: root
    // title: "具体规格选择"

    signal fileSelected(string fileName, string fileUrl)

    width: 600
    height: 600
    modal: true
    anchors.centerIn: Overlay.overlay

    property string folderPath: ""

    FolderListModel {
        id: folderModel
        folder: root.folderPath
        nameFilters: ["*.png", "*.jpg"]
        // --- 排序逻辑：按名称从小到大排序 ---
        sortField: FolderListModel.Name
        sortCaseSensitive: false
    }

    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.color: "#2C3E50"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 15
        width: parent.width

        Text {
            text: "请选择电缆种类："
            font.pixelSize: 22; font.bold: true; color: "#2C3E50"
            Layout.topMargin: 15; Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        GridView {
            id: fileGridView
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 10
            clip: true
            cellWidth: width / 3  // 强制三列
            cellHeight: cellWidth + 30

            model: folderModel
            currentIndex: -1

            delegate: Item {
                width: fileGridView.cellWidth
                height: fileGridView.cellHeight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileGridView.currentIndex = index
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 10
                    color: fileGridView.currentIndex === index ? "#D6EAF8" : "#F8F9FA"
                    border.color: fileGridView.currentIndex === index ? "#2980B9" : "#DCDDE1"
                    border.width: fileGridView.currentIndex === index ? 3 : 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 15; spacing: 10

                        Item {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            Image {
                                anchors.fill: parent
                                source: fileURL
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        // --- 文件名处理逻辑 ---
                        Text {
                            // 使用 split('.')[0] 去掉后缀名
                            text: fileName.split('.')[0]
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 16  // 稍微大一点
                            font.bold: fileGridView.currentIndex === index
                            color: fileGridView.currentIndex === index ? "#2980B9" : "#34495E"
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        background: Rectangle { color: "transparent" }
        alignment: Qt.AlignHCenter; spacing: 50; bottomPadding: 25

        Button {
            id: cancelButton
            text: "返回"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            background: Rectangle {
                implicitWidth: 120; implicitHeight: 45
                color: cancelButton.hovered ? "#F2F5F7" : "#FFFFFF"
                radius: 22; border.color: "#BDC3C7"
            }
            contentItem: Text {
                text: cancelButton.text; color: "#7F8C8D"; font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            id: okButton
            text: "确认选择"
            enabled: fileGridView.currentIndex !== -1
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            background: Rectangle {
                implicitWidth: 120; implicitHeight: 45
                color: !okButton.enabled ? "#BDC3C7" :
                       (okButton.pressed ? "#1C5980" : (okButton.hovered ? "#3498DB" : "#2980B9"))
                radius: 22
            }
            contentItem: Text {
                text: okButton.text; color: "white"; font.bold: true; font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                var currentFileName = folderModel.get(fileGridView.currentIndex, "fileName")
                var currentFileUrl = folderModel.get(fileGridView.currentIndex, "fileURL")
                root.fileSelected(currentFileName, currentFileUrl)
                root.accept()
            }
        }
    }
}
