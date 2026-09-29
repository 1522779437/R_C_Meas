import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {  // 直接用 Page 开头
    id: userInfoPageRoot

    background: Rectangle { color: "#E6E9ED" }

    header: Rectangle {
        height: 60
        color: "#2C3E50"
        Text {
            text: "第一步：录入用户信息"
            color: "white"
            font.pixelSize: 22
            anchors.centerIn: parent
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 40
        contentWidth: width

        ColumnLayout {
            width: parent.width
            spacing: 30

            // 只要 UserInfoPanel.qml 在 CMake 里，这里就能直接用
            UserInfoPanel {
                Layout.fillWidth: true
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 300
                implicitHeight: 60

                text: "确认信息并开始测量 >>"
                // 这里的 stackView 是在 Main.qml 里定义的 id
                onClicked: stackView.push("MeasurementPage.qml")
            }
        }
    }
}
