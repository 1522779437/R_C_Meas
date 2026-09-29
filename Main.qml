import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia // 必须导入

Window {
    id: rootWindow
    width: 1100
    height: 800
    visible: true
    title: qsTr("电缆检测系统")
    minimumWidth: 1100
    minimumHeight: 800

    // --- 逻辑控制变量 ---
    property bool isVideoFinished: false

    // ==================== 1. 启动视频层 ====================
    Rectangle {
        id: videoLayer
        anchors.fill: parent
        color: "black"
        z: 100 // 确保在最上层
        visible: !isVideoFinished

        MediaPlayer {
            id: player
            source: "file:///D:/Qt/Qt_code/r_c_meas_v5/r_c_meas_v5/video/开机视频.mp4"
            videoOutput: videoOutput
            autoPlay: true

            // 播放结束后的处理
            onPlaybackStateChanged: {
                if (playbackState === MediaPlayer.StoppedState) {
                    enterMainUI()
                }
            }

            // 错误处理（防止视频损坏导致程序卡死）
            onErrorOccurred: (error, errorString) => {
                console.log("Video Error: " + errorString)
                enterMainUI()
            }
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
        }

        // 跳过按钮 (可选)
        Button {
            text: "跳过"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20
            onClicked: enterMainUI()
            opacity: 0.7
        }
    }

    // ==================== 2. 主界面加载器 ====================
    Loader {
        id: mainLoader
        anchors.fill: parent
        active: false // 初始不加载主 UI，节省内存
    }

    // ==================== 3. 主界面组件 (你原本的代码) ====================
    Component {
        id: mainUIComponent

        // 这里包裹你原本 mainBg 之后的所有内容
        Rectangle {
            id: mainBg
            anchors.fill: parent
            color: "#E6E9ED"

            // 装饰纹路
            Canvas {
                anchors.fill: parent
                opacity: 0.1
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.strokeStyle = "#000000";
                    ctx.lineWidth = 1;
                    for (var i = 0; i < width + height; i += 20) {
                        ctx.beginPath();
                        ctx.moveTo(i, 0);
                        ctx.lineTo(i - height, height);
                        ctx.stroke();
                    }
                }
            }

            // 顶部状态栏
            Rectangle {
                id: header
                width: parent.width; height: 50
                color: "#2C3E50"; z: 2
                Text {
                    text: qsTr("电缆检测系统"); color: "#FFFFFF"
                    font.pixelSize: 24; font.bold: true; anchors.centerIn: parent
                }
            }

            // 内容区域容器
            Rectangle {
                id: contentContainer
                anchors.fill: parent
                anchors.margins: 20
                anchors.topMargin: header.height + 20
                color: "#FFFFFF"; radius: 8; border.color: "#BDC3C7"; border.width: 2

                Flickable {
                    anchors.fill: parent; anchors.margins: 15
                    contentWidth: width; contentHeight: mainLayout.implicitHeight; clip: true

                    ColumnLayout {
                        id: mainLayout; width: parent.width; spacing: 20
                        //信息登记
                        UserInfoPanel { id: userInfoPart; Layout.fillWidth: true }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 15
                            Layout.alignment: Qt.AlignTop
                            //电缆测试参数
                            CableTestParameters {
                                id: cableTestParameters
                                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                            }

                            ColumnLayout {
                                spacing: 20; Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                                //测试项目配置
                                MeasProject { id: measProject; Layout.fillWidth: true; Layout.fillHeight: true }
                                //控制面板
                                ControlButton { id: controlButton; Layout.fillWidth: true; Layout.fillHeight: true }
                            }
                        }
                        //测试状态
                        TestStatus { id: testStatus; Layout.fillWidth: true; Layout.fillHeight: true }
                        //测试结果
                        MeasResult { id: measResult; Layout.fillWidth: true; Layout.fillHeight: true }
                        Item { Layout.preferredHeight: 5 }
                    }

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }
        }
    }

    // --- 切换函数 ---
    function enterMainUI() {
        if (!isVideoFinished) {
            player.stop()
            isVideoFinished = true
            mainLoader.sourceComponent = mainUIComponent
            mainLoader.active = true
        }
    }
}
