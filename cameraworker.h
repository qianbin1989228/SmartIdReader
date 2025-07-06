#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QTimer>

#include <opencv2/core.hpp>   // 包含OpenCV头文件
#include <opencv2/imgproc.hpp>

class CameraWorker : public QObject
{
    Q_OBJECT
public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

signals:
    // 信号：通知UI更新进度
    void progressUpdated(int step, const QString &message);
    // 信号：一张新图像已捕获完成，传递的是QImage对象
    void imageCaptured(const QImage &image, int frameIndex);
    // 信号：全部采集流程结束
    void captureFinished();
    // 信号：发生错误
    void errorOccurred(const QString &errorMessage);

public slots:
    // 槽：从Backend接收开始采集的指令
    void startCapture();
    // 接收调节图像参数的槽
    void setContrast(double contrast);
    void setBrightness(int brightness);

private slots:
    // 槽：当QImageCapture捕获到一帧图像时被调用
    void processCapturedImage(int requestId, const QImage &img);
    // 槽：用于触发下一次拍照的辅助定时器
    void triggerNextCapture();

public:
    QCamera *m_camera = nullptr;

private:

    QMediaCaptureSession *m_captureSession = nullptr;
    QImageCapture *m_imageCapture = nullptr;

    int m_captureStep;
    const int m_totalSteps = 6;

    // 存储处理参数
    double m_contrast = 1.0;
    int m_brightness = 0;
};

#endif // CAMERAWORKER_H
