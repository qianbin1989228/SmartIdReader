#include "backend.h"
#include "imageprovider.h" // 包含 ImageProvider
#include <QTimer>
#include <QStandardPaths> // 用于获取桌面路径
#include <QDir>
#include <QDateTime>
#include <opencv2/imgproc.hpp> // 确保包含imgproc

#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>

Backend::Backend(ImageProvider* provider, QObject *parent) // 修改构造函数
    : QObject{parent}, m_imageProvider(provider)
{
    // 1. 确定配置文件的路径
    m_settingsFilePath = QCoreApplication::applicationDirPath() + "/settings.ini";
    qDebug() << "Settings file path:" << m_settingsFilePath;

    m_cameraThread = new QThread(this);
    m_cameraWorker = new CameraWorker();
    m_cameraWorker->moveToThread(m_cameraThread);

    // 连接：Backend请求 -> Worker执行
    connect(this, &Backend::startCaptureRequested, m_cameraWorker, &CameraWorker::startCapture);
    connect(this, &Backend::contrastChanged, m_cameraWorker, &CameraWorker::setContrast);
    connect(this, &Backend::brightnessChanged, m_cameraWorker, &CameraWorker::setBrightness);


    // 连接：Worker信号 -> Backend槽 -> Backend信号 -> QML
    connect(m_cameraWorker, &CameraWorker::progressUpdated, this, &Backend::scanProgressChanged);
    connect(m_cameraWorker, &CameraWorker::imageCaptured, this, &Backend::handleCapturedImage);
    connect(m_cameraWorker, &CameraWorker::captureFinished, [this](){
        // 这里可以添加完成后的逻辑，比如自动触发识别
    });
    connect(m_cameraWorker, &CameraWorker::errorOccurred, this, [this](const QString& msg){
        emit scanProgressChanged(0, msg); // 将错误显示在状态栏
    });

    // 线程退出时，自动删除worker
    connect(m_cameraThread, &QThread::finished, m_cameraWorker, &QObject::deleteLater);

    // 当整个应用程序准备退出时，安全地停止我们的线程
    // QCoreApplication::aboutToQuit() 是一个全局信号，非常适合用于清理工作
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        if (m_cameraThread->isRunning()) {
            m_cameraWorker->m_camera->stop(); // 停止摄像头
            m_cameraThread->terminate(); // 终止线程
        }
    });

    m_cameraThread->start();

    // *** 初始化网络管理器 ***
    m_networkManager = new QNetworkAccessManager(this);
}

Backend::~Backend()
{
    qDebug() << "Backend destructor called.";
}

void Backend::startScan()
{
    // 通过信号安全地调用工作线程中的方法
    emit startCaptureRequested();
}

void Backend::handleCapturedImage(const QImage &image, int frameIndex)
{
    // 构造一个唯一的图片ID
    QString imageId = QString("capture_%1").arg(frameIndex);
    // 更新ImageProvider中的图片
    m_imageProvider->updateImage(imageId, image);

    // 构造给QML的URL
    QString imageUrl = QString("image://livefeed/%1").arg(imageId);
    // 发射带有URL的信号给QML
    emit newImageCaptured(imageUrl, frameIndex);
}


void Backend::processAndRecognize()
{
    emit scanProgressChanged(8, "正在发送图像至AI服务器...");

    QImage image_to_process = m_imageProvider->requestImage("capture_0", nullptr, QSize());
    if (image_to_process.isNull()) {
        emit scanProgressChanged(0, "错误: 无法获取用于识别的图像");
        return;
    }

    // 1. 将QImage转换为JPEG格式的QByteArray
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image_to_process.save(&buffer, "JPEG");

    // 2. 将图像数据进行Base64编码
    QString imageBase64 = QString::fromLatin1(imageData.toBase64());

    // 3. 创建JSON对象
    QJsonObject jsonObject;
    jsonObject["image_base64"] = imageBase64;

    // 4. 将JSON对象转换为QByteArray
    QJsonDocument jsonDoc(jsonObject);
    QByteArray postData = jsonDoc.toJson();

    // 5. 创建HTTP请求
    QUrl url("http://127.0.0.1:8000/ocr");
    QNetworkRequest request(url);
    // **关键：设置正确的Content-Type头**
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 6. 发送POST请求，这次直接发送JSON数据
    QNetworkReply *reply = m_networkManager->post(request, postData);

    // 7. 连接回复的finished信号
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit scanProgressChanged(0, "网络错误: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        // 解析JSON回复
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        // JSON解析、图像解码和UI更新逻辑
        QString recognizedText = jsonObj["text"].toString();
        QString returnedImageBase64 = jsonObj["annotated_image"].toString();

        QByteArray returnedImageData = QByteArray::fromBase64(returnedImageBase64.toUtf8());
        QImage annotatedImage;
        annotatedImage.loadFromData(returnedImageData, "JPG");

        if (!annotatedImage.isNull()) {
            m_imageProvider->updateImage("capture_0", annotatedImage);
            emit newImageCaptured("image://livefeed/capture_0?timestamp=" + QString::number(QDateTime::currentMSecsSinceEpoch()), 0);
        }

        QVariantMap results;
        results["idNumber"] = recognizedText;
        emit recognitionComplete(results);
        emit scanProgressChanged(9, "AI识别完成！");
        reply->deleteLater();
    });
}

