#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>

#include "logger.h"
#include "recursive_processor.h"

namespace {

int parseArguments(const QCoreApplication& app, QString& directoryPath) {
  QCommandLineParser parser;
  parser.addHelpOption();

  const QCommandLineOption directoryOption({"d", "directory"},
                                           "Root directory path",
                                           "directory");
  parser.addOption(directoryOption);
  parser.process(app);

  if (!parser.isSet(directoryOption)) {
    QTextStream(stderr) << "Missing required arg: --directory <path>" << Qt::endl;
    return 1;
  }

  directoryPath = parser.value(directoryOption).trimmed();
  const QDir dir(directoryPath);
  if (directoryPath.isEmpty() || !dir.exists()) {
    QTextStream(stderr) << "Directory does not exist: " << directoryPath << Qt::endl;
    return 2;
  }

  directoryPath = dir.absolutePath();
  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  Logger::instance().init(QDir::current().filePath(QStringLiteral("encryptor.log")));

  QString directoryPath;
  const int parseResult = parseArguments(app, directoryPath);
  if (parseResult != 0) {
    return parseResult;
  }

  RecursiveProcessor processor(true);
  const QStringList files = processor.collectValidFilePaths(directoryPath);
  QTextStream(stdout) << "Collected valid files: " << files.size() << Qt::endl;
  return 0;
}
