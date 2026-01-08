#include "ui/MetalSurfaceDetection.h"
#include "data/DatasetModel.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QDir>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString defaultModel = "../../models/my_svm_model.yml";
    bool loaded = DatasetModel::instance()->loadModel(defaultModel);
    if(loaded)
        qDebug() << "默认模型记载成功！";
    else
        qDebug() << "默认模型记载失败！";

    // 初始化日志系统
    Logger::setConsoleOutput(true);  // 输出到控制台
    Logger::setLogLevel(LOG_INFO);   // 设置日志级别

    LOG_INFO("=========================================");
    LOG_INFO("金属表面缺陷检测系统启动");
    LOG_INFO(QString("Qt版本: %1").arg(qVersion()));
    LOG_INFO(QString("OpenCV版本: %1").arg(CV_VERSION));
    LOG_INFO("=========================================");

    // 创建主窗口
    MetalSurfaceDetection w;
    w.show();
    LOG_INFO("应用程序窗口已显示");
    int result = a.exec();
    LOG_INFO("应用程序退出");

    return result;
}
