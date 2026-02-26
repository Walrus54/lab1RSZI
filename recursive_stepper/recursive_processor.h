#ifndef RECURSIVE_PROCESSOR_H
#define RECURSIVE_PROCESSOR_H

#include <QString>
#include <QStringList>

/**
 * @brief Recursive directory crawler with basic filtering.
 */
class RecursiveProcessor final {
 public:
  explicit RecursiveProcessor(bool skipSystemFiles);

  QStringList collectValidFilePaths(const QString& rootDirectory) const;

 private:
  bool isShortcutFile(const QString& filePath) const;
  bool isSystemFile(const QString& filePath) const;

  bool skipSystemFiles_;
};

#endif  // RECURSIVE_PROCESSOR_H
