#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QVariantMap>
#include <QThread> // 1. 包含QThread
#include "cameraworker.h" // 2. 包含CameraWorker
#include "imageprovider.h"
#include <QSettings>
#include <QCoreApplication> // 2. 包含QCoreApplication以获取程序路径
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(ImageProvider* provider, QObject *parent = nullptr);
    ~Backend();

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void processAndRecognize();

    Q_INVOKABLE void setContrast(double contrast);
    Q_INVOKABLE void setBrightness(int brightness);

    // *** 一个给QML调用的方法 ***
    Q_INVOKABLE void requestInitialSettings();

    // *** 保存所有图像的槽 ***
    Q_INVOKABLE void saveAllImages();

signals:
    // 这些信号保持不变，QML已经连接好了
    void scanProgressChanged(int step, const QString &message);
    void newImageCaptured(const QString &imageUrl, int frameIndex); // 注意，参数类型改为URL字符串
    void recognitionComplete(const QVariantMap &results);

    // 将加载的设置传递给QML
    void settingsLoaded(double initialContrast, int initialBrightness);

    // 内部信号，用于安全地启动工作线程中的任务
    void startCaptureRequested();

    // 图像参数调整信号
    void contrastChanged(double contrast);
    void brightnessChanged(int brightness);

    // *** 保存状态反馈信号 ***
    void saveStatusUpdated(const QString &status);

public slots:
    // 从worker接收捕获的图像
    void handleCapturedImage(const QImage &image, int frameIndex);

private:
    // 加载和保存配置
    void loadSettings();
    void saveSettings();

public:
    double m_currentContrast = 1.0;
    int m_currentBrightness = 0;
    QString m_settingsFilePath; // 4. 新增：存储配置文件的完整路径

private:
    QThread* m_cameraThread;
    CameraWorker* m_cameraWorker;
    ImageProvider* m_imageProvider; // 后端将拥有ImageProvider
    QNetworkAccessManager *m_networkManager;
};

#endif // BACKEND_H
