import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: cableTestParameters
    width: parent.width
    height: contentColumn.implicitHeight + 40
    color: "#FFFFFF"
    radius: 8
    border.color: "#DCDDE1"
    border.width: 1

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        spacing: 20

        // 标题部分 - 保持靠左
        RowLayout {
            spacing: 12
            Rectangle { width: 4; height: 20; color: "#2C3E50"; radius: 2 }
            Text {
                text: "电缆测试参数"
                font.pixelSize: 20
                font.bold: true
                color: "#2C3E50"
            }
        }

        // ================= 居中的表单主体部分 =================
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: false
            spacing: 15

            // --- 第一行：电缆类型 ---
            RowLayout {
                spacing: 15

                Text {
                    text: "电缆类型："
                    font.pixelSize: 16
                    color: "#57606F"
                    font.bold: true
                    Layout.preferredWidth: 85
                    horizontalAlignment: Text.AlignRight
                }

                // 显示选中内容的背景框
                Rectangle {
                    width: 200
                    Layout.preferredHeight: 40
                    color: "#F1F2F6"
                    radius: 4
                    border.color: "#CED6E0"
                    clip: true

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        verticalAlignment: Text.AlignVCenter
                        text: cableParaModel.selectedCableType
                        font.pixelSize: 16
                        // 如果没选，文字颜色浅一点
                        color: cableParaModel.selectedCableType === "请选择电缆类型..." ? "#A4B0BE" : "#2F3542"
                    }
                }

                // 选择类型按钮
                Button {
                    id: choosecableButton
                    text: "选择类型"

                    layer.enabled: true
                    layer.effect: DropShadow {
                        transparentBorder: true
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 16
                        color: "#40000000"
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 40
                        color: choosecableButton.pressed ? "#1C5980" : (choosecableButton.hovered ? "#3498DB" : "#2980B9")
                        radius: 6
                        border.color: "#FFFFFF"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.InOutQuad } }
                    }

                    contentItem: Text {
                        text: choosecableButton.text
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: myCableDialog.open()
                }
            }

            // --- 第二行：电缆种类 ---
            RowLayout {
                spacing: 15

                Text {
                    text: "电缆种类："
                    font.pixelSize: 16
                    color: "#57606F"
                    font.bold: true
                    Layout.preferredWidth: 85
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                // 图片显示区域
                Rectangle {
                    id: imageContainer
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 200
                    Layout.alignment: Qt.AlignLeft
                    color: "#F1F2F6"
                    radius: 6
                    border.color: "#CED6E0"
                    clip: true

                    Image {
                        id: cableImage
                        anchors.fill: parent
                        anchors.margins: 5
                        source: ""
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                    }

                    layer.enabled: true
                    layer.effect: DropShadow {
                        transparentBorder: true
                        verticalOffset: 2
                        radius: 6
                        color: "#15000000"
                    }
                }

                // 选择种类按钮
                Button {
                    id: choosecablesortButton
                    text: "选择种类"

                    enabled: cableParaModel.selectedCableType !== "" &&
                             cableParaModel.selectedCableType !== "请选择电缆类型..."

                    layer.enabled: true
                    layer.effect: DropShadow {
                        transparentBorder: true
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 16
                        color: "#40000000"
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 40
                        color: !choosecablesortButton.enabled ? "#BDC3C7" :
                               (choosecablesortButton.pressed ? "#1C5980" :
                               (choosecablesortButton.hovered ? "#3498DB" : "#2980B9"))
                        radius: 6
                        border.color: "#FFFFFF"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.InOutQuad } }
                    }

                    contentItem: Text {
                        text: choosecablesortButton.text
                        font.pixelSize: 16
                        font.bold: true
                        color: choosecablesortButton.enabled ? "white" : "#ECF0F1"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        myCableNumberDialog.folderPath = cableParaModel.imagesPath + cableParaModel.selectedCableType + "/"
                        myCableNumberDialog.open()
                    }
                }
            }

            // --- 第三行：电缆长度 ---
            RowLayout {
                spacing: 15

                Text {
                    text: "电缆长度："
                    font.pixelSize: 16
                    color: "#57606F"
                    font.bold: true
                    Layout.preferredWidth: 85
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                TextField {
                    id: inputField
                    width: 100
                    height: 50
                    implicitWidth: 100
                    Layout.preferredHeight: 50
                    text: Number(cableParaModel.cableLen).toFixed(2)
                    clip: true
                    font.pixelSize: 15
                    color: "#2F3542"

                    validator: DoubleValidator {
                        bottom: 0.00
                        top: 9999.99
                        decimals: 2
                        notation: DoubleValidator.StandardNotation
                    }

                    inputMethodHints: Qt.ImhFormattedNumbersOnly

                    background: Rectangle {
                        implicitHeight: 34
                        color: inputField.enabled ? "#F1F2F6" : "#F7F8FA"
                        border.color: inputField.activeFocus ? "#3498DB" :
                                      (inputField.hovered ? "#A4B0BE" : "#CED6E0")
                        border.width: inputField.activeFocus ? 2 : 1
                        radius: 4
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }

                    onEditingFinished: {
                        let val = parseFloat(text)
                        if (!isNaN(val)) {
                            cableParaModel.cableLen = val
                        }
                    }
                }

                Text {
                    text: "长度单位："
                    font.pixelSize: 16
                    color: "#57606F"
                    font.bold: true
                    Layout.preferredWidth: 85
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                ComboBox {
                    id: unitSelector
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 40
                    model: ["m", "km"]

                    currentIndex: find(cableParaModel.unit)

                    onActivated: (index) => {
                        cableParaModel.unit = textAt(index)
                    }

                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: unitSelector.indicator.width + unitSelector.spacing
                        text: unitSelector.currentText
                        font.pixelSize: 18
                        font.bold: false
                        color: "#2C3E50"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                    }

                    popup: Popup {
                        y: unitSelector.height + 3
                        width: unitSelector.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: unitSelector.popup.visible ? unitSelector.delegateModel : null
                            currentIndex: unitSelector.highlightedIndex
                            ScrollIndicator.vertical: ScrollIndicator { }
                        }

                        enter: Transition {
                            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
                            NumberAnimation { property: "y"; from: unitSelector.height - 10; to: unitSelector.height + 3; duration: 200; easing.type: Easing.OutCubic }
                        }

                        exit: Transition {
                            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
                        }

                        background: Rectangle {
                            border.color: "#CED6E0"
                            radius: 4
                            layer.enabled: true
                            layer.effect: DropShadow {
                                radius: 8
                                color: "#20000000"
                            }
                        }
                    }

                    delegate: ItemDelegate {
                        id: itemDelegate
                        width: unitSelector.width
                        height: 40
                        hoverEnabled: true

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }

                        contentItem: Text {
                            text: modelData
                            color: (itemDelegate.highlighted || itemDelegate.hovered) ? "#2980B9" : "#2C3E50"
                            font.pixelSize: 16
                            font.bold: itemDelegate.hovered
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        background: Rectangle {
                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: itemDelegate.hovered ? parent.height * 0.6 : 0
                                color: "#2980B9"
                                radius: 2
                                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                            }
                        }
                    }

                    background: Rectangle {
                        radius: 4
                        border.color: unitSelector.activeFocus ? "#2980B9" : "#CED6E0"
                        border.width: unitSelector.activeFocus ? 2 : 1
                        color: "#FFFFFF"
                    }
                }
            }
        }
    }

    // ================= 弹窗组件保持不变 =================
    CableNumberDialog {
        id: myCableNumberDialog

        onFileSelected: function(fileName, fileUrl) {
            console.log("当前选择的全路径: ", fileUrl)
            console.log("当前文件名: ", fileName)

            cableImage.source = fileUrl

            var match = fileName.match(/\d+/);
            if (match && match.length > 0) {
                var coreNum = parseInt(match[0], 10);
                testStatus.cableCoreCount = coreNum;
                cableParaModel.cableCoreCount = coreNum;
                console.log("电缆芯数: ", coreNum, "芯");
            } else {
                console.log("警告：文件名中未发现数字，无法识别芯数");
                testStatus.cableCoreCount = 0;
            }

            if (cableParaModel.selectedCableType === "内屏蔽数字信号电缆") {
                var shieldMatch = fileName.match(/[-一-龥]*([AB])\./);
                if (shieldMatch && shieldMatch.length > 1) {
                    cableParaModel.shieldType = shieldMatch[1];
                    console.log("内屏蔽类型: ", cableParaModel.shieldType);
                } else {
                    cableParaModel.shieldType = "";
                    console.log("警告：未识别到 A/B 类型");
                }
            } else {
                cableParaModel.shieldType = "";
            }
        }
    }

    CableTypeDialog {
        id: myCableDialog

        onTypeSelected: (typeName) => {
            cableParaModel.selectedCableType = typeName
            console.log("电缆类型:", typeName)
        }
    }
}
