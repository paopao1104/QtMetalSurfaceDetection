#include "utils/Logger.h"

#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QDebug>

Logger* Logger::m_instance = nullptr;
QMutex Logger::m_mutex;

Logger* Logger::instance()
{
    QMutexLocker locker(&m_mutex);

    if(!m_instance)
        m_instance = new Logger();

    return m_instance;
}

Logger::Logger()
    : m_logLevel(LOG_INFO)
    , m_logFile(nullptr)
    , m_textStream(nullptr)
    , m_consoleOutput(true)
{
    // 设置默认日志文件路径
    QString logDir = "../../logs";
    QDir dir(logDir);
    if(!dir.exists())
        dir.mkdir(logDir);

    m_logFilePath = QString("%1/app_%2.log").arg(logDir).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    // 打开日志文件
    m_logFile= new QFile(m_logFilePath);
    if(m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        m_textStream = new QTextStream(m_logFile);
        m_textStream->setEncoding(QStringConverter::Utf8);
    }
    else
    {
        delete m_logFile;
        m_logFile = nullptr;
        m_textStream = nullptr;
        qDebug() << "Failed to open log file: " << m_logFilePath;
    }
}

Logger::~Logger()
{
    if(m_textStream)
    {
        m_textStream->flush();
        delete m_textStream;
        m_textStream = nullptr;
    }

    if(m_logFile)
    {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

void Logger::setLogLevel(LogLevel level)
{
    instance()->m_logLevel = level;
}

void Logger::setLogFile(const QString& filePath)
{
    QMutexLocker locker(&instance()->m_writeMutex);

    Logger* logger = instance();

    // 关闭当前文件
    if(logger->m_textStream)
    {
        logger->m_textStream->flush();
        delete logger->m_textStream;
        logger->m_textStream = nullptr;
    }
    if(logger->m_logFile)
    {
        logger->m_logFile->flush();
        delete logger->m_logFile;
        logger->m_logFile = nullptr;
    }

    // 打开新文件
    logger->m_logFilePath = filePath;
    logger->m_logFile= new QFile(logger->m_logFilePath);
    if(logger->m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        logger->m_textStream = new QTextStream(logger->m_logFile);
        logger->m_textStream->setEncoding(QStringConverter::Utf8);
    }
    else
    {
        delete logger->m_logFile;
        logger->m_logFile = nullptr;
        qDebug() << "Failed to open log file: " << logger->m_logFilePath;
    }

}

// 设置是否输出到控制台
void Logger::setConsoleOutput(bool enabled)
{
    instance()->m_consoleOutput = enabled;
}

void Logger::log(LogLevel level, const QString& message, const char* file, int line)
{
    if(level < instance()->m_logLevel)
        return;
    instance()->writeLog(level, message, file, line);
}

void Logger::debug(const QString& message, const char* file, int line)
{
    log(LOG_DEBUG, message, file, line);
}

void Logger::info(const QString& message, const char* file, int line)
{
    log(LOG_INFO, message, file, line);
}

void Logger::warning(const QString& message, const char* file, int line)
{
    log(LOG_WARNING, message, file, line);
}

void Logger::error(const QString& message, const char* file, int line)
{
    log(LOG_ERROR, message, file, line);
}

void Logger::critical(const QString& message, const char* file, int line)
{
    log(LOG_CRITICAL, message, file, line);
}

void Logger::writeLog(LogLevel level, const QString& message, const char* file, int line)
{
    QMutexLocker locker(&m_writeMutex);

    QString logEntry;
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh::mm::ss.zzz");
    QString levelStr = levelToString(level);

    // 构建日志条目
    if(file && line > 0)
    {
        QString fileName = QFileInfo(file).fileName();
        logEntry = QString("[%1] [%2] [%3:%4] [Thread:0x%5] %6")
                       .arg(timeStr)
                       .arg(levelStr)
                       .arg(fileName)
                       .arg(line)
                       .arg(QString::number((quintptr)QThread::currentThreadId(), 16))
                       .arg(message);
    }
    else
    {
        logEntry = QString("[%1] [%2] [Thread:0x%3] %4")
            .arg(timeStr)
            .arg(levelStr)
            .arg(QString::number((quintptr)QThread::currentThreadId(), 16))
            .arg(message);
    }

    // 输出到控制台
    if(m_consoleOutput)
    {
        QTextStream console(stdout);

        // 根据级别设置颜色（在支持ANSI颜色的终端中）
        if(level == LOG_ERROR || level == LOG_CRITICAL)
            console << "\033[1;31m" << logEntry << "\033[0m" << Qt::endl; // 红色
        else if(level == LOG_WARNING)
            console << "\033[1;32m" << logEntry << "\033[0m" << Qt::endl; // 黄色
        else if(level == LOG_INFO)
            console << "\033[1;33m" << logEntry << "\033[0m" << Qt::endl; // 绿色
        else
            console << logEntry << Qt::endl;
    }

    // 写入文件
    if(m_textStream)
    {
        *m_textStream << logEntry << Qt::endl;
        m_textStream->flush();
    }

}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG:    return "DEBUG";
        case LOG_INFO:     return "DEBUG";
        case LOG_WARNING:  return "DEBUG";
        case LOG_ERROR:    return "DEBUG";
        case LOG_CRITICAL: return "DEBUG";
        default:           return "UNKNOWN";
    }
}
