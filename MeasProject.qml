import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: testStatus
    width: parent.width
    height: contentColumn.implicitHeight + 50
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    // 状态属性
    // property bool checkDC: false
    // property bool checkIns: false
    // property bool checkCap: false

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        spacing: 30

        // --- 1. 标题 ---
        RowLayout {
            spacing: 12
            Rectangle { width: 4; height: 20; color: "#2C3E50"; radius: 2 }
            Text {
                text: "测试项目配置"
                font.pixelSize: 20; font.bold: true; color: "#2C3E50"
            }
        }

        // --- 2. 项目显示区 ---
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            spacing: 30 // 项目之间的间距

            // ==========================================
            // 1. 直流电阻
            // ==========================================
            Item {
                implicitWidth: 120
                implicitHeight: 110

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15

                    Item {
                        id: diagDC
                        width: 80; height: 50; Layout.alignment: Qt.AlignHCenter
                        property color iconColor: testConfig.checkDC ? "#2980B9" : "#DCDDE1"

                        // 直流符号 (DC)
                        Column {
                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 3
                            Rectangle { width: 30; height: 3; color: diagDC.iconColor; radius: 1 }
                            Row {
                                spacing: 3
                                Rectangle { width: 8; height: 2; color: diagDC.iconColor }
                                Rectangle { width: 8; height: 2; color: diagDC.iconColor }
                                Rectangle { width: 8; height: 2; color: diagDC.iconColor }
                            }
                        }

                        // 电阻体 (Ω)
                        Rectangle {
                            width: 40; height: 16
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 5
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: "transparent"; border.color: diagDC.iconColor; border.width: 2; radius: 2
                            Text { text: "Ω"; anchors.centerIn: parent; font.pixelSize: 12; font.bold: true; color: diagDC.iconColor }

                            // 左右连线
                            Rectangle { width: 10; height: 2; color: diagDC.iconColor; x: -10; y: 7 }
                            Rectangle { width: 10; height: 2; color: diagDC.iconColor; x: 40; y: 7 }
                        }
                        Behavior on iconColor { ColorAnimation { duration: 200 } }
                    }

                    CustomCheckBox {
                        text: "直流电阻"
                        checked: testConfig.checkDC
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
                // 透明遮罩层：点击整个区域触发
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: testConfig.checkDC = !testConfig.checkDC
                }
            }

            // ==========================================
            // 2. 绝缘电阻
            // ==========================================
            Item {
                implicitWidth: 120
                implicitHeight: 110

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15

                    Item {
                        id: diagIns
                        width: 80; height: 50; Layout.alignment: Qt.AlignHCenter
                        property color iconColor: testConfig.checkIns ? "#2980B9" : "#DCDDE1"

                        // MΩ 兆欧电阻体
                        Rectangle {
                            width: 40; height: 16
                            anchors.top: parent.top; anchors.topMargin: 5
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: "transparent"; border.color: diagIns.iconColor; border.width: 2; radius: 2
                            Text { text: "MΩ"; anchors.centerIn: parent; font.pixelSize: 11; font.bold: true; color: diagIns.iconColor }

                            // 左右横线
                            Rectangle { width: 10; height: 2; color: diagIns.iconColor; x: -10; y: 7 }
                            Rectangle { width: 10; height: 2; color: diagIns.iconColor; x: 40; y: 7 }
                        }

                        // 连接到接地的竖线
                        Rectangle { width: 2; height: 10; color: diagIns.iconColor; x: 39; y: 21 }

                        // 国际标准接地符号 (三条递减横线)
                        Column {
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 2
                            Rectangle { width: 16; height: 2; color: diagIns.iconColor; anchors.horizontalCenter: parent.horizontalCenter }
                            Rectangle { width: 10; height: 2; color: diagIns.iconColor; anchors.horizontalCenter: parent.horizontalCenter }
                            Rectangle { width: 4; height: 2; color: diagIns.iconColor; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        Behavior on iconColor { ColorAnimation { duration: 200 } }
                    }

                    CustomCheckBox {
                        text: "绝缘电阻"
                        checked: testConfig.checkIns
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: testConfig.checkIns = !testConfig.checkIns
                }
            }

            // ==========================================
            // 3. 工作电容
            // ==========================================
            Item {
                implicitWidth: 120
                implicitHeight: 110

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15

                    Item {
                        id: diagCap
                        width: 80; height: 50; Layout.alignment: Qt.AlignHCenter
                        property color iconColor: testConfig.checkCap ? "#2980B9" : "#DCDDE1"

                        // 标准电容器符号 --| |--
                        Item {
                            width: 60; height: 30
                            anchors.top: parent.top; anchors.topMargin: 5
                            anchors.horizontalCenter: parent.horizontalCenter

                            // 左侧导线与极板
                            Rectangle { width: 22; height: 2; color: diagCap.iconColor; x: 0; y: 14 }
                            Rectangle { width: 2; height: 22; color: diagCap.iconColor; x: 22; y: 4 }

                            // 右侧极板与导线
                            Rectangle { width: 2; height: 22; color: diagCap.iconColor; x: 36; y: 4 }
                            Rectangle { width: 22; height: 2; color: diagCap.iconColor; x: 38; y: 14 }
                        }

                        // μF 微法单位标识
                        Text {
                            text: "μF"
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: 12; font.bold: true; color: diagCap.iconColor
                        }
                        Behavior on iconColor { ColorAnimation { duration: 200 } }
                    }

                    CustomCheckBox {
                        text: "工作电容"
                        checked: testConfig.checkCap
                        Layout.alignment: Qt.AlignHCenter
                    }

                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: testConfig.checkCap = !testConfig.checkCap
                }
            }

            Item { Layout.fillWidth: true } // 右侧留白弹簧
        }
    }

    // --- 通用自定义复选框组件 (禁用自身点击，全权交由外部 MouseArea 控制) ---
    component CustomCheckBox: CheckBox {
        id: control

        background: Item {}

        // 左侧方框
        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            radius: 4
            border.width: 2
            border.color: control.checked ? "#2980B9" : "#BDC3C7"
            color: "transparent"

            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                width: 12
                height: 12
                anchors.centerIn: parent
                radius: 2
                color: "#2980B9"
                visible: control.checked
            }
        }

        // 内容（用 Row 保证绝对对齐）
        contentItem: Row {
            spacing: control.spacing
            anchors.verticalCenter: parent.verticalCenter

            // 给 indicator 留位置（关键点）
            Item {
                width: control.indicator.width
                height: 1
            }

            Text {
                text: control.text
                font.pixelSize: 16
                font.bold: control.checked
                color: control.checked ? "#2980B9" : "#34495E"
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
