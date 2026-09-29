import QtQuick
import QtQuick.Effects

// Qt6 原生 MultiEffect 替代 Qt5Compat.GraphicalEffects 的 DropShadow
MultiEffect {
    id: root

    // 兼容 DropShadow 的属性
    property color color: "#40000000"
    property real radius: 8
    property real horizontalOffset: 0
    property real verticalOffset: 0
    property int samples: 16
    property bool transparentBorder: true

    shadowEnabled: true
    shadowColor: root.color
    shadowBlur: Math.min(1.0, root.radius / 16.0)
    shadowHorizontalOffset: root.horizontalOffset
    shadowVerticalOffset: root.verticalOffset
}
