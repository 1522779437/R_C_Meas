import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: userInfoRoot
    width: parent.width
    height: contentColumn.implicitHeight + 40
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"

    // 内部组件定义
    component InfoInput: RowLayout {
        property string label: ""
        property alias text: inputField.text
        Layout.fillWidth: true
        spacing: 10

        signal valueUpdated(string newValue)

        Text {
            text: label + " :"
            font.pixelSize: 16 // 稍微减小字号增加精致感
            color: "#57606F"
            font.bold: true
            Layout.preferredWidth: 85 // 统一标签宽度
            horizontalAlignment: Text.AlignRight
        }

        TextField {
            id: inputField
            Layout.fillWidth: true
            implicitWidth: 100
            clip: true
            font.pixelSize: 15
            color: "#2F3542"

            background: Rectangle {
                implicitHeight: 34
                color: inputField.enabled ? "#F1F2F6" : "#F7F8FA"
                border.color: inputField.activeFocus ? "#3498DB" :
                             (inputField.hovered ? "#A4B0BE" : "#CED6E0") // 增加 Hover 颜色
                border.width: inputField.activeFocus ? 2 : 1
                radius: 4

                // 丝滑的颜色过渡动画
                Behavior on border.color { ColorAnimation { duration: 150 } }
            }

            onEditingFinished: {
                parent.valueUpdated(text)
            }
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20 }
        spacing: 20

        // 标题部分
        RowLayout {
            spacing: 10
            Rectangle { width: 4; height: 18; color: "#2C3E50"; radius: 2 }
            Text { text: "信息登记"; font.pixelSize: 20; font.bold: true; color: "#2C3E50" }
        }

        // 使用 GridLayout 一次性解决所有对齐问题
        GridLayout {
            columns: 4 // 每行4个
            columnSpacing: 20
            rowSpacing: 15
            Layout.fillWidth: true

            InfoInput {
                label: "出场编号"
                text: backendModel.outId
                onValueUpdated: (newValue) => backendModel.outId = newValue
            }
            InfoInput {
                label: "规格"
                text: backendModel.spec
                onValueUpdated: (newValue) => backendModel.spec = newValue
            }
            InfoInput {
                label: "外端"
                text: backendModel.outerEnd
                onValueUpdated: (newValue) => backendModel.outerEnd = newValue
            }
            InfoInput {
                label: "长度"
                text: backendModel.length
                onValueUpdated: (newValue) => backendModel.length = newValue
            }

            InfoInput {
                label: "外观"
                text: backendModel.appearance
                onValueUpdated: (newValue) => backendModel.appearance = newValue
            }
            InfoInput {
                label: "是否漏气"
                text: backendModel.leak
                onValueUpdated: (newValue) => backendModel.leak = newValue
            }
            InfoInput {
                label: "自编号"
                text: backendModel.serialNum
                onValueUpdated: (newValue) => backendModel.serialNum = newValue
            }
            InfoInput {
                label: "测试日期"
                text: backendModel.testDate
                onValueUpdated: (newValue) => backendModel.testDate = newValue
            }

            InfoInput {
                label: "测试环境"
                text: backendModel.testEnvironment
                onValueUpdated: (newValue) => backendModel.testEnvironment = newValue
            }
            InfoInput {
                label: "测试地点"
                text: backendModel.testLocation
                onValueUpdated: (newValue) => backendModel.testLocation = newValue
            }
            InfoInput {
                label: "技术负责人"
                text: backendModel.techDirector
                onValueUpdated: (newValue) => backendModel.techDirector = newValue
            }
            InfoInput {
                label: "监理单位"
                text: backendModel.superUnit
                onValueUpdated: (newValue) => backendModel.superUnit = newValue
            }
        }
    }
}
