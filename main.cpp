#include <QDir>
#include <QStringList>
#include <QTextStream>

#include "encoder.h"
#include "logger.h"
#include "recursive_processor.h"

namespace {

constexpr int kContinueExecution = -1;

/**
 * @brief Параметры, полученные из командной строки.
 */
struct CommandLineOptions {
  QString mode;
  QString directoryPath;
  QString password;
  QString logPath;
};

/**
 * @brief Сводная информация о выполненной обработке файлов.
 */
struct ProcessingSummary {
  qsizetype collectedValidFiles = 0;
  quint64 processedFiles = 0;
  quint64 skippedEmpty = 0;
  quint64 skippedAlreadyEncrypted = 0;
  quint64 skippedNotEncrypted = 0;
  quint64 errors = 0;
};

/**
 * @brief Преобразовать argc/argv в QStringList аргументов запуска.
 * @param[in] argc Количество аргументов.
 * @param[in] argv Массив аргументов.
 * @return Список аргументов без имени исполняемого файла (argv[0]).
 */
QStringList buildArgumentList(int argc, char* argv[]) {
  QStringList argumentList;
  argumentList.reserve(argc > 1 ? argc - 1 : 0);
  for (int index = 1; index < argc; ++index) {
    argumentList.append(QString::fromLocal8Bit(argv[index]));
  }
  return argumentList;
}

/**
 * @brief Проверить, соответствует ли аргумент короткому или длинному имени.
 * @param[in] argument Разбираемый аргумент.
 * @param[in] shortName Короткое имя опции (например, "-m").
 * @param[in] longName Длинное имя опции (например, "--mode").
 * @return `true`, если аргумент совпадает с одним из имён.
 */
bool matchesOption(const QString& argument, const QString& shortName,
                   const QString& longName) {
  return argument == shortName || argument == longName;
}

/**
 * @brief Разобрать параметры командной строки вручную.
 * @param[in] arguments Список аргументов командной строки.
 * @param[out] commandLineOptions Выходные параметры запуска.
 * @return Код завершения, либо kContinueExecution для продолжения работы.
 */
int parseCommandLine(const QStringList& arguments,
                     CommandLineOptions& commandLineOptions) {
  bool modeSet = false;
  bool directorySet = false;
  bool passwordSet = false;

  for (qsizetype index = 0; index < arguments.size(); ++index) {
    const QString& argument = arguments.at(index);

    const bool isMode = matchesOption(argument, "-m", "--mode");
    const bool isDirectory = matchesOption(argument, "-d", "--directory");
    const bool isPassword = matchesOption(argument, "-p", "--password");
    const bool isLog = matchesOption(argument, "-l", "--log");

    if (!isMode && !isDirectory && !isPassword && !isLog) {
      QTextStream(stderr)
          << "Неизвестный аргумент: " << argument << Qt::endl;
      return 1;
    }

    if (index + 1 >= arguments.size()) {
      QTextStream(stderr)
          << "Для аргумента " << argument << " не указано значение." << Qt::endl;
      return 1;
    }

    const QString& value = arguments.at(++index);
    if (isMode) {
      commandLineOptions.mode = value.trimmed().toLower();
      modeSet = true;
    } else if (isDirectory) {
      commandLineOptions.directoryPath = value.trimmed();
      directorySet = true;
    } else if (isPassword) {
      commandLineOptions.password = value;
      passwordSet = true;
    } else {
      commandLineOptions.logPath = value.trimmed();
    }
  }

  if (!modeSet || !directorySet || !passwordSet) {
    QTextStream(stderr)
        << "Требуются параметры: --mode <encrypt|decrypt> --directory <path> "
           "--password <value>"
        << Qt::endl;
    return 1;
  }

  return kContinueExecution;
}

/**
 * @brief Вычислить фактический путь к лог-файлу.
 * @param[in] logPath Путь из CLI.
 * @return Нормализованный абсолютный/относительный путь.
 */
QString resolveLogPath(const QString& logPath) {
  return logPath.isEmpty() ? QDir::current().filePath("encryptor.log")
                           : QDir::cleanPath(logPath);
}

/**
 * @brief Проверить корректность бизнес-параметров после инициализации логера.
 * @param[in] commandLineOptions Параметры запуска.
 * @return `true`, если параметры корректны.
 */
bool validateOptions(const CommandLineOptions& commandLineOptions) {
  if (commandLineOptions.mode != QStringLiteral("encrypt") &&
      commandLineOptions.mode != QStringLiteral("decrypt")) {
    Logger::instance().error("Некорректный режим. Допустимо: encrypt|decrypt");
    return false;
  }

  if (commandLineOptions.directoryPath.isEmpty()) {
    Logger::instance().error("Пустой путь к директории.");
    return false;
  }

  const QDir targetDirectory(commandLineOptions.directoryPath);
  if (!targetDirectory.exists()) {
    Logger::instance().error(QString("Указанная директория не существует: %1")
                                 .arg(commandLineOptions.directoryPath));
    return false;
  }

  return true;
}

/**
 * @brief Выполнить обработку всех валидных файлов.
 * @param[in] filesToProcess Список файлов для обработки.
 * @param[in] password Пароль для шифрования/дешифрования.
 * @param[in] isEncryptMode Флаг режима шифрования.
 * @return Сводные результаты обработки.
 */
ProcessingSummary processFiles(const QStringList& filesToProcess,
                               const QString& password,
                               bool isEncryptMode) {
  ProcessingSummary summary;
  summary.collectedValidFiles = filesToProcess.size();

  for (const QString& filePath : filesToProcess) {
    Encoder::Result result = Encoder::Result::kCryptoError;
    if (isEncryptMode) {
      result = Encoder::instance().encryptFile(filePath, password);
    } else {
      result = Encoder::instance().decryptFile(filePath, password);
    }

    switch (result) {
      case Encoder::Result::kSuccess:
        ++summary.processedFiles;
        Logger::instance().info(
            QString("Успешно обработан файл: %1").arg(filePath));
        break;
      case Encoder::Result::kAlreadyEncrypted:
        ++summary.skippedAlreadyEncrypted;
        Logger::instance().warning(
            QString("Файл уже зашифрован, пропуск: %1").arg(filePath));
        break;
      case Encoder::Result::kNotEncrypted:
        ++summary.skippedNotEncrypted;
        Logger::instance().warning(
            QString("Файл не зашифрован, пропуск: %1").arg(filePath));
        break;
      case Encoder::Result::kEmptyFile:
        ++summary.skippedEmpty;
        Logger::instance().warning(
            QString("Пустой файл, пропуск: %1").arg(filePath));
        break;
      case Encoder::Result::kOpenError:
      case Encoder::Result::kReadError:
      case Encoder::Result::kWriteError:
      case Encoder::Result::kCryptoError:
      case Encoder::Result::kInvalidFormat:
        ++summary.errors;
        Logger::instance().error(
            QString("Ошибка обработки файла: %1").arg(filePath));
        break;
    }
  }

  return summary;
}

/**
 * @brief Вывести итоговую сводку выполнения.
 * @param[in] summary Сводные результаты обработки.
 */
void printSummary(const ProcessingSummary& summary) {
  QTextStream(stdout)
      << "\n===== SUMMARY =====\n"
      << "Collected valid files: " << summary.collectedValidFiles << '\n'
      << "Processed files: " << summary.processedFiles << '\n'
      << "Skipped empty: " << summary.skippedEmpty << '\n'
      << "Skipped already encrypted: " << summary.skippedAlreadyEncrypted
      << '\n'
      << "Skipped not encrypted: " << summary.skippedNotEncrypted << '\n'
      << "Errors: " << summary.errors << '\n'
      << "===================\n"
      << Qt::endl;
}

/**
 * @brief Основной сценарий работы приложения.
 * @param[in] commandLineOptions Параметры запуска.
 * @return Код завершения процесса.
 */
int runApplication(const CommandLineOptions& commandLineOptions) {
  const QString resolvedLogPath = resolveLogPath(commandLineOptions.logPath);
  Logger::instance().initialize(resolvedLogPath);

  if (!validateOptions(commandLineOptions)) {
    return 2;
  }

  const bool isEncryptMode =
      (commandLineOptions.mode == QStringLiteral("encrypt"));
  const QDir targetDirectory(commandLineOptions.directoryPath);

  RecursiveProcessor recursiveProcessor(resolvedLogPath, isEncryptMode);
  const QStringList filesToProcess = recursiveProcessor.collectValidFilePaths(
      targetDirectory.absolutePath());

  const ProcessingSummary summary =
      processFiles(filesToProcess, commandLineOptions.password, isEncryptMode);
  printSummary(summary);

  return (summary.errors == 0) ? 0 : 3;
}

}  // namespace

/**
 * @brief Точка входа консольного приложения.
 * @param[in] argc Количество аргументов командной строки.
 * @param[in] argv Массив аргументов командной строки.
 * @return Код завершения процесса.
 * @throws Не выбрасывает исключений.
 */
int main(int argc, char* argv[]) {
  const QStringList arguments = buildArgumentList(argc, argv);
  CommandLineOptions commandLineOptions;
  const int parseResult = parseCommandLine(arguments, commandLineOptions);
  if (parseResult != kContinueExecution) {
    return parseResult;
  }

  return runApplication(commandLineOptions);
}
