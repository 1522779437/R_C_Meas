import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Dialog {
    id: root
    // title: "电缆类型"

    // 定义一个自定义信号，方便把数据传出去
    signal typeSelected(string typeName)

    width: 400
    height: 400
    modal: true
    anchors.centerIn: Overlay.overlay // 确保在窗口正中间

    // 自定义弹窗背景（延续你的工业风）
    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.color: "#2C3E50"
        border.width: 1

        layer.enabled: true
        layer.effect: DropShadow {
            radius: 15
            samples: 20
            color: "#60000000"
            verticalOffset: 5
        }
    }

    contentItem: ColumnLayout {
        spacing: 20

        Text {
            text: "请选择电缆类型："
            font.pixelSize: 18
            font.bold: true
            color: "#2C3E50"
            Layout.alignment: Qt.AlignHCenter
        }

        // 模拟一个选择列表
        ListView {
            id: typeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ["信号电缆", "数字信号电缆", "内屏蔽数字信号电缆"]

            delegate: ItemDelegate {
                width: parent.width
                height: 45 // 适当增加高度，配合大字体更美观

                // 1. 定义文字内容和样式
                contentItem: Text {
                    text: modelData
                    font.pixelSize: 18         // 调大字号
                    font.bold: typeList.currentIndex === index // 选中时加粗（可选）
                    color: typeList.currentIndex === index ? "#2C3E50" : "#57606F"

                    // 核心：设置居中
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 处理点击逻辑
                onClicked: {
                    typeList.currentIndex = index
                }

                // 2. 选中的背景样式
                background: Rectangle {
                    color: typeList.currentIndex === index ? "#D6EAF8" : "transparent"
                    // 增加一个简单的圆角，看起来更精致
                    radius: 4
                }
            }
        }
    }

    // 底部操作按钮
    footer: DialogButtonBox {
        background: Rectangle { color: "transparent" } // 让背景透明，更清爽
        alignment: Qt.AlignHCenter // 按钮组整体居中
        spacing: 40
        bottomPadding: 25

        // --- 取消按钮 ---
        Button {
            id: cancelButton
            text: "取消"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole

            contentItem: Text {
                text: cancelButton.text
                font.pixelSize: 15
                color: "#7F8C8D" // 深灰色文字
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 120
                implicitHeight: 38
                color: cancelButton.hovered ? "#F2F5F7" : "#FFFFFF" // 悬停时稍微变灰
                radius: height / 2 // 完美圆角（胶囊状）
                border.color: "#BDC3C7"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // --- 确定按钮 ---
        Button {
            id: okButton
            text: "确认选择"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

            contentItem: Text {
                text: okButton.text
                font.pixelSize: 15
                font.bold: true
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 120
                implicitHeight: 38
                // 使用更有活力的“科技蓝”
                color: okButton.pressed ? "#1C5980" :
                       (okButton.hovered ? "#3498DB" : "#2980B9")
                radius: height / 2 // 完美圆角

                // 加入一个微妙的阴影效果（可选）
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    radius: 8
                    samples: 16
                    verticalOffset: 2
                    color: okButton.hovered ? "#402980B9" : "#20000000"
                }

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            onClicked: {
                var selectedValue = typeList.model[typeList.currentIndex]
                root.typeSelected(selectedValue)
                root.accept()
            }
        }
    }
}
