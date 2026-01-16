#ifndef PREDICTIONRESULT_H
#define PREDICTIONRESULT_H

#include <QString>
#include <opencv2/opencv.hpp>
#include <QDateTime>


enum DefectType
{
    Unknown = 0,
    Crazing = 1,   // 龟裂/网裂（表面细密网状裂纹，常见于涂层或镀层）
    Inclusion = 2, // 夹杂物（金属内部嵌入的非金属或异物杂质）
    Patches = 3,   // 斑块/补丁状瑕疵（表面颜色或质地不均匀的局部区域）
    Pitted = 4,    // 点蚀/麻点（局部腐蚀形成的小孔状凹陷）
    Rolled = 5,    // 轧制缺陷（轧制过程中产生的皱褶、翘皮等不规则痕迹）
    Scratches = 6    // 划痕（线性机械刮伤）
};

struct PredictionResult
{
    QString imgName;
    QString imgPath;
    cv::Size imgSize;
    qint64 fileSize;

    QDateTime timeStamp;    // 预测开始的时间戳
    QString predictLabel;   // 预测标签
    QString error;          // 错误信息
    double confidence;      // 置信度
    DefectType predictClassId;     // 预测类别Id

    bool processingSuccess;
    QString errorMessage;
    QString trueLabel;      // 真实标签
    DefectType trueClassId;        // 真实类Id

    QString featureDimension;
    QString featureType;

    qint64 totalProcessingTime;
    QString modelName;
    QString modelPath;

    bool isCorrect() const
    {
        return processingSuccess && trueLabel == predictLabel;
    }

    bool isFalsePositive() const {
        return processingSuccess && !trueLabel.isEmpty() &&
               trueLabel != predictLabel;
    }

    bool isFalseNegative() const {
        return processingSuccess && !trueLabel.isEmpty() &&
               trueLabel != predictLabel &&
               predictLabel == "无缺陷" &&
               trueLabel != "无缺陷";
    }

    bool isTruePositive() const {
        return processingSuccess && !trueLabel.isEmpty() &&
               trueLabel != "无缺陷" &&
               trueLabel == predictLabel;
    }

    bool isTrueNegative() const {
        return processingSuccess && !trueLabel.isEmpty() &&
               trueLabel == "无缺陷" &&
               predictLabel == "无缺陷";
    }

    QString getErrorType() const
    {
        if(!processingSuccess)  return "处理失败";
        if(trueLabel.isEmpty()) return "未标注";
        if(isCorrect())         return "正确";
        if (isFalsePositive())  return "误报(FP)";
        if (isFalseNegative())  return "漏报(FN)";
        return "分类错误";
    }

    DefectType getClassIdFromName(const QString& className){
        DefectType type;
        if(className == "Crazing")
            type = Crazing;
        else if(className == "Inclusion")
            type = Inclusion;
        else if(className == "Patches")
            type = Patches;
        else if(className == "Pitted")
            type = Pitted;
        else if(className == "Rolled")
            type = Rolled;
        else if(className == "Scratches")
            type = Scratches;
        else
            type = Unknown;
        return type;
    }

    static QStringList sortCategoryNames(const QSet<QString>& names)
    {
        QStringList sorted;
        QStringList Categories = {"Unknown","Crazing","Inclusion","Patches","Pitted","Rolled","Scratches"};
        for(int i = 0; i < Categories.size(); ++i)
        {
            if(names.contains(Categories[i]))
                sorted.append(Categories[i]);
        }
        return sorted;
    }
};

#endif // PREDICTIONRESULT_H
