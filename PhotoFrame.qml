import QtQuick
import QtQuick.Controls

Frame {
    id: root
    width: 300
    height: 300
    padding: 0
    background: Rectangle {
        color: "#2c3e50"
        border.color: sourceImage.status === Image.Ready ? "#1abc9c" : "#34495e"
        border.width: 2
    }

    // *** 1. 定义一个自定义信号 ***
    signal doubleClicked()

    property alias imageSource: sourceImage.source
    property string title: "待采集"

    Image {
        id: sourceImage
        anchors.fill: parent
        anchors.margins: 2
        fillMode: Image.PreserveAspectFit
        asynchronous: true
    }

    Label {
        text: title
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8
        color: "white"
        font.bold: true
        background: Rectangle {
            color: "#000000"
            opacity: 0.5
            radius: 3
        }
        padding: 4
        visible: sourceImage.status !== Image.Ready
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: sourceImage.status === Image.Loading
    }

    // *** 2. 添加MouseArea来捕捉双击事件 ***
    MouseArea {
        anchors.fill: parent
        // 当检测到双击时，发射我们定义的信号
        onDoubleClicked: root.doubleClicked()
    }

}
