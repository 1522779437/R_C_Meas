import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
    property int workState: controlMgr.workState  // 0=空闲 1=工作 2=暂停

    // 电缆参数全部给定后才允许操作
    readonly property bool canOperate: cableParaModel.selectedCableType !== "" &&
                                       cableParaModel.selectedCableType !== "请选择电缆类型..." &&
                                       cableParaModel.cableCoreCount > 0 &&
                                       cableParaModel.cableLen > 0 &&
                                       cableParaModel.unit !== ""

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
            spacing: 16

            // 测量控制区
            RowLayout {
                spacing: 10
                // Rectangle {
                //     id: measureIndicator
                //     width: 6; height: 60; radius: 3
                //     color: workState === 1 ? "#27AE60" : (workState === 2 ? "#E67E22" : "#C0392B")
                //     layer.enabled: true
                //     layer.effect: DropShadow {
                //         color: measureIndicator.color
                //         radius: 8; samples: 16; spread: 0.01
                //     }
                //     Behavior on color { ColorAnimation { duration: 200 } }
                // }

                CustomButton {
                    id: startStopBtn
                    enabled: canOperate || workState !== 0
                    btnText: workState === 1 ? "暂停测量" : (workState === 2 ? "恢复测量" : "开始测量")
                    btnColor: workState === 1 ? "#E67E22" : (workState === 2 ? "#2980B9" : "#27AE60")
                    hoverColor: workState === 1 ? "#F39C12" : (workState === 2 ? "#3498DB" : "#2ECC71")
                    onClicked:
                    {
                        // 空闲状态下检查是否有勾选测试项目
                        if (workState === 0 && !testConfig.checkDC && !testConfig.checkIns && !testConfig.checkCap) {
                            noItemDialog.open()
                        } else {
                            controlMgr.processStartStop()
                        }
                    }
                }

                CustomButton {
                    id: endBtn
                    enabled: canOperate || workState !== 0
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
                spacing: 10

                CustomButton {
                    id: exportBtn
                    btnText: "导出报表"
                    btnColor: "#2980B9"
                    hoverColor: "#3498DB"
                    // onClicked: console.log("导出报表中...")
                    onClicked: controlMgr.processExport(backendModel)
                }

                CustomButton {
                    id: retestBtn
                    btnText: "重测选中"
                    enabled: workState === 0 && !controlMgr.isRetestMode
                    btnColor: "#8E44AD"
                    hoverColor: "#9B59B6"
                    onClicked: {
                        var checkedList = measResult.getCheckedPositions()
                        if (checkedList.length === 0) {
                            noRetestDialog.open()
                        } else {
                            controlMgr.startRetest(checkedList)
                        }
                    }
                }
            }
        }

        // --- 4. 下方占位符 (吃掉多余空间) ---
        Item {
            Layout.fillHeight: true
        }
    }

    // --- 导出完成信号处理 ---
    Connections {
        target: controlMgr
        function onExportFinished(success, filePath) {
            exportDialog.succeeded = success
            exportDialog.filePath = filePath
            exportDialog.open()
        }
        function onMeasAllComplete() {
            completeDialog.open()
        }
    }

    // --- 导出结果弹窗 ---
    ExportDialog {
        id: exportDialog
    }

    // --- 测量完成弹窗 ---
    Dialog {
        id: completeDialog
        modal: true
        anchors.centerIn: Overlay.overlay
        title: "提示"
        implicitWidth: 320

        background: Rectangle {
            color: "#FFFFFF"
            radius: 8
            border.color: "#DCDDE1"
        }

        contentItem: ColumnLayout {
            spacing: 20

            Text {
                text: "所有测量项目已完成！"
                font.pixelSize: 15
                color: "#27AE60"
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
            }
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignHCenter
            bottomPadding: 20

            Button {
                text: "知道了"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                contentItem: Text {
                    text: "知道了"
                    font.pixelSize: 15
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 38
                    color: parent.pressed ? "#1C5820" : (parent.hovered ? "#2ECC71" : "#27AE60")
                    radius: 6
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: completeDialog.close()
            }
        }
    }

    // --- 通用自定义按钮组件 (干净的扁平化风格) ---
    component CustomButton: Button {
        property string btnText: ""
        property color btnColor: "#2980B9"
        property color hoverColor: "#3498DB"

        id: controlBtn
        Layout.preferredWidth: 100
        Layout.preferredHeight: 60
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        background: Rectangle {
            // 背景色逻辑：禁用时灰色，按下时变深，悬停时变亮
            color: !controlBtn.enabled ? "#BDC3C7" :
                   (controlBtn.pressed ? Qt.darker(btnColor, 1.2) :
                   (controlBtn.hovered ? hoverColor : btnColor))
            radius: 6

            // --- 干净的关键：去掉阴影，改用细边框 ---
            border.color: Qt.darker(color, 1.1) // 边框比背景略深一点，增加精致感
            border.width: 1

            // 彻底关闭 layer 效果以消除任何模糊感
            layer.enabled: false

            Behavior on color { ColorAnimation { duration: 150 } }
        }

        contentItem: Item {
            Text {
                anchors.centerIn: parent
                text: btnText
                font.pixelSize: 15
                font.bold: true
                color: "white"
            }
        }

        MouseArea {
            anchors.fill: parent
            // 仅当按钮可用时显示手型
            cursorShape: controlBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            acceptedButtons: Qt.NoButton
        }
    }

    // --- 未勾选重测项提示弹窗 ---
    Dialog {
        id: noRetestDialog
        modal: true
        anchors.centerIn: Overlay.overlay
        title: "提示"
        implicitWidth: 320

        background: Rectangle {
            color: "#FFFFFF"
            radius: 8
            border.color: "#DCDDE1"
        }

        contentItem: ColumnLayout {
            spacing: 20

            Text {
                text: "请先勾选需要重测的行！"
                font.pixelSize: 15
                color: "#2C3E50"
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
            }
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignHCenter
            bottomPadding: 20

            Button {
                text: "知道了"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                contentItem: Text {
                    text: "知道了"
                    font.pixelSize: 15
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 38
                    color: parent.pressed ? "#1C5980" : (parent.hovered ? "#3498DB" : "#2980B9")
                    radius: 6
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: noRetestDialog.close()
            }
        }
    }

    // --- 未选择测试项目提示弹窗 ---
    Dialog {
        id: noItemDialog
        modal: true
        anchors.centerIn: Overlay.overlay
        title: "提示"
        implicitWidth: 320

        background: Rectangle {
            color: "#FFFFFF"
            radius: 8
            border.color: "#DCDDE1"
        }

        contentItem: ColumnLayout {
            spacing: 20

            Text {
                text: "请至少选择一个测试项目！"
                font.pixelSize: 15
                color: "#2C3E50"
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
            }
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignHCenter
            bottomPadding: 20

            Button {
                text: "知道了"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                contentItem: Text {
                    text: "知道了"
                    font.pixelSize: 15
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 38
                    color: parent.pressed ? "#1C5980" : (parent.hovered ? "#3498DB" : "#2980B9")
                    radius: 6
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: noItemDialog.close()
            }
        }
    }
}
