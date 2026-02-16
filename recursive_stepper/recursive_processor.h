#ifndef RECURSIVE_PROCESSOR_H
#define RECURSIVE_PROCESSOR_H

#include <QString>
#include <QStringList>

class RecursiveProcessor final {
 public:
  QStringList collectFilePaths(const QString& rootDirectory) const;
};

#endif  // RECURSIVE_PROCESSOR_H
