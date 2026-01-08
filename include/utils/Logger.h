#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMutex>
#include <QFile>
#include <QTextStream>

enum LogLevel
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
};

class Logger
{
public:
    static Logger* instance();

    // 设置日志级别
    static void setLogLevel(LogLevel level);

    // 设置日志文件
    static void setLogFile(const QString& filePath);

    // 设置是否输出到控制台
    static void setConsoleOutput(bool enabled);

    // 日志记录方法
    static void log(LogLevel level, const QString& message, const char* file = nullptr, int line = -1);

    // 快捷方法
    static void debug(const QString& message, const char* file = nullptr, int line = -1);
    static void info(const QString& message, const char* file = nullptr, int line = -1);
    static void warning(const QString& message, const char* file = nullptr, int line = -1);
    static void error(const QString& message, const char* file = nullptr, int line = -1);
    static void critical(const QString& message, const char* file = nullptr, int line = -1);

private:
    Logger();
    ~Logger();

    void writeLog(LogLevel level, const QString& message, const char* file = nullptr, int line = -1);
    QString levelToString(LogLevel level);
private:
    static Logger* m_instance;
    static QMutex m_mutex;

    LogLevel m_logLevel;
    QString m_logFilePath;
    QFile* m_logFile;
    QTextStream* m_textStream;
    bool m_consoleOutput;
    QMutex m_writeMutex;
};

// ============== 日志宏定义 ============== //

// 基础宏日志
#define LOG(level, message) \
    Logger::log(level, message, __FILE__, __LINE__)

// 快捷日志宏
#define LOG_DEBUG(message) \
    Logger::debug(message, __FILE__, __LINE__)

#define LOG_INFO(message) \
    Logger::info(message, __FILE__, __LINE__)

#define LOG_WARNING(message) \
    Logger::warning(message, __FILE__, __LINE__)

#define LOG_ERROR(message) \
    Logger::error(message, __FILE__, __LINE__)

#define LOG_CRITICAL(message) \
    Logger::critical(message, __FILE__, __LINE__)

// 带条件的日志宏
#define LOG_IF(condition, level, message) \
    if(condition) { LOG(level, message); }

#define LOG_INFO_IF(condition, message) \
    if(condition) { LOG_INFO(message); }

// 自动跟踪日志（进入、退出函数）
#define LOG_ENTRY() \
    LOG_DEBUG(QString("Entering %1").arg(Q_FUNC_INFO))

#define LOG_EXIT() \
    LOG_DEBUG(QString("Exiting %1").arg(Q_FUNC_INFO))

// 带作用域的日志助手
class LogScope {
public:
    LogScope(const QString& functionName) : m_functionName(functionName) {
        LOG_DEBUG(QString("Entering %1").arg(m_functionName));
    }
    ~LogScope() {
        LOG_DEBUG(QString("Exiting %1").arg(m_functionName));
    }
private:
    QString m_functionName;
};

#endif // LOGGER_H