void Backend::setContrast(double contrast)
{   
    if (m_currentContrast != contrast) {
        m_currentContrast = contrast;
        emit contrastChanged(contrast);
        saveSettings();
    }
}

void Backend::setBrightness(int brightness)
{
    if (m_currentBrightness != brightness) {
        m_currentBrightness = brightness;
        emit brightnessChanged(brightness);
        saveSettings();
    }
}

// --- 实现设置读写方法 (基于文件) ---
void Backend::loadSettings()
{
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);
    settings.beginGroup("ImageSettings");
    m_currentContrast = settings.value("contrast", 1.0).toDouble();
    m_currentBrightness = settings.value("brightness", 0).toInt();
    settings.endGroup();

    // 将加载的值应用到worker
    emit contrastChanged(m_currentContrast);
    emit brightnessChanged(m_currentBrightness);

    // *** 发射信号，但这次是在QML请求时才发射 ***
    emit settingsLoaded(m_currentContrast, m_currentBrightness);
}

void Backend::saveSettings()
{
    // *** 这是关键修改 ***
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);

    settings.beginGroup("ImageSettings");
    settings.setValue("contrast", m_currentContrast);
    settings.setValue("brightness", m_currentBrightness);
    settings.endGroup();
}

// --- 新增方法的实现 ---
void Backend::requestInitialSettings()
{
    qDebug() << "QML requested initial settings.";
    // 当QML请求时，才加载并发送设置
    loadSettings();
}

// *** 实现保存图像的方法 ***
void Backend::saveAllImages()
{
    // 1. 获取桌面路径
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        emit saveStatusUpdated("错误: 无法获取桌面路径");
        return;
    }

    // 2. 创建主保存目录 "images"
    QDir desktopDir(desktopPath);
    QString mainFolderName = "images";
    if (!desktopDir.exists(mainFolderName)) {
        desktopDir.mkdir(mainFolderName);
    }
    desktopDir.cd(mainFolderName);

    // 3. 创建日期子目录 "YYYY-MM-DD"
    QString dateFolderName = QDate::currentDate().toString("yyyy-MM-dd");
    if (!desktopDir.exists(dateFolderName)) {
        desktopDir.mkdir(dateFolderName);
    }
    desktopDir.cd(dateFolderName);

    emit saveStatusUpdated("正在保存图像...");

    // 4. 遍历并保存6张图片
    bool all_saved = true;
    for (int i = 0; i < 6; ++i) {
        QString imageId = QString("capture_%1").arg(i);
        // 从ImageProvider获取原始图像
        QImage originalImage = m_imageProvider->requestImage(imageId, nullptr, QSize());

        if (originalImage.isNull()) {
            qWarning() << "Could not retrieve image for index" << i;
            all_saved = false;
            continue; // 如果某张图不存在，跳过
        }

        // --- 使用OpenCV进行缩放 ---
        // 将QImage转换为cv::Mat
        QImage tempImg = originalImage.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(tempImg.height(), tempImg.width(), CV_8UC3, tempImg.bits(), tempImg.bytesPerLine());

        // 计算新的尺寸
        int newWidth = 1200;
        double scale = static_cast<double>(newWidth) / mat.cols;
        int newHeight = static_cast<int>(mat.rows * scale);
        cv::Size newSize(newWidth, newHeight);

        // 执行缩放
        cv::Mat resizedMat;
        cv::resize(mat, resizedMat, newSize, 0, 0, cv::INTER_AREA);

        // 将处理后的cv::Mat转回QImage以便保存
        QImage finalImage(resizedMat.data, resizedMat.cols, resizedMat.rows, resizedMat.step, QImage::Format_RGB888);
        // --- OpenCV处理结束 ---

        // 5. 生成文件名 "hh_mm_ss_index.jpg"
        QString timeStr = QTime::currentTime().toString("hh_mm_ss");
        QString fileName = QString("%1_%2.jpg").arg(timeStr).arg(i + 1);
        QString fullPath = desktopDir.filePath(fileName);

        // 6. 保存文件
        if (!finalImage.save(fullPath, "JPG", 80)) { // 80是JPEG质量
            qWarning() << "Failed to save image to" << fullPath;
            all_saved = false;
        }
    }

    if (all_saved) {
        emit saveStatusUpdated("所有图像已成功保存至桌面/images/" + dateFolderName);
    } else {
        emit saveStatusUpdated("部分图像保存失败，请检查");
    }
}
