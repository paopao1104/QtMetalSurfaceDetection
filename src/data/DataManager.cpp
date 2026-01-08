
// void load()
// {
//     QFile f("D:/Qt_src/opencvTest/NEUMetalSurfaceDefectsData/train/my_svm_model.yml");
//     if(f.exists())
//     {
//         qDebug() << "模型已生成过！";
//         return;
//     }

//     // 1. 用train文件夹下的所有图片创建SVM模型
//     qDebug() << "===========创建SVM模型===========" ;
//     QString root = "D:/Qt_src/opencvTest/NEUMetalSurfaceDefectsData/train";
//     QDir dir(root);
//     if(!dir.exists()){
//         qDebug() << "路径不存在" ;
//         return;
//     }

//     Mat allLabels;
//     Mat allFeatures;

//     // 只获取文件夹
//     dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

//     // 获取所有文件夹(Crazing、inclusion、patches)
//     QFileInfoList folderList = dir.entryInfoList();

//     for(const QFileInfo& folderInfo : folderList)
//     {
//         QString folderName = folderInfo.fileName();
//         QString folderPath = folderInfo.absoluteFilePath();
//         // 遍历每个文件夹下的图片

//         qDebug() << "当前遍历目录:" << folderPath;

//         // 设置图片过滤器，只获取文件
//         QDir folderDir(folderPath);
//         folderDir.setNameFilters({"*.bmp"});
//         folderDir.setFilter(QDir::Files);

//         QFileInfoList imgList = folderDir.entryInfoList();
//         for(const QFileInfo& imgInfo : imgList)
//         {
//             QString imgName = imgInfo.fileName();
//             qDebug() << "图片名称：" << imgName;

//             // 1) 图片加载到opencv
//             Mat imgMat = imread(imgInfo.absoluteFilePath().toStdString());
//             if(imgMat.empty())
//             {
//                 qDebug() << "图片加载失败！路径：" << imgInfo.absoluteFilePath();
//                 continue;
//             }

//             // 2) 预处理
//             // 色彩空间转换
//             cvtColor(imgMat, imgMat, COLOR_RGB2GRAY);
//             // 尺寸归一化
//             resize(imgMat, imgMat, cv::Size(64, 64)); // 统一缩放到64x64
//             // 高斯去噪
//             GaussianBlur(imgMat, imgMat, cv::Size(5,5), 0);
//             // 直方图增加对比度
//             equalizeHist(imgMat,imgMat);

//             // 3) 提取特征
//             HOGDescriptor hog(Size(64,64), Size(16,16),Size(8,8),Size(8,8),9);
//             std::vector<float> descriptors;
//             hog.compute(imgMat, descriptors);
//             Mat featureMat(1, descriptors.size(), CV_32FC1, descriptors.data());
//             qDebug() << "特征提取完成！特征向量维度：" << featureMat.cols << "(长度)" ;

//             // 4）设置标签，存储到模型
//             allFeatures.push_back(featureMat);
//             int label = 0;
//             if(folderName == "Crazing")
//                 label = 1;
//             else if(folderName == "Inclusion")
//                 label = 2;
//             else if(folderName == "Patches")
//                 label = 3;
//             else if(folderName == "Pitted")
//                 label = 4;
//             else if(folderName == "Rolled")
//                 label = 5;
//             else if(folderName == "Scratches")
//                 label = 6;
//             allLabels.push_back(label);
//         }
//     }

//     allLabels.convertTo(allLabels, CV_32SC1);
//     allFeatures.convertTo(allFeatures, CV_32FC1);

//     cv::Ptr<cv::ml::SVM> svm = cv::ml::SVM::create();
//     svm->setType(ml::SVM::C_SVC);
//     svm->setKernel(ml::SVM::RBF); // 让Auto调C和Gamma
//     svm->setTermCriteria(cv::TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6));
//     // 调用trainAuto, 自动在合理范围内搜索最佳C和Gamma
//     svm->trainAuto(ml::TrainData::create(allFeatures, ml::ROW_SAMPLE, allLabels));
//     svm->save("D:/Qt_src/opencvTest/NEUMetalSurfaceDefectsData/train/my_svm_model.yml");
//     qDebug() << "===========训练结果已保存至文件中==============" ;
// }

// void usemodel()
// {
//     // 加载模型
//     Ptr<ml::SVM> model = ml::SVM::load("D:/Qt_src/opencvTest/NEUMetalSurfaceDefectsData/train/my_svm_model.yml");
//     if(model.empty())
//     {
//         qDebug() << "模型加载失败";
//         return;
//     }
//     qDebug() << "模型加载成功！";

