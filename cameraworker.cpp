#include "cameraworker.h"
#include <QMediaDevices>
#include <QDebug>

CameraWorker::CameraWorker(QObject *parent)
    : QObject{parent}, m_captureStep(0)
{
    // 查找默认摄像头
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        emit errorOccurred("未找到摄像头设备");
        return;
    }
    m_camera = new QCamera(cameras.first(), this);

    m_captureSession = new QMediaCaptureSession(this);
    m_imageCapture = new QImageCapture(this);

    m_captureSession->setCamera(m_camera);
    m_captureSession->setImageCapture(m_imageCapture);

    // 连接图像捕获信号到处理槽
    connect(m_imageCapture, &QImageCapture::imageCaptured, this, &CameraWorker::processCapturedImage);

    m_camera->start(); // 预启动相机以加快首次捕获速度
}

CameraWorker::~CameraWorker()
{
    m_camera->stop();
    disconnect(m_imageCapture, nullptr, this, nullptr);
    qDebug() << "CameraWorker destructor called, camera stopped.";
}

void CameraWorker::startCapture()
{
    if (!m_camera) {
        emit errorOccurred("摄像头未初始化");
        return;
    }
    if (m_camera->error() != QCamera::NoError) {
        emit errorOccurred("摄像头错误: " + m_camera->errorString());
        return;
    }

    m_captureStep = 0;
    // 使用 QTimer::singleShot 来确保 triggerNextCapture 在 CameraWorker 的线程中执行
    QTimer::singleShot(200, this, &CameraWorker::triggerNextCapture);
}

void CameraWorker::triggerNextCapture()
{
    if (m_captureStep < m_totalSteps) {
        const QString messages[] = {
            "正在采集: 正面-白光...", "正在采集: 正面-红外...", "正在采集: 正面-紫外...",
            "正在采集: 反面-白光...", "正在采集: 反面-红外...", "正在采集: 反面-紫外..."
        };
        emit progressUpdated(m_captureStep + 1, messages[m_captureStep]);
        m_imageCapture->capture();
    }
}

void CameraWorker::processCapturedImage(int requestId, const QImage &img)
{
    Q_UNUSED(requestId);
    qDebug() << "Captured image for step" << m_captureStep + 1 ;

    // --- 开始OpenCV处理 ---
    // 1. 将QImage转换为cv::Mat
    // 注意：需要确保QImage的格式是OpenCV兼容的
    QImage processedImg = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(processedImg.height(), processedImg.width(), CV_8UC3, processedImg.bits(), processedImg.bytesPerLine());

    // 2. 应用对比度和亮度调节
    // convertScaleAbs 是一个高效的线性变换函数
    // new_pixel = alpha * old_pixel + beta
    cv::Mat processedMat;
    mat.convertTo(processedMat, -1, m_contrast, m_brightness);

    // 3. 将处理后的cv::Mat转换回QImage
    QImage finalImage(processedMat.data, processedMat.cols, processedMat.rows, processedMat.step, QImage::Format_RGB888);
    // --- 结束OpenCV处理 ---

    // 发射带有QImage的信号
    emit imageCaptured(finalImage.copy(), m_captureStep);

    m_captureStep++;

    if (m_captureStep < m_totalSteps) {
        // 启动定时器，500ms后进行下一次捕获
        QTimer::singleShot(500, this, &CameraWorker::triggerNextCapture);
    } else {
        // 全部完成
        emit progressUpdated(m_totalSteps + 1, "采集完成");
        emit captureFinished();
    }
}


// 新增槽函数的实现
void CameraWorker::setContrast(double contrast)
{
    // 参数范围限制
    m_contrast = std::max(1.0, std::min(3.0, contrast));
}

void CameraWorker::setBrightness(int brightness)
{
    m_brightness = std::max(-100, std::min(100, brightness));
}

