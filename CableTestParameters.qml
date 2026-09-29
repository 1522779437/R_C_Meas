import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

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
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20 }
        spacing: 20

        // 标题部分
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

        // 显示区域：将标签、内容框、按钮横向排列
        RowLayout {
            spacing: 15
            Layout.fillWidth: true
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
                // Layout.fillWidth: true
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
                    // 【关键】这里绑定了上面定义的变量
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

                // 阴影和样式保持你原来的设置...
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0    // 水平偏移（不偏）
                    verticalOffset: 3      // 垂直偏移（向下偏 3 像素，产生高度感）
                    radius: 8              // 阴影模糊范围，越大越散
                    samples: 16            // 采样率，越高阴影越细腻
                    color: "#40000000"     // 25% 透明度的黑色。工业风忌用纯黑，半透明才自然
                }
                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 40

                    // 定义颜色逻辑
                    color: choosecableButton.pressed ? "#1C5980" :
                    (choosecableButton.hovered ? "#3498DB" : "#2980B9")

                    Behavior on color {
                        ColorAnimation {
                            duration: 150      // 动画持续 150 毫秒
                            easing.type: Easing.InOutQuad // 渐入渐出，让颜色变化更柔和
                        }
                    }
                    radius: 6
                    border.color: "#FFFFFF"
                    border.width: 1
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

        RowLayout {
            spacing: 15
            Layout.fillWidth: true
            // 关键：为了防止图片撑大界面，建议限制这一行的首选高度
            // Layout.preferredHeight: 120

            Text {
                text: "电缆种类："
                font.pixelSize: 16
                color: "#57606F"
                font.bold: true
                Layout.preferredWidth: 85
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter // 垂直居中，与图片对齐
            }

            // --- 在这里添加图片显示区域 ---
            Rectangle {
                id: imageContainer
                // 1. 设置图片区域的大小
                Layout.preferredWidth: 200
                Layout.preferredHeight: 200
                Layout.alignment: Qt.AlignLeft // 靠左对齐，紧跟文字

                color: "#F1F2F6" // 给个背景色，即使图片没加载也好看
                radius: 6 // 工业风圆角
                border.color: "#CED6E0" // 与输入框风格一致的边框
                clip: true // 裁切超出圆角的部分

                Image {
                    id: cableImage
                    anchors.fill: parent
                    anchors.margins: 5 // 给图片留一点内边距，更美观

                    // 2. 指定图片源（替换为你的图片路径）
                    // 例如：source: "qrc:/images/cable_28pin.png"
                    // 或者：source: "file:///D:/Qt/Qt_code/r_c_meas_v5/r_c_meas_v5/images/cable.png"
                    source: ""
                    fillMode: Image.PreserveAspectFit

                    horizontalAlignment: Image.AlignHCenter // 图片在框内水平居中
                    verticalAlignment: Image.AlignVCenter // 图片在框内垂直居中
                }

                // 可选：给图片加个简单的阴影增加立体感
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    verticalOffset: 2
                    radius: 6
                    color: "#15000000" // 非常淡的阴影
                }
            }
            // ----------------------------

            Button {
                id: choosecablesortButton
                text: "选择种类"

                // --- 核心逻辑：只有当类型不为空且不是默认提示语时才可用 ---
                enabled: cableParaModel.selectedCableType !== "" &&
                         cableParaModel.selectedCableType !== "请选择电缆类型..."

                // 阴影和样式保持你原来的设置...
                layer.enabled: true
                layer.effect: DropShadow {
                    transparentBorder: true
                    horizontalOffset: 0    // 水平偏移（不偏）
                    verticalOffset: 3      // 垂直偏移（向下偏 3 像素，产生高度感）
                    radius: 8              // 阴影模糊范围，越大越散
                    samples: 16            // 采样率，越高阴影越细腻
                    color: "#40000000"     // 25% 透明度的黑色。工业风忌用纯黑，半透明才自然
                }
                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 40

                    // 定义颜色逻辑
                    color: !choosecablesortButton.enabled ? "#BDC3C7" :
                                   (choosecablesortButton.pressed ? "#1C5980" :
                                   (choosecablesortButton.hovered ? "#3498DB" : "#2980B9"))

                    Behavior on color {
                        ColorAnimation {
                            duration: 150      // 动画持续 150 毫秒
                            easing.type: Easing.InOutQuad // 渐入渐出，让颜色变化更柔和
                        }
                    }
                    radius: 6
                    border.color: "#FFFFFF"
                    border.width: 1
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
                    if (cableParaModel.selectedCableType === "信号电缆")
                    {
                        myCableNumberDialog.folderPath = "file:///D:/Qt/Qt_code/r_c_meas_v5/r_c_meas_v5/images/信号电缆/"
                    }
                    else if (cableParaModel.selectedCableType === "数字信号电缆")
                    {
                        myCableNumberDialog.folderPath = "file:///D:/Qt/Qt_code/r_c_meas_v5/r_c_meas_v5/images/数字信号电缆/"
                    }
                    else if (cableParaModel.selectedCableType === "内屏蔽数字信号电缆")
                    {
                        myCableNumberDialog.folderPath = "file:///D:/Qt/Qt_code/r_c_meas_v5/r_c_meas_v5/images/内屏蔽数字信号电缆/"
                    }

                    myCableNumberDialog.open()
                }
            }
        }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            Text {
                text: "电缆长度："
                font.pixelSize: 16
                color: "#57606F"
                font.bold: true
                Layout.preferredWidth: 85
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter // 垂直居中，与图片对齐
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

                // ================== 新增代码：输入限制 ==================
                // 1. 使用 DoubleValidator 限制只能输入浮点数
                validator: DoubleValidator {
                    bottom: 0.00         // 最小值：不能为负数
                    top: 9999.99        // 最大值：根据你的实际需求设置
                    decimals: 2          // 小数位数：最多允许输入2位小数
                    notation: DoubleValidator.StandardNotation // 标准数字格式（禁用科学计数法）
                }

                // 2. （可选）如果你用的工业机是触摸屏，这行能让点击时自动弹出数字键盘
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                // =======================================================

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
                verticalAlignment: Text.AlignVCenter // 垂直居中，与图片对齐
            }

            ComboBox {
                id: unitSelector
                Layout.preferredWidth: 80
                Layout.preferredHeight: 40
                model: ["m", "km"]

                currentIndex: find(cableParaModel.unit)

                    // --- 【连接 C++：写入】当用户在界面点击选择时，把值传给 C++ ---
                onActivated: (index) => {
                    cableParaModel.unit = textAt(index)
                }

                // --- 1. 修改选完后的文字大小 ---
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: unitSelector.indicator.width + unitSelector.spacing
                    text: unitSelector.currentText
                    font.pixelSize: 18  // 这里调大选中的文字
                    font.bold: false     // 加粗让它更醒目
                    color: "#2C3E50"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft // 通常靠左好看，你也可以改成 HCenter
                }

                // --- 2. 增加下拉菜单的弹出动画 ---
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

                    // 弹出动画：从上方滑入并伴随透明度变化
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
                        // 给下拉列表加个阴影，显得高级
                        layer.enabled: true
                        layer.effect: DropShadow {
                            radius: 8
                            color: "#20000000"
                        }
                    }
                }

                // --- 3. 下拉项样式 (保持之前的逻辑，微调字号) ---
                delegate: ItemDelegate {
                        id: itemDelegate
                        width: unitSelector.width
                        height: 40
                        hoverEnabled: true // 必须开启悬停使能

                        // --- 1. 修复鼠标形状报错：使用 MouseArea 穿透 ---
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton // 关键：不拦截点击事件，只改变形状
                        }

                        // --- 2. 文字内容响应 ---
                        contentItem: Text {
                            text: modelData
                            // 逻辑：如果当前项被选中或是鼠标悬停，文字颜色变蓝
                            color: (itemDelegate.highlighted || itemDelegate.hovered) ? "#2980B9" : "#2C3E50"
                            font.pixelSize: 16
                            font.bold: itemDelegate.hovered // 悬停时稍微加粗
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter

                            // 颜色平滑过渡动画
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        // --- 3. 背景响应 ---
                        background: Rectangle {
                            // 左侧指示条：鼠标滑过时显示一个小蓝块，增加精致感
                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: itemDelegate.hovered ? parent.height * 0.6 : 0
                                color: "#2980B9"
                                radius: 2

                                // 指示条的高度变化动画
                                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                            }
                        }
                    }
                // 主框背景保持不变
                background: Rectangle {
                    radius: 4
                    border.color: unitSelector.activeFocus ? "#2980B9" : "#CED6E0"
                    border.width: unitSelector.activeFocus ? 2 : 1
                    color: "#FFFFFF"
                }
            }
        }
    }

    CableNumberDialog {
        id: myCableNumberDialog

        // 当你在任何一个文件夹选好文件并点击“确认”时
        onFileSelected: function(fileName, fileUrl) {
            console.log("当前选择的全路径: ", fileUrl)
            console.log("当前文件名: ", fileName)

            // 1. 更新图片显示
            cableImage.source = fileUrl

            // 2. 提取芯数
            // \d+ 表示匹配一个或多个数字
            // 无论文件名是 "04芯.png" 还是 "内屏蔽_12芯.jpg"，它都能抓到数字
            var match = fileName.match(/\d+/);

            if (match && match.length > 0) {
                var coreNum = parseInt(match[0], 10);

                // 3. 将提取到的数字传给测试状态组件
                // 假设你的测试状态组件 id 是 testStatusPanel
                testStatus.cableCoreCount = coreNum;
                cableParaModel.cableCoreCount = coreNum // 更新芯数到 C++
                console.log("电缆芯数: ", coreNum, "芯");
            } else {
                console.log("警告：文件名中未发现数字，无法识别芯数");
                testStatus.cableCoreCount = 0;
            }
        }
    }

    // 弹窗组件
    CableTypeDialog {
        id: myCableDialog
        // 【3】处理弹窗传回来的数据
        onTypeSelected: (typeName) => {
            // 将选中的内容更新到变量中
            cableParaModel.selectedCableType = typeName
            console.log("电缆类型:", typeName)
        }
    }

}
