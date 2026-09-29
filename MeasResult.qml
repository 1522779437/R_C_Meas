import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: testStatus
    width: parent.width
    // 高度改为自适应或给定一个固定高度容纳表格
    height: 600
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    // 定义列宽，方便统一管理和对齐 (总宽度需根据实际屏幕调整)
    // 【关键2】定义百分比权重
    // 确保这 11 个系数相加等于 1.0 (或者略小于 1.0 留出边框余量)
    readonly property var colRatios: [
        0.08, // 位置
        0.10, // 直阻
        0.12, // 直阻换算
        0.09, // 电阻不平衡
        0.09, // 工作电容
        0.12, // 电容换算
        0.08, // 绝缘-线间
        0.08, // 绝缘-换算
        0.08, // 绝缘-对地
        0.08, // 绝缘-换算
        0.08  // 重测
    ]

    // 根据总宽度动态计算每一列的像素宽度
    property var colWidths: [
        (width - 40) * colRatios[0],
        (width - 40) * colRatios[1],
        (width - 40) * colRatios[2],
        (width - 40) * colRatios[3],
        (width - 40) * colRatios[4],
        (width - 40) * colRatios[5],
        (width - 40) * colRatios[6],
        (width - 40) * colRatios[7],
        (width - 40) * colRatios[8],
        (width - 40) * colRatios[9],
        (width - 40) * colRatios[10]
    ]
    property int rowHeight: 35

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // --- 1. 标题 ---
        RowLayout {
            spacing: 12
            Layout.fillWidth: true
            Rectangle { width: 4; height: 20; color: "#2C3E50"; radius: 2 }
            Text {
                text: "测试结果"
                font.pixelSize: 20; font.bold: true; color: "#2C3E50"
            }
        }

        // --- 2. 动态表格区域 ---
        Rectangle {
            id: tableContainer
            Layout.fillWidth: true
            Layout.fillHeight: true // 如果你想让这个框填满剩余高度
            border.color: "#DCDDE1"
            border.width: 1
            clip: true // 加上裁剪，防止数据溢出边框

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                // ==================== 表头区域 ====================
                Rectangle {
                    id: headerRect
                    Layout.fillWidth: true
                    height: rowHeight * 2
                    width: parent.width
                    color: "#F5F6FA"
                    border.color: "#DCDDE1"
                    border.width: 1

                    Row {
                        anchors.fill: parent

                        // 1-6列：基本列 (跨两行的高度)
                        HeaderCell { width: colWidths[0]; height: parent.height; text: "位置" }
                        HeaderCell { width: colWidths[1]; height: parent.height; text: "直阻\n(Ω)" }
                        HeaderCell { width: colWidths[2]; height: parent.height; text: "电阻换算值\n(Ω/km)" }
                        HeaderCell { width: colWidths[3]; height: parent.height; text: "电阻不平衡\n(%)" }

                        // 7列：复杂的绝缘电阻组合列
                        Column {
                            width: colWidths[4] + colWidths[5] + colWidths[6] + colWidths[7]
                            height: parent.height

                            // 上半部分：总标题
                            HeaderCell {
                                width: parent.width; height: rowHeight
                                text: "绝缘电阻(MΩ)"
                            }
                            // 下半部分：4个子项
                            Row {
                                HeaderCell { width: colWidths[4]; height: rowHeight; text: "线间" }
                                HeaderCell { width: colWidths[5]; height: rowHeight; text: "换算值" }
                                HeaderCell { width: colWidths[6]; height: rowHeight; text: "对地" }
                                HeaderCell { width: colWidths[7]; height: rowHeight; text: "换算值" }
                            }
                        }

                        HeaderCell { width: colWidths[8]; height: parent.height; text: "工作电容\n(nF)" }
                        HeaderCell { width: colWidths[9]; height: parent.height; text: "电容换算值\n(nF/km)" }

                        // 8列：重测
                        HeaderCell { width: colWidths[10]; height: parent.height; text: "重测" }
                    }
                }

                // ==================== 数据列表区域 ====================
                ListView {
                    id: resultList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    // 模拟测试数据 (实际开发中这里将替换为 C++ 的 QAbstractListModel)
                    model: ListModel {
                        ListElement { pos: "1"; dc: "0.8708"; dcConv: "21.42"; unbal: "0.00"; insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "1.285"; capConv: "32.86"; isPairStart: true }
                        ListElement { pos: "2"; dc: "0.8730"; dcConv: "21.47"; unbal: "";     insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "";      capConv: "";      isPairStart: false }
                        ListElement { pos: "3"; dc: "0.8750"; dcConv: "21.52"; unbal: "0.00"; insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "1.275"; capConv: "32.61"; isPairStart: true }
                        ListElement { pos: "4"; dc: "0.8722"; dcConv: "21.46"; unbal: "";     insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "";      capConv: "";      isPairStart: false }
                    }

                    delegate: Rectangle {
                        width: parent.width
                        height: rowHeight
                        // 隔行变色，或者选中高亮 (模仿图片中第一行被选中的蓝色)
                        color: index === 0 ? "#D9E8F5" : (index % 2 === 0 ? "#FFFFFF" : "#FDFDFD")

                        Row {
                            anchors.fill: parent

                            DataCell { width: colWidths[0]; text: model.pos; isBold: true }
                            DataCell { width: colWidths[1]; text: model.dc }
                            DataCell { width: colWidths[2]; text: model.dcConv }
                            DataCell { width: colWidths[3]; text: model.unbal}
                            DataCell { width: colWidths[4]; text: model.insL1 }
                            DataCell { width: colWidths[5]; text: model.insL1Conv }
                            DataCell { width: colWidths[6]; text: model.insGND }
                            DataCell { width: colWidths[7]; text: model.insGNDConv }
                            DataCell { width: colWidths[8]; text: model.cap; borderBottom: model.isPairStart }
                            DataCell { width: colWidths[9]; text: model.capConv; borderBottom: model.isPairStart }

                            // 重测复选框
                            Rectangle {
                                width: colWidths[10]
                                height: rowHeight
                                border.color: "#DCDDE1"; border.width: 1
                                color: "transparent"
                                CheckBox {
                                    anchors.centerIn: parent
                                    scale: 0.8 // 稍微缩小适应表格
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ==================== 内部组件 ====================

    // 表头单元格组件
    component HeaderCell: Rectangle {
        property string text: ""
        color: "transparent"
        border.color: "#DCDDE1"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: 13
            font.bold: true
            color: "#2C3E50"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            lineHeight: 1.2
        }
    }

    // 数据单元格组件
    component DataCell: Rectangle {
        property string text: ""
        property bool isBold: false
        // 用于模拟单元格合并的属性
        property bool borderBottom: true

        height: rowHeight
        color: "transparent"

        // 自定义边框绘制，为了实现合并效果
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.right: parent.right }
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.left: parent.left }
        Rectangle {
            width: parent.width; height: 1; color: "#DCDDE1"
            anchors.bottom: parent.bottom
            visible: parent.borderBottom // 控制是否显示底边框
        }

        Text {
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: 13
            font.bold: parent.isBold
            color: "#34495E"
        }
    }
}
