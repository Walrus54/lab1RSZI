#include "recursive_processor.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "logger.h"

RecursiveProcessor::RecursiveProcessor(QString excludedFilePath,
                                       bool skipSystemFiles)
    : skipSystemFiles_(skipSystemFiles),
      // Нормализация пути нужна, чтобы корректно сравнивать его с путями iterator-а.
      excludedFilePath_(QFileInfo(std::move(excludedFilePath)).absoluteFilePath()) {}

QStringList RecursiveProcessor::collectValidFilePaths(
    const QString& rootDirectory) const {
  QStringList validFiles;

  const QDir root(rootDirectory);
  if (!root.exists()) {
    Logger::instance().error(QString("Каталог не существует: %1").arg(rootDirectory));
    return validFiles;
  }

  Logger::instance().info(
      QString("Начало рекурсивного сбора файлов: %1").arg(root.absolutePath()));

  QDirIterator iterator(root.absolutePath(),
                        QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden |
                            QDir::System,
                        QDirIterator::Subdirectories);

  while (iterator.hasNext()) {
    const QString filePath = iterator.next();
    const QFileInfo info(filePath);

    if (!excludedFilePath_.isEmpty()) {
#ifdef Q_OS_WIN
      // На Windows файловая система обычно регистронезависима.
      const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
      const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif
      if (info.absoluteFilePath().compare(excludedFilePath_, pathCaseSensitivity) == 0) {
        Logger::instance().warning(
            QString("Пропуск служебного файла приложения: %1").arg(filePath));
        continue;
      }
    }

    if (info.isSymLink()) {
      Logger::instance().warning(
          QString("Пропуск символьной ссылки: %1").arg(filePath));
      continue;
    }

    if (isShortcutFile(filePath)) {
      Logger::instance().warning(
          QString("Пропуск ярлыка/shortcut: %1").arg(filePath));
      continue;
    }

    if (info.size() == 0) {
      Logger::instance().warning(
          QString("Пропуск пустого файла: %1").arg(filePath));
      continue;
    }

    if (skipSystemFiles_ && isSystemFile(filePath)) {
      Logger::instance().warning(
          QString("Пропуск системного файла: %1").arg(filePath));
      continue;
    }

    validFiles.append(info.absoluteFilePath());
  }

  Logger::instance().info(
      QString("Сбор завершён. Валидных файлов: %1").arg(validFiles.size()));

  return validFiles;
}

bool RecursiveProcessor::isShortcutFile(const QString& filePath) const {
  const QFileInfo info(filePath);
  const QString suffix = info.suffix().toLower();
  return suffix == QStringLiteral("lnk") || suffix == QStringLiteral("url");
}

bool RecursiveProcessor::isSystemFile(const QString& filePath) const {
#ifdef Q_OS_WIN
  const std::wstring nativePath = QDir::toNativeSeparators(filePath).toStdWString();
  const DWORD attributes = GetFileAttributesW(nativePath.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return (attributes & FILE_ATTRIBUTE_SYSTEM) != 0;
#else
  const QString normalizedPath = QDir::cleanPath(filePath);
  return normalizedPath.startsWith(QStringLiteral("/proc/")) ||
         normalizedPath.startsWith(QStringLiteral("/sys/")) ||
         normalizedPath.startsWith(QStringLiteral("/dev/"));
#endif
}