//     // 遍历valid图片集，生成预测结果
//     QString root = "D:/Qt_src/opencvTest/NEUMetalSurfaceDefectsData/valid";
//     QDir dir(root);
//     if(!dir.exists()){
//         qDebug() << "路径不存在" ;
//         return;
//     }

//     // 只获取文件夹
//     dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

//     // 获取所有文件夹(Crazing、inclusion、patches)
//     QFileInfoList folderList = dir.entryInfoList();

//     std::vector<int> trueLabels;
//     std::vector<int> predictedLabels;
//     int totalImgs = 0;
//     int correctPredictions = 0;
//     QStringList imgPaths;

//     for(const QFileInfo& folderInfo : folderList)
//     {
//         QString folderName = folderInfo.fileName();
//         QString folderPath = folderInfo.absoluteFilePath();
//         // 遍历每个文件夹下的图片

//         qDebug() << "当前遍历目录:" << folderPath;

//         // 设置图片过滤器，只获取文件
//         QDir folderDir(folderPath);
//         folderDir.setNameFilters({"*.bmp"});
//         folderDir.setFilter(QDir::Files);

//         // 文件夹名映射为整数标签
//         int trueLabel = 0;
//         if(folderName == "Crazing")
//             trueLabel = 1;
//         else if(folderName == "Inclusion")
//             trueLabel = 2;
//         else if(folderName == "Patches")
//             trueLabel = 3;
//         else if(folderName == "Pitted")
//             trueLabel = 4;
//         else if(folderName == "Rolled")
//             trueLabel = 5;
//         else if(folderName == "Scratches")
//             trueLabel = 6;

//         QFileInfoList imgList = folderDir.entryInfoList();
//         for(const QFileInfo& imgInfo : imgList)
//         {
//             QString imgName = imgInfo.fileName();
//             QString imgPath = imgInfo.absolutePath();
//             qDebug() << "图片名称：" << imgName;

//             // 1) 图片加载到opencv
//             Mat imgMat = imread(imgInfo.absoluteFilePath().toStdString());
//             if(imgMat.empty())
//             {
//                 qDebug() << "图片加载失败！路径：" << imgInfo.absoluteFilePath();
//                 continue;
//             }

//             // 2) 预处理
//             // 色彩空间转换
//             cvtColor(imgMat, imgMat, COLOR_RGB2GRAY);
//             // 尺寸归一化
//             resize(imgMat, imgMat, cv::Size(64, 64)); // 统一缩放到64x64
//             // 高斯去噪
//             GaussianBlur(imgMat, imgMat, cv::Size(5,5), 0);
//             // 直方图增加对比度
//             equalizeHist(imgMat,imgMat);

//             // 3) 提取特征
//             HOGDescriptor hog(Size(64,64), Size(16,16),Size(8,8),Size(8,8),9);
//             std::vector<float> descriptors;
//             hog.compute(imgMat, descriptors);
//             Mat featureMat(1, descriptors.size(), CV_32FC1, descriptors.data());
//             qDebug() << "特征提取完成！特征向量维度：" << featureMat.cols << "(长度)" ;

//             // 4) 预测
//             float prediction = model->predict(featureMat);

//             // 5) 记录结果
//             trueLabels.push_back(trueLabel);
//             predictedLabels.push_back(static_cast<int>(prediction));
//             imgPaths.push_back(imgPath);

//             // 6) 统计
//             totalImgs++;
//             if(trueLabel == static_cast<int>(prediction))
//                 correctPredictions++;
//             else
//             {
//                 // std::cout << "[误分类]图片：" << imgName.toStdString()
//                 //           << " | 真实：" << trueLabel
//                 //           << " | 预测：" << prediction << std::endl;

//                 qDebug() << "[误分类]图片：" << imgName
//                          << " | 真实：" << trueLabel
//                          << " | 预测：" << prediction;
//             }
//         }
//     }

//     if(totalImgs > 0)
//     {
//         double accuracy = static_cast<double>(correctPredictions) / totalImgs * 100.0;
//         qDebug() << "==============验证结果================" ;
//         qDebug() << "总图片数量：" << totalImgs;
//         qDebug() << "正确分类数：" << correctPredictions;
//         qDebug() << "整体准确率：" << accuracy << "%" ;
//         qDebug() << "最优参数：C：" << model->getC() << "  Gamma: "<<model->getGamma();
//     }
// }
