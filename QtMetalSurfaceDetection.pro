QT += core gui printsupport
QT += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    src/core/FeatureExtractor.cpp \
    src/ui/MetalSurfaceDetection.cpp \
    src/ui/tabs/ConfusionMatrixWidget.cpp \
    src/ui/tabs/SingleImageTab.cpp \
    src/ui/tabs/BatchValidationTab.cpp \
    src/ui/tabs/PrecisionRecallBarChart.cpp \
    src/core/Classifier.cpp \
    src/core/ImageProcessor.cpp \
    src/data/DataManager.cpp \
    src/data/DatasetModel.cpp \
    src/utils/ImageUtils.cpp \
    src/utils/Logger.cpp \
    src/utils/TimerUtils.cpp \
    src/utils/qcustomplot.cpp

HEADERS += \
    include/core/FeatureExtractor.h \
    include/core/PredictionResult.h \
    include/ui/MetalSurfaceDetection.h \
    include/ui/tabs/ConfusionMatrixWidget.h \
    include/ui/tabs/SingleImageTab.h \
    include/ui/tabs/BatchValidationTab.h \
    include/ui/tabs/PrecisionRecallBarChart.h \
    include/core/Classifier.h \
    include/core/ImageProcessor.h \
    include/data/DataManager.h \
    include/data/DatasetModel.h \
    include/utils/ImageUtils.h \
    include/utils/Logger.h \
    include/utils/TimerUtils.h \
    include/utils/qcustomplot.h

FORMS += \
    MetalSurfaceDetection.ui

RESOURCES += \
    models/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../library/opencv-4.11.0/build/x64/vc16/lib/ -lopencv_world4110d
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../library/opencv-4.11.0/build/x64/vc16/lib/ -lopencv_world4110d

INCLUDEPATH += $$PWD/../../../library/opencv-4.11.0/build/include
DEPENDPATH += $$PWD/../../../library/opencv-4.11.0/build/include

INCLUDEPATH += ./include

