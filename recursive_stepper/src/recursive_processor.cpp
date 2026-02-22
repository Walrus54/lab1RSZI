#include "recursive_processor.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

RecursiveProcessor::RecursiveProcessor(bool skipSystemFiles)
    : skipSystemFiles_(skipSystemFiles) {}

QStringList RecursiveProcessor::collectValidFilePaths(
    const QString& rootDirectory) const {
  QStringList files;

  const QDir root(rootDirectory);
  if (!root.exists()) {
    return files;
  }

  QDirIterator iterator(root.absolutePath(),
                        QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden |
                            QDir::System,
                        QDirIterator::Subdirectories);

  while (iterator.hasNext()) {
    const QString path = iterator.next();
    const QFileInfo info(path);

    if (info.isSymLink()) {
      continue;
    }

    if (isShortcutFile(path)) {
      continue;
    }

    if (info.size() == 0) {
      continue;
    }

    if (skipSystemFiles_ && isSystemFile(path)) {
      continue;
    }

    files.append(info.absoluteFilePath());
  }

  return files;
}

bool RecursiveProcessor::isShortcutFile(const QString& filePath) const {
  const QFileInfo info(filePath);
  const QString suffix = info.suffix().toLower();
  return suffix == QStringLiteral("lnk") || suffix == QStringLiteral("url");
}

bool RecursiveProcessor::isSystemFile(const QString& filePath) const {
#ifdef Q_OS_WIN
  Q_UNUSED(filePath);
  return false;
#else
  const QString normalizedPath = QDir::cleanPath(filePath);
  return normalizedPath.startsWith(QStringLiteral("/proc/")) ||
         normalizedPath.startsWith(QStringLiteral("/sys/")) ||
         normalizedPath.startsWith(QStringLiteral("/dev/"));
#endif
}
