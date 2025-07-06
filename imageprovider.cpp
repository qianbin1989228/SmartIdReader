#include "imageprovider.h"

ImageProvider::ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    QUrl url(id);
    QString imageKey = url.path();

    if (m_images.contains(imageKey)) {
        QImage img = m_images.value(imageKey);
        if (size) {
            *size = img.size();
        }
        return img;
    }
    // 如果没有找到图片，返回一个空的QImage
    return QImage();
}

void ImageProvider::updateImage(const QString &id, const QImage &image)
{
    m_images.insert(id, image);
}
