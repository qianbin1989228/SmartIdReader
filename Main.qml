import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 960
    height: 600
    visible: true
    title: "证照智能识别软件 V1.0"
    color: "#1e2a38" // 深色背景

    // *** 1. 定义一个属性来追踪当前是否处于放大状态，以及哪个框被放大了 ***
    property var zoomedFrame: null

    // 当这个ApplicationWindow组件完全加载完成后，执行这里的代码
    Component.onCompleted: {
        console.log("QML is ready. Requesting initial settings from C++.")
        backend.requestInitialSettings()
    }

    // 添加一个Connections元素来监听C++信号
    Connections {
        target: backend // 目标是我们在main.cpp中注册的backend对象

        // 当C++发射scanProgressChanged信号时，这个函数会被调用
        function onScanProgressChanged(step, message) {
            statusLabel.text = message;
            // 当采集完成时（我们虚构了第7步作为完成标志）
            if (step >= 7) {
                processButton.enabled = true; // 启用处理按钮
            }
        }

        // 当C++发射newImageCaptured信号时，这个函数会被调用
        function onNewImageCaptured(imageUrl, frameIndex) {
            switch(frameIndex) {
            case 0: frameFrontWhite.imageSource = imageUrl; break;
            case 1: frameFrontIR.imageSource = imageUrl; break;
            case 2: frameFrontUV.imageSource = imageUrl; break;
            case 3: frameBackWhite.imageSource = imageUrl; break;
            case 4: frameBackIR.imageSource = imageUrl; break;
            case 5: frameBackUV.imageSource = imageUrl; break;
            }
        }

        // 处理识别完成信号
        function onRecognitionComplete(results) {
            // 将QVariantMap直接当做JS对象使用
            idNumberLabel.text = results.idNumber

            // 显示结果区域
            resultsFrame.visible = true
        }

        // 响应设置加载完成的信号
        function onSettingsLoaded(initialContrast, initialBrightness) {
            console.log("Applying initial settings to UI:", initialContrast, initialBrightness)
            contrastSlider.value = initialContrast
            brightnessSlider.value = initialBrightness
        }

        // *** 响应保存状态更新的信号 ***
        function onSaveStatusUpdated(status) {
            statusLabel.text = status;
        }
    }

    // --- 新增：设置抽屉 ---
    Drawer {
        id: settingsDrawer
        width: 250
        height: parent.height/2
        edge: Qt.RightEdge // 从右侧滑出
        background: Rectangle { color: "#1c3e50" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15

            Label {
                text: "图像调节"
                font.bold: true
                color: "#ecf0f1"
                Layout.alignment: Qt.AlignHCenter
            }

            // 对比度
            Label { text: "对比度: " + contrastSlider.value.toFixed(2); color: "white" }
            Slider {
                id: contrastSlider
                from: 1.0
                to: 3.0
                value: 1.0 // 默认值
                Layout.fillWidth: true
                // 当滑块值变化时，实时调用C++槽函数
                onValueChanged: backend.setContrast(value)
            }

            // 亮度
            Label { text: "亮度: " + brightnessSlider.value; color: "white" }
            Slider {
                id: brightnessSlider
                from: -100
                to: 100
                value: 0 // 默认值
                Layout.fillWidth: true
                onValueChanged: backend.setBrightness(value)
            }
        }
    }

    // 状态栏，用于显示提示信息
    footer: Frame {
        height: 40
        background: Rectangle { color: "#2c3e50" }
        Label {
            id: statusLabel
            text: "准备就绪"
            anchors.centerIn: parent
            color: "white"
        }
        Button {
            id: settingsButton
            ToolTip.text: "图像参数设置"
            icon.source: "qrc:/icons/settings.svg" // 需要一个settings.svg图标
            icon.color: "white"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            flat: true
            onClicked: settingsDrawer.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // 图像显示区
        GridLayout {
            id: photoGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 3
            columnSpacing: 15
            rowSpacing: 15
            Layout.alignment: Qt.AlignHCenter

            PhotoFrame {
                id: frameFrontWhite
                title: "正面-白光"
                Layout.fillWidth: true
                Layout.fillHeight: true
                onDoubleClicked: {
                    // 如果当前没有放大的，就放大自己
                    // 如果当前放大的是自己，就复原
                    root.zoomedFrame = (root.zoomedFrame === null ? frameFrontWhite : null)
                }
                // 当没有放大的框，或者放大的框是自己时，我才可见
                visible: root.zoomedFrame === null || root.zoomedFrame === this

            }
            PhotoFrame {
                id: frameFrontIR
                title: "正面-红外"
                Layout.fillWidth: true
                Layout.fillHeight: true
                onDoubleClicked: {
                    root.zoomedFrame = (root.zoomedFrame === null ? frameFrontIR : null)
                }
                visible: root.zoomedFrame === null || root.zoomedFrame === this
            }
            PhotoFrame {
                id: frameFrontUV
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "正面-紫外"
                onDoubleClicked: {
                    root.zoomedFrame = (root.zoomedFrame === null ? frameFrontUV : null)
                }
                visible: root.zoomedFrame === null || root.zoomedFrame === this
            }
            PhotoFrame {
                id: frameBackWhite
                title: "反面-白光"
                Layout.fillWidth: true
                Layout.fillHeight: true
                onDoubleClicked: {
                     root.zoomedFrame = (root.zoomedFrame === null ? frameBackWhite : null)
                 }
                visible: root.zoomedFrame === null || root.zoomedFrame === this
            }
            PhotoFrame {
                id: frameBackIR
                title: "反面-红外"
                Layout.fillWidth: true
                Layout.fillHeight: true
                onDoubleClicked: {
                    root.zoomedFrame = (root.zoomedFrame === null ? frameBackIR : null)
                }
                visible: root.zoomedFrame === null || root.zoomedFrame === this
            }
            PhotoFrame {
                id: frameBackUV
                title: "反面-紫外"
                Layout.fillWidth: true
                Layout.fillHeight: true
                onDoubleClicked: {
                    root.zoomedFrame = (root.zoomedFrame === null ? frameBackUV : null)
                }
                visible: root.zoomedFrame === null || root.zoomedFrame === this
            }
            // *** 2. 定义状态 ***
            states: [
                State {
                    name: "zoomed"
                    // 当 zoomedFrame 属性不为 null 时，进入此状态
                    when: root.zoomedFrame !== null

                    PropertyChanges {
                        target: photoGrid // 改变GridLayout本身的属性
                        // 让放大的元素占据所有行和列
                        rows: 1
                        columns: 1
                    }
                }
            ]
        }

        // 识别结果展示区
        Frame {
            id: resultsFrame
            Layout.fillWidth: true
            Layout.topMargin: 20
            visible: false // 默认隐藏
            background: Rectangle { color: "#2c3e50" }

            GridLayout {
                anchors.fill: parent
                anchors.margins: 10
                columns: 2
                columnSpacing: 20
                rowSpacing: 5

                // 左侧标签
                Label { text: "识别结果:"; color:"#bdc3c7"; font.bold: true }
                Label { id: idNumberLabel; color: "white" }

                // 右侧标签
            }
        }


        // 操作区
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 50

            Button {
                id: scanButton
                Layout.preferredWidth: 120
                Layout.preferredHeight: 50
                text: "  开始采集"
                icon.source: "qrc:/icons/camera.svg" // 这里需要添加一个图标
                onClicked: {
                    // 重置UI状态
                    processButton.enabled = false
                    // 清空所有图片
                    frameFrontWhite.imageSource = ""
                    frameFrontIR.imageSource = ""
                    frameFrontUV.imageSource = ""
                    frameBackWhite.imageSource = ""
                    frameBackIR.imageSource = ""
                    frameBackUV.imageSource = ""

                    // 调用C++后端的采集函数
                    backend.startScan()

                    resultsFrame.visible = false // 点击扫描时隐藏结果
                }
                background: Rectangle {
                    color: parent.down ? "#16a085" : "#1abc9c"
                    radius: 5
                }
            }

            Button {
                id: processButton
                text: "  处理并识别"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 50
                enabled: false // 采集完成后才可用
                icon.source: "qrc:/icons/zap.svg"
                onClicked: {
                    backend.processAndRecognize()
                }
                background: Rectangle {
                    color: parent.down ? "#16a085" : "#1abc9c"
                    radius: 5
                }
            }
            // *** 保存图像按钮 ***
            Button {
                id: saveButton
                Layout.preferredWidth: 120
                Layout.preferredHeight: 50
                text: "  保存图像"
                icon.source: "qrc:/icons/save.svg" // 需要一个新的save.svg图标
                enabled: processButton.enabled // 当处理按钮可用时，保存按钮也可用
                onClicked: {
                    // 调用C++后端的保存函数
                    backend.saveAllImages()
                }
                background: Rectangle {
                    color: parent.down ? "#2980b9" : "#3498db"
                    radius: 5
                }
            }
        }
    }
}
