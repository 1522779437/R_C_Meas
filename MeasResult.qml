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
        0.07, // 位置
        0.08, // 直阻
        0.10, // 直阻换算
        0.10, // 电阻不平衡
        0.09, // 绝缘-线间
        0.12, // 绝缘-换算
        0.09, // 绝缘-对地
        0.12, // 绝缘-换算
        0.08, // 工作电容
        0.10, // 电容换算
        0.05  // 重测
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
    property var dcRawValues: ({})  // 存储各位置原始DC值(Ω)，用于计算不平衡率

    // 收集勾选的重测位置
    function getCheckedPositions() {
        var result = []
        for (var i = 0; i < resultModel.count; i++) {
            if (resultModel.get(i).checked === true)
                result.push(parseInt(resultModel.get(i).pos))
        }
        return result
    }

    // 清除所有重测勾选
    function clearAllChecks() {
        for (var i = 0; i < resultModel.count; i++) {
            resultModel.setProperty(i, "checked", false)
        }
    }

    // 电阻不平衡率查找表：电缆类型 → 芯数 → 计算组数
    readonly property var unbalGroupMap: ({
        "信号电缆":         { 4: 1, 12: 3, 14: 3, 16: 4, 19: 4, 21: 4, 24: 5, 28: 7, 30: 7, 33: 7, 37: 7, 42: 7, 44: 7, 48: 12 },
        "数字信号电缆":     { 4: 1, 8: 2, 9: 2, 12: 3, 14: 3, 16: 4, 19: 4, 21: 5, 24: 6, 28: 7, 30: 7, 33: 7, 37: 7, 42: 7, 44: 7, 48: 12 },
        "内屏蔽数字信号电缆": { 8: 2, 12: 3, 14: 3, 16: 4, 19: 4, 21: 5, 24: 6, 28: 7, 30: 7, 33: 8, 37: 9, 42: 10, 44: 11, 48: 12 }
    })

    // 计算电阻不平衡率：信号电缆/数字信号电缆/内屏蔽数字信号电缆/其它电缆，按查找表或动态计算确定计算范围
    function updateUnbalance(pos) {
        var coreCount = cableParaModel.cableCoreCount
        var groups
        if (cableParaModel.selectedCableType === "其它电缆") {
            groups = Math.ceil(coreCount / 4) // 全部按四芯组计算，向上取整
        } else {
            var typeMap = unbalGroupMap[cableParaModel.selectedCableType]
            if (typeMap === undefined) return
            groups = typeMap[coreCount]
        }
        if (groups === undefined || groups <= 0) return
        var maxCalcPos = groups * 4
        if (pos > maxCalcPos) return

        var pairStart, pairEnd
        if (pos % 2 === 1) {
            pairStart = pos
            pairEnd = pos + 1
        } else {
            pairStart = pos - 1
            pairEnd = pos
        }
        var r1 = dcRawValues[pairStart]
        var r2 = dcRawValues[pairEnd]
        // 两个值都有效时才计算
        if (r1 !== undefined && r2 !== undefined && r1 > 0 && r2 > 0) {
            var unbal = (r1 - r2) / (r1 + r2) * 100
            // 更新奇数位置行的不平衡率
            for (var i = 0; i < resultModel.count; i++) {
                if (parseInt(resultModel.get(i).pos) === pairStart) {
                    resultModel.setProperty(i, "unbal", unbal.toFixed(2))
                    break
                }
            }
        }
    }

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
                        id: resultModel
                        // ListElement { pos: "1"; dc: "0.8708"; dcConv: "21.42"; unbal: "0.00"; insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "1.285"; capConv: "32.86"; isPairStart: true }
                        // ListElement { pos: "2"; dc: "0.8730"; dcConv: "21.47"; unbal: "";     insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "";      capConv: "";      isPairStart: false }
                        // ListElement { pos: "3"; dc: "0.8750"; dcConv: "21.52"; unbal: "0.00"; insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "1.275"; capConv: "32.61"; isPairStart: true }
                        // ListElement { pos: "4"; dc: "0.8722"; dcConv: "21.46"; unbal: "";     insL1: "20000"; insL1Conv: "512820"; insGND: "20000"; insGNDConv: "512820"; cap: "";      capConv: "";      isPairStart: false }
                    }

                    delegate: Rectangle {
                        width: resultList.width
                        height: rowHeight
                        // 隔行变色
                        color: index % 2 === 0 ? "#FFFFFF" : "#FDFDFD"
                        // 奇数行 z 轴提升，使不平衡率文字溢出时覆盖偶数行
                        z: parseInt(model.pos) % 2 !== 0 ? 2 : 1

                        Row {
                            anchors.fill: parent

                            DataCell { width: colWidths[0]; text: model.pos; isBold: true }
                            DataCell { width: colWidths[1]; text: model.dc;      isOvld: model.dc === "OVLD" }
                            DataCell { width: colWidths[2]; text: model.dcConv;  isOvld: model.dcConv === "OVLD" }
                            // 不平衡率 — 合并单元格
                            MergedUnbalCell { width: colWidths[3] }
                            DataCell { width: colWidths[4]; text: model.insL1;    isOvld: model.insL1 === "OVLD" || (model.insL1 !== "" && parseFloat(model.insL1) < 1) }
                            DataCell { width: colWidths[5]; text: model.insL1Conv; isOvld: model.insL1Conv === "OVLD" || model.insL1 === "OVLD" || (model.insL1 !== "" && parseFloat(model.insL1) < 1) }
                            DataCell { width: colWidths[6]; text: model.insGND;   isOvld: model.insGND === "OVLD" || (model.insGND !== "" && parseFloat(model.insGND) < 1) }
                            DataCell { width: colWidths[7]; text: model.insGNDConv; isOvld: model.insGNDConv === "OVLD" || model.insGND === "OVLD" || (model.insGND !== "" && parseFloat(model.insGND) < 1) }
                            MergedCapCell { width: colWidths[8]; cellText: model.cap; isOvld: model.cap === "OVLD" }
                            MergedCapCell { width: colWidths[9]; cellText: model.capConv; isOvld: model.capConv === "OVLD" || model.cap === "OVLD" }

                            // 重测复选框
                            Rectangle {
                                width: colWidths[10]
                                height: rowHeight
                                border.color: "#DCDDE1"; border.width: 1
                                color: "transparent"
                                CheckBox {
                                    anchors.centerIn: parent
                                    scale: 0.8
                                    checked: model.checked
                                    enabled: !controlMgr.isRetestMode
                                    onCheckStateChanged: {
                                        resultModel.setProperty(index, "checked", checked)
                                    }
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
        property bool isOvld: false
        // 用于模拟单元格合并的属性
        property bool borderBottom: true

        height: rowHeight
        color: isOvld ? "#FDEDEC" : "transparent"

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

    // 不平衡率合并单元格组件 (奇数行显示，偶数行留空)
    component MergedUnbalCell: Rectangle {
        height: rowHeight
        color: "transparent"

        // 左右边框
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.left: parent.left }
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.right: parent.right }
        // 底边框：仅偶数行显示
        Rectangle {
            width: parent.width; height: 1; color: "#DCDDE1"
            anchors.bottom: parent.bottom
            visible: parseInt(model.pos) % 2 === 0
        }

        // 文本：仅奇数行显示，溢出到下半行，居中于合并双行
        Text {
            visible: parseInt(model.pos) % 2 !== 0 && model.unbal !== ""
            y: rowHeight / 2
            width: parent.width
            height: rowHeight
            text: model.unbal
            font.pixelSize: 13
            font.bold: false
            color: "#34495E"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // 工作电容合并单元格组件 (奇数行显示，偶数行留空，与不平衡率同款)
    component MergedCapCell: Rectangle {
        property string cellText: ""
        property bool isOvld: false

        height: rowHeight
        color: isOvld ? "#FDEDEC" : "transparent"

        // 左右边框
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.left: parent.left }
        Rectangle { width: 1; height: parent.height; color: "#DCDDE1"; anchors.right: parent.right }
        // 底边框：仅偶数行显示
        Rectangle {
            width: parent.width; height: 1; color: "#DCDDE1"
            anchors.bottom: parent.bottom
            visible: parseInt(model.pos) % 2 === 0
        }

        // 文本：仅奇数行显示，y偏移半个行高居中于合并双行
        Text {
            visible: parseInt(model.pos) % 2 !== 0 && parent.cellText !== ""
            y: rowHeight / 2
            width: parent.width
            height: rowHeight
            text: parent.cellText
            font.pixelSize: 13
            font.bold: false
            color: "#34495E"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // --- 监听 C++ 的测量数据信号 ---
    Connections {
        target: controlMgr
        function onNewDcData(pos, dc, dcConv) {
            // 如果该位置已有数据则移除
            for (var i = 0; i < resultModel.count; i++) {
                if (resultModel.get(i).pos === String(pos)) {
                    resultModel.remove(i)
                    break
                }
            }
            // 按位置数值找到正确的插入索引，保持顺序不乱
            var insertIdx = resultModel.count
            for (var j = 0; j < resultModel.count; j++) {
                if (parseInt(resultModel.get(j).pos) > pos) {
                    insertIdx = j
                    break
                }
            }
            resultModel.insert(insertIdx, {
                "pos": String(pos),
                "dc": dc < 0 ? "OVLD" : dc.toFixed(4),
                "dcConv": dcConv < 0 ? "OVLD" : dcConv.toFixed(4),
                "unbal": "",
                "insL1": "",
                "insL1Conv": "",
                "insGND": "",
                "insGNDConv": "",
                "cap": "",
                "capConv": "",
                "isPairStart": (pos % 2 !== 0),
                "checked": false
            })
            // 存储原始DC值用于不平衡率计算
            if (dc >= 0) {
                dcRawValues[pos] = dc
            }
            updateUnbalance(pos)
        }
        function onNewInsData(pos, insL1, insL1Conv, insGND, insGNDConv) {
            // 查找该位置，已存在则更新绝缘列（线间+对地同值），否则插入新行
            for (var i = 0; i < resultModel.count; i++) {
                if (resultModel.get(i).pos === String(pos)) {
                    resultModel.setProperty(i, "insL1", insL1 < 0 ? "OVLD" : insL1.toFixed(2))
                    resultModel.setProperty(i, "insL1Conv", insL1Conv < 0 ? "OVLD" : insL1Conv.toFixed(2))
                    resultModel.setProperty(i, "insGND", insGND < 0 ? "OVLD" : insGND.toFixed(2))
                    resultModel.setProperty(i, "insGNDConv", insGNDConv < 0 ? "OVLD" : insGNDConv.toFixed(2))
                    return
                }
            }
            // 不存在则新建行
            var insertIdx = resultModel.count
            for (var j = 0; j < resultModel.count; j++) {
                if (parseInt(resultModel.get(j).pos) > pos) {
                    insertIdx = j
                    break
                }
            }
            resultModel.insert(insertIdx, {
                "pos": String(pos),
                "dc": "", "dcConv": "", "unbal": "",
                "insL1": insL1 < 0 ? "OVLD" : insL1.toFixed(2),
                "insL1Conv": insL1Conv < 0 ? "OVLD" : insL1Conv.toFixed(2),
                "insGND": insGND < 0 ? "OVLD" : insGND.toFixed(2),
                "insGNDConv": insGNDConv < 0 ? "OVLD" : insGNDConv.toFixed(2),
                "cap": "", "capConv": "",
                "isPairStart": (pos % 2 !== 0), "checked": false
            })
        }
        function onNewCapData(pos, cap, capConv) {
            // 两行存相同值，MergedCapCell 只奇数行显示文字，但两行都知 OVLD 状态用于标红
            var capStr = cap < 0 ? "OVLD" : cap.toFixed(3)
            var convStr = capConv < 0 ? "OVLD" : capConv.toFixed(3)
            // 查找该位置，已存在则更新电容列，否则插入新行
            for (var i = 0; i < resultModel.count; i++) {
                if (resultModel.get(i).pos === String(pos)) {
                    resultModel.setProperty(i, "cap", capStr)
                    resultModel.setProperty(i, "capConv", convStr)
                    return
                }
            }
            // 不存在则新建行
            var insertIdx = resultModel.count
            for (var j = 0; j < resultModel.count; j++) {
                if (parseInt(resultModel.get(j).pos) > pos) {
                    insertIdx = j
                    break
                }
            }
            resultModel.insert(insertIdx, {
                "pos": String(pos),
                "dc": "", "dcConv": "", "unbal": "",
                "insL1": "", "insL1Conv": "", "insGND": "", "insGNDConv": "",
                "cap": capStr,
                "capConv": convStr,
                "isPairStart": (pos % 2 !== 0), "checked": false
            })
        }
        function onClearResults() {
            resultModel.clear()
            dcRawValues = ({})
        }
        function onRetestCompleted() {
            testStatus.clearAllChecks()
        }
    }
}
