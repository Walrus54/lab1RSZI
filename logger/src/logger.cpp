#include "logger.h"

#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

Logger::Logger() : isInitialized_(false) {}

Logger::~Logger() {
  QMutexLocker locker(&mutex_);
  if (logFile_.isOpen()) {
    logStream_.flush();
    logFile_.close();
  }
}

Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::initialize(const QString& logFilePath) {
  // Пустой путь = лог в текущей рабочей директории.
  const QString resolvedPath =
      logFilePath.trimmed().isEmpty() ? QDir::current().filePath("encryptor.log")
                                      : QDir::cleanPath(logFilePath.trimmed());

  QMutexLocker locker(&mutex_);

  if (logFile_.isOpen()) {
    logStream_.flush();
    logFile_.close();
  }

  logFile_.setFileName(resolvedPath);
  isInitialized_ = logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

  if (!isInitialized_) {
    QTextStream(stderr) << "[LOGGER] [ERROR] Не удалось открыть файл журнала: " << resolvedPath
                        << Qt::endl;
    return;
  }

  logStream_.setDevice(&logFile_);
  const QString line = QString("[%1] [%2] %3")
                           .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                QStringLiteral("INFO"),
                                QStringLiteral("Логер инициализирован: %1").arg(resolvedPath));
  QTextStream(stdout) << line << Qt::endl;
  logStream_ << line << Qt::endl;
  logStream_.flush();
}

void Logger::info(const QString& message) { log(Level::kInfo, message); }

void Logger::warning(const QString& message) { log(Level::kWarning, message); }

void Logger::error(const QString& message) { log(Level::kError, message); }

void Logger::log(Level level, const QString& message) {
  const QString line = QString("[%1] [%2] %3")
                           .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                levelToString(level), message);

  QMutexLocker locker(&mutex_);

  QTextStream(level == Level::kError ? stderr : stdout) << line << Qt::endl;
  // При недоступном файле журнала оставляем хотя бы вывод в консоль.
  if (isInitialized_ && logFile_.isOpen()) {
    logStream_ << line << Qt::endl;
    logStream_.flush();
  }
}

QString Logger::levelToString(Level level) const {
  switch (level) {
    case Level::kInfo:
      return QStringLiteral("INFO");
    case Level::kWarning:
      return QStringLiteral("WARN");
    case Level::kError:
      return QStringLiteral("ERROR");
  }
  return QStringLiteral("UNKNOWN");
}
