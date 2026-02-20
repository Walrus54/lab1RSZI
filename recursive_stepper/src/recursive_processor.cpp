#include "recursive_processor.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

QStringList RecursiveProcessor::collectFilePaths(
    const QString& rootDirectory) const {
  QStringList files;

  const QDir root(rootDirectory);
  if (!root.exists()) {
    return files;
  }

  QDirIterator iterator(root.absolutePath(), QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
  while (iterator.hasNext()) {
    const QString path = iterator.next();
    const QFileInfo info(path);

    if (info.isSymLink()) {
      continue;
    }

    files.append(info.absoluteFilePath());
  }

  return files;
}
