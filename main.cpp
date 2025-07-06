#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include "backend.h" // 1. 包含 Backend 头文件
#include <QQmlContext>
#include "imageprovider.h" // 2. 包含ImageProvider

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/icons/appicon.png"));

    QQmlApplicationEngine engine;

    // 1. 实例化ImageProvider
    ImageProvider *imageProvider = new ImageProvider();

    // 2. 创建 Backend 实例
    Backend backend(imageProvider);

    // 3. 将 C++ 对象注册为 QML 的上下文属性
    //    第一个参数是 QML 中使用的名字，第二个参数是 C++ 对象的引用
    engine.rootContext()->setContextProperty("backend", &backend);

    // 4. 向QML引擎注册Provider，命名为 "livefeed"
    engine.addImageProvider("livefeed", imageProvider);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("SmartIdReader", "Main");

    return app.exec();
}
