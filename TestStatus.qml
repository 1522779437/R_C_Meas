import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: testStatus
    width: parent.width
    // 【修复1】高度完全由内部内容（contentColumn.implicitHeight）决定
    implicitHeight: contentColumn.implicitHeight + 40
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    // 【核心变量】记录当前电缆的芯数
    property int cableCoreCount: 0 // 测试用，可设为 0 观察缺省状态

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        spacing: 5

        // --- 标题区域 ---
        RowLayout {
            spacing: 6
            Rectangle { width: 4; height: 20; color: "#2C3E50"; radius: 2 }
            Text {
                text: "测试状态"
                font.pixelSize: 20; font.bold: true; color: "#2C3E50"
            }
        }

        // --- 缺省提示（未选择电缆时显示） ---
        Text {
            visible: testStatus.cableCoreCount === 0
            text: "请先选择电缆类型以及种类"
            color: "#95A5A6"
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 30
            Layout.bottomMargin: 30
        }

        // --- 动态指示灯区域（选择了电缆才显示） ---
        // 【修复2】移除了 ScrollView，直接使用 RowLayout，保证高度自适应
        RowLayout {
            visible: testStatus.cableCoreCount > 0
            Layout.fillWidth: true
            spacing: 15 // 【修复3】加大左右两端的间距
            Layout.alignment: Qt.AlignTop

            // --- A端 面板 ---
            GroupBox {
                id: leftGroup
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                // 自定义标题：居中、加粗、加大
                label: Text {
                    width: leftGroup.width
                    text: "设备左端"
                    font.pixelSize: 18  // 字体加大
                    font.bold: true     // 字体加黑
                    color: "#2C3E50"
                    horizontalAlignment: Text.AlignHCenter // 文字居中
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: "#F8F9FA"; radius: 6; border.color: "#E2E6EA";
                    y: 10 // 稍微往下移一点，给大标题留出空间
                }

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 10 // 内部内容距离大标题的间距
                    columns: 6
                    columnSpacing: 8
                    rowSpacing: 12

                    Repeater {
                        model: testStatus.cableCoreCount
                        RowLayout {
                            spacing: 4
                            Rectangle {
                                width: 14; height: 14; radius: 6
                                color: {
                                    var board = Math.floor(index / 16)
                                    var bit = 1 << (index % 16)
                                    var active = false
                                    if (board === 0) active = (controlMgr.wireLeftSelection1State & bit) !== 0
                                    else if (board === 1) active = (controlMgr.wireLeftSelection2State & bit) !== 0
                                    else if (board === 2) active = (controlMgr.wireLeftSelection3State & bit) !== 0
                                    return active ? "#E74C3C" : "#BDC3C7"
                                }
                                border.color: "#7F8C8D"
                            }
                            Text {
                                text: "CH" + (index + 1)
                                font.pixelSize: testStatus.cableCoreCount > 24 ? 11 : 13
                                color: "#2C3E50"
                            }
                        }
                    }
                }
            }

            // 中间的视觉分割线
            Rectangle {
                Layout.preferredWidth: 2
                Layout.fillHeight: true
                Layout.topMargin: 40 // 让线稍微低一点，不要穿过标题栏
                color: "#E2E6EA"
            }

            // --- B端 面板 ---
            GroupBox {
                id: rightGroup
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                // 自定义标题：居中、加粗、加大
                label: Text {
                    width: rightGroup.width
                    text: "设备右端"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#2C3E50"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: "#F8F9FA"; radius: 6; border.color: "#E2E6EA"; y: 10
                }

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 10
                    columns: 6
                    columnSpacing: 8
                    rowSpacing: 12

                    Repeater {
                        model: testStatus.cableCoreCount
                        RowLayout {
                            spacing: 4
                            Rectangle {
                                width: 14; height: 14; radius: 6
                                color: {
                                    var board = Math.floor(index / 16)
                                    var bit = 1 << (index % 16)
                                    var active = false
                                    if (board === 0) active = (controlMgr.wireRightSelection1State & bit) !== 0
                                    else if (board === 1) active = (controlMgr.wireRightSelection2State & bit) !== 0
                                    else if (board === 2) active = (controlMgr.wireRightSelection3State & bit) !== 0
                                    return active ? "#E74C3C" : "#BDC3C7"
                                }
                                border.color: "#7F8C8D"
                            }
                            Text {
                                text: "CH" + (index + 1)
                                font.pixelSize: testStatus.cableCoreCount > 24 ? 11 : 13
                                color: "#2C3E50"
                            }
                        }
                    }
                }
            }
        }
    }
}
