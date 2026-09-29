import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: controlButton
    width: parent.width
    // 设置一个固定高度，方便观察垂直居中效果
    height: 180
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    property bool isWorking: controlMgr.isWorking

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // --- 1. 标题部分 (固定在顶部) ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Rectangle { width: 4; height: 20; color: "#2C3E50"; radius: 2 }
            Text {
                text: "控制面板"
                font.pixelSize: 20
                font.bold: true
                color: "#2C3E50"
            }
        }

        // --- 2. 上方占位符 (吃掉多余空间) ---
        Item {
            Layout.fillHeight: true
        }

        // --- 3. 按钮行 (被上下挤到中间) ---
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 30

            // 测量控制区
            RowLayout {
                spacing: 12
                Rectangle {
                    id: measureIndicator
                    width: 6; height: 60; radius: 3
                    color: isWorking ? "#27AE60" : "#C0392B"
                    layer.enabled: true
                    layer.effect: DropShadow {
                        color: measureIndicator.color
                        radius: 8; samples: 16; spread: 0.01
                    }
                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                CustomButton {
                    id: startStopBtn
                    btnText: isWorking ? "暂停测量" : "开始测量"
                    btnColor: isWorking ? "#E67E22" : "#27AE60"
                    hoverColor: isWorking ? "#F39C12" : "#2ECC71"
                    // onClicked: isWorking = !isWorking
                    onClicked:
                    {
                        controlMgr.processStartStop() // 调用接口 1
                    }
                }

                CustomButton {
                    id: endBtn
                    btnText: "结束测量"
                    btnColor: "#C0392B"
                    hoverColor: "#E74C3C"
                    // onClicked: {
                    //     isWorking = false
                    //     console.log("测量彻底结束")
                    // }
                    onClicked: controlMgr.processEnd() // 调用接口 2
                }
            }

            // 报表控制区
            RowLayout {
                spacing: 12

                CustomButton {
                    id: exportBtn
                    btnText: "导出报表"
                    btnColor: "#2980B9"
                    hoverColor: "#3498DB"
                    // onClicked: console.log("导出报表中...")
                    onClicked: controlMgr.processExport(backendModel)
                }
            }
        }

        // --- 4. 下方占位符 (吃掉多余空间) ---
        Item {
            Layout.fillHeight: true
        }
    }

    // --- 通用自定义按钮组件 (干净的扁平化风格) ---
    component CustomButton: Button {
        property string btnText: ""
        property color btnColor: "#2980B9"
        property color hoverColor: "#3498DB"

        id: controlBtn
        Layout.preferredWidth: 120
        Layout.preferredHeight: 80

        background: Rectangle {
            // 背景色逻辑：按下时变深，悬停时变亮
            color: controlBtn.pressed ? Qt.darker(btnColor, 1.2) :
                   (controlBtn.hovered ? hoverColor : btnColor)
            radius: 6

            // --- 干净的关键：去掉阴影，改用细边框 ---
            border.color: Qt.darker(color, 1.1) // 边框比背景略深一点，增加精致感
            border.width: 1

            // 彻底关闭 layer 效果以消除任何模糊感
            layer.enabled: false

            Behavior on color { ColorAnimation { duration: 150 } }
        }

        contentItem: Text {
            text: btnText
            font.pixelSize: 18
            font.bold: true
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            anchors.fill: parent
            // 仅当按钮可用时显示手型
            cursorShape: controlBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            acceptedButtons: Qt.NoButton
        }
    }
}
