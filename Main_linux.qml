import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Window {
    id: rootWindow
    width: 1150
    height: 750
    visible: true
    title: qsTr("电缆检测系统")
    minimumWidth: 1150
    minimumHeight: 750

    // --- 逻辑控制变量 ---
    property bool isVideoFinished: false

    // ==================== 视频路径 ====================
    property string videoPath: "file:///home/linaro/work/r_c_meas_v5/video/开机视频.mp4"

    // ==================== 1. 启动视频层 ====================
    Rectangle {
        id: videoLayer
        anchors.fill: parent
        color: "black"
        z: 100
        visible: !isVideoFinished

        MediaPlayer {
            id: player
            source: rootWindow.videoPath
            videoOutput: videoOutput
            autoPlay: true

            onPlaybackStateChanged: {
                if (playbackState === MediaPlayer.StoppedState) {
                    enterMainUI()
                }
            }

            // Qt6 新写法：用函数参数接收 errorString
            onErrorOccurred: function(error, errorString) {
                console.log("Video Error:", errorString)
                enterMainUI()
            }
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
        }

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
        active: false
    }

    // ==================== 3. 主界面组件 ====================
    Component {
        id: mainUIComponent

        Rectangle {
            id: mainBg
            anchors.fill: parent
            color: "#E6E9ED"

            Canvas {
                anchors.fill: parent
                opacity: 0.1
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.strokeStyle = "#000000"
                    ctx.lineWidth = 1
                    for (var i = 0; i < width + height; i += 20) {
                        ctx.beginPath()
                        ctx.moveTo(i, 0)
                        ctx.lineTo(i - height, height)
                        ctx.stroke()
                    }
                }
            }

            Rectangle {
                id: header
                width: parent.width; height: 50
                color: "#2C3E50"; z: 2
                Text {
                    text: qsTr("电缆检测系统"); color: "#FFFFFF"
                    font.pixelSize: 24; font.bold: true; anchors.centerIn: parent
                }
            }

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
                        UserInfoPanel { id: userInfoPart; Layout.fillWidth: true }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 15
                            Layout.alignment: Qt.AlignTop
                            CableTestParameters {
                                id: cableTestParameters
                                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                            }

                            ColumnLayout {
                                spacing: 20; Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                                MeasProject { id: measProject; Layout.fillWidth: true; Layout.fillHeight: true }
                                ControlButton { id: controlButton; Layout.fillWidth: true; Layout.fillHeight: true }
                            }
                        }
                        TestStatus { id: testStatus; Layout.fillWidth: true; Layout.fillHeight: true }
                        MeasResult { id: measResult; Layout.fillWidth: true; Layout.fillHeight: true }
                        Item { Layout.preferredHeight: 5 }
                    }

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }
        }
    }

    function enterMainUI() {
        if (!isVideoFinished) {
            player.stop()
            isVideoFinished = true
            mainLoader.sourceComponent = mainUIComponent
            mainLoader.active = true
        }
    }
}
