#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QHash>

class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider();

    // QML引擎会调用这个函数来请求图片
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    // 一个公共方法，用于从C++端更新图片
    void updateImage(const QString &id, const QImage &image);

private:
    // 使用哈希表存储图片，用ID作为键
    QHash<QString, QImage> m_images;
};

#endif // IMAGEPROVIDER_H
