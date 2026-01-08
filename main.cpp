#include "ui/MetalSurfaceDetection.h"
#include "data/DatasetModel.h"
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
    MetalSurfaceDetection w;
    w.show();
    return a.exec();
}
