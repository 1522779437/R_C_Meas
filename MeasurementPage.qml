import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    background: Rectangle { color: "#E6E9ED" }

    header: Rectangle {
        height: 60; color: "#2C3E50"

        // 返回按钮
        Button {
            text: "<- 返回修改"; anchors.left: parent.left; anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            onClicked: stackView.pop()
        }

        Text {
            text: "第二步：实时电缆检测"; color: "white"
            font.pixelSize: 22; anchors.centerIn: parent
        }
    }

    // 测量核心区域
    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        color: "white"; radius: 8; border.color: "#BDC3C7"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20

            Text {
                text: "正在监测 48 路信号..."; font.pixelSize: 18; font.bold: true
            }

            // 这里将来放置你的 48 根线矩阵布局
            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // ... 之前的网格代码 ...
            }

            Button {
                text: "停止测试"; Layout.alignment: Qt.AlignRight
                palette.buttonText: "red"
            }
        }
    }
}
