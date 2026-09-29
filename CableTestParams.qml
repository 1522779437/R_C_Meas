import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: paramsRoot
    width: parent.width
    height: mainLayout.implicitHeight + 30
    color: "#F8F9FA" // 浅灰背景区分用户信息区
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // --- 标题 ---
        Text {
            text: "电缆测试参数"
            font.pixelSize: 18
            font.bold: true
            color: "#2C3E50"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            // --- 左侧：下拉选择区 ---
            ColumnLayout {
                spacing: 10
                Layout.preferredWidth: 250

                RowLayout {
                    Text { text: "电缆类型:"; Layout.preferredWidth: 70; font.pixelSize: 14 }
                    ComboBox {
                        model: ["普通信号电缆", "电力电缆", "控制电缆"]
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    Text { text: "电缆芯数:"; Layout.preferredWidth: 70; font.pixelSize: 14 }
                    ComboBox {
                        model: ["48", "24", "12"]
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    Text { text: "长    度:"; Layout.preferredWidth: 70; font.pixelSize: 14 }
                    TextField {
                        text: "39.1m";
                        Layout.fillWidth: true
                        background: Rectangle { border.color: "#CED6E0"; radius: 4 }
                    }
                }
            }

            // --- 中间：测试项目 (带边框的 GroupBox 效果) ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"
                border.color: "#DCDDE1"
                radius: 4

                // “测试项目”标签挂在边框上
                Text {
                    text: " 测试项目 "
                    backgroundColor: "#F8F9FA"
                    font.pixelSize: 13
                    color: "#7F8C8D"
                    x: 15; y: -8 // 向上偏移压在边框上
                    background: Rectangle { color: "#F8F9FA" }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    CheckBox { text: "直流电阻: 自动电流 <=1A"; checked: true }
                    CheckBox { text: "工作电容: 测试频率: 1000Hz" }

                    RowLayout {
                        CheckBox { text: "绝缘电阻: 测试电压: "; checked: true }
                        ComboBox {
                            model: ["500V", "1000V", "2500V"]
                            implicitWidth: 100
                        }
                    }
                }
            }

            // --- 右侧：逻辑表按钮 ---
            Button {
                text: "测试位置逻辑表"
                Layout.preferredHeight: 40
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    color: "#2C3E50"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#DCDDE1" : "#FFFFFF"
                    border.color: "#BDC3C7"
                    radius: 4
                }
            }
        }
    }
}
